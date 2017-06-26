#include <Arduino.h>
#include <TM1637Display.h>
#include <DS3232RTC.h>        // http://github.com/JChristensen/DS3232RTC
#include <Time.h>             // http://playground.arduino.cc/Code/Time
#include <Wire.h>             // http://arduino.cc/en/Reference/Wire

// Display Module connection pins (Digital Pins)
#define CLK 11
#define DIO 10

#define HANGUP 5
#define DIALERINPUT 2
#define SOLENOID 8

int needToPrint = 0;
int count;
int lastState = LOW;
int trueState = LOW;
unsigned long lastStateChangeTime = 0;
int cleared = 0;

int cursorPosition = 0;
String alarmTimeTemp = "";
int alarmTime[2] = {0, 0}; // mm:ss
bool alarmSet = false;
time_t timeStop;
bool timeStopSet = false;

bool phoneHanged = true;
int maxPhoneRingTime = 30; // maximal time of phone ringing (seconds)


unsigned int dialHasFinishedRotatingAfterMs = 100;
unsigned int debounceDelay = 10;

const uint8_t SEG_DASHES[] = { SEG_G, SEG_G, SEG_G, SEG_G	};
const uint8_t CLEARDATA[] = { 0x00, 0x00, 0x00, 0x00 };
TM1637Display display(CLK, DIO);

time_t timeToTimestampStop(tmElements_t actualTime, int hmsStopArray[]){
  tmElements_t elTimeStop;

  elTimeStop.Year = actualTime.Year;
  elTimeStop.Month = actualTime.Month;
  elTimeStop.Day = actualTime.Day;
  elTimeStop.Hour = actualTime.Hour;
  elTimeStop.Minute = actualTime.Minute + hmsStopArray[0];
  elTimeStop.Second = actualTime.Second + hmsStopArray[1];
  Serial.print("Prevadim: ");
  Serial.print(hmsStopArray[0]);
  Serial.print(":");
  Serial.println(hmsStopArray[1]);
  Serial.print("Prevedeno na: ");
  Serial.println(makeTime(elTimeStop));
  return makeTime(elTimeStop);
}

void countdownTime(int pTime[]){
  if (pTime[1] <= 59 && pTime[1] >= 0) {
    pTime[1] -= 1;
  }
  if (pTime[0] >= 1 && pTime[1] < 0) {
    pTime[1] = 59;
    pTime[0] -= 1;
  }
  if (pTime[0] <= 0 && pTime[1] <= 0) {
    pTime[1] = 0;
    pTime[0] = 0;
  }
}

void setup(){


  Serial.begin(9600);
  pinMode(DIALERINPUT, INPUT_PULLUP);
  pinMode(HANGUP, INPUT_PULLUP);
  pinMode(SOLENOID, OUTPUT);

	setSyncProvider(RTC.get);
	Serial.println("RTC sync...");
	if (timeStatus() != timeSet) Serial.println("FAIL!");

	// clear display
	display.setBrightness(0x0f);
	display.setSegments(CLEARDATA);

  /*
  // uncomment to set time to RTC
  time_t t;
  tmElements_t tm;
  tm.Year = CalendarYrToTm(2017);
  tm.Month = 6;
  tm.Day = 25;
  tm.Hour = 20;
  tm.Minute = 11;
  tm.Second = 0;
  t = makeTime(tm);
  if (RTC.set(t) == 0) {
    Serial.println("Time set!");
  }
  */

}

void loop(){

  int reading = digitalRead(DIALERINPUT);
  int hangupButt = digitalRead(HANGUP);

	static time_t tLast;

  time_t t = now();
  tmElements_t tm;
  tmElements_t almTm;
  breakTime(t, tm);

  if (hangupButt == LOW) {
    if (timeStopSet == true) {
      timeStop = timeToTimestampStop(tm, alarmTime);
      timeStop -= 1;
      timeStopSet = false;
    }
    if (t != tLast) {
        tLast = t;
        // do every second

        if (alarmSet == false) {
          // show actual time
          if ((tm.Second % 2) == 0) { // pulsing doubledot between time
            display.showNumberDecEx(tm.Hour * 100 + tm.Minute, (0x80 >> 1), true);
          } else {
            display.showNumberDecEx(tm.Hour * 100 + tm.Minute, 0, true);
          }
        } else {
          // show alarm countdown

          countdownTime(alarmTime);
          Serial.print("TEST PO: ");
          Serial.print(alarmTime[0]);
          Serial.print(":");
          Serial.println(alarmTime[1]);

          if ((alarmTime[1] % 2) == 0) { // pulsing doubledot between time
            display.showNumberDecEx(alarmTime[0] * 100 + alarmTime[1], (0x80 >> 1), true);
          } else {
            display.showNumberDecEx(alarmTime[0] * 100 + alarmTime[1], 0, true);
          }


          Serial.print("Aktualni cas je:   ");
          Serial.println(t);
          Serial.print("Alarm nastaven na: ");
          Serial.println(timeStop);
          if (t >= timeStop && t < (timeStop + maxPhoneRingTime)) {
            Serial.println("CRRRRRRR, pyco");
            digitalWrite(SOLENOID, HIGH);
          }

        }


    }
    phoneHanged = true;

  }
  else {
    if (phoneHanged == true) {
      // reset values
      display.setSegments(SEG_DASHES);
      digitalWrite(SOLENOID, LOW);
      phoneHanged = false;
      cursorPosition = 0;
      alarmSet = false;
      alarmTime[0] = 0;
      alarmTime[1] = 0;
      alarmTimeTemp = "";
    }
  }


  if ((millis() - lastStateChangeTime) > dialHasFinishedRotatingAfterMs) {
  // the dial isn't being dialed, or has just finished being dialed.
    if (needToPrint) {
      // if it's only just finished being dialed, we need to send the number down the serial
      // line and reset the count. We mod the count by 10 because '0' will send 10 pulses.
			if (cursorPosition == 0) {
				display.setSegments(SEG_DASHES);
			}
			count = count % 10;
      if ((cursorPosition == 0 || cursorPosition == 2) && count > 5) {
        Serial.println("nope");
      } else {
        display.showNumberDec(count, false, 1, cursorPosition);
  			alarmTimeTemp += count;
  			cursorPosition++;
      }


			if (cursorPosition > 3) {
				alarmTime[0] = alarmTimeTemp.substring(0,2).toInt();
        alarmTime[1] = alarmTimeTemp.substring(2,4).toInt();
        timeStopSet = true;
        alarmSet = true;
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

  if ((millis() - lastStateChangeTime) > debounceDelay && hangupButt == HIGH) {
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
