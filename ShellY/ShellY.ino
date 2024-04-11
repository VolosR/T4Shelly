#include <ESP32Time.h>
#include "time.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#define LILYGO_TDISPLAY_AMOLED_SERIES
#include "LilyGo_AMOLED.h"     
#include "Noto.h"
#include "Font1.h"
#include "middleFont.h"
#include "largestFont.h"
#include "hugeFatFont.h"
#include "fatFont.h"
#include "Latin_Hiragana_24.h"
#include "NotoSansBold15.h"
#include "NotoSansMonoSCB20.h"
#include <TFT_eSPI.h>   

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);
LilyGo_Class amoled;

ESP32Time rtc(0); 

const char* ssid = "HomeMyDear";
const char* password = "12345678";
const char* url = "https://shelly-100-eu.shelly.cloud/device/status?id=08F9345E751E4&auth_key=TjJjMTJjdWLk21CF89A5eFD3861B5E38B10B11CC8349282280C896417B97A6F7EBr3E65CB88065B037F11CC5F";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec =7200;            //time zone * 3600 , my time zone is  +2 GTM
const int   daylightOffset_sec = 0; 

HTTPClient http;
String payload;
JsonDocument doc;

String days2[7]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};

int bright[6]={40,60,100,126,180,220}; //brightness levels
int b=2; //chosen brightness level

//colors
unsigned short grays[24];
unsigned short back=TFT_MAGENTA;
unsigned short blue=0x0250;
unsigned short lightblue=0x3D3F;


double KWH;
double todayKWH=0;
double WH;
int dot=0;

#define latin Latin_Hiragana_24
#define small NotoSansBold15
#define digits NotoSansMonoSCB20

#define c1 0xBDD7  //white
#define c2 0x18C3  //gray
#define c3 0x9986  //red
#define c4 0x2CAB  //green
#define c5 0xBDEF  //gold

float power=0;
String lbl[3]={"VOLTAGE","CURRENT","FREQUENCY"};
float data[3];
String todayLbl[2]={"TODAY:","MAX W:"};
double today[2];  // 0 is today kWh ,  1 is today max W
bool started=0;
int graph[70]={0};
int graphP[70]={0};
int graphTMP[70]={0};
float maax=0;
int p,m;
String ip;

int fromTop = 328;
int left = 200;
int width = 240;
int heigth = 74;

uint16_t gra[60] = { 0 };
uint16_t lines[11] = { 0 };
String sec="67";
int pos = 0;

String digit1="1";
String digit2="2";
String digit3="3";
String digit4="4";
String digit5="5";

unsigned long cur_time=0;
int period=1000;

int deb=0;  //touch debounce

void setTime()
{
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)){
    rtc.setTimeStruct(timeinfo); 
  }
}

void setup()
{
    amoled.begin();
    amoled.setBrightness(100);
    spr.createSprite(600, 450);
    spr.setSwapBytes(1);

       //define level of grays or greys
     int co=240;
     for(int i=0;i<24;i++)
     {grays[i]=tft.color565(co, co, co);
     co=co-10;}

       for (int i = 0; i < 50; i++)
    gra[i] = tft.color565(i * 5, i * 5, i * 5);

  lines[0] = gra[5];
  lines[1] = gra[10];
  lines[2] = gra[20];
  lines[3] = gra[30];
  lines[4] = gra[40];
  lines[5] = gra[49];
  lines[6] = gra[40];
  lines[7] = gra[30];
  lines[8] = gra[20];
  lines[9] = gra[10];
  lines[10] = gra[5];


    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.println("Connecting to WiFi...");
    }
     setTime();
     initDraw();
}

