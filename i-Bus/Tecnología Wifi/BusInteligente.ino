/****************************************************************

 Module NodeEsp8266 Pin  ZX Sensor Board  Function
 ---------------------------------------
 5V           VCC              Power
 GND          GND              Ground
 D1           DA               I2C Data
 D2           CL               I2C Clock

Resources:
Include Wire.h and TinyGps

Development environment specifics:
Written in Arduino 1.8.11
Autor: Cristian Càrdenas - Carlos Amarillo
Tested with a SparkFun RedBoard

****************************************************************/

#include <SoftwareSerial.h>//incluimos SoftwareSerial
#include <TinyGPS.h>//incluimos TinyGPS
#include <ESP8266WiFi.h>
#include <aREST.h>
#include <Adafruit_INA219.h>

TinyGPS gps;//Declaramos el objeto gps
SoftwareSerial serialgps(D3,D4);//Declaramos el pin 4 Tx y 3 Rx

//Declaramos la variables para la obtención de datos

unsigned long chars;
unsigned short sentences, failed_checksum;

//Define Apirest
aREST rest = aREST();

// Define Ina219 Monitorin-Voltaje

Adafruit_INA219 ina219;

//Define Variables Globales
float shuntvoltage = 0;
float busvoltage = 0;
float current_mA = 0;
float loadvoltage = 0;
float power_mW = 0;

//Variables API

float latitud,longitud,altitud,velocidad,voltaje,current,power;

const char* ssid = "ARRIS-C572";
const char* password = "DCA6333CC572";

//Port
#define LISTEN_PORT 80

//create instance of server
WiFiServer server (LISTEN_PORT);

void setup()
{
  Serial.begin(115200);//Iniciamos el puerto serie
  Wire.begin(D1,D2);
  serialgps.begin(9600);//Iniciamos el puerto serie del gps
  
  //Initialize INA219 Sensor
  if (!ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }
  
  //Imprimimos:
  Serial.println("");
  Serial.println("GPS GY-GPS6MV2 Leantec");
  Serial.println(" ---Buscando senal--- ");
  Serial.println("");

  //init variables API
  rest.variable("latitud", &latitud);
  rest.variable("longitud", &longitud);
  rest.variable("altitud", &altitud);
  rest.variable("velocidad", &velocidad);
  rest.variable("voltageBus",&loadvoltage);
  rest.variable("currentBus",&current_mA);
  rest.variable("powerBus",&power_mW);

  
  //Name ID 
  rest.set_id("2");
  rest.set_name("sensor_bus");

  //conect to wifi
  WiFi.begin(ssid,password);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
    }
  Serial.print("");
  Serial.println("Wifi connected");

  //Start server
  server.begin();
  Serial.println("Server started!");

  //IP
  Serial.println(WiFi.localIP());
}

void loop()
{
  while(serialgps.available()) 
  {
    //Read voltaje of battery
    // Get mesasures
    shuntvoltage = ina219.getShuntVoltage_mV();
    busvoltage = ina219.getBusVoltage_V();
    current_mA = ina219.getCurrent_mA();
    power_mW = ina219.getPower_mW();
    loadvoltage = busvoltage + (shuntvoltage / 1000);
    int c = serialgps.read(); 
    if(gps.encode(c))  
    {
      Serial.print("Entro a tomar datos");
      gps.f_get_position(&latitud, &longitud);
      altitud = gps.f_altitude();
      velocidad = gps.f_speed_kmph(); 
    }
      Serial.print("Latitud/Longitud: "); 
      Serial.print(latitud,5); 
      Serial.print(", "); 
      Serial.println(longitud,5);

      Serial.print("Altitud (metros): ");
      Serial.println(altitud); 
      //Serial.print("Rumbo (grados): "); Serial.println(gps.f_course()); 
      Serial.print("Velocidad(kmph): ");
      Serial.println(velocidad);
      Serial.println();
      gps.stats(&chars, &sentences, &failed_checksum); 

     WiFiClient client = server.available();
     rest.handle(client);  
     delay(2000);
      
    }    
}

