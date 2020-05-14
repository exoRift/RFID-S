#include <EEPROM.h>

#include <RHReliableDatagram.h>
#include <RH_ASK.h>
#include <SPI.h>

#include <sha256.h>

#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 2

RH_ASK rd;
RHReliableDatagram manager(rd, CLIENT_ADDRESS);

const String cardKey("eThWmZq4t6w9z$C&F)J@NcRfUjXn2r5u"); // DEBUG

uint8_t cardKeyAddress = 0;

const String defaultMsg('1');
const String msg = defaultMsg;
uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];

void setup () {
  Serial.begin(9600); // DEBUG

  const bool managerStatus = manager.init();

  if (!managerStatus) Serial.println("Manager init failed"); // DEBUG

  writeMem(cardKeyAddress, cardKey); // DEBUG
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

    if (index == 0) break;

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

void loop () {
  if (manager.sendtoWait(msg.c_str(), msg.length(), SERVER_ADDRESS)) {
    uint8_t buflen = sizeof(buf);
    uint8_t from;

    if (manager.recvfromAckTimeout(buf, &buflen, 2000, &from)) {
      Serial.println("RECEIVED");
      if (from == SERVER_ADDRESS) {
        String bufString((char *)buf);
        bufString = bufString.substring(0, buflen);
  
        const char status = bufString[0];
        const String param(bufString.substring(bufString.indexOf(':') + 1, bufString.length()));
  
        Serial.print(status); // DEBUG
//        Serial.print(' '); // DEBUG
        Serial.print("P: ");
        Serial.println(param); // DEBUG
  
        switch (status) {
          case '2': { // received hashed unix
            const String dHashed("3:" + hash(param, readMem(cardKeyAddress)));
            Serial.println(readMem(cardKeyAddress));
            Serial.print("H: ");
            Serial.println(dHashed);
  
            msg = dHashed;
            break;
          }
        }          
      }
    } else msg = defaultMsg;
  }
}
