#include <EEPROM.h>

#include <RHReliableDatagram.h>
#include <RH_ASK.h>
#include <SPI.h>

#include <sha256.h>

#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 2

RH_ASK rd;
RHReliableDatagram manager(rd, CLIENT_ADDRESS);

uint8_t cardKeyAddress = 0;

char *msg = "1";
uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
long messageDelay;

void setup () {
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
  uint8_t *hashed;
  String res;
  res.reserve(64);

  Sha256.initHmac(key.c_str(), key.length());

  Sha256.print(msg.c_str());

  hashed = Sha256.resultHmac();

  for (uint8_t i = 0; i < key.length(); i++) res.concat(String(hashed[i], HEX));

  return res;
}

void loop () {
  if (manager.sendtoWait(msg, strlen(msg), SERVER_ADDRESS)) {
    uint8_t buflen = sizeof(buf);
    uint8_t from;

    if (manager.recvfromAckTimeout(buf, &buflen, 2000, &from)) {
      if (from == SERVER_ADDRESS) {
        String bufString((char *)buf);

        const char status = bufString[0];
        const String param = bufString.substring(bufString.indexOf(':') + 1, bufString.length());

        switch (status) {
          case '2': { // received hashed unix
            char dHashed[36] = "3:";
            strcat(dHashed, hash(param, readMem(cardKeyAddress)).c_str());

            msg = dHashed;
            messageDelay = millis();

            break;
          }
        }          
      }
    } else msg = "1";
  } else if (millis() - messageDelay > 2000) msg = "1";
}
