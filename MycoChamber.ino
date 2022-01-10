#include <stdio.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_AHTX0.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include "ArduinoJson.h"
#include "AsyncJson.h"

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 32
#define OLED_RESET     4
#define SCREEN_ADDRESS 0x3C
#define INCLUDE_vTaskSuspend 1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Adafruit_AHTX0 aht;

AsyncWebServer server(80);

TaskHandle_t UpdateLCD;
TaskHandle_t AHTData;
TaskHandle_t FanFAE;
TaskHandle_t LightTask;
TaskHandle_t MisterControl;

static QueueHandle_t humQueue = xQueueCreate( 10, sizeof( float ) );//humidity queue
static QueueHandle_t humQueue2 = xQueueCreate( 1, sizeof( float ) );//humidity queue
static QueueHandle_t humQueue3 = xQueueCreate( 10, sizeof( float ) );//humidity queue
static QueueHandle_t tempQueue = xQueueCreate( 100, sizeof( float ) );//temperature queue

const char* ssid = "YOUR WIFI SSID HERE";
const char* password = "YOUR WIFI PASSWORD HERE";
//for /timer
const char* PARAM_INPUT_1 = "FAEon";
const char* PARAM_INPUT_2 = "FAEoff";
const char* PARAM_INPUT_3 = "LEDon";
const char* PARAM_INPUT_4 = "LEDoff";
const char* PARAM_INPUT_5 = "MisterOn";
const char* PARAM_INPUT_6 = "MisterOff";
const char* PARAM_INPUT_7 = "state";
 
volatile boolean restart = 0;
//pins
const int fae1 = 16;
const int fae2 = 17;
const int light1 = 32;
const int light2 = 33;
const int mister = 25;
const int mistFan = 26;

//timers
volatile int faeFanOn = 8000;
volatile int faeFanOff = 60000*10;
volatile int lightOn = 3600 * 1000 * 12;
volatile int lightOff= 3600 * 1000 * 12;
volatile int misterOn = 93;
volatile int misterOff = 95;

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

String outputState(){
  if(restart){
    return "checked";
  }
  else {
    return "";
  }
}

String processor(const String& var){
  //Serial.println(var);
  if(var == "CHECKBOX"){
    String buttons = "";
    buttons += "Restart Task Upon Value Change: <label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"restart\" " + outputState() + "></label>";
    return buttons;
  }else if(var == "TEMPERATURE"){
    float temperature = 0;
    xQueuePeek(tempQueue, &temperature, portMAX_DELAY);
    return String(temperature);
  }else if(var == "HUMIDITY"){
    float humidity = 0;
    xQueueReceive(humQueue2, &humidity, portMAX_DELAY);
    return String(humidity);
  }else if(var == "ONFAE"){
    return String(faeFanOn);
  }else if(var == "OFFFAE"){
    return String(faeFanOff);
  }else if(var == "ONLED"){
    return String(lightOn);
  }else if(var == "OFFLED"){
    return String(lightOff);
  }else if(var == "ONHUM"){
    return String(misterOn);
  }else if(var == "OFFHUM"){
    return String(misterOff);
  }
  
  return String();
}


