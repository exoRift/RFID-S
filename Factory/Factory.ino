#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
#include <EEPROM.h>

const String months[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

const uint8_t errPin = 3;
const uint8_t address = 280;
int iterator = 0;

tmElements_t tm;

void setup () {
  pinMode(errPin, OUTPUT);

  if (getDate() && getTime()) {
    RTC.write(tm);
  } else digitalWrite(errPin, HIGH);
}

void loop () {
  if (Serial.available()) {
    const uint8_t tokenChar = Serial.read();

    EEPROM.write(iterator, tokenChar);
    iterator++;
  }
}

const bool getDate () {
  String mthStr;
  uint8_t mth, dy, yr;
  
  if (sscanf(__DATE__, "%s %d %d", mthStr, &dy, &yr) != 3) return false;

  for (uint8_t i = 0; i < 12; i ++) {
    if (months[i] == mthStr) mth = i + 1;
  }

  if (!mth) return false;

  tm.Month = mth;
  tm.Day = dy;
  tm.Year = CalendarYrToTm(yr);

  return true;
}

const bool getTime () {
  uint8_t hr, mn, sc;
  
  if (sscanf(__TIME__, "%d:%d:%d", &hr, &mn, &sc) != 3) return false;

  tm.Hour = hr;
  tm.Minute = mn;
  tm.Second = sc;

  return true;
}
