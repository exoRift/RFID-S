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

const String recKey("mZq4t7w!z%C*F)J@NcRfUjXn2r5u8x/A"); // DEBUG
const String tranKey("eThWmZq4t6w9z$C&F)J@NcRfUjXn2r5u"); // DEBUG

const uint8_t recKeyAddress = 380;
uint8_t tranKeyAddress;
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

  tranKeyAddress = writeMem(recKeyAddress, recKey) + 1; // DEBUG

  dumpAddress = writeMem(tranKeyAddress, tranKey) + 1; // DEBUG

  Serial.println(readMem(recKeyAddress));
  Serial.println(readMem(tranKeyAddress));
}

const uint8_t writeMem (const uint8_t startAddr, const String value) {
  const uint8_t valueLen = value.length();

  for (uint8_t i = 0; i < valueLen; i++) EEPROM.update(startAddr + i, value[i]);
  EEPROM.update(startAddr + valueLen, 0);

  return valueLen;
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
  uint8_t *hash;

  Sha256.initHmac(key.c_str(), key.length());

  Sha256.print(msg);

  hash = Sha256.resultHmac();

  return hash;
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
              Serial.println("sending"); // DEBUG
              const String hashed("2:" + hash(String(unix), readMem(recKeyAddress)));
  
              writeMem(dumpAddress, hashed);
  
              manager.sendtoWait(hashed.c_str(), hashed.length(), from);
            }

            break;
          }
          case '3': { // recieved double-hashed unix
            Serial.println(readMem(tranKeyAddress));
            const String compare(hash(readMem(dumpAddress), readMem(tranKeyAddress)));
            Serial.print("C: ");
            Serial.println(compare);

            Serial.print("COMPARE ");
            Serial.println(param == compare);
  
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
