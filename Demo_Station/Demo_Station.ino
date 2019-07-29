
#include <Arduino.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ILI9341.h> // Hardware-specific library
#include <SPI.h>
#include <Wire.h>  // required even though we do not use I2C 
#include <XPT2046_Touchscreen.h>

#include <WebSocketsClient.h>

const String stationID = "1";

// Additional UI functions
#include "GfxUi.h"

// Fonts created by http://oleddisplay.squix.ch/
#include "ArialRoundedMTBold_14.h"
#include "ArialRoundedMTBold_36.h"

// Download helper
#include "WebResource.h"

#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>

// Helps with connecting to internet
#include <WiFiManager.h>

// check settings.h for adapting to your needs
#include "settings.h"
#include <JsonListener.h>
#include "OpenWeatherMapCurrent.h"
#include "OpenWeatherMapForecast.h"
#include "TimeClient.h"

#include "DHTesp.h"
#define DHTPIN D0     // what pin we're connected to
#define DHTTYPE DHT11   // DHT 11 
DHTesp dht;

#define TOUCH_CS_PIN D3
#define TOUCH_IRQ_PIN D2

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 320
#define TS_MINY 250
#define TS_MAXX 3800
#define TS_MAXY 3900

XPT2046_Touchscreen spitouch(TOUCH_CS_PIN,TOUCH_IRQ_PIN);

// HOSTNAME for OTA update
#define HOSTNAME "ESP8266-OTA-"

/*****************************
 * Important: see settings.h to configure your settings!!!
 * ***************************/

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
GfxUi ui = GfxUi(&tft);

WebResource webResource;
TimeClient timeClient(UTC_OFFSET);

// Set to false, if you prefere imperial/inches, Fahrenheit
OpenWeatherMapCurrent OWMCclient;
OpenWeatherMapCurrentData OWMCdata;
OpenWeatherMapForecast OWMFclient;
#define MAX_FORECASTS 15
OpenWeatherMapForecastData OWMFdata[MAX_FORECASTS];
uint8_t allowedHours[] = {0,12};
 
//declaring prototypes
void configModeCallback (WiFiManager *myWiFiManager);
void downloadCallback(String filename, int16_t bytesDownloaded, int16_t bytesTotal);
ProgressCallback _downloadCallback = downloadCallback;
void downloadResources();
void updateData();
void drawProgress(uint8_t percentage, String text);
void drawTime();
void drawCurrentWeather();
void drawForecast();
void drawForecastDetail(uint16_t x, uint16_t y, uint8_t dayIndex);
String getMeteoconIcon(String iconText);
void drawAstronomy();
void drawSeparator(uint16_t y);
void sleepNow(uint8_t wakepin);

long lastDownloadUpdate = millis();

