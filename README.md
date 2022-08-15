![Banner](assets/Splash.png)
### RFID-S is to RFID what HTTPS is to HTTP&mdash; a more secure rendition of an existing protocol.
---
### RFID currently works by sending an authentication token over radio frequency. Unfortunately, this leaves that token vulnerable to being "skimmed" by malicious scanners.

> <img alt='current' src='assets/flowcharts/Current.png' width='500'>

### <br/>RFID-S, however, generates a one-time-use code using an encryption key where the scanner sends a random string of bytes to the client, both the client and scanner encrypt that string with the same token, and the client sends it back to the scanner to compare and authenticate. This is far more secure than RFID because no static code or encryption key is sent over RF to be stolen.

> <img alt='new' src='assets/flowcharts/New.png' width='500'>

### <br/>Besides for classical RFID applications such as building-access ID cards, this method can also be applied to credit card checkout as a means of authorizing purchases.

> <img alt='Checkout' src='assets/flowcharts/Checkout.png' width='500'>

Necessary Materials for barebones operation
---
## Scanner:
- Microprocessor
- EEPROM
- RF transmitter
- RF receiver
- Induction emission coil (to power the card/client)

## Card/Client:
- Microprocessor
- RF transmitter
- RF receiver
- EEPROM
- Induction reception coil

Pins
---
- RX: 11
- TX: 12

---
##### *RFID-S Is originally written in Arduino but is a general concept that can be applied to any lanuage and use-case*
