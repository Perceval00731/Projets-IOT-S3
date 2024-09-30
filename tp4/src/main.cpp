#include <Arduino.h>
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <Wire.h>
#include <M5Stack.h>
#include <SCD30.h>

// Identifiants LoRaWAN (remplacez par vos propres valeurs)
static const u1_t PROGMEM APPEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

static const u1_t PROGMEM DEVEUI[8] = { 0x0c, 0x01, 0x03, 0x02, 0x01, 0x45, 0x7E, 0x0C};
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

static const u1_t PROGMEM APPKEY[16] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x01, 0x0c};
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

// Pin mapping pour M5Stack avec module LoRa
const lmic_pinmap lmic_pins = {
  .nss = 5,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 26,
  .dio = {36, 35, LMIC_UNUSED_PIN},
};

SCD30 airSensor;
static osjob_t sendjob;
const unsigned TX_INTERVAL = 300; // Intervalle de transmission de 5 minutes (300 secondes)

void printHex2(unsigned v) {
    v &= 0xff;
    if (v < 16)
        Serial.print('0');
    Serial.print(v, HEX);
}

void do_send(osjob_t* j) {
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.
        static float result[3] = {0};
        if (airSensor.isAvailable()) {
            airSensor.getCarbonDioxideConcentration(result);
            uint16_t co2 = (uint16_t)result[0]; // Convertir la concentration de CO2 en entier
            uint16_t temp = (uint16_t)(result[1] * 100); // Convertir la température en entier (en centièmes de degré)
            uint16_t hum = (uint16_t)(result[2] * 100); // Convertir l'humidité en entier (en centièmes de pourcentage)

            // Créer le message à envoyer
            uint8_t mydata[6];
            mydata[0] = highByte(co2);
            mydata[1] = lowByte(co2);
            mydata[2] = highByte(temp);
            mydata[3] = lowByte(temp);
            mydata[4] = highByte(hum);
            mydata[5] = lowByte(hum);

            // Convertir la chaîne de caractères en octets
            const char* texte = "yo le sang";
            uint8_t stringData[10];
            strncpy((char*)stringData, texte, sizeof(stringData));

            // Concaténer les deux tableaux d'octets
            uint8_t payload[16];
            memcpy(payload, mydata, sizeof(mydata));
            memcpy(payload + sizeof(mydata), stringData, sizeof(stringData));

            // Envoyer le paquet
            LMIC_setTxData2(1, payload, sizeof(payload), 0);
            Serial.println(F("Packet queued"));
        } else {
            Serial.println(F("SCD30 data not available"));
        }
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void onEvent(ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            {
              u4_t netid = 0;
              devaddr_t devaddr = 0;
              u1_t nwkKey[16];
              u1_t artKey[16];
              LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
              Serial.print("netid: ");
              Serial.println(netid, DEC);
              Serial.print("devaddr: ");
              Serial.println(devaddr, HEX);
              Serial.print("AppSKey: ");
              for (size_t i=0; i<sizeof(artKey); ++i) {
                if (i != 0)
                  Serial.print("-");
                printHex2(artKey[i]);
              }
              Serial.println("");
              Serial.print("NwkSKey: ");
              for (size_t i=0; i<sizeof(nwkKey); ++i) {
                      if (i != 0)
                              Serial.print("-");
                      printHex2(nwkKey[i]);
              }
              Serial.println();
            }
            LMIC_setLinkCheckMode(0);
            break;
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
            if (LMIC.dataLen) {
              Serial.print(F("Received "));
              Serial.print(LMIC.dataLen);
              Serial.println(F(" bytes of payload"));
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
        case EV_TXSTART:
            Serial.println(F("EV_TXSTART"));
            break;
        case EV_TXCANCELED:
            Serial.println(F("EV_TXCANCELED"));
            break;
        case EV_RXSTART:
            break;
        case EV_JOIN_TXCOMPLETE:
            Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
            break;
        default:
            Serial.print(F("Unknown event: "));
            Serial.println((unsigned) ev);
            break;
    }
}

void setup() {
    // Initialisation de la carte M5Stack
    M5.begin();
    M5.Power.begin();
    Serial.begin(115200); // Initialisation de la communication série
    M5.Lcd.setTextColor(RED);
    M5.Lcd.setTextSize(3);
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.print("Hello, world!");

    // Attendre 3 secondes avant de continuer
    while (millis() < 3000) {
        M5.Lcd.print("millis: ");
        M5.Lcd.println(millis());
        delay(100);
    }

    #ifdef VCC_ENABLE
    // Pour les cartes Pinoccio Scout
    pinMode(VCC_ENABLE, OUTPUT);
    digitalWrite(VCC_ENABLE, HIGH);
    delay(1000);
    #endif

    Wire.begin();
    while (!airSensor.isAvailable()) {
        Serial.println("SCD30 not detected. Please check wiring.");
        delay(1000); // Attendre 1 seconde avant de réessayer
    }
    Serial.println("SCD30 detected.");

    // Initialisation de LMIC
    os_init();
    LMIC_reset();

    // Démarrer la tâche (l'envoi démarre automatiquement OTAA aussi)
    do_send(&sendjob);
}

void loop() {
    os_runloop_once();
}