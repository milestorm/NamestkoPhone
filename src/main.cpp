#include <Arduino.h>
#include <TM1637Display.h>

// Display Module connection pins (Digital Pins)
#define CLK 11
#define DIO 10

int needToPrint = 0;
int count;
int dialerInput = 2;
int lastState = LOW;
int trueState = LOW;
unsigned long lastStateChangeTime = 0;
int cleared = 0;

int cursorPosition = 0;
String alarmTimeTemp = "";
String alarmTime = "";

// constants

unsigned int dialHasFinishedRotatingAfterMs = 100;
unsigned int debounceDelay = 10;

uint8_t clearData[] = { 0x00, 0x00, 0x00, 0x00 };
TM1637Display display(CLK, DIO);


void setup(){


  Serial.begin(9600);
  pinMode(dialerInput, INPUT_PULLUP);

	// clear display
	display.setBrightness(0x0f);
	display.setSegments(clearData);

}

void loop(){

  int reading = digitalRead(dialerInput);

  if ((millis() - lastStateChangeTime) > dialHasFinishedRotatingAfterMs) {
  // the dial isn't being dialed, or has just finished being dialed.
    if (needToPrint) {
      // if it's only just finished being dialed, we need to send the number down the serial
      // line and reset the count. We mod the count by 10 because '0' will send 10 pulses.
			if (cursorPosition == 0) {
				display.setSegments(clearData);
			}
			count = count % 10;
			//Serial.print(count, DEC);
			display.showNumberDec(count, false, 1, cursorPosition);
			alarmTimeTemp += count;
			cursorPosition++;

			if (cursorPosition > 3) {
				alarmTime = alarmTimeTemp;
				Serial.println(alarmTime);
				alarmTimeTemp = "";
				cursorPosition = 0;
			}

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
