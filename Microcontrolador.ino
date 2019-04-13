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
#define DELAY 500

//Variables
const int closedWindowDegree = 0;
const int openWindowDegree = 90;

int maxTemperature = 20;
int maxHumidity = 60;
int sensorSample = 0;
int publishSample = 1150;

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
String srvPass = "PnSjOYBouNhm";
int port = 12771;

String deviceId = "house-device";
String topicSensor = "house/environment";
String topicWindow = "house/window";

//Sensor and servo objects
DHT dht(DHTPIN, DHTTYPE);
Servo windowServo1;
Servo windowServo2;
ESP_XYZ esp;

//System setup
void setup() {
  while(!esp.connectAP(ssid, pass));

  esp.MQTTConfig(deviceId);

  esp.MQTTSetServer(server, port, user, srvPass);

  esp.MQTTSubscribe(topicWindow);
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

//Auto control window function
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

//Process mqtt request 
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
    break;
}
}

//Callback function
void mqtt_callback(char* topic, byte* payload, unsigned int len) {

  String response;
  for (int i = 0; i < len; i++) {
    response.concat((char)payload[i]);
  }
  processRequest(getJsonInt(response, "window"), getJsonInt(response, "state"));
}

//Environment data publish function
void publishSensors() {
  String jsonMsg = "";
  jsonInit(&jsonMsg);
  addToJson(&jsonMsg, "temperature", temperature);
  addToJson(&jsonMsg, "humidity", humidity); 
  jsonClose(&jsonMsg);

  bool published = esp.MQTTPublish(topicSensor, jsonMsg);
  
  jsonClear(&jsonMsg);
}


//Loop function
void loop() {
  delay(500);

  if (!esp.MQTTConnected()) {
    esp.MQTTReconnect(deviceId);
  }

  
  //Read humidity and temperature
  sensorSample++;
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
