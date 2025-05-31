#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

//MQTT MACROS
#define MQTT
#define WIFI_SSID "Qasim's 2.4"
#define WIFI_PASS "0627923882"
#define MQTT_BROKER_USER "qasimmustafa"
#define MQTT_BROKER_PASS "I3w4b$96"
#define MQTT_BROKER "homeassistant.local"

//Pump Macros
#define PUMP
#define PUMP_PIN 2  //water pump relay control pin
#define PUMP_ON_TIME 20000 //Pump on time in ms

//MQTT vars
WiFiClient espClient;
PubSubClient client(espClient);
char* MQTTSensorOutTopic = "/study/plantMoisture";
char* MQTTPumpInTopic = "/living/PumpOn";
char* MQTTPumpOutTopic = "/living/PumpState";
char MQTTMessage[50] = {};

//Pump vars
bool setPumpOn = false;

//Function declarations
void wifi_init();
void MQTT_reconnect();

void MQTT_callback(char* topic, byte* payload, unsigned int length){
  Serial.print("Message received [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i<length; i++){
    Serial.print((char)payload[i]);
  }
  Serial.println();

  setPumpOn = true;
}

void setup() {
  Serial.begin(9600);

  wifi_init();
  client.setServer(MQTT_BROKER, 1883);
  client.setCallback(MQTT_callback);

  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, 1);

  MQTT_reconnect();

  delay(2000);

  Serial.println("Initialized");
}

void loop() {
  #ifdef MQTT
  //Connect to MQTT server and display status LED
  if(!client.connected()){
    MQTT_reconnect();
  }
  // Serial.print("MQTT connection status: ");
  // Serial.println(client.connected());

  if(setPumpOn){
    Serial.print("Turning pump on for ");
    Serial.print(PUMP_ON_TIME);
    Serial.println(" ms");

    client.publish(MQTTPumpOutTopic, "ON");

    digitalWrite(PUMP_PIN, 0);
    delay(2000);
    digitalWrite(PUMP_PIN, 1);

    Serial.println("Turning Pump off");
    client.publish(MQTTPumpOutTopic, "OFF");

    setPumpOn = false;
  }

  // Serial.print("Client loop return: ");
  // Serial.println(client.loop());
  client.loop();
  // delay(1000);
  #endif
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
    // Serial.print("Connecting to ");
    // Serial.println(MQTT_BROKER);
    // Serial.print("-");

    if(client.connect("ESP32_2", MQTT_BROKER_USER, MQTT_BROKER_PASS)){
      // Serial.print("Connected to ");
      // Serial.println(MQTT_BROKER);
      // digitalWrite(LEDPin, HIGH);
      client.subscribe(MQTTPumpInTopic);
    }

    // delay(1000);
  }
}