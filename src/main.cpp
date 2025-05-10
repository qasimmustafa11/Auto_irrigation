#include <Arduino.h>

#define PUMP_PIN 2
#define SENSOR_PIN 33

int pinState = false;
int currentTime, previousTime = 0;
const int sensorMin = 1000, sensorMax = 2610, sensorSum = 0;
int sensorAvgs = 1000;

double calculate_moisture_perc(int sensorVal);

void setup() {
  Serial.begin(9600);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(SENSOR_PIN, INPUT);
}

void loop() {

  //Read and average sensor value
  int sensorSum = 0;
  for (int i = 0; i < sensorAvgs; i++){
    sensorSum += analogRead(SENSOR_PIN);
  }
  int sensorVal = sensorSum/sensorAvgs;
  sensorVal = calculate_moisture_perc(sensorVal);
  Serial.print("Sensor value: ");
  Serial.print(sensorVal);
  Serial.println(" %");

  //If sensor val < 50%, turn on pump for 3 seconds
  if(sensorVal<50%){
    digitalWrite(PUMP_PIN, 1);
    delay(3000);
  }
}

/* convert sensor analog value to moisture percentage */
double calculate_moisture_perc(int sensorVal){
  return (((float)(sensorMax - sensorVal)/(sensorMax - sensorMin)) * 100);
}
