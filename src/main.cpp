#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

//MQTT MACROS
#define WIFI_SSID "Qasim's 2.4"
#define WIFI_PASS "0627923882"
#define MQTT_BROKER_USER "qasimmustafa"
#define MQTT_BROKER_PASS "I3w4b$96"
#define MQTT_BROKER "homeassistant.local"

//MQTT vars
WiFiClient espClient;
PubSubClient client(espClient);
char* MQTTSnakePlantTopic = "/study/snakePlant";
char MQTTMessage[50] = {};
const int writeDelay = 60 * 1* 1000;    //mqtt write time period = 1 minute

//Sensor variables
const int sensorPin  = 33;  //sensor input pint
int sensorVal, sensorSum, currentTime, previousTime = 0;
const int sensorMax = 2610, sensorMin = 1000;  //sensor ranges
const int sensorAvgs = 100; //number of sensor reads

//LEDPin
const int LEDPin = 2;

//Function declarations
double calculate_moisture_perc(int sensorVal);
void wifi_init();
void MQTT_reconnect();

void setup() {
  Serial.begin(9600);

  wifi_init();
  client.setServer(MQTT_BROKER, 1883);

  pinMode(sensorPin, INPUT);
  pinMode(LEDPin, OUTPUT);

  Serial.println("Initialized");
}

void loop() {

  //Connect to MQTT server and display status LED
  if(!client.connected()){
    digitalWrite(LEDPin, LOW);
    MQTT_reconnect();
  }
    currentTime = millis();

    if((currentTime - previousTime) > writeDelay){

      sensorSum = 0;

      for (int i = 0; i < sensorAvgs; i++){
        sensorSum += analogRead(sensorPin);
      }

      sensorSum /= sensorAvgs;
      double moisturePerc = calculate_moisture_perc(sensorSum);
      sprintf(MQTTMessage, "%.0f", moisturePerc);

      Serial.print("Soil sensor val: ");
      Serial.println(sensorSum);

      Serial.print("Moisture percentage: ");
      Serial.print(moisturePerc);
      Serial.println("%");

      Serial.print("MQTT message: ");
      Serial.println(MQTTMessage);

      Serial.println("Publishing to MQTT server...");
      client.publish(MQTTSnakePlantTopic, MQTTMessage);

      previousTime = currentTime;
  }
}

/* convert sensor analog value to moisture percentage */
double calculate_moisture_perc(int sensorVal){
  return (((float)(sensorMax - sensorVal)/(sensorMax - sensorMin)) * 100);
}

void wifi_init(){
  Serial.println("Wifi connecting...");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while(WiFi.status() != WL_CONNECTED){
    delay(100);
    Serial.print("-");
  }
  Serial.println();
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
}

void MQTT_reconnect(){
  while(!client.connected()){
    Serial.print("Connecting to ");
    Serial.println(MQTT_BROKER);
    Serial.print("-");

    if(client.connect("hello", MQTT_BROKER_USER, MQTT_BROKER_PASS)){
      Serial.print("Connected to ");
      Serial.println(MQTT_BROKER);
      digitalWrite(LEDPin, HIGH);
    }

    delay(1000);
  }
}