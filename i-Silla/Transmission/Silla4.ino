/******************************** INFORMACION *************************************

 * Proyecto: Adaptación del Sistema de Transporte Público Colectivo Urbano -
  Tradicional de Ciudades Intermedias al Paradigma de las Ciudades Inteligentes, 
  Sostenibles y Receptivas
 
 * Descripción: Módulo i-Silla #4
  Código para la transmisión de datos correspondientes a dos sensores digitales, los
  cuales permiten identificar la ocupación de la silla. 
  Nota: El diagrama de conexiones de cada elemento se anexa a este archivo.

 * Autores:
  Juan Esteban Bautista Gaona
  Jhon Alexander Callejas Ochoa
  Cristian Leonardo Cárdenas Garcia
  Carlos Andrés González Amarillo
 
 * Fecha de creación: 15/08/2024
 * Última actualización: 05/02/2025
 * Version: 3.0

 * Hardware utilizado:
  Módulo ESP8266 V3.1, - Módulo RYLR998
  Sensor ZX, Sensor INA219, Sensor de presión
 
 * Librerías utilizadas:
  Wire.h (Comunicación I2C)
  ZX_Sensor.h (Librería para ZX Sensor)
  Adafruit_INA219.h (Librería para INA219)

 **********************************************************************************/
//*************************** INICIO DE CODIGO **************************************

//************************** Declaración de librerias *******************************
#include <Wire.h>              // Librería para comunicación I2C.
#include <ZX_Sensor.h>         // Librería del sensor ZX.
#include <Adafruit_INA219.h>   // Librería para el sensor INA219.
//***********************************************************************************/

//************************** Declaración de objetos, pines y variables **************
#define pushButton D8      // Pin para el sensor de presión
Adafruit_INA219 ina219; 
const int ZX_ADDR = 0x10;  // Dirección I2C del ZX Sensor.
ZX_Sensor zx_sensor = ZX_Sensor(ZX_ADDR);

uint8_t z_pos;             // Variable del sensor de proximidad.
int buttonState;           // Variable del pushbutton
float shuntvoltage = 0;    // Variables para el sensor INA219
float busvoltage = 0;      
float current_mA = 0;      
float loadvoltage = 0;     
float power_mW = 0;        
//***********************************************************************************/

//***************** Función para envio de comandos al módulo RYRL998 ****************
void sendCmd(String cmd) {
  Serial.println(cmd); 
  delay(500);                           // Retardo para la instruccion AT.
  while (Serial.available()) {
    Serial.print(char(Serial.read()));  // Ver la respuesta del módulo.
  }
}
//***********************************************************************************/

//***************** Función para configuracion y comprobacion de estado *************
void setup() {
  Serial.begin(115200);       // Unico serial para el envio y muestreo en el monitor
  // Nota: Se puede hacer uso de Software serial para separar el envio de data al 
  // módulo RYLR998 (115200 baud) y el muestreo en el monitor (9600 baud)
  delay(3000);                   
  Wire.begin(D1, D2);         // Pines para I2C (D1: SDA, D2: SCL)
  pinMode(pushButton, INPUT); // Configuración del pin para pushbutton.

  if (!ina219.begin()) {      // Inicialización del sensor INA219.
    Serial.println("Error al encontrar el chip INA219.");
    while (1) { delay(10); }
  }

  if (zx_sensor.init()) {     // Inicialización del sensor ZX.
    Serial.println("Sensor ZX inicializado correctamente");
  } else {
    Serial.println("Error al inicializar el sensor ZX");
    while (1);  // Detener si hay un error
  }

  // Inicialización del RYLR998 mediante comandos AT.
  Serial.println("Configurando parámetros RYLR998");
  delay(1000); 
  sendCmd("AT+ADDRESS=4");          // i-Silla con direccion de nodo 1
  delay(1000);
  sendCmd("AT+NETWORKID=5");        // Asignacion a la ID de red 5
  delay(1000);
  sendCmd("AT+BAND=915000000");     // Frecuencia de trabajo 915MHz.
  delay(1000);
  // Configuraciones de SF, BW, CRC
  sendCmd("AT+PARAMETER=9,7,1,12"); 
  delay(1000);
  sendCmd("AT+MODE?");              // Modo de operacion del módulo
  delay(1000);
}
//***********************************************************************************/

//*************************** Función de trabajo ciclica ****************************
void loop() {
  // Adquisicion de la data para cada uno de los sensores
  buttonState = digitalRead(pushButton);  
  z_pos = zx_sensor.readZ();             
  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  power_mW = ina219.getPower_mW();
  loadvoltage = busvoltage + (shuntvoltage / 1000);

  // Concatenacion de la data en un unico String para el envio en una sola trama
  String data = String(z_pos) + "," + (buttonState == HIGH ? "1" : "0") + "," + busvoltage + "," + shuntvoltage + "," + loadvoltage + "," + current_mA + "," + power_mW;

  // Identificacion del tamaño del string
  String datalen = String(data.length());  

  // Envio del mensaje y la longitud como unico string al nodo con ID 5
  sendCmd("AT+SEND=5," + datalen + "," + data); 
  // Nota: El nodo con ID 5 corresponde al receptor, se puede encontrar mas info a -
  // detalle en el archivo RX_ModuloSilla_RYLR998.ino adjunto a este.

  // Configuración de retardo para evitar las colisiones con los otros nodos de -
  // transmision
  int randomDelay = random(48, 60);  
  delay(randomDelay * 1000);  
}

//******************************** FIN DE CODIGO *************************************
