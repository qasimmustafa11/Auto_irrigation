#include <Arduino.h>

const int sensorPin  = 15;  //sensor input pint
int sensorVal;
const int sensorMax = 2610, sensorMin = 1000;  //sensor ranges

double calculate_moisture_perc(int sensorVal);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(sensorPin, INPUT);
  Serial.println("Initialized");
}

void loop() {
  sensorVal = analogRead(sensorPin);
  double moisturePerc = calculate_moisture_perc(sensorVal);

  Serial.print("Soil sensor val: ");
  Serial.println(sensorVal);
  Serial.print("Moisture percentage: ");
  Serial.print(moisturePerc);
  Serial.println("%");

  delay(500);
  }

/* convert sensor analog value to moisture percentage */
double calculate_moisture_perc(int sensorVal){
  return (((float)(sensorMax - sensorVal)/(sensorMax - sensorMin)) * 100);
}