static int lunar_month_days[] =
        {
                1887, 0x1694, 0x16aa, 0x4ad5, 0xab6, 0xc4b7, 0x4ae, 0xa56, 0xb52a,
                0x1d2a, 0xd54, 0x75aa, 0x156a, 0x1096d, 0x95c, 0x14ae, 0xaa4d, 0x1a4c, 0x1b2a, 0x8d55, 0xad4, 0x135a, 0x495d,
                0x95c, 0xd49b, 0x149a, 0x1a4a, 0xbaa5, 0x16a8, 0x1ad4, 0x52da, 0x12b6, 0xe937, 0x92e, 0x1496, 0xb64b, 0xd4a,
                0xda8, 0x95b5, 0x56c, 0x12ae, 0x492f, 0x92e, 0xcc96, 0x1a94, 0x1d4a, 0xada9, 0xb5a, 0x56c, 0x726e, 0x125c,
                0xf92d, 0x192a, 0x1a94, 0xdb4a, 0x16aa, 0xad4, 0x955b, 0x4ba, 0x125a, 0x592b, 0x152a, 0xf695, 0xd94, 0x16aa,
                0xaab5, 0x9b4, 0x14b6, 0x6a57, 0xa56, 0x1152a, 0x1d2a, 0xd54, 0xd5aa, 0x156a, 0x96c, 0x94ae, 0x14ae, 0xa4c,
                0x7d26, 0x1b2a, 0xeb55, 0xad4, 0x12da, 0xa95d, 0x95a, 0x149a, 0x9a4d, 0x1a4a, 0x11aa5, 0x16a8, 0x16d4,
                0xd2da, 0x12b6, 0x936, 0x9497, 0x1496, 0x1564b, 0xd4a, 0xda8, 0xd5b4, 0x156c, 0x12ae, 0xa92f, 0x92e, 0xc96,
                0x6d4a, 0x1d4a, 0x10d65, 0xb58, 0x156c, 0xb26d, 0x125c, 0x192c, 0x9a95, 0x1a94, 0x1b4a, 0x4b55, 0xad4,
                0xf55b, 0x4ba, 0x125a, 0xb92b, 0x152a, 0x1694, 0x96aa, 0x15aa, 0x12ab5, 0x974, 0x14b6, 0xca57, 0xa56, 0x1526,
                0x8e95, 0xd54, 0x15aa, 0x49b5, 0x96c, 0xd4ae, 0x149c, 0x1a4c, 0xbd26, 0x1aa6, 0xb54, 0x6d6a, 0x12da, 0x1695d,
                0x95a, 0x149a, 0xda4b, 0x1a4a, 0x1aa4, 0xbb54, 0x16b4, 0xada, 0x495b, 0x936, 0xf497, 0x1496, 0x154a, 0xb6a5,
                0xda4, 0x15b4, 0x6ab6, 0x126e, 0x1092f, 0x92e, 0xc96, 0xcd4a, 0x1d4a, 0xd64, 0x956c, 0x155c, 0x125c, 0x792e,
                0x192c, 0xfa95, 0x1a94, 0x1b4a, 0xab55, 0xad4, 0x14da, 0x8a5d, 0xa5a, 0x1152b, 0x152a, 0x1694, 0xd6aa,
                0x15aa, 0xab4, 0x94ba, 0x14b6, 0xa56, 0x7527, 0xd26, 0xee53, 0xd54, 0x15aa, 0xa9b5, 0x96c, 0x14ae, 0x8a4e,
                0x1a4c, 0x11d26, 0x1aa4, 0x1b54, 0xcd6a, 0xada, 0x95c, 0x949d, 0x149a, 0x1a2a, 0x5b25, 0x1aa4, 0xfb52,
                0x16b4, 0xaba, 0xa95b, 0x936, 0x1496, 0x9a4b, 0x154a, 0x136a5, 0xda4, 0x15ac
        };

