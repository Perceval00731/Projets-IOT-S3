#include <M5Stack.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>

const char *ssid = "Ekip";
const char *password = "Romswifi09_";
HTTPClient client;
float previousPrice = 0.0;

void setup() { 
  M5.begin(); // Init M5Core. Initialize M5Core
  M5.Power.begin(); // Init Power module. Initialize the power module
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(3);
  M5.Lcd.println("Allumage"); 
  Serial.begin(115200); 

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    M5.Lcd.println(".");
  }
  M5.Lcd.println("\nConnected to WiFi");
  //M5.Lcd.print("IP address: ");
  //M5.Lcd.println(WiFi.localIP());
}

void loop() {
  client.begin("https://api.coindesk.com/v1/bpi/currentprice.json");
  int httpCode = client.GET();
  M5.Lcd.println(httpCode);
  String machin = client.getString();
  StaticJsonDocument<1024> doc;

  DeserializationError error = deserializeJson(doc, machin);

  if (error) {
    M5.Lcd.println("deserializeJson() failed: ");
    M5.Lcd.println(error.c_str());
    return;
  }

  float price = doc["bpi"]["EUR"]["rate_float"];
  
  if (price > previousPrice) {
    M5.Lcd.setTextColor(GREEN);
  } else {
    M5.Lcd.setTextColor(RED);
  }

  M5.Lcd.fillScreen(BLACK); // Clear the screen before displaying new price
  M5.Lcd.setCursor(0, 0); // Reset cursor position
  M5.Lcd.print("Bitcoin price: ");
  M5.Lcd.println(price);

  float variation = ((price - previousPrice) / previousPrice) * 100;
  previousPrice = price;

  delay(60000); // Wait for 1 minute before updating the price again
  M5.Lcd.print("variation: ");
  M5.Lcd.println(variation);
  
}