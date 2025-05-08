#include <Arduino.h>

#define PUMP_PIN 2

int pinState = false;



void setup() {
  Serial.begin(9600);
  pinMode(PUMP_PIN, OUTPUT);
}

void loop() {
  delay(500);
  if(pinState){
    digitalWrite(PUMP_PIN, 0);
    pinState = false;
  }
  else{
    digitalWrite(PUMP_PIN, 1);
    pinState = true;
  }
}
