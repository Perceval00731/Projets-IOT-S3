#include <M5Stack.h> 
#include <HTTPClient.h>
#include <WiFi.h>
#include <SCD30.h>

uint8_t i = 0; 
uint8_t previousMillis = 0;

void setup() { 
  M5.begin(); // Init M5Core
  M5.Power.begin(); // Init Power module
  M5.Lcd.setTextColor(RED);
  M5.Lcd.setTextSize(3);
  Serial.begin(115200); 
  
  if (scd30.isAvailable()) {
    float result[3] = {0};
    scd30.getCarbonDioxideConcentration(result);
    
    
    TFT_eSprite disp_buffer = TFT_eSprite(&M5.Lcd);

    disp_buffer.createSprite(240, 200); // Sprite 1

    disp_buffer.setTextColor(TFT_ORANGE, TFT_BLACK);

    disp_buffer.setTextSize(1); // Taille du texte

    //disp_buffer.drawString("CO2 : " + String(result[0]) + " ppm", 0, 0, 4);
    //disp_buffer.drawString("Temperature : " + String(result[1]) + " °C", 0, 50, 4);
    //disp_buffer.drawString("Humidity : " + String(result[2]) + " %", 0, 100, 4);


    disp_buffer.drawString("Bouton A: Temperature ", 0, 0, 4);
    disp_buffer.drawString("Bouton B: Humidite", 0, 50, 4);
    disp_buffer.drawString("Bouton C: Taux de CO2", 0, 100, 4);


    disp_buffer.pushSprite(0, 0); // Affiche le sprite

  }

}

void loop() { 
  if (millis() - previousMillis > 1000) {
    
    M5.update();

    if (M5.BtnA.isPressed()) {
      if (scd30.isAvailable()) {
        float result[3] = {0};
        scd30.getCarbonDioxideConcentration(result);
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.print("Temperature : ");
        M5.Lcd.println(result[1]);
      }
    }

    if (M5.BtnB.isPressed()) {
      if (scd30.isAvailable()) {
        float result[3] = {0};
        scd30.getCarbonDioxideConcentration(result);
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.print("Humidite : ");
        M5.Lcd.println(result[2]);
      }
    }

    if (M5.BtnC.isPressed()) {
      if (scd30.isAvailable()) {
        float result[3] = {0};
        scd30.getCarbonDioxideConcentration(result);
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.print("Taux de CO2 : ");
        M5.Lcd.println(result[0]);
      }
    }
  }
}