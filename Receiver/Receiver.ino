#include <EEPROM.h>

#include <RH_ASK.h>
#include <SPI.h>

#include <sha256.h>

#include <DS1307RTC.h>

const RH_ASK rd;

const String recKey("mZq4t7w!z%C*F)J@NcRfUjXn2r5u8x/A"); // DEBUG
const String tranKey("eThWmZq4t6w9z$C&F)J@NcRfUjXn2r5u"); // DEBUG

const uint8_t recKeyAddress = 0;
uint8_t tranKeyAddress;
uint8_t dumpAddress;

bool state;
String msg;
long stateTm;
long statusDelay;

const uint8_t statusPin = 8;

void setup () {
  pinMode(statusPin, OUTPUT);

  Serial.begin(9600); // DEBUG

  const bool driverStatus = rd.init();

  if (!driverStatus) Serial.println("Driver init failed"); // DEBUG

  tranKeyAddress = readMem(recKeyAddress).length();
  dumpAddress = readMem(tranKeyAddress).length();
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

const uint32_t getUnix () {
  const uint32_t unix = RTC.get();

  if (!unix) Serial.println(String("Unable to fetch time: ") + String((RTC.chipPresent() ? "chip present" : "chip not present"))); // DEBUG

  return unix;
}

const void setState (const bool newState, String newMsg = String()) {
  state = newState;
  msg = newMsg;

  stateTm = millis();
}

const void setStatus (const bool state) {
   digitalWrite(statusPin, state);
}

void loop () {
  uint8_t buf[256];
  uint8_t buflen;

  if (statusDelay && millis() - statusDelay >= 5000) {
    setStatus(false);
    statusDelay = 0;
  }

  if (state && millis() - stateTm >= 50) setState(false);

  if (state) {
    rd.send(msg.c_str(), msg.length());
    rd.waitPacketSent();
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
        case 1: // ready
          const uint32_t unix = getUnix();

          if (unix) {
            Serial.println("sending"); // DEBUG
            const String hashed("2:" + hash(String(unix), readMem(recKeyAddress)));

            writeMem(dumpAddress, hashed);

            setState(true, hashed);
          }

          break;
        case 3: // recieved double-hashed unix
          const String compare(hash(String(readMem(dumpAddress)), readMem(tranKeyAddress)));

          setStatus(param == compare);

          statusDelay = millis();
          break;
        default:
          const char msg = '9';

          rd.send(msg, strlen(msg));

          break;
      }
    }
  }
}
