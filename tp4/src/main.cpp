// modifie la couleur de l'écran en rouge si on reçoit un taux de CO2 supérieur à 1000 ppm
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <M5Stack.h>
#include <SCD30.h>
#include <Arduino.h>

// Identifiants LoRaWAN
static const u1_t PROGMEM APPEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x06 };
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

static const u1_t PROGMEM DEVEUI[8] = { 0x06, 0x01, 0x03, 0x02, 0x01, 0x45, 0x7e, 0x0c };
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

static const u1_t PROGMEM APPKEY[16] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x01, 0x06 };
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

static osjob_t sendjob;
const unsigned TX_INTERVAL = 20; // Intervalle de transmission de 20 secondes

// Pin mapping pour M5Stack avec module LoRa
const lmic_pinmap lmic_pins = {
  .nss = 5,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 26,
  .dio = {36, 35, LMIC_UNUSED_PIN},
};

SCD30 airSensor;

void printHex2(unsigned v) {
    v &= 0xff;
    if (v < 16)
        Serial.print('0');
    Serial.print(v, HEX);
}

bool isBase64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

// Fonction de décodage base64
String base64_decode(String input) {
    const char* base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    int in_len = input.length();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    String ret;

    while (in_len-- && ( input[in_] != '=') && isBase64(input[in_])) {
        char_array_4[i++] = input[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = strchr(base64_chars, char_array_4[i]) - base64_chars;

            char_array_3[0] = ( char_array_4[0] << 2       ) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) +   char_array_4[3];

            for (i = 0; (i < 3); i++)
                ret += char_array_3[i];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 4; j++)
            char_array_4[j] = 0;

        for (j = 0; j < 4; j++)
            char_array_4[j] = strchr(base64_chars, char_array_4[j]) - base64_chars;

        char_array_3[0] = ( char_array_4[0] << 2       ) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) +   char_array_4[3];

        for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
    }

    return ret;
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

            // Envoyer le paquet
            LMIC_setTxData2(1, mydata, sizeof(mydata), 0);
            Serial.println(F("Packet queued"));
        } else {
            Serial.println(F("SCD30 data not available"));
        }
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void do_if_data_received() {
    if (LMIC.dataLen) {
        // Exemple de décodage des données reçues
        uint16_t co2 = (LMIC.frame[LMIC.dataBeg + 0] << 8) | LMIC.frame[LMIC.dataBeg + 1];
        uint16_t temp = (LMIC.frame[LMIC.dataBeg + 2] << 8) | LMIC.frame[LMIC.dataBeg + 3];
        uint16_t hum = (LMIC.frame[LMIC.dataBeg + 4] << 8) | LMIC.frame[LMIC.dataBeg + 5];

        float temperature = temp / 100.0;
        float humidity = hum / 100.0;

        Serial.print(F("Received "));
        Serial.print(LMIC.dataLen);
        Serial.println(F(" bytes of payload"));

        Serial.print(F("le CO2 est de: "));
        Serial.print(co2);
        Serial.println(F(" ppm"));

        Serial.print(F("la temperature est de: "));
        Serial.print(temperature);
        Serial.println(F(" °C"));

        Serial.print(F("l'humidite est de: "));
        Serial.print(humidity);
        Serial.println(F(" %"));


        if(co2 > 1000){
            M5.Lcd.fillScreen(TFT_RED);
            Serial.println(F("CO2 > 1000 ppm"));
        }
    }
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
            do_if_data_received(); // Appeler la fonction pour traiter les données reçues
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
    Serial.begin(115200);
    M5.begin();
    M5.Power.begin();
    Serial.println(F("Starting"));
    while (millis() < 4000) {
        Serial.print("millis() = "); Serial.println(millis());
        delay(500);
    }

    Wire.begin();
    while (!airSensor.isAvailable()) {
        Serial.println("SCD30 not detected. Please check wiring.");
        delay(1000); // Attendre 1 seconde avant de réessayer
    }
    Serial.println("SCD30 detected.");

    #ifdef VCC_ENABLE
    pinMode(VCC_ENABLE, OUTPUT);
    digitalWrite(VCC_ENABLE, HIGH);
    delay(1000);
    #endif

    os_init();
    LMIC_reset();
    do_send(&sendjob);
}

void loop() {
    os_runloop_once();
}


// pour envoyer un message à l'appareil qui mettra l'écran en rouge
//mosquitto_pub -h chirpstack.iut-blagnac.fr -t application/6/device/0c7e450102030106/command/down -m '{"confirmed":false,"fPort":5,"data":"c2hlZWVzaCB0aG9tYXM="}'