static int solar_1_1[] =
        {
                1887, 0xec04c, 0xec23f, 0xec435, 0xec649, 0xec83e, 0xeca51, 0xecc46, 0xece3a,
                0xed04d, 0xed242, 0xed436, 0xed64a, 0xed83f, 0xeda53, 0xedc48, 0xede3d, 0xee050, 0xee244, 0xee439, 0xee64d,
                0xee842, 0xeea36, 0xeec4a, 0xeee3e, 0xef052, 0xef246, 0xef43a, 0xef64e, 0xef843, 0xefa37, 0xefc4b, 0xefe41,
                0xf0054, 0xf0248, 0xf043c, 0xf0650, 0xf0845, 0xf0a38, 0xf0c4d, 0xf0e42, 0xf1037, 0xf124a, 0xf143e, 0xf1651,
                0xf1846, 0xf1a3a, 0xf1c4e, 0xf1e44, 0xf2038, 0xf224b, 0xf243f, 0xf2653, 0xf2848, 0xf2a3b, 0xf2c4f, 0xf2e45,
                0xf3039, 0xf324d, 0xf3442, 0xf3636, 0xf384a, 0xf3a3d, 0xf3c51, 0xf3e46, 0xf403b, 0xf424e, 0xf4443, 0xf4638,
                0xf484c, 0xf4a3f, 0xf4c52, 0xf4e48, 0xf503c, 0xf524f, 0xf5445, 0xf5639, 0xf584d, 0xf5a42, 0xf5c35, 0xf5e49,
                0xf603e, 0xf6251, 0xf6446, 0xf663b, 0xf684f, 0xf6a43, 0xf6c37, 0xf6e4b, 0xf703f, 0xf7252, 0xf7447, 0xf763c,
                0xf7850, 0xf7a45, 0xf7c39, 0xf7e4d, 0xf8042, 0xf8254, 0xf8449, 0xf863d, 0xf8851, 0xf8a46, 0xf8c3b, 0xf8e4f,
                0xf9044, 0xf9237, 0xf944a, 0xf963f, 0xf9853, 0xf9a47, 0xf9c3c, 0xf9e50, 0xfa045, 0xfa238, 0xfa44c, 0xfa641,
                0xfa836, 0xfaa49, 0xfac3d, 0xfae52, 0xfb047, 0xfb23a, 0xfb44e, 0xfb643, 0xfb837, 0xfba4a, 0xfbc3f, 0xfbe53,
                0xfc048, 0xfc23c, 0xfc450, 0xfc645, 0xfc839, 0xfca4c, 0xfcc41, 0xfce36, 0xfd04a, 0xfd23d, 0xfd451, 0xfd646,
                0xfd83a, 0xfda4d, 0xfdc43, 0xfde37, 0xfe04b, 0xfe23f, 0xfe453, 0xfe648, 0xfe83c, 0xfea4f, 0xfec44, 0xfee38,
                0xff04c, 0xff241, 0xff436, 0xff64a, 0xff83e, 0xffa51, 0xffc46, 0xffe3a, 0x10004e, 0x100242, 0x100437,
                0x10064b, 0x100841, 0x100a53, 0x100c48, 0x100e3c, 0x10104f, 0x101244, 0x101438, 0x10164c, 0x101842, 0x101a35,
                0x101c49, 0x101e3d, 0x102051, 0x102245, 0x10243a, 0x10264e, 0x102843, 0x102a37, 0x102c4b, 0x102e3f, 0x103053,
                0x103247, 0x10343b, 0x10364f, 0x103845, 0x103a38, 0x103c4c, 0x103e42, 0x104036, 0x104249, 0x10443d, 0x104651,
                0x104846, 0x104a3a, 0x104c4e, 0x104e43, 0x105038, 0x10524a, 0x10543e, 0x105652, 0x105847, 0x105a3b, 0x105c4f,
                0x105e45, 0x106039, 0x10624c, 0x106441, 0x106635, 0x106849, 0x106a3d, 0x106c51, 0x106e47, 0x10703c, 0x10724f,
                0x107444, 0x107638, 0x10784c, 0x107a3f, 0x107c53, 0x107e48
};

typedef struct Lunar
{
    bool isleap;
    int lunarDay;
    int lunarMonth;
    int lunarYear;
} Lunar;

int GetBitInt(int data, int length, int shift) {
    return (data & (((1 << length) - 1) << shift)) >> shift;
}

long SolarToInt(int y, int m, int d) {
    m = (m + 9) % 12;
    y = y - m / 10;
    return 365 * y + y / 4 - y / 100 + y / 400 + (m * 306 + 5) / 10 + (d - 1);
}

