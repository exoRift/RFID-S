#include <EEPROM.h>

#include <RH_ASK.h>
#include <SPI.h>

#include <sha256.h>

const RH_ASK rd;

const String tranKey("eThWmZq4t6w9z$C&F)J@NcRfUjXn2r5u"); // DEBUG

uint8_t tranKeyAddress = 0;

bool state = true;
String msg('1');
long tm;

void setup () {
  Serial.begin(9600); // DEBUG

  const bool driverStatus = rd.init();

  if (!driverStatus) Serial.println("Driver init failed"); // DEBUG
}

const uint8_t writeMem (const uint8_t startAddr, const String value) {
  const uint8_t valueLen = value.length();

  for (uint8_t i = 0; i < valueLen; i++) EEPROM.update(startAddr + i, value[i]);
  EEPROM.update(valueLen, 0);

  return valueLen;
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
  uint8_t *hash;

  Sha256.initHmac(key.c_str(), key.length());

  Sha256.print(msg);

  hash = Sha256.resultHmac();

  return hash;
}

const void setState (const bool newState, String newMsg = String('1')) {
  state = newState;
  msg = newMsg;

  if (newState) tm = millis();
}

void loop () {
  uint8_t buf[256];
  uint8_t buflen;

  if (!state && millis() - tm >= 50) setState(true);

  if (state) {
    rd.send(msg.c_str(), msg.length());
    rd.waitPacketSent();

    setState(true);
  } else {
    if (rd.recv(buf, &buflen)) {
      String bufString((char *)buf);
      bufString = bufString.substring(0, buflen);

      const String status(bufString.substring(0, bufString.indexOf(':')));
      const String param(bufString.substring(bufString.indexOf(':') + 1, bufString.length()));

      Serial.print(status); // DEBUG
      Serial.print(' '); // DEBUG
      Serial.println(param); // DEBUG

      switch (status.toInt()) {
        case 2: // received hashed unix
          const String dHashed("3:" + hash(String(param), readMem(tranKeyAddress)));

          setState(true, dHashed);
          break;
      }
    }
  }
}
