#include <M5Stack.h> 
#include <HTTPClient.h>
#include <WiFi.h>
#include <SCD30.h>

uint64_t i = 0; 
uint64_t previousMillis = 0;
TFT_eSprite disp_buffer = TFT_eSprite(&M5.Lcd);


void setup() { 
  M5.begin(); // Init M5Core
  M5.Power.begin(); // Init Power module
  M5.Lcd.setTextColor(RED);
  M5.Lcd.setTextSize(3);
  Serial.begin(115200); 
  disp_buffer.createSprite(240, 200); 
  
  disp_buffer.setTextColor(TFT_ORANGE, TFT_BLACK);
  
  disp_buffer.setTextSize(1);
  
  disp_buffer.drawString("Bouton A: Temperature", 0, 0, 4);
  disp_buffer.drawString("Bouton B: Humidite", 0, 50, 4);
  disp_buffer.drawString("Bouton C: Taux de CO2", 0, 100, 4);
  
  disp_buffer.pushSprite(0, 0); 
  

}



void loop() { 
  static float result[3] = {0};

  if (millis()-previousMillis>6000){
    if(scd30.isAvailable()){
      scd30.getCarbonDioxideConcentration(result);
      previousMillis=millis();
    } 
  }

  M5.update();

  if (M5.BtnA.isPressed()) {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.print("Temperature : ");
    M5.Lcd.println(result[1]);
  }

  if (M5.BtnB.isPressed()) {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.print("Humidite : ");
    M5.Lcd.println(result[2]);
  }

  if (M5.BtnC.isPressed()) {
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.print("Taux de CO2 : ");
      M5.Lcd.println(result[0]);
  }
}