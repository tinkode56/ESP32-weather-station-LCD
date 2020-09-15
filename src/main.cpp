#include <Arduino.h>

#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>
#include "soc/timer_group_reg.h"
#include "soc/timer_group_struct.h"

#include "ArduinoJson.h"
#include "WiFi.h"
#include "HTTPClient.h"

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library with default width and height

TaskHandle_t RenderingTask;
TaskHandle_t NetworkTask;

const char *ssid = "DIGI-3C77";
const char *pass = "9V57jNUn";
String serverURL = "http://api.openweathermap.org";
String APIKey = "6a9c5295c0af1d4e99e9fd9f5832b778";
String response = "{\"coord\":{\"lon\":27.6,\"lat\":47.17},\"weather\":[{\"id\":800,\"main\":\"Clear\",\"description\":\"clear sky\",\"icon\":\"01n\"}],\"base\":\"stations\",\"main\":{\"temp\":21.61,\"feels_like\":18.39,\"temp_min\":20,\"temp_max\":22.22,\"pressure\":1027,\"humidity\":43},\"visibility\":10000,\"wind\":{\"speed\":4.1,\"deg\":330},\"clouds\":{\"all\":0},\"dt\":1600112219,\"sys\":{\"type\":1,\"id\":6916,\"country\":\"RO\",\"sunrise\":1600055171,\"sunset\":1600100654},\"timezone\":10800,\"id\":675810,\"name\":\"Iasi\",\"cod\":200}";

typedef struct
{
  float lon;            //degrees
  float lat;            //degrees
  String description;   //weather condition
  float temp;           //celsius
  float feels_like;     //celsius
  float temp_min;       //celsius
  float temp_max;       //celsius
  float pressure;       //hPa
  float humidity;       //%
  float visibility;     //
  float windSpeed;      //meters/second
  float windDeg;        //degrees
  float clouds;         //%
  String country;       //country
  String name;          //city name
} sWeatherInfo;

sWeatherInfo weather;

void feedTheDog();
void RenderingTaskFunc(void *pvParameters);
void NetworkTaskFunc(void *pvParameters);
void DeserializeJson(String jsonString, sWeatherInfo *weatherStruct);
void UpdateScreen(TFT_eSPI *tft, sWeatherInfo *weather);

/*Update screen informations*/
void UpdateScreen(TFT_eSPI *tTft, sWeatherInfo *sWeather)
{
  tTft->setCursor(1, 1, 4);
  tTft->setTextFont(4);
  tTft->setTextColor(TFT_BLACK, TFT_LIGHTGREY);
  tTft->print("Weather " + sWeather->name);
  tTft->setCursor(241, 1, 4);
  tTft->setTextFont(4);
  tTft->setTextColor(TFT_BLACK, TFT_LIGHTGREY);
  tTft->print("Wind " + String(sWeather->windDeg) + "\n" + String(sWeather->windSpeed));  
}

/*Task for display rendering on CORE 0*/
void RenderingTaskFunc(void *pvParameters)
{

  tft.begin();
  tft.fillScreen(TFT_LIGHTGREY);
  tft.setRotation(1);
  tft.drawLine(240, 160, 480, 160, TFT_BLACK);
  tft.drawLine(240, 0, 240, 320, TFT_BLACK);
  delay(300);
  tft.setCursor(1, 1, 4);
  tft.setTextFont(4);
  tft.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
  tft.print("Weather " + weather.name);
  tft.setCursor(241, 1, 4);
  tft.setTextFont(4);
  tft.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
  tft.print("Wind " + String(weather.windDeg) + "\n" + String(weather.windSpeed));

  for (;;)
  {
    feedTheDog();
  }
}

/*Deserialize received JSON message*/
void DeserializeJson(String jsonString, sWeatherInfo *weatherStruct)
{
  const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + 2 * JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(13) + 260;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, jsonString);
  weatherStruct->lon = doc["coord"]["lon"];
  weatherStruct->lat = doc["coord"]["lat"];

  JsonObject weather_0 = doc["weather"][0];
  const char *weather_0_description = weather_0["description"];
  weatherStruct->description = String(weather_0_description);

  JsonObject main = doc["main"];
  weatherStruct->temp = main["temp"];
  weatherStruct->feels_like = main["feels_like"];
  weatherStruct->temp_min = main["temp_min"];       
  weatherStruct->temp_max = main["temp_max"];
  weatherStruct->pressure = main["pressure"];
  weatherStruct->humidity = main["humidity"];       

  weatherStruct->visibility = doc["visibility"];;
  weatherStruct->windSpeed = doc["wind"]["speed"];
  weatherStruct->windDeg = doc["wind"]["deg"];
  weatherStruct->clouds = doc["clouds"]["all"];

  JsonObject sys = doc["sys"];
  const char* sys_country = sys["country"];
  const char* name = doc["name"];
  weatherStruct->country = String(sys_country);
  weatherStruct->name = String(name);
}

/*Task for network communication on CORE 1*/
void NetworkTaskFunc(void *pvParameters)
{
  HTTPClient httpClient;
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println(WiFi.localIP());
  Serial.println();

  String finalURL = serverURL + "/data/2.5/weather?q=Iasi&units=metric&appid=" + APIKey;
  // httpClient.begin(finalURL);
  // int httpCode = httpClient.GET();

  // if(httpCode > 0 && httpCode == HTTP_CODE_OK) {
  //   String weatherInfo = httpClient.getString();
  //   Serial.println(weatherInfo);
  // }
  // httpClient.end();
  DeserializeJson(response, &weather);
  for (;;)
  {
    feedTheDog();
  }
}

void setup()
{
  Serial.begin(115200);
  xTaskCreatePinnedToCore(RenderingTaskFunc, "RenderingTsk", 10000, NULL, 1, &RenderingTask, 0);
  xTaskCreatePinnedToCore(NetworkTaskFunc, "NetworkTsk", 10000, NULL, 1, &NetworkTask, 1);
}

void loop(void)
{
}

void feedTheDog()
{
  // feed dog 0
  TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE; // write enable
  TIMERG0.wdt_feed = 1;                       // feed dog
  TIMERG0.wdt_wprotect = 0;                   // write protect
  // feed dog 1
  TIMERG1.wdt_wprotect = TIMG_WDT_WKEY_VALUE; // write enable
  TIMERG1.wdt_feed = 1;                       // feed dog
  TIMERG1.wdt_wprotect = 0;                   // write protect
}