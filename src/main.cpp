#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define PUMP_PIN 15  //water pump relay control pin
#define SENSOR_PIN 33 //moisture sensor pin

//Pump Macros
#define AUTO_PUMP
#define PUMP_ON_TIME_MS 30000
#define PUMP_DELAY 10800000 //Min delay of 3 hours between pump runs

//Deep Sleep macros
#define DEEP_SLEEP false
#define SLEEP_TIME_S 300
#define S_TO_US 1000000
#define SLEEP_TIME_US (SLEEP_TIME_S * S_TO_US)

//MQTT MACROS
#define MQTT
#define WIFI_SSID "Qasim's 2.4"
#define WIFI_PASS "0627923882"
#define MQTT_BROKER_USER "qasimmustafa"
#define MQTT_BROKER_PASS "I3w4b$96"
#define MQTT_BROKER "homeassistant.local"

//MQTT vars
WiFiClient espClient;
PubSubClient client(espClient);
char* MQTTSnakePlantTopic = "/study/plantMoisture";
char MQTTMessage[50] = {};
char* MQTTSensorOutTopic = "/study/plantMoisture";
char* MQTTPumpInTopic = "/living/PumpOn";
char* MQTTPumpOutTopic = "/living/PumpState";

//Sensor variables
#define SENSOR_THRESHOLD 50
int sensorVal, sensorSum, currentTime, sensReadPreviousTime = 0, sensLowTimer = 0, sensHighPrevTime = 0;;
const int sensorMax = 2600, sensorMin = 900;  //sensor ranges
const int sensorAvgs = 1000; //number of sensor reads
const int sensorReadDelay = DEEP_SLEEP?  0 : 60000;     //sensor read delay 1 minute (Set to zero if using deep sleep timer)
const int sensLowDelay = 3600000; //1 hour

//Pump variables
int pumpPrevTime = 0;

//LEDPin
const int LEDPin = 2;

//Other vars
bool firstRun = 1;

//Function declarations
double calculate_moisture_perc(int sensorVal);
void wifi_init();
void MQTT_reconnect();
void print_wakeup_reason();
void pump_run();

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

  pinMode(SENSOR_PIN, INPUT);
  pinMode(LEDPin, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, 1);

  print_wakeup_reason();

  if(DEEP_SLEEP){
    Serial.print("Deep sleep time [s]: ");
    Serial.println(SLEEP_TIME_US/1000000);
  }
  else{
    Serial.print("Sensor read delay [ms]: ");
    Serial.println(sensorReadDelay);
  }

  #ifdef AUTO_PUMP
  Serial.print("Delay between pump ON [ms]: ");
  Serial.println(PUMP_DELAY);
  #endif

  esp_sleep_enable_timer_wakeup(SLEEP_TIME_US);    //deep sleep wake up every second
  client.setCallback(MQTT_callback);
  delay(2000);
  Serial.println("Initialized");
}

void loop() {

  #ifdef MQTT
  //Connect to MQTT server and display status LED
  if(!client.connected()){
    digitalWrite(LEDPin, LOW);
    MQTT_reconnect();
  }
  #endif

  currentTime = millis();

  int sensReadTimer = currentTime - sensReadPreviousTime;
  // Serial.print("Time in ms since last MQTT write: ");
  // Serial.println(MQTTlastWriteTime);

  if((sensReadTimer > sensorReadDelay) || firstRun){    //Wake up delay set to 0 as using deep sleep timer

    sensorSum = 0;

    for (int i = 0; i < sensorAvgs; i++){
      sensorSum += analogRead(SENSOR_PIN);
    }

    sensorSum /= sensorAvgs;
    double moisturePerc = calculate_moisture_perc(sensorSum);
    sprintf(MQTTMessage, "%.0f", moisturePerc);

    Serial.print("Soil sensor val: ");
    Serial.println(sensorSum);

    Serial.print("Moisture percentage: ");
    Serial.print(moisturePerc);
    Serial.println("%");

    #ifdef MQTT
    Serial.print("MQTT message: ");
    Serial.println(MQTTMessage);

    Serial.println("Publishing to MQTT server...");
    client.publish(MQTTSnakePlantTopic, MQTTMessage);
    #endif

    #ifdef AUTO_PUMP
    int pumpLastRunTimer = currentTime - pumpPrevTime;
    Serial.print("Time in ms since last pump ON: ");
    Serial.println(pumpLastRunTimer);

    //If sensor val < sensor threshold, update sensLowTimer 
    if (moisturePerc < SENSOR_THRESHOLD){
      sensLowTimer = currentTime - sensHighPrevTime;
      Serial.print("Time in ms since sensor low: ");
      Serial.println(sensLowTimer);

      //If pump last run timer>PUMP_DELAY, & sens low timer > sens low delay, turn on pump for x seconds
      if(((pumpLastRunTimer > PUMP_DELAY) || firstRun) && (sensLowTimer > sensLowDelay)){
        pump_run();
        pumpPrevTime = currentTime; //reset pump timer
      }
    } else {
      sensHighPrevTime = currentTime;
    }
    #endif

    sensReadPreviousTime = currentTime; //reset sensor timer
    firstRun = 0;
  }

  #ifndef AUTO_PUMP
  if(setPumpOn){
    pump_run();
    setPumpOn = false;
  }
  #endif

  client.loop();
  
  if(DEEP_SLEEP){
    Serial.println("Going to sleep...");
    Serial.flush(); 
    esp_deep_sleep_start(); 
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
    // Serial.print("Connecting to ");
    // Serial.println(MQTT_BROKER);
    // Serial.print("-");

    if(client.connect("ESP32_1", MQTT_BROKER_USER, MQTT_BROKER_PASS)){
      // Serial.print("Connected to ");
      // Serial.println(MQTT_BROKER);
      digitalWrite(LEDPin, HIGH);
      client.subscribe(MQTTPumpInTopic);
    } 
  }
}

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void pump_run(){
    Serial.print("Turning pump on for ");
    Serial.print(PUMP_ON_TIME_MS);
    Serial.println(" ms");

    client.publish(MQTTPumpOutTopic, "ON");

    digitalWrite(PUMP_PIN, 0);
    delay(2000);
    digitalWrite(PUMP_PIN, 1);

    Serial.println("Turning Pump off");
    client.publish(MQTTPumpOutTopic, "OFF");


}
