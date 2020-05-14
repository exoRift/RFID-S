#include <EEPROM.h>

#include <RHReliableDatagram.h>
#include <RH_ASK.h>
#include <SPI.h>

#include <sha256.h>

#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 2

RH_ASK rd;
RHReliableDatagram manager(rd, CLIENT_ADDRESS);

const char *cardKey = "eThWmZq4t6w9z$C&F)J@NcRfUjXn2r5u"; // DEBUG

uint8_t cardKeyAddress = 0;

char *msg = "1";
uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];

void setup () {
  Serial.begin(9600); // DEBUG

  const bool managerStatus = manager.init();

  if (!managerStatus) Serial.println("Manager init failed"); // DEBUG

  writeMem(cardKeyAddress, cardKey); // DEBUG
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
  Serial.println(msg);
  if (manager.sendtoWait(msg, strlen(msg), SERVER_ADDRESS)) {
    Serial.println("sent");
    uint8_t buflen = sizeof(buf);
    uint8_t from;

    if (manager.recvfromAckTimeout(buf, &buflen, 2000, &from)) {
      Serial.println("rec");
      if (from == SERVER_ADDRESS) {
        String bufString((char *)buf);
  
        const char status = bufString[0];
        const String param = bufString.substring(bufString.indexOf(':') + 1, bufString.length());
  
        Serial.print(status); // DEBUG
        Serial.print(' '); // DEBUG
        Serial.print("P: ");
        Serial.println(param); // DEBUG
  
        switch (status) {
          case '2': { // received hashed unix
            char dHashed[66] = "3:";
            strcat(dHashed, hash(param, readMem(cardKeyAddress)).c_str());

            Serial.println(readMem(cardKeyAddress));
            Serial.print("H: ");
            Serial.println(dHashed);
  
            msg = dHashed;
            break;
          }
        }          
      }
    } else msg = "1";
  }
}