void setup() {  
  Serial.begin(115200);
  // put your setup code here, to run once:
  pinMode(mister, OUTPUT);
  pinMode(mistFan, OUTPUT);
  pinMode(fae1, OUTPUT);
  pinMode(fae2, OUTPUT);
  pinMode(light1, OUTPUT);
  pinMode(light2, OUTPUT);

  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  aht.begin();
  
  display.display();
  display.clearDisplay();  
  
  
  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Not connected to WIFI");
    return;
  }
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  server.on("/mystyle.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/mystyle.css","text/css");
  });

  server.on("/timer", HTTP_POST, [](AsyncWebServerRequest *request){
    String inputMessage1 = request->getParam(PARAM_INPUT_1)->value();
    String inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
    String inputMessage3 = request->getParam(PARAM_INPUT_3)->value();
    String inputMessage4 = request->getParam(PARAM_INPUT_4)->value();
    String inputMessage5 = request->getParam(PARAM_INPUT_5)->value();
    String inputMessage6 = request->getParam(PARAM_INPUT_6)->value();

    if(inputMessage1 != ""){
      faeFanOn = inputMessage1.toInt();
      if(restart){
        vTaskDelete(FanFAE);
        xTaskCreate(FanFAETask, "FAE Fan Control", 3000, NULL, 1, &FanFAE);
        Serial.print("Change FAE ON: ");
        Serial.println(faeFanOn);
      }
    }
    if(inputMessage2 != ""){
      faeFanOff = inputMessage2.toInt();
      if(restart){
        vTaskDelete(FanFAE);
        xTaskCreate(FanFAETask, "FAE Fan Control", 3000, NULL, 1, &FanFAE);
        Serial.print("Change FAE OFF: ");
        Serial.println(faeFanOff);
      }
    }
    if(inputMessage3 != ""){
      lightOn = inputMessage3.toInt();
      if(restart){
        vTaskDelete(LightTask);
        xTaskCreate(LightControl, "LED Light Control", 3000, NULL, 1, &LightTask);
        Serial.print("Change LED ON: ");
        Serial.println(lightOn);
      }
    }
    if(inputMessage4 != ""){
      lightOff = inputMessage4.toInt();
      if(restart){
        vTaskDelete(LightTask);
        xTaskCreate(LightControl, "LED Light Control", 3000, NULL, 1, &LightTask);
        Serial.print("Change LED OFF: ");
        Serial.println(lightOff);
      }
    }
    if(inputMessage5 != ""){
      misterOn = inputMessage5.toInt();
      if(restart){
        vTaskDelete(MisterControl);
        xTaskCreate(MistControlTask, "Control Mister", 10000, NULL, 1, &MisterControl);
        Serial.print("Change MISTER ON: ");
        Serial.println(misterOn);
      }
    }
    if(inputMessage6 != ""){
      misterOff = inputMessage6.toInt();
      if(restart){
        vTaskDelete(MisterControl);
        xTaskCreate(MistControlTask, "Control Mister", 10000, NULL, 1, &MisterControl);
        Serial.print("Change MISTER OFF: ");
        Serial.println(misterOff);
      }
    }
    String newValues;
    StaticJsonDocument<200> doc;
    doc["newON1"] = faeFanOn;
    doc["newOFF2"] = faeFanOff;
    doc["newON3"] = lightOn;
    doc["newOFF4"] = lightOff;
    doc["newON5"] = misterOn;
    doc["newOFF6"] = misterOff;
    serializeJson(doc, newValues);
    request->send(200, "text/plain", newValues.c_str());
  });

  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(PARAM_INPUT_7)) {
      inputMessage = request->getParam(PARAM_INPUT_7)->value();
      restart = inputMessage.toInt();
    }
    Serial.print("restart: ");
    Serial.print(restart);
    Serial.println();
    request->send(200, "text/plain", "OK");
  });
  
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    float temp = 0;
    xQueuePeek(tempQueue, &temp, portMAX_DELAY);
    request->send_P(200, "text/plain", String(temp).c_str());
  });
  
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    float humidity = 0;
    xQueuePeek(humQueue2, &humidity, portMAX_DELAY);
    request->send_P(200, "text/plain", String(humidity).c_str());
  });
  server.onNotFound(notFound);
  server.begin();

  xTaskCreate(UpdateLCDTask, "Update LCD", 10000, NULL, 1 , &UpdateLCD);
  xTaskCreate(GetAHTData, "AHT Data Acquisition", 10000, NULL, 1 , &AHTData);
  xTaskCreate(MistControlTask, "Control Mister", 10000, NULL, 1, &MisterControl);
  xTaskCreate(FanFAETask, "FAE Fan Control", 3000, NULL, 1, &FanFAE);
  xTaskCreate(LightControl, "LED Light Control", 3000, NULL, 1, &LightTask);
}

void loop() {

}

void UpdateLCDTask(void *parameter) {
  while (1) {
    float humidity = 0;
    float temp = 0;
    xQueueReceive(humQueue, &humidity, portMAX_DELAY);
    xQueueReceive(tempQueue, &temp, portMAX_DELAY);
    String t = "T: " + String(temp) +  (char)247 + "C";
    String hum = "H: " + String(humidity) + "%";
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println(hum);
    display.println(t);
    display.display();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
  }
}

void GetAHTData(void *parameter) {
  while (1) {
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);

    float hum = humidity.relative_humidity;
    float tem =  temp.temperature;

    xQueueSend(humQueue, &hum, portMAX_DELAY);
    xQueueOverwrite(humQueue2, &hum);
    xQueueSend(humQueue3, &hum, portMAX_DELAY);
    xQueueSend(tempQueue, &tem, portMAX_DELAY);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

void MistControlTask(void *parameter) {
  Serial.println("START MISTCONTROLTASK");
  while (1) {

    float humidity = 0;
    xQueueReceive(humQueue3, &humidity, portMAX_DELAY);

    if (humidity <= misterOn) {
      digitalWrite(mister, LOW);//turn on mister
      digitalWrite(mistFan, LOW);//turn on mister fan
    }

    if (humidity >= misterOff) {
      digitalWrite(mister, HIGH);//turn off mister
      digitalWrite(mistFan, HIGH);//turn off mister fan
    }

    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}


void FanFAETask(void *parameter) {
  Serial.println("START FANFAETASK");
  while (1) {
    
    digitalWrite(fae1, HIGH);//turn off FAE fan
    digitalWrite(fae2, HIGH);
    vTaskDelay(faeFanOff / portTICK_PERIOD_MS);
    digitalWrite(fae1, LOW);//turn on FAE fan
    digitalWrite(fae2, LOW);
    vTaskDelay(faeFanOn / portTICK_PERIOD_MS);
    
  }
}

void LightControl(void *parameter) { 
  Serial.println("START LIGHTCONTROL");
  while(1){

    digitalWrite(light1, LOW);//turn on light
    digitalWrite(light2, LOW);
    vTaskDelay(lightOn / portTICK_PERIOD_MS);
    digitalWrite(light1, HIGH);//turn off light
    digitalWrite(light2, HIGH);
    vTaskDelay(lightOff / portTICK_PERIOD_MS);
    
  }
}