Lunar SolarToLunar(int sday, int smonth, int syear) {
    Lunar lunar;
    int index = syear - solar_1_1[0];
    int data = (syear<< 9) | (smonth << 5) | (sday);
    int solar11 = 0;
    if (solar_1_1[index] > data) {
        index--;
    }
    solar11 = solar_1_1[index];
    int y = GetBitInt(solar11, 12, 9);
    int m = GetBitInt(solar11, 4, 5);
    int d = GetBitInt(solar11, 5, 0);
    long offset = SolarToInt(syear, smonth, sday) - SolarToInt(y, m, d);

    int days = lunar_month_days[index];
    int leap = GetBitInt(days, 4, 13);

    int lunarY = index + solar_1_1[0];
    int lunarM = 1;
    int lunarD;
    offset += 1;

    for (int i = 0; i < 13; i++) {
        int dm = GetBitInt(days, 1, 12 - i) == 1 ? 30 : 29;
        if (offset > dm) {
            lunarM++;
            offset -= dm;
        }
        else {
            break;
        }
    }
    lunarD = (int) (offset);
    lunar.lunarYear = lunarY;
    lunar.lunarMonth = lunarM;
    lunar.isleap = false;
    if (leap != 0 && lunarM > leap) {
        lunar.lunarMonth = lunarM - 1;
        if (lunarM == leap + 1) {
            lunar.isleap = true;
        }
    }

    lunar.lunarDay = lunarD;
    return lunar;
}



//WiFiManager
//Local intialization. Once its business is done, there is no need to keep it around
WiFiManager wifiManager;

void setup() {
  dht.setup(DHTPIN, DHTesp::DHTTYPE); // Connect DHT sensor 
  Serial.begin(115200);
  pinMode(BL_LED,OUTPUT); //BackLight
  if (! spitouch.begin()) {
    Serial.println("TouchSensor not found?");
  }

  // if using deep sleep, we can use the TOUCH's IRQ pin to wake us up
  if (DEEP_SLEEP) {
    pinMode(TOUCH_IRQ_PIN, INPUT_PULLUP); 
  }
  
  digitalWrite(BL_LED,HIGH); // backlight on

  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(3);
  tft.setFont(&ArialRoundedMTBold_14);
  ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  ui.setTextAlignment(CENTER);
  ui.drawString(160, 120, "Connecting to WiFi");
  
  // Uncomment for testing wifi manager, ony once
  //wifiManager.resetSettings();
  wifiManager.setAPCallback(configModeCallback);

  //or use this for auto generated name ESP + ChipID
  wifiManager.autoConnect();

  //Manual Wifi
  WiFi.begin("DemoAP", "demoap123456789");

  // OTA Setup
  String hostname(HOSTNAME);
  hostname += String(ESP.getChipId(), HEX);
  WiFi.hostname(hostname);
  ArduinoOTA.setHostname((const char *)hostname.c_str());
  ArduinoOTA.begin();
  SPIFFS.begin();
  
  //Uncomment if you want to update all internet resources
  //SPIFFS.format();

  // download images from the net. If images already exist don't download
  downloadResources();

  // load the weather information
  updateData();
}

long lastDrew = 0;
void loop() {
  if (USE_TOUCHSCREEN_WAKE) {     // determine in settings.h!
    
    // for AWAKE_TIME seconds we'll hang out and wait for OTA updates
    for (uint16_t i=0; i<AWAKE_TIME; i++  ) {
      // Handle OTA update requests
      ArduinoOTA.handle();
      delay(10000);
      yield();
    }

    // turn off the display and wait for a touch!
    // flush the touch buffer
//    while (spitouch.bufferSize() != 0) {
//      spitouch.getPoint();
//      Serial.print('.');
//      yield();
//    }
    
    Serial.println("Zzzz");
    digitalWrite(BL_LED,LOW);// backlight off  

    Serial.println("Touch detected!");
  
    // wipe screen & backlight on
    tft.fillScreen(ILI9341_BLACK);
    digitalWrite(BL_LED,HIGH); // backlight on
    updateData();
  } 
  else // "standard setup"
  {
    // Handle OTA update requests
    ArduinoOTA.handle();

    // Check if we should update the clock
    if (millis() - lastDrew > 2500 /*&& wunderground.getSeconds() == "00"*/) {
      drawTime();
      drawLocalTH();
      lastDrew = millis();
    }

    // Check if we should update weather information
    if (millis() - lastDownloadUpdate > 1000 * UPDATE_INTERVAL_SECS) {
      updateData();
      lastDownloadUpdate = millis();
    }
  }
}

