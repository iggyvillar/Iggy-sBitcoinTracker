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
#define HTTPS_PORT 443

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

const String apiURL = "http://api.coindesk.com/v1/bpi/currentprice/BTC.json";

const char* networkName = "";                     //replace with network name
const char* networkPassword = "";                 //replace with network password

WiFiClient client;      //to make wifi connections
HTTPClient http;        //for http get requests


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
  display.display();

  //connect to wifi
  WiFi.begin(networkName, networkPassword);
  Serial.print("Connecting to Network");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("."); // Print dots while waiting for connection.
  }
  Serial.println();

  //show connection success
  display.println("Connected to ");
  display.print(networkName);
  display.display();
  delay(1500);
  display.clearDisplay();

}

void loop() {
  Serial.print("Connecting to");
  Serial.println(apiURL);

  //GET request for current BTC price in USD
  http.begin(apiURL);
  int httpCode = http.GET();      //perform GET request
  StaticJsonDocument<2000> JSONDOC;
  DeserializationError error = deserializeJson(JSONDOC, http.getString());
  //JSON parsing handling
  if (error){
    Serial.print(F("Json parsing failed"));
    Serial.println(error.f_str());
    delay(2500);
    return;
  }

  //send status code to serial monitor for debugging
  Serial.print("HTTP Status Code:");
  Serial.println(httpCode);

  //store BTC price in string
  String currentBTCprice = JSONDOC["bpi"]["USD"]["rate_float"].as<String>();
  http.end();

  //display results on oled

  //display BTC header
  display.clearDisplay();


  display.setTextSize(1);
  displayCentered("BTC/USD", 0, 0);

  //display current price of BTC
  displayCentered("$" + currentBTCprice, 0, 25);

  //display project name
  display.setTextSize(1);
  displayCentered("IGGY'S ESP32 PROJECT", 0, 55); //change text here

  display.display();
  http.end();
  delay(10000);
}

void displayCentered(const String txBuffer, int x, int y){
  int16_t xCoord, yCoord;
  uint16_t width, height; 

  //calc bounding box of string at x and y positions
  display.getTextBounds(txBuffer, x, y, &xCoord, &yCoord, &width, &height);  
  display.setCursor((x - width / 2) + (128 / 2), y);    //center string horizontally by subtracting half screen width 
  display.print(txBuffer);

}
