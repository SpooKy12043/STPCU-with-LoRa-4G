/****************************************************************
 NodeESP8266 + ADS1115 + PMSx003 + DHT11 – Monitor Serial

 Descripción:
 Lee datos de temperatura, humedad, voltaje, ruido, UV, CO₂ (simulado), 
 y partículas en suspensión (PM1.0, PM2.5, PM10) y los muestra en el 
 monitor serial.

 Autor: Cristian Cárdenas - Carlos Amarillo
****************************************************************/

#include <Wire.h>
#include <DHT.h>
#include <Adafruit_ADS1X15.h>
#include <PMserial.h>

// Configuración DHT11
#define DHTPIN  D7
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Configuración ADS1115
Adafruit_ADS1115 ads;
const float multiplier = 0.1875F;

// Variables globales
float humidity, temperature, co2, uv, noise, voltage;

// Configuración PMSx003
SerialPM pms(PMSx003, Serial); // Se utiliza UART por defecto

void setup() {
  Serial.begin(115200);
  Wire.begin();
  dht.begin();
  ads.begin();
  pms.init(); 

  Serial.println(F("Sistema de monitoreo ambiental iniciado."));
  Serial.println(F("-------------------------------------------------"));
}

void loop() {
  leerTemperaturaHumedad();
  delay(2000);

  leerADC();
  delay(2000);

  leerCalidadAire();
  delay(4000);

  Serial.println(F("-------------------------------------------------"));
  delay(3000); // Espera antes del siguiente ciclo completo
}

// ----------- Función: Leer temperatura y humedad ----------- //
void leerTemperaturaHumedad() {
  humidity = dht.readHumidity(); // RH % 0-100
  temperature = dht.readTemperature(); // °C
  Serial.print(F("Humedad: ")); Serial.print(humidity); Serial.println(F(" %"));
  Serial.print(F("Temperatura: ")); Serial.print(temperature); Serial.println(F(" °C"));
}

// ----------- Función: Leer entradas analógicas via ADS1115 ----------- //
void leerADC() {
  float adc0 = ads.readADC_SingleEnded(0);
  float adc1 = ads.readADC_SingleEnded(1);
  float adc2 = ads.readADC_SingleEnded(2);
  float adc3 = ads.readADC_SingleEnded(3);

  co2 = adc0 * multiplier;
  uv = adc1 * multiplier;
  noise = adc2 * multiplier;
  voltage = adc3 * multiplier * 3; // Ajuste de ganancia

  Serial.print(F("CO₂ simulado: ")); Serial.print(co2); Serial.println(F(" V"));
  Serial.print(F("UV: ")); Serial.print(uv); Serial.println(F(" V"));
  Serial.print(F("Ruido: ")); Serial.print(noise); Serial.println(F(" V"));
  Serial.print(F("Voltaje: ")); Serial.print(voltage); Serial.println(F(" V"));
}

// ----------- Función: Leer partículas con PMSx003 ----------- //
void leerCalidadAire() {
  pms.read();
  if (pms) {
    Serial.print(F("PM1.0: ")); Serial.print(pms.pm01); Serial.print(F(" µg/m³, "));
    Serial.print(F("PM2.5: ")); Serial.print(pms.pm25); Serial.print(F(" µg/m³, "));
    Serial.print(F("PM10: ")); Serial.print(pms.pm10); Serial.println(F(" µg/m³"));
  } else {
    Serial.println(F("Error al leer el sensor de partículas."));
  }
}