// Called if WiFi has not been configured yet
void configModeCallback (WiFiManager *myWiFiManager) {
  ui.setTextAlignment(CENTER);
  tft.setFont(&ArialRoundedMTBold_14);
  tft.setTextColor(ILI9341_CYAN);
  ui.drawString(160, 28, "Wifi Manager");
  ui.drawString(160, 44, "Please connect to AP");
  tft.setTextColor(ILI9341_WHITE);
  ui.drawString(160, 60, myWiFiManager->getConfigPortalSSID());
  tft.setTextColor(ILI9341_CYAN);
  ui.drawString(160, 76, "To setup Wifi Configuration");
}

// callback called during download of files. Updates progress bar
void downloadCallback(String filename, int16_t bytesDownloaded, int16_t bytesTotal) {
  Serial.println(String(bytesDownloaded) + " / " + String(bytesTotal));

  int percentage = 100 * bytesDownloaded / bytesTotal;
  if (percentage == 0) {
    ui.drawString(160, 112, filename);
  }
  if (percentage % 5 == 0) {
    ui.setTextAlignment(CENTER);
    ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
    //ui.drawString(160, 125, String(percentage) + "%");
    ui.drawProgressBar(10, 125, 320 - 20, 15, percentage, ILI9341_WHITE, ILI9341_BLUE);
  }

}

// Download the bitmaps
void downloadResources() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setFont(&ArialRoundedMTBold_14);
  char id[5];
  for (int i = 0; i < 21; i++) {
    sprintf(id, "%02d", i);
    tft.fillRect(0, 100, 320, 18, ILI9341_BLACK);
    webResource.downloadFile("http://www.squix.org/blog/wunderground/" + wundergroundIcons[i] + ".bmp", wundergroundIcons[i] + ".bmp", _downloadCallback);
  }
  for (int i = 0; i < 21; i++) {
    sprintf(id, "%02d", i);
    tft.fillRect(0, 100, 320, 18, ILI9341_BLACK);
    webResource.downloadFile("http://www.squix.org/blog/wunderground/mini/" + wundergroundIcons[i] + ".bmp", "/mini/" + wundergroundIcons[i] + ".bmp", _downloadCallback);
  }
  for (int i = 0; i < 23; i++) {
    tft.fillRect(0, 100, 320, 18, ILI9341_BLACK);
    webResource.downloadFile("http://www.squix.org/blog/moonphase_L" + String(i) + ".bmp", "/moon" + String(i) + ".bmp", _downloadCallback);
  }
}


String lastDate = "";
String lastTime = "";
String lastTemp="";
String lastHum="";

// Update the internet based information and update screen
void updateData() {
//  tft.fillScreen(ILI9341_BLACK);
//  tft.setFont(&ArialRoundedMTBold_14);
//  drawProgress(20, "Updating time...");
    timeClient.updateTime();
//
//  drawProgress(60, "Updating conditions...");
    OWMCclient.setLanguage(OPENWEATHERMAP_LANGUAGE);
    OWMCclient.setMetric(IS_METRIC);
// 
//  drawProgress(80, "Updating conditions...");
    OWMCclient.updateCurrent(&OWMCdata, OPENWEATHERMAP_APP_ID, OPENWEATHERMAP_CITY+","+OPENWEATHERMAP_COUNTRY);
//
    OWMFclient.setMetric(IS_METRIC);
    OWMFclient.setLanguage(OPENWEATHERMAP_LANGUAGE);
    OWMFclient.setAllowedHours(allowedHours, 2);
    uint8_t foundForecasts = OWMFclient.updateForecasts(OWMFdata, OPENWEATHERMAP_APP_ID, OPENWEATHERMAP_CITY+","+OPENWEATHERMAP_COUNTRY, MAX_FORECASTS);
//
//  //lastUpdate = timeClient.getFormattedTime();
//  //readyForWeatherUpdate = false;
//  drawProgress(100, "Done...");
//  delay(1000);
  tft.fillScreen(ILI9341_BLACK);
    lastDate = "";
    lastTime = "";
    lastTemp = "";
    lastHum = "";
  drawTime();
  drawCurrentWeather();
  drawForecast();
  drawLocalTH();
  drawAstronomy();
}

