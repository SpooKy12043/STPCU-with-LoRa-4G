#if !defined(ESP32)
  #pragma error ("This is not the example your device is looking for - ESP32 only")
#endif

#include <Preferences.h>
Preferences store;

#include "config.h" 
#include <RadioLib.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

RTC_DATA_ATTR uint16_t bootCount = 0;
RTC_DATA_ATTR uint16_t bootCountSinceUnsuccessfulJoin = 0;
RTC_DATA_ATTR uint8_t LWsession[RADIOLIB_LORAWAN_SESSION_BUF_SIZE];

void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
    Serial.println(F("Wake from sleep"));
  } else {
    Serial.print(F("Wake not caused by deep sleep: "));
    Serial.println(wakeup_reason);
  }
  Serial.print(F("Boot count: "));
  Serial.println(++bootCount);
}

int16_t lwActivate() {
  int16_t state = RADIOLIB_ERR_UNKNOWN;
  node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
  Serial.println(F("Recalling LoRaWAN nonces & session"));
  store.begin("radiolib");
  if (store.isKey("nonces")) {
    uint8_t buffer[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];
    store.getBytes("nonces", buffer, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
    state = node.setBufferNonces(buffer);
    debug(state != RADIOLIB_ERR_NONE, F("Restoring nonces buffer failed"), state, false);
    state = node.setBufferSession(LWsession);
    debug((state != RADIOLIB_ERR_NONE) && (bootCount > 1), F("Restoring session buffer failed"), state, false);
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("Succesfully restored session - now activating"));
      state = node.activateOTAA();
      debug((state != RADIOLIB_LORAWAN_SESSION_RESTORED), F("Failed to activate restored session"), state, true);
      store.end();
      return(state);
    }
  } else {
    Serial.println(F("No Nonces saved - starting fresh."));
  }
  state = RADIOLIB_ERR_NETWORK_NOT_JOINED;
  while (state != RADIOLIB_LORAWAN_NEW_SESSION) {
    Serial.println(F("Join ('login') to the LoRaWAN Network"));
    state = node.activateOTAA();
    Serial.println(F("Saving nonces to flash"));
    uint8_t buffer[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];
    uint8_t *persist = node.getBufferNonces();
    memcpy(buffer, persist, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
    store.putBytes("nonces", buffer, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
    if (state != RADIOLIB_LORAWAN_NEW_SESSION) {
      Serial.print(F("Join failed: "));
      Serial.println(state);
      uint32_t sleepForSeconds = min((bootCountSinceUnsuccessfulJoin++ + 1UL) * 60UL, 180UL);
      Serial.print(F("Boots since unsuccessful join: "));
      Serial.println(bootCountSinceUnsuccessfulJoin);
      Serial.print(F("Retrying join in "));
      Serial.print(sleepForSeconds);
      Serial.println(F(" seconds"));
    }
  }
  Serial.println(F("Joined"));
  bootCountSinceUnsuccessfulJoin = 0;
  delay(1000);
  store.end();  
  return(state);
}

void setup() {
  Serial.begin(9600);
  Serial2.begin(9600);
  delay(2000);
  Serial.println(F("\nSetup"));

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(30, 5);
  display.println(F("TelemaTICs"));
  display.display();
  delay(1000);
  display.setCursor(15, 25);
  display.println(F("Modulo LoRa i-Bus"));
  display.display();
  delay(1000);
  display.startscrollleft(0x03, 0x0F);
  delay(3000);
  display.stopscroll();
  display.clearDisplay();
  display.setCursor(0, 15);
  display.println(F("En espera..."));
  display.display();
  delay(1000);

  int16_t state = 0;
  Serial.println(F("Initialise the radio"));
  state = radio.begin();
  debug(state != RADIOLIB_ERR_NONE, F("Initialise radio failed"), state, true);

  state = lwActivate();

  // --- CONFIGURACIÓN DE DATA RATE ---
  //node.setDatarate(1);  // DR1: SF9 / 125 kHz
  node.setDatarate(2);  // DR2: SF8 / 125 kHz
  //node.setDatarate(3);  // DR3: SF7 / 125 kHz
  //node.setDatarate(4);  // DR4: SF8 / 500 kHz
  // -----------------------------------

  node.setDwellTime(false);
  node.setADR(false);
  Serial.print(F("Payload máximo permitido: "));
  Serial.println(node.getMaxPayloadLen());
}

void loop() {
  if (Serial.available()) {
    String receivedData = Serial.readStringUntil('\n');
    receivedData.trim();
    if (receivedData.length() == 0) return;

    Serial.println("Datos UART recibidos: " + receivedData);

    char buffer[200];
    receivedData.toCharArray(buffer, sizeof(buffer));
    char *token = strtok(buffer, ",");
    int tokenCount = 0;
    char *tokens[20];
    while (token != NULL) {
      tokens[tokenCount++] = token;
      token = strtok(NULL, ",");
    }

    if (tokenCount < 16) {
      Serial.println("La trama UART no contiene suficientes tokens para extraer datos del GPS y pasajeros.");
      return;
    }

    uint8_t uplinkPayload1[4];
    uplinkPayload1[0] = 1;
    uplinkPayload1[1] = atoi(tokens[0]);
    uplinkPayload1[2] = atoi(tokens[2]);
    uplinkPayload1[3] = atoi(tokens[3]);

    float latVal   = atof(tokens[11]);
    float lonVal   = atof(tokens[12]);
    float altVal   = atof(tokens[13]);
    float speedVal = atof(tokens[14]);
    int personCount = atoi(tokens[15]);

    int16_t latInt   = (int16_t)(latVal * 100);
    int16_t lonInt   = (int16_t)(lonVal * 100);
    int32_t altInt   = (int32_t)(altVal * 100);
    int16_t speedInt = (int16_t)(speedVal * 100);

    uint8_t uplinkPayload2[11];
    uplinkPayload2[0] = latInt & 0xFF;
    uplinkPayload2[1] = (latInt >> 8) & 0xFF;
    uplinkPayload2[2] = lonInt & 0xFF;
    uplinkPayload2[3] = (lonInt >> 8) & 0xFF;
    uplinkPayload2[4] = altInt & 0xFF;
    uplinkPayload2[5] = (altInt >> 8) & 0xFF;
    uplinkPayload2[6] = (altInt >> 16) & 0xFF;
    uplinkPayload2[7] = (altInt >> 24) & 0xFF;
    uplinkPayload2[8] = speedInt & 0xFF;
    uplinkPayload2[9] = (speedInt >> 8) & 0xFF;
    uplinkPayload2[10] = (uint8_t)personCount;

    uint8_t uplinkPayloadCombined[15];
    for (int i = 0; i < 4; i++) {
      uplinkPayloadCombined[i] = uplinkPayload1[i];
    }
    for (int i = 0; i < 11; i++) {
      uplinkPayloadCombined[4 + i] = uplinkPayload2[i];
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(F("DataUplink OK"));
    delay(200);
    display.setCursor(0, 15);
    display.print(F("i-Silla: "));
    display.println(uplinkPayload1[1]);
    display.display();

    Serial.println("Uplink payload:");
    for (int i = 0; i < 15; i++){
      Serial.print(uplinkPayloadCombined[i], HEX);
      Serial.print(" ");
    }
    Serial.println();

    int16_t state = node.sendReceive(uplinkPayloadCombined, sizeof(uplinkPayloadCombined));
    debug((state < RADIOLIB_ERR_NONE) && (state != RADIOLIB_ERR_NONE), F("Error in sendReceive (combined payload)"), state, false);
    Serial.print(F("FCntUp: "));
    Serial.println(node.getFCntUp());
  }
  delay(100);
}
