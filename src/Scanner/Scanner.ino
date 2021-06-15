#include <EEPROM.h>

#include <RHReliableDatagram.h>
#include <RH_ASK.h>
#include <SPI.h>

#include <sha256.h>

#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 2

RH_ASK rd;
RHReliableDatagram manager(rd, SERVER_ADDRESS);

const uint8_t scannerKeyAddress = 0;
const uint8_t cardKeyAddress = scannerKeyAddress + readMem(scannerKeyAddress).length();
const uint8_t dumpAddress = cardKeyAddress + readMem(cardKeyAddress).length();

uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
long statusDelay;

const uint8_t statusPin = 8;

void setup () {
  pinMode(statusPin, OUTPUT);

  manager.init();
}

const uint8_t writeMem (const uint8_t startAddr, const char *value) {
  const uint8_t valueLen = strlen(value);

  for (uint8_t i = 0; i <= valueLen; i++) EEPROM.update(startAddr + i, value[i]);

  return startAddr + valueLen;
}

const String readMem (const uint8_t startAddr) {
  String res;
  res.reserve(32);

  for (uint8_t i = 0; i < 32; i++) {
    const uint8_t index = EEPROM.read(startAddr + i);

    if (!index) break;

    res.concat(char(index));
  }

  return res;
}

const String hash (const String msg, const String key) {
  uint8_t *hashed = "";
  String res;
  res.reserve(64);

  Sha256.initHmac(key.c_str(), key.length());

  Sha256.print(msg.c_str());

  hashed = Sha256.resultHmac();

  for (uint8_t i = 0; i < key.length(); i++) res.concat(String(hashed[i], HEX));

  return res;
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

        const char status = bufString[0];
        const String param = bufString.substring(bufString.indexOf(':') + 1, bufString.indexOf('|'));

        switch (status) {
          case '1': { // ready
            const String unix(millis());

            if (unix) {
              const String hashed = hash(unix, readMem(scannerKeyAddress));
              char msg[36] = "2:";
              strcat(msg, hashed.c_str());

              writeMem(dumpAddress, hashed.c_str());

              manager.sendtoWait(msg, strlen(msg), from);
            }

            break;
          }
          case '3': { // recieved double-hashed unix
            const String compare = hash(readMem(dumpAddress), readMem(cardKeyAddress));

            setStatus(param == compare);

            statusDelay = millis();

            break;
          }
          default: {
            const char msg = '9';

            manager.sendtoWait(msg, strlen(msg), from);

            break;
          }
        }
      }
    }
  }
}