void initDraw()
{
   spr.setTextDatum(0);
   spr.fillSprite(blue);
   spr.fillSmoothRoundRect(6, 6, 588,438 ,12, TFT_BLACK,blue);
   spr.fillSmoothRoundRect(160, 20, 420,410 ,9,grays[19], TFT_BLACK);
   spr.fillSmoothRoundRect(342, 230, 26,32 ,4,TFT_ORANGE,grays[19]); //small orange w sighn
  
   spr.loadFont(middleFont);
   spr.setTextColor(grays[8],grays[19]);
   spr.drawString("ACTUAL POWER",196,186);
    spr.setTextColor(TFT_BLACK,TFT_ORANGE);
   spr.drawString("W",348,236);
   spr.setTextColor(grays[8],grays[22]);

   //topThree
   for(int i=0;i<3;i++){
   spr.fillSmoothRoundRect(180+(i*134), 75 , 114, 100, 4, grays[22],grays[19]);
   spr.drawString(lbl[i],180+(i*134)+8,86);
   }
   spr.unloadFont();

   spr.loadFont(hugeFatFont);
   spr.setTextColor(grays[4],grays[22]);
   for(int i=0;i<3;i++)
   spr.drawString(String(data[i]),180+(i*134)+8,120);
   spr.unloadFont();

   spr.loadFont(fatFont);
   spr.setTextColor(TFT_ORANGE,grays[19]);
   spr.drawString("ELECTRIC METER",180,36);
   spr.unloadFont();

   //midle
   spr.fillRect(184, 190,2, 90, grays[7]);
   spr.fillRect(184, 214,186,2, grays[7]);
   spr.fillSmoothCircle(184, 190,4, grays[7],grays[19]);
   spr.fillSmoothCircle(184, 190+90,4, grays[7],grays[19]);
   spr.fillSmoothCircle(184+184, 214,4, grays[7],grays[19]);
   

   // graph
   spr.fillSmoothRoundRect(388, 188 , 172, 100, 4, grays[22],grays[19]);
 

   for(int i=0;i<4;i++)
   spr.drawFastHLine(399,220+(i*14),150,grays[16]);
   spr.fillRect(399,220,2,55,grays[7]);
   spr.fillRect(399,275,150,2,grays[7]);
     
     for(int i=0;i<70;i++)
      spr.fillRect(402+(i*2),275-graph[i],2,graph[i],0x7020);
    
   spr.loadFont(Noto);
   spr.setTextColor(grays[12],grays[22]);
   spr.drawString("LAST 2 MIN",399,196);
    spr.setTextColor(grays[4],grays[19]);
   spr.drawString("VOLOS PROJECTS",434,34);
   spr.unloadFont();

   spr.fillSmoothRoundRect(434, 52 , 126, 7, 2, lightblue,grays[19]);

   //under grapg TODAY adn MAX W
   spr.setTextColor(grays[8],grays[19]);
   
   for(int i=0;i<2;i++)
   {
   spr.loadFont(Noto); 
   spr.setTextColor(grays[10],grays[19]);
   spr.drawString(todayLbl[i],490,300+(55*i));
   spr.unloadFont();
   spr.drawLine(490,318+(55*i),560,318+(55*i),grays[5]);
   spr.loadFont(middleFont); 
   spr.setTextColor(grays[4],grays[19]);
   spr.drawString(String(today[i]),490,324+(55*i));
   spr.unloadFont();
   }
   
   //ACTUAL POWER
   spr.loadFont(largestFont);
   spr.setTextColor(grays[2],grays[22]);
   spr.drawFloat(power,1,195,230);
   spr.unloadFont();

   //meter
     spr.setTextDatum(4);
    spr.fillSmoothRoundRect(180, 300 , 300, 118, 7, c1,grays[19]);
  
  //........................khw..............
  spr.fillSmoothRoundRect(left + 40, fromTop - 16, width - 60, heigth, 8, c2, c1);
  spr.fillSmoothRoundRect(left + 42, fromTop - 14, width - 64, heigth, 8, c1, c2);
  spr.fillRect(left + 40, fromTop - 3, width - 60, 3, c1);
  spr.fillRect(left + 90, fromTop - 19, width - 160, 5, c1);
  
  spr.fillSmoothRoundRect(left, fromTop, width, heigth, 8, blue, c1);
  spr.fillSmoothRoundRect(left+200, fromTop, 60, heigth, 8, c3, c1);
  spr.fillRect(left+199, fromTop, 4, heigth, c1);

  spr.fillSmoothCircle(left+200, fromTop + heigth, 9, c1);
  spr.fillSmoothCircle(left+200, fromTop + heigth, 5, c2);


  for (int i = 0; i < 5; i++) {
    spr.fillRectHGradient(left + (20) + (i * 36), fromTop + 9, 15, 44, TFT_BLACK, gra[2]);
    spr.fillRectHGradient(left + (35) + (i * 36), fromTop + 9, 15, 44, gra[2], TFT_BLACK);

    for (int j = 0; j < 11; j++)
      if (j == 5)
        spr.drawLine(left + (40) + (i * 36), fromTop + 11 + (j * 4), left + (47) + (i * 36), fromTop + 11 + (j * 4), lines[j]);
      else
        spr.drawLine(left + (43) + (i * 36), fromTop + 11 + (j * 4), left + (47) + (i * 36), fromTop + 11 + (j * 4), lines[j]);
  }

  spr.fillRectHGradient(left+211, fromTop + 9, 20, 44, TFT_BLACK, gra[2]);
  spr.fillRectHGradient(left+231, fromTop + 9, 20, 44, gra[2], TFT_BLACK);

  for (int j = 0; j < 11; j++)
    if (j == 7)
      spr.drawLine(left+242, fromTop + 11 + (j * 4), left+249, fromTop + 11 + (j * 4), lines[j]);
    else
      spr.drawLine(left+245, fromTop + 11 + (j * 4), left+249, fromTop + 11 + (j * 4), lines[j]);

  spr.drawLine(left + 34, fromTop + 58, left + 34, fromTop + 64, c1);   
  spr.drawLine(left + 34, fromTop + 64, left + 38, fromTop + 64, c1);
  spr.drawLine(left + 180, fromTop + 58, left + 180, fromTop + 64, c1);
  spr.drawLine(left + 174, fromTop + 64, left + 180, fromTop + 64, c1);

  spr.loadFont(latin);
  spr.setTextColor(c2, c1);
  spr.drawString("kWh", left+130, fromTop-12);
  spr.unloadFont();

  spr.loadFont(small);
  spr.setTextColor(c1, c3);
  spr.drawString("x1", left+230, fromTop + 64);
   spr.setTextColor(c1,blue);
  spr.drawString("..TOTAL..", left+100, fromTop + 64);
  spr.setTextColor(c1, c2);
 
  spr.setTextDatum(4);
  spr.unloadFont();
  
  spr.loadFont(digits);
  spr.setTextColor(c1, TFT_BLACK);

  spr.drawString(String(digit1), left+30, fromTop + 33);
  spr.drawString(String(digit2), left+30+36, fromTop + 33);
  spr.drawString(String(digit3), left+30+72, fromTop + 33);
  spr.drawString(String(digit4), left+30+108, fromTop + 33);
  spr.drawString(String(digit5), left+30+144, fromTop + 33);
  spr.drawString(String(sec), left+229, fromTop + 33);
  spr.unloadFont();

  //left part of screen
  spr.loadFont(largestFont);
  spr.setTextColor(grays[6],TFT_BLACK);
  spr.setTextDatum(0);
  spr.drawString(rtc.getTime().substring(0,5),24,24);
  spr.unloadFont();
  
  spr.fillRect(38,86,2,60,grays[17]);
  spr.fillRect(38,116,94,2,grays[17]);

  spr.loadFont(middleFont);
  spr.setTextColor(grays[10],TFT_BLACK);
  spr.drawString(String(rtc.getMonth()+1)+"/"+String(rtc.getDay()),48,86);
  spr.unloadFont();
  spr.loadFont(Noto);
  spr.setTextColor(grays[10],TFT_BLACK);
  spr.drawString(days2[rtc.getDayofWeek()],48,126);
  spr.drawString("ONLINE",52,394);
  spr.drawString(ip,52,414);
  spr.fillRect(116,394,12,12,TFT_DARKGREEN);

  spr.drawRect(24,394,22,32,TFT_ORANGE);
  for(int i=0;i<b+1;i++)
  spr.fillRect(27,421-(i*5),16,2,TFT_ORANGE);

  spr.unloadFont();
  
   spr.setTextColor(grays[16],TFT_BLACK);
   amoled.pushColors(0, 0, 600, 450, (uint16_t *)spr.getPointer());
}

