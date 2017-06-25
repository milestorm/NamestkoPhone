#include <Arduino.h>


int needToPrint = 0;
int count;
int dialerInput = 2;
int lastState = LOW;
int trueState = LOW;
unsigned long lastStateChangeTime = 0;
int cleared = 0;

// constants

unsigned int dialHasFinishedRotatingAfterMs = 100;
unsigned int debounceDelay = 10;

void setup(){

  Serial.begin(9600);
  pinMode(dialerInput, INPUT_PULLUP);

}

void loop(){

  int reading = digitalRead(dialerInput);

  if ((millis() - lastStateChangeTime) > dialHasFinishedRotatingAfterMs) {
  // the dial isn't being dialed, or has just finished being dialed.
    if (needToPrint) {
      // if it's only just finished being dialed, we need to send the number down the serial
      // line and reset the count. We mod the count by 10 because '0' will send 10 pulses.
      Serial.print(count % 10, DEC);
      needToPrint = 0;
      count = 0;
      cleared = 0;
    }
  }

  if (reading != lastState) {
    lastStateChangeTime = millis();
  }

  if ((millis() - lastStateChangeTime) > debounceDelay) {
    // debounce - this happens once it's stablized
    if (reading != trueState) {
      // this means that the switch has either just gone from closed->open or vice versa.
      trueState = reading;
      if (trueState == HIGH) {
        // increment the count of pulses if it's gone high.
        count++;
        needToPrint = 1; // we'll need to print this number (once the dial has finished rotating)
      }
    }
  }

  lastState = reading;

}
