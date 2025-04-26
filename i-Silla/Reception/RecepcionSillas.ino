/******************************** INFORMACION *************************************

 * Proyecto: Adaptación del Sistema de Transporte Público Colectivo Urbano -
  Tradicional de Ciudades Intermedias al Paradigma de las Ciudades Inteligentes, 
  Sostenibles y Receptivas
 
 * Descripción: Módulo i-Silla-Bus 
  Código para la recepcion de datos correspondientes a los cuatro modulos i-Silla
  Nota: El diagrama de conexiones de cada elemento se anexa a este archivo.
  

 * Autor: Hector & Ignacio Aguilera
 
  Este codigo fue tomado de  https://proyectocorrecaminos.com/como-usar-modulos-lora-con-esp32/, 
  se realizaron ajustes para la data que se envia en este proyecto

 * Cambios: 
  Juan Esteban Bautista Gaona
  Jhon Alexander Callejas Ochoa
  Cristian Leonardo Cárdenas Garcia
  Carlos Andrés González Amarillo
 
 * Fecha de creación: 23/05/2024
 * Última actualización: 05/02/2025
 * Version: 2.0

 * Hardware utilizado:
  Módulo ESP8266 V3.1, - Módulo RYLR998
 
 * Librerías utilizadas: Software Serial
  // Nota: Se especifican los pines pero no se hace uso de la libreria por temas
  // practicos del PCB ya empleado (Se uso RX0 y TX0)

  // La recomendacion es usar los pines 13 y 15 y la libreria para no usar los pines
  de programacion del ESP8266 o tarjeta que se use.

 **********************************************************************************/
//*************************** INICIO DE CODIGO **************************************

//************************** Declaración de librerias *******************************
//***********************************************************************************/

//************************** Declaración de objetos, pines y variables **************
#define RXD2 13                   // Pin RX para la recepción de los datos del módulo LoRa.
#define TXD2 15                   // Pin TX para la transmisión de los datos del módulo LoRa.

int varInMsg = 7;                 // Cantidad de variables presentes en el mensaje recibido.
int commasInMsg = varInMsg - 1;   // Se calculan la cantidad de comas que habrá en el mensaje.
int keyPos[10];                   // Arreglo para almacenar las posiciones de las comas en el mensaje.

// Variables de tipo string donde guardaremos la data del mensaje recibido
String msg;
String value1;
String value2;
String value3;
//***********************************************************************************/

//***************** Función para envio de comandos al módulo RYRL998 ****************
void sendCmd(String cmd) {
  Serial.println(cmd);                  
  delay(500);                           // Retardo para la instruccion AT.
  while (Serial.available()) {
    Serial.print(char(Serial.read()));  // Ver la respuesta del módulo.
  }
}//***********************************************************************************/

//***************** Función para configuracion y comprobacion de estado *************
void setup() {
  Serial.begin(115200);         // Unico serial para el envio y muestreo en el monitor
  // Nota: Se puede hacer uso de Software serial para separar el envio de data al 
  // módulo RYLR998 (115200 baud) y el muestreo en el monitor (9600 baud)
  
  // Inicialización del RYLR998 mediante comandos AT.
  Serial.println("Configurando parámetros RYLR998");
  delay(1000);
  sendCmd("AT+ADDRESS=5");           // i-Silla-Bus con direccion de nodo 5
  delay(1000);
  sendCmd("AT+NETWORKID=5");         // Asignacion a la ID de red 5
  delay(1000);
  sendCmd("AT+BAND=915000000");      // Frecuencia de trabajo 915MHz.
  delay(1000);
  // Configuraciones de SF, BW, CRC
  sendCmd("AT+PARAMETER=9,7,1,12"); 
  delay(1000);
  sendCmd("AT+MODE=0");              // Modo de operacion del módulo receptor
  delay(1000);
}
//***********************************************************************************/

//*************************** Función de trabajo ciclica ****************************
void loop() {
  // Espera para la recepción del mensaje desde el transmisor
  while (Serial.available()) {
    msg = Serial.readString();  // Leemos el mensaje completo desde Serial
  }
  msg.trim();  // Se eliminan los espacios en blanco al inicio y final del mensaje.

// Se detectan las posiciones de las comas en el mensaje recibido.
// Verificar que el mensaje comienza con "+RCV="
if (msg.startsWith("+RCV=")) {
    // Se detectan las posiciones de las comas en el mensaje recibido.
    for (int i = 0; i < 3; i++) {  
        keyPos[i] = msg.indexOf(',', keyPos[i - 1] + 1);
    }

    // Se obtienen los valores correspondientes a cada variable del mensaje.
    value1 = msg.substring(keyPos[0] + 1, keyPos[1]);  // +1 para evitar el primer carácter después de la coma
    value2 = msg.substring(keyPos[1] + 1, keyPos[2]);
    value3 = msg.substring(keyPos[2] + 1);

    // Se imprimen las variables recibidas en el mensaje.
    Serial.println("OK:");
    Serial.print("Source: " + value1);
    Serial.print(", Length " + value2);
    Serial.print(", Data: " + value3);
    Serial.println();
    delay(500);
}
}

//******************************** FIN DE CODIGO *************************************