void getData()
{   
    http.begin(url); //Specify the URL
    int httpCode = http.GET();  //Make the request
 
    if (httpCode > 0) { //Check for the returning cod
        payload = http.getString();
     }else {
     Serial.println("Error on HTTP request");
    }
    http.end(); //Free the resources
  
    DeserializationError error = deserializeJson(doc, payload);
    power = doc["data"]["device_status"]["em1:1"]["act_power"];
    data[0]=doc["data"]["device_status"]["em1:1"]["voltage"];
    data[1]=doc["data"]["device_status"]["em1:1"]["current"];
    data[2]=doc["data"]["device_status"]["em1:1"]["freq"];
    const char* staIp=doc["data"]["device_status"]["wifi"]["sta_ip"];
      ip=String(staIp);

      p=power;
      m=0;
      graphP[69]=p;

      for(int i=68;i>0;i--)
      graphP[i]=graphTMP[i+1];

      for(int i=0;i<70;i++)
      graphTMP[i]=graphP[i];

      for(int i=0;i<70;i++)
      if(graphP[i]>m) 
      m=graphP[i];

      for(int i=0;i<70;i++)
      graph[i]=map(graphP[i],0,m,0,55);

      WH = doc["data"]["device_status"]["em1data:1"]["total_act_energy"];
      KWH=WH/1000.00;

      if(started==0)
      {
        started=1;
        todayKWH=KWH;
        today[1]=0;
      }

      if(power>today[1]) today[1]=power;
      today[0]=KWH-todayKWH;
      dot=String(KWH).length();

    if(dot-8>=0)digit1 = String(String(KWH)[dot-8]); else digit1="0";
    if(dot-7>=0)digit2 = String(String(KWH)[dot-7]); else digit2="0";
    if(dot-6>=0)digit3 = String(String(KWH)[dot-6]); else digit3="0";
    if(dot-5>=0)digit4 = String(String(KWH)[dot-5]); else digit4="0";
    if(dot-4>=0) digit5=String(String(KWH)[dot-4]); else digit5="0"; 
    sec = String(String(String(KWH)[dot-2]) + String(String(KWH)[dot-1]));
}


void loop()
{
    if(rtc.getSecond()==0 && rtc.getMinute()==0 && rtc.getHour()==0)
    {
      started=0;
    }

   if(rtc.getSecond()==0 && rtc.getMinute()==0)
   setTime();

   
   static int16_t x, y;
   bool touched = amoled.getPoint(&x, &y);
   
   if(touched)
   {
     if(y>320 && y<450 && x>0 && x<180)
      if(deb==0)
        {deb=1;b++; if(b==6) b=0; amoled.setBrightness(bright[b]); initDraw();}
   }else deb=0;


   if(cur_time+period<millis())
   {
   cur_time=millis(); 
   getData();
   initDraw();
   }
  
}




