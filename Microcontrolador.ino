//Sensor library
#include "DHT.h"
//Servo library
#include <Servo.h>
//Imagine XYZ library
#include "ESP_XYZ_StandAlone.h"

//Pin and sensor model definition
#define DHTPIN 4
#define DHTTYPE DHT11   // DHT 11

//Servos pins
#define SERVOPIN_1 5
#define SERVOPIN_2 2

//System delay
#define DELAY 2000

//Variables
const int closedWindowDegree = 0;
const int openWindowDegree = 90;

int maxTemperature = 20;
int maxHumidity = 60;
int sensorSample = 0;
int publishSample = 0;

float humidity = 0;
float temperature = 0;

bool autoWindow[] = {
  true,
  true
};

//Wireless set up
char* ssid = "PorrasBarrantes";
char* pass = "deniol0212";

//mqtt server
String server = "m16.cloudmqtt.com";
String user = "zpsqnfvh";
String srv_pass = "PnSjOYBouNhm";
int port = 12771;

String device_id = "house-device";
String topic_sensor = "house/environment";
String topic_window = "house/window";

//Sensor and servo objects
DHT dht(DHTPIN, DHTTYPE);
Servo windowServo1;
Servo windowServo2;
ESP_XYZ esp;

//System setup
void setup() {
  Serial.begin(9600);
  Serial.println(F("Test!"));

  while(!Serial);
  while(!esp.connectAP(ssid, pass));
  Serial.println("Configuracion exitosa");

  //Se establece el id del dispositivo
  esp.MQTTConfig(device_id);

  //Se configura el servidor destino
  esp.MQTTSetServer(server, port, user, srv_pass);

  //Suscripción a servidor MQTT
  esp.MQTTSubscribe(topic_window);
 //Configuración de función de callback
  esp.MQTTSetCallback(mqtt_callback);
   
  dht.begin();
  windowServo1.attach(SERVOPIN_1);
  windowServo2.attach(SERVOPIN_2);
}

//Open window function
void openWindow(int window) {
  if (window == 1)
    windowServo1.write(closedWindowDegree);
  else
    windowServo2.write(openWindowDegree);
}

//Close window function
void closeWindow(int window) {
  if (window == 1)
    windowServo1.write(openWindowDegree);
  else
    windowServo2.write(closedWindowDegree);
}

//Read humidity function
bool readHumiditySensor() {
  humidity = dht.readHumidity();

  if (isnan(humidity)) {
    return false;
  }
  return true;
}

//Read temperature function
bool readTemperatureSensor() {
  temperature = dht.readTemperature();

  if (isnan(temperature)) {
    return false;
  }
  return true;
}

void autoControlWindows(int window) {
  bool isAuto = autoWindow[window - 1];

  if (isAuto) {
    if (humidity > maxHumidity || temperature <= maxTemperature) {
      closeWindow(window);
    } else {
      openWindow(window);
    }
  }
}

void processRequest(int window, int state) {
  switch (state) {
  case 0:
    closeWindow(window);
    autoWindow[window - 1] = false;
    break;
  case 1:
    openWindow(window);
    autoWindow[window - 1] = false;
    break;
  case 2:
    autoWindow[window - 1] = true;
    break;
  default:
    // statements
    break;
}
}

//Función de callback
//Debe retornar void y tener los mismos argumentos
void mqtt_callback(char* topic, byte* payload, unsigned int len) {
  //Notifica en puerto UART la recepción de un mensaje
  Serial.print("Mensaje recibido [");
  Serial.print(topic);
  Serial.println("] ");

  String response;
  //Se imprime el mensaje caracter por caracter
  for (int i = 0; i < len; i++) {
    response.concat((char)payload[i]);
  }
  processRequest(getJsonInt(response, "window"), getJsonInt(response, "state"));
}

void publishSensors() {
  String json_msg = "";
  jsonInit(&json_msg);
  addToJson(&json_msg, "temperature", temperature);
  addToJson(&json_msg, "humidity", humidity); 
  jsonClose(&json_msg);

  bool published = esp.MQTTPublish(topic_sensor, json_msg);
  Serial.println("Mensaje publicado: ");
  Serial.println(published);
  
  jsonClear(&json_msg);
}


//Loop function
void loop() {
  delay(500);

  //Se verifica que el dispositivo este conectado
  if (!esp.MQTTConnected()) {
    //De lo contrario se conecta nuevamente
    esp.MQTTReconnect(device_id);
  }
  
  sensorSample++;
  //Read humidity and temperature
  if (sensorSample == 10) {
    readHumiditySensor(); 
    readTemperatureSensor();
    autoControlWindows(1);
    autoControlWindows(2);
    sensorSample = 0;
  }
  
  publishSample++;
  if(publishSample == 100) {
    publishSensors();
    publishSample = 0;
  }
  
  esp.MQTTLoop();
}