// Progress bar helper
void drawProgress(uint8_t percentage, String text) {
  ui.setTextAlignment(CENTER);
  ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  tft.fillRect(0, 100, 320, 40, ILI9341_BLACK);
  ui.drawString(160, 112, text);
  ui.drawProgressBar(10, 125, 320 - 20, 15, percentage, ILI9341_WHITE, ILI9341_BLUE);
}

String int2str(int i) {
    String res="";
    while (i>0) {
        int t=i%10;
        i/=10;
        res=(char)(t+'0')+res;
    }
    return res;
}



// draws the clock
void drawTime() {
  time_t Ftime = OWMFdata[0].observationTime;
  ui.setTextAlignment(CENTER);
  ui.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setFont(&ArialRoundedMTBold_14);
  String CFtime = ctime(&Ftime);
  String Dte=CFtime.substring(0,10)+CFtime.substring(CFtime.length()-6);
  struct tm *gt;
  gt=gmtime(&Ftime);
  int syear=gt->tm_year, smonth=gt->tm_mon, sday=gt->tm_mday;
  Lunar lun=SolarToLunar(sday,smonth+1,syear+1900);
  Serial.println(lun.lunarDay);
  Serial.println(lun.lunarMonth);
  Serial.println(lun.lunarYear);
  String Luns=int2str(lun.lunarMonth)+"/"+int2str(lun.lunarDay);
  if (Dte != lastDate) {
    ui.drawString(155, 20, Dte);
    ui.drawString(30, 20, " Lunar: "+Luns);
    lastDate = Dte;
  }
  
  tft.setFont(&ArialRoundedMTBold_36);
  String time = timeClient.getHours() + ":" + timeClient.getMinutes();
  if (time != lastTime) {
    ui.drawString(160, 56, time);
    lastTime = time;
  }
  drawSeparator(65);
}

// draws current weather information
void drawCurrentWeather() {
  ui.setTextAlignment(LEFT);
  tft.setFont(&ArialRoundedMTBold_14);
  ui.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  String city = OWMCdata.cityName;
  ui.drawString(10, 50, city);
  
  // Weather Icon
  String weatherIcon = OWM_to_WU_Icon_Mapping(OWMCdata.icon);
  ui.drawBmp(weatherIcon + ".bmp", 0, 55);
  // Weather Text
  tft.setFont(&ArialRoundedMTBold_14);
  ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  ui.setTextAlignment(RIGHT);
  ui.drawString(220, 90, OWMCdata.main);
  tft.setFont(&ArialRoundedMTBold_36);
  ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  ui.setTextAlignment(RIGHT);
  String degreeSign = "F";
  if (IS_METRIC) {
    degreeSign = "C";
  }
  String dash = "-";
  String temp = String((int)(OWMCdata.tempMin+0.5)) + dash + String((int)(OWMCdata.tempMax+0.5)) + degreeSign;
  ui.drawString(220, 125, temp);
  drawSeparator(135);

}

// draws the three forecast columns
void drawForecast() {
  drawForecastDetail(5, 165, 2);
  drawForecastDetail(65, 165, 4);
  drawForecastDetail(125, 165, 6);
  drawForecastDetail(185, 165, 8);
  drawSeparator(165 + 65 + 10);
}

