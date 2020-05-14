#include <EEPROM.h>

#include <RHReliableDatagram.h>
#include <RH_ASK.h>
#include <SPI.h>

#include <sha256.h>

#include <DS1307RTC.h>

#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 2

RH_ASK rd;
RHReliableDatagram manager(rd, SERVER_ADDRESS);

const String scannerKey("mZq4t7w!z%C*F)J@NcRfUjXn2r5u8x/A"); // DEBUG
const String cardKey("eThWmZq4t6w9z$C&F)J@NcRfUjXn2r5u"); // DEBUG

const uint8_t scannerKeyAddress = 0;
uint8_t cardKeyAddress;
uint8_t dumpAddress;

String msg;
uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
long statusDelay;

const uint8_t statusPin = 8;

void setup () {
  pinMode(statusPin, OUTPUT);

  Serial.begin(9600); // DEBUG

  const bool managerStatus = manager.init();

  if (!managerStatus) Serial.println("Manager init failed"); // DEBUG

  cardKeyAddress = writeMem(scannerKeyAddress, scannerKey) + 1; // DEBUG

  dumpAddress = writeMem(cardKeyAddress, cardKey) + 1; // DEBUG
}

const uint8_t writeMem (const uint8_t startAddr, const String value) {
  const uint8_t valueLen = value.length();
  const uint8_t endAddr = startAddr + valueLen;

  for (uint8_t i = 0; i < valueLen; i++) EEPROM.update(startAddr + i, value[i]);
  EEPROM.update(endAddr, 0);

  return endAddr;
}

const String readMem (const uint8_t startAddr) {
  String res;

  for (uint8_t i = 0; i < 64; i++) {
    const uint8_t index = EEPROM.read(startAddr + i);

    if (!index) break;

    res.concat(char(index));
  }

  return res;
}

const String hash (const String msg, const String key) {
  uint8_t *hashed;
  String res;

  Sha256.initHmac(key.c_str(), key.length());

  Sha256.print(msg.c_str());

  hashed = Sha256.resultHmac();

  for (uint8_t i = 0; i < key.length(); i++) res.concat(String(hashed[i], HEX));

  return res;
}

const uint32_t getUnix () {
  const uint32_t unix = RTC.get();

  if (!unix) Serial.println(String("Unable to fetch time: ") + String((RTC.chipPresent() ? "chip present" : "chip not present"))); // DEBUG

  return unix;
}

const void setStatus (const bool state) {
   digitalWrite(statusPin, state);
}

void loop () {
  if (statusDelay && millis() - statusDelay >= 5000) {
    setStatus(false);
    statusDelay = 0;
  }

  if (manager.available()) {
    uint8_t buflen = sizeof(buf);
    uint8_t from;

    if (manager.recvfromAck(buf, &buflen, &from)) {
      if (from == CLIENT_ADDRESS) {
        String bufString((char *)buf);
        bufString = bufString.substring(0, buflen);

        const char status = bufString[0];
        const String param(bufString.substring(bufString.indexOf(':') + 1, bufString.length()));

        Serial.print(status); // DEBUG
//        Serial.print(' '); // DEBUG
//        Serial.println(param); // DEBUG

        switch (status) {
          case '1': { // ready
            const uint32_t unix = getUnix();

            if (unix) {
              String hashed("2:" + hash(String(unix), readMem(scannerKeyAddress)));
              Serial.println(hashed);

              writeMem(dumpAddress, hashed);

              manager.sendtoWait(hashed.c_str(), hashed.length(), from);
            }

            break;
          }
          case '3': { // recieved double-hashed unix
            const String compare(hash(readMem(dumpAddress), readMem(cardKeyAddress)));
            Serial.print("C: ");
            Serial.println(compare);

            Serial.print("I: ");
            Serial.println(param);

            setStatus(param == compare);

            statusDelay = millis();

            break;
          }
          default: {
            const char msg = '9';

            manager.sendtoWait(msg, sizeof(msg), from);

            break;
          }
        }
      }
    }
  }
}
