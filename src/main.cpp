#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define PUMP_PIN 15  //water pump relay control pin
#define SENSOR_PIN 33 //moisture sensor pin

//Pump Macros
#define PUMP
#define PUMP_ON_TIME_MS 10000

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
const int writeDelay = DEEP_SLEEP?  0 : 3600000;     //mqtt write time period (Set to zero if using deep sleep timer)

//Sensor variables
#define SENSOR_THRESHOLD 50
int sensorVal, sensorSum, currentTime, previousTime = 0;
const int sensorMax = 2600, sensorMin = 900;  //sensor ranges
const int sensorAvgs = 1000; //number of sensor reads

//LEDPin
const int LEDPin = 2;

//Other vars
bool firstRun = 1;

//Function declarations
double calculate_moisture_perc(int sensorVal);
void wifi_init();
void MQTT_reconnect();
void print_wakeup_reason();

void setup() {
  Serial.begin(9600);

  wifi_init();
  client.setServer(MQTT_BROKER, 1883);

  pinMode(SENSOR_PIN, INPUT);
  pinMode(LEDPin, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, 1);

  print_wakeup_reason();

  Serial.print("Deep sleep time [s]: ");
  Serial.println(SLEEP_TIME_US/1000000);

  if(!DEEP_SLEEP){
    Serial.print("MQTT write time delay [s]: ");
    Serial.println(writeDelay/1000);
  }

  esp_sleep_enable_timer_wakeup(SLEEP_TIME_US);    //deep sleep wake up every second

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

  if(((currentTime - previousTime) > writeDelay) || firstRun){    //Wake up delay set to 0 as using deep sleep timer

    sensorSum = 0;
    firstRun = 0;

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

    #ifdef PUMP
    //If sensor val < sensor threshold, turn on pump for x seconds
    if(moisturePerc < SENSOR_THRESHOLD ){
      Serial.print("Turning on pump for ");
      Serial.print(PUMP_ON_TIME_MS/1000);
      Serial.println(" seconds");
      digitalWrite(PUMP_PIN, 0);
      delay(PUMP_ON_TIME_MS);
      digitalWrite(PUMP_PIN, 1);
    }
    #endif

    previousTime = currentTime;
  }

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
