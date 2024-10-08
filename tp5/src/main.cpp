/*******************************************************************************
 * Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
 * Copyright (c) 2018 Terry Moore, MCCI
 * TP 4 : IoT IUT de Blagnac
 * uplink/downlink avec SC30
 * écran bleu sur mqqt publish
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * This example sends a valid LoRaWAN packet with payload "Hello,
 * world!", using frequency and encryption settings matching those of
 * the The Things Network.
 *
 * This uses OTAA (Over-the-air activation), where where a DevEUI and
 * application key is configured, which are used in an over-the-air
 * activation procedure where a DevAddr and session keys are
 * assigned/generated for use with all further communication.
 *
 * Note: LoRaWAN per sub-band duty-cycle limitation is enforced (1% in
 * g1, 0.1% in g2), but not the TTN fair usage policy (which is probably
 * violated by this sketch when left running for longer)!

 * To use this sketch, first register your application and device with
 * the things network, to set or generate an AppEUI, DevEUI and AppKey.
 * Multiple devices can use the same AppEUI, but each device has its own
 * DevEUI and AppKey.
 *
 * Do not forget to define the radio type correctly in
 * arduino-lmic/project_config/lmic_project_config.h or from your BOARDS.txt.
 *
 *******************************************************************************/
#include <M5Stack.h>

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

#include <SCD30.h>

uint8_t compteur = 0;

// Compteur pour attente non bloquante
// Va mesurer les données toutes les 5 sec
#define DUREE_ATTENTE 5000
int debut = 0;


// This EUI must be in little-endian format, so least-significant-byte
// first. When copying an EUI from ttnctl output, this means to reverse
// the bytes. For TTN issued EUIs the last bytes should be 0xD5, 0xB3,
// 0x70.
static const u1_t PROGMEM APPEUI[8]={ 0 };
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

// This should also be in little endian format, see above.
static const u1_t PROGMEM DEVEUI[8]={  0x10, 0x01, 0x03, 0x02, 0x01, 0x45, 0x7E, 0x0C };
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
static const u1_t PROGMEM APPKEY[16] = {0x01, 0x02, 0x03, 0x04, 
                                        0x05, 0x06, 0x07, 0x08,
                                        0x09, 0x0A, 0x0B, 0x0C,
                                        0x0D, 0x0E, 0x01, 0x10};
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

uint8_t mydata[256];
static uint8_t donneeFixe[] = "thomas";
static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 20;

// Pin mapping
// https://docs.m5stack.com/en/module/lora868
const lmic_pinmap lmic_pins = {
  .nss = 5,                       
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 26,                       
  .dio = {36, 35, LMIC_UNUSED_PIN}, 
};

void printHex2(unsigned v) {
    v &= 0xff;
    if (v < 16)
        Serial.print('0');
    Serial.print(v, HEX);
}

void do_if_data_received()
{ 
    M5.Lcd.println("Données reçues : ");
    // Ecrire sur la console locale l'ensemble du message reçu en hexadécimal
    // Extrait la data de LMIC.frame

    char donnee[256]; // taille max msg LoRa

    for (int i = 0; i < LMIC.dataLen; i++)
    {
    Serial.print(LMIC.frame[LMIC.dataBeg + i], HEX);
    Serial.print("|");
    donnee[i] = LMIC.frame[LMIC.dataBeg + i];
    }
    donnee[LMIC.dataLen] = '\0'; // ou 0

    if (strcmp(donnee, "blue") == 0)
    {
        M5.Lcd.fillScreen(BLUE);
        Serial.print("\nBleu reçu");
    }
    else
    {
        M5.Lcd.fillScreen(ORANGE);
        Serial.print("\nAutre message reçu");
    
    }
}

void do_send(osjob_t* j){
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, donneeFixe, sizeof(donneeFixe)-1, 1); // 1 pour confirmation du serveur
        //compteur = compteur + 1;
        //Serial.print(" Envoi du message : ");Serial.println(compteur);
        Serial.println(F("do_send : Packet queued"));
    }
    // Next TX is scheduled after TX_COMPLETE event.
}


void onEvent (ev_t ev) {
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
            // Disable link check validation (automatically enabled
            // during join, but because slow data rates change max TX
	    // size, we don't use it in this example.
            LMIC_setLinkCheckMode(0);
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_RFU1:
        ||     Serial.println(F("EV_RFU1"));
        ||     break;
        */
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
              do_if_data_received();
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
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_SCAN_FOUND:
        ||    Serial.println(F("EV_SCAN_FOUND"));
        ||    break;
        */
        case EV_TXSTART:
            Serial.println(F("EV_TXSTART"));
            break;
        case EV_TXCANCELED:
            Serial.println(F("EV_TXCANCELED"));
            break;
        case EV_RXSTART:
            /* do not print anything -- it wrecks timing */
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
  M5.begin(true, false, true, false);
  //M5.Power.begin();
  // LCD display
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextColor(RED);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextWrap(true);
  M5.Lcd.println("Hello IUT ! - LoRaWAN + SCD30 - TP4");
  //Serial.begin(115200);

  Serial.println(F("Starting"));
    while (millis() < 4000) {
        Serial.print("millis() = "); Serial.println(millis());
        delay(500);    
    }

    // SCD30 init
    scd30.initialize();

    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();
    // Start job (sending automatically starts OTAA too)
    do_send(&sendjob);
}

void loop() {

    // Attente non bloquante pour mesure données
    // toutes les XX secondes
    float result[3] = {0};

    if (millis()> debut + DUREE_ATTENTE)
    {
    debut=millis();
    
    if(scd30.isAvailable()){
        float result[3];
        scd30.getCarbonDioxideConcentration(result);
        // Affiche longueur data ET fabrique la chaîne
        // Comme on a la main sur l'objet on peut se permettre de faire
        // un "pré JSON" !
        snprintf((char*) mydata, sizeof(mydata), "{\"Name\":\"RB\", \"CO2\" : %.1f, \"Temperature\": %.1f, \"Humidity\": %.1f}", result[0], result[1], result[2]);
        //snprintf((char*) mydata, sizeof(mydata), "CO2 : %.1f, Tem : %.1f, Humidité : %.1f", result[0], result[1], result[2]);
        // LMIC_setTxData2(1, mydata, strlen((char*)mydata), 0);
        //LMIC_setTxData2(1, mydata, sizeof(mydata)-1, 0); // 1 pour confirmation du serveur
        LMIC_setTxData2(1, mydata, strlen((char*)mydata), 0); // 1 pour confirmation du serveur
        Serial.println("LMIC_setTxData2 appelée et données SCD30 envoyées !");

        compteur = compteur + 1;
        Serial.print("Paquet numéro : ");Serial.println(compteur);
        Serial.print("CO2 mesurée : ");Serial.println(result[0]);    
    }
    }

    os_runloop_once();
}
