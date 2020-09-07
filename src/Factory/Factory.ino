#include <Wire.h>
#include <EEPROM.h>

const uint8_t readyPin = 3;
const uint8_t address = 0;
int iterator = 0;

void setup () {
  pinMode(readyPin, OUTPUT);

  digitalWrite(readyPin, HIGH);
}

void loop () {
  if (Serial.available()) {
    const uint8_t tokenChar = Serial.read();

    EEPROM.write(iterator, tokenChar);
    iterator++;
  }
}