// helper for the forecast columns
void drawForecastDetail(uint16_t x, uint16_t y, uint8_t dayIndex) {
  ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  tft.setFont(&ArialRoundedMTBold_14);
  ui.setTextAlignment(CENTER);
  time_t Ftime = OWMFdata[dayIndex].observationTime;
  String CFtime = ctime(&Ftime);
  String day = CFtime.substring(4,10);
  //day.toUpperCase();
  ui.drawString(x + 25, y, day);

  ui.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  ui.drawString(x + 25, y + 14, String((int)(OWMFdata[dayIndex].tempMin+0.5)) + "|" + String((int)(OWMFdata[dayIndex].tempMax+0.5)));

  String weatherIcon = OWM_to_WU_Icon_Mapping(OWMFdata[dayIndex].icon);
  ui.drawBmp("/mini/" + weatherIcon + ".bmp", x, y + 15);
}



void drawLocalTH()
{
  float h = dht.getHumidity();
  float t = dht.getTemperature();

  String degC = "C";
  String percent = "%";
  String temp = String((int)t) + degC;
  String hum = String((int)h) + percent;
  String local ="Local";
  ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  tft.setFont(&ArialRoundedMTBold_14);
  ui.drawString(280, 150, local);
  tft.setFont(&ArialRoundedMTBold_36);
  if (temp != lastTemp) {
    ui.drawString(280, 185, temp);
    lastTemp = temp;
  }
  if (hum != lastHum) {
    ui.drawString(280, 225, hum);
    lastHum = hum;
  }
  // drawSeparator(300);


    String json = "{\"Temp\":" + String((int)t) + ", \"Humid\":" + String((int)h) + "}";
    Serial.println(json);
    
    HTTPClient http; //Declare object of class HTTPClient
    http.begin("http://192.168.88.100:8080/stationupdate?id=" + stationID); //Specify request destination
    http.addHeader("Content-Type", "application/json"); //Specify content-type header

    int httpCode = http.POST(json); //Send the request
    String payload = http.getString(); //Get the response payload
    Serial.println(httpCode); //Print HTTP return code
    Serial.println(payload); //Print request response payload
    http.end(); //Close connection
}

float JDN(int Y, int M, int D)
{ float jdn = (1461.0 * (Y + 4800.0 + (M -14.0)/12.0))/4.0 +(367.0 * (M -2.0 -12.0 * ((M -14.0)/12.0)))/12.0 -(3.0 * ((Y + 4900.0 + (M -14.0)/12.0)/100.0))/4.0 + D -32075.0;
  //--------------------------------exercise
  return jdn;
}

// draw moonphase and sunrise/set and moonrise/set
void drawAstronomy() {
  struct tm * timeinfo;
  String Moon ="Moon";
  ui.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.setFont(&ArialRoundedMTBold_14);
  ui.drawString(280, 20, Moon);
  time_t Ftime = OWMFdata[0].observationTime;
  timeinfo = localtime(&Ftime);  
  float jdn_months = (JDN(timeinfo->tm_year+1900, timeinfo->tm_mon+1, timeinfo->tm_mday)-JDN(1900, 1, 1)-2)/29.53;
  float moonphase_now = jdn_months - floor(jdn_months);
  int moonAgeImage = (int) (23.9 * moonphase_now);
  ui.drawBmp("/moon" + String(moonAgeImage) + ".bmp", 250, 30);
}

String OWM_to_WU_Icon_Mapping(String icon){
  if (icon.substring(0,2) == "01") return "sunny";
  if (icon.substring(0,2) == "02") return "partlycloudy";
  if (icon.substring(0,2) == "03") return "cloudy";
  if (icon.substring(0,2) == "04") return "mostlycloudy";
  if (icon.substring(0,2) == "09") return "chancerain";
  if (icon.substring(0,2) == "10") return "rain";
  if (icon.substring(0,2) == "11") return "tstorms";  
  if (icon.substring(0,2) == "13") return "snow";
  if (icon.substring(0,2) == "50") return "hazy";  
  return "unknown";
}

// if you want separators, uncomment the tft-line
void drawSeparator(uint16_t y) {
 //  tft.drawFastHLine(10, y, 240 - 2 * 10, 0x4228);
}
