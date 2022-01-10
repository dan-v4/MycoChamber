#include <stdio.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_GFX.h>

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 32
#define OLED_RESET     4
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Adafruit_AHTX0 aht;

TaskHandle_t UpdateLCD;
TaskHandle_t AHTData;
TaskHandle_t LightTask;

static QueueHandle_t humQueue;
static QueueHandle_t tempQueue;


const int light1 = 32;
const int light2 = 33;
int lightOn = 3600 * 1000 * 12;
int lightOff= 3600 * 1000 * 12;


void setup() {
  // put your setup code here, to run once:
  pinMode(light1, OUTPUT);
  pinMode(light2, OUTPUT);

  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  aht.begin();
  
  display.display();
  display.clearDisplay();  
  
  humQueue = xQueueCreate( 100, sizeof( float ) );
  tempQueue = xQueueCreate( 100, sizeof( float ) );

  xTaskCreate(UpdateLCDTask, "Update LCD", 10000, NULL, 1 , &UpdateLCD);
  xTaskCreate(GetAHTData, "AHT Data Acquisition", 10000, NULL, 1 , &AHTData);
  xTaskCreate(LightControl, "LED Light Control", 3000, NULL, 1, &LightTask);
}

void loop() {
  // put your main code here, to run repeatedly:

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
    xQueueSend(tempQueue, &tem, portMAX_DELAY);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

void LightControl(void *parameter) { 
  while(1){

    digitalWrite(light1, LOW);//turn on light
    digitalWrite(light2, LOW);
    vTaskDelay(lightOn / portTICK_PERIOD_MS);
    digitalWrite(light1, HIGH);//turn off light
    digitalWrite(light2, HIGH);
    vTaskDelay(lightOff / portTICK_PERIOD_MS);
    
  }
}
