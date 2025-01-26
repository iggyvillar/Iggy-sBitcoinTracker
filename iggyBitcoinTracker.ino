#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Wire.h>
#include <HTTPClient.h>
#include <NTPClient.h> 
#include <WiFiUdp.h>
#include <ArduinoJson.h>


#define OLED_WIDTH 128            //ssd1306 px width
#define OLED_HEIGHT 64            //ssd1306 px height
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C         //default for most ssd1306

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

const String apiURL = "http://api.coindesk.com/v1/bpi/currentprice/BTC.json";   //api URL to fetch current BTC price

const char* networkName = "";                     //replace with network ssid
const char* networkPassword = "";                 //replace with network password

WiFiClient client;      //to make wifi connections
HTTPClient http;        //for http get requests

String btcPrice = "Fetching BTC price...";            //variable to hold bitcoin price
SemaphoreHandle_t xMutex;                             //mutex to synchronize access to BTC price

void setup() {
  Serial.begin(115200);     //initialize serial communication

  //initialize oled display
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)){
    Serial.println(F("Unable to allocate SSD1306\n"));
    for(;;);    //stop program if oled initialization fails
  }

  display.clearDisplay();     //clear display
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println("Connecting to Wi-Fi...");
  display.display();          //display message

  xMutex = xSemaphoreCreateMutex();     //create mutex for synchronizing access to data

  //create tasks for different functionalaties
  xTaskCreatePinnedToCore(connectToWifi, "WiFiTask", 4096, NULL, 1, NULL, 0);     //task for connecting to wifi
  xTaskCreatePinnedToCore(getBTCPrice, "APITask", 4096, NULL, 2, NULL, 1);        //task for fetching updated BTC price
  xTaskCreatePinnedToCore(updateDisplay, "DisplayTask", 4096, NULL, 1, NULL, 1);  //task for updating oled display
}

void loop() { 
}

//function to connect to wifi
void connectToWifi(void* parameter) {
  display.clearDisplay();                                         //clear display
  display.println("Connecting to Wi-Fi...");
  display.display();                                              //display previous line on oled

  WiFi.begin(networkName, networkPassword);                       //begin wifi connection
  while (WiFi.status() != WL_CONNECTED) {                         //wait until wifi is connected
    delay(500);
    Serial.print(".");
  }

  Serial.println("Wi-Fi Connected");                              //print success message in serial terminal
  display.clearDisplay();                                         //clear display on oled
  display.println("Wi-Fi Connected");
  display.display();                                              //print success message on SSD1306
  delay(1000);                                                    //delay before deleting task

  vTaskDelete(NULL);                                              //delete this task after Wi-Fi connection
}

//task for fetching current BTC price
void getBTCPrice(void* parameter) {
  while (true) {
    if (WiFi.status() == WL_CONNECTED) {                                //check if wifi is connected
      http.begin(apiURL);                                               //initialize http request
      int httpCode = http.GET();                                        //get request
      if (httpCode == 200) {                                            //if response code is 200 indicating success
        String payload = http.getString();                              //
        StaticJsonDocument<2000> doc;
        DeserializationError error = deserializeJson(doc, payload);     //parse json
        if (!error) {                                                   //check json parsing status
          String price = doc["bpi"]["USD"]["rate"].as<String>();        //extract BTC price
          xSemaphoreTake(xMutex, portMAX_DELAY);                        //take mutex to access btc price
          btcPrice = "$" + price;                                       //update var
          xSemaphoreGive(xMutex);                                        //release mutex
        }
      }
      http.end();                                                        //end http req
    }
    vTaskDelay(10000 / portTICK_PERIOD_MS);                              //wait 10 seconds until next fetch
  }
}


//function for updating display 
void updateDisplay(void* parameter) {
  while (true) {
    xSemaphoreTake(xMutex, portMAX_DELAY);              //take mutex for safe access to btc price  
    String price = btcPrice;                            //get latest BTC price
    xSemaphoreGive(xMutex);                             //release mutex

    display.clearDisplay();                                                 //clear display
    display.setTextSize(1);
    displayCentered("BTC/USD", 0, 0);
    displayCentered(price, 0, 25);                                          //display BTC price
    displayCentered("IGGY'S ESP32 PROJECT", 0, 55);                    //display project name
    display.display();

    vTaskDelay(1000 / portTICK_PERIOD_MS);                              //update every second
  }
}

void displayCentered(const String txBuffer, int x, int y){
  int16_t xCoord, yCoord;
  uint16_t width, height; 

  //calc bounding box of string at x and y positions
  display.getTextBounds(txBuffer, x, y, &xCoord, &yCoord, &width, &height);  
  display.setCursor((x - width / 2) + (128 / 2), y);    //center string horizontally by subtracting half screen width 
  display.print(txBuffer);

}
