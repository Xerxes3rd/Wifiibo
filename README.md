# Wifiibo - Wifi amiibo Management Tool
Wifiibo is a tool for managing an amiibo collection.  Using an ESP8266 (with the Arduino development environment) and an NFC reader, you can read & store existing amiibos, create new amiibos using amiibo data files, and view your amiibo data.  All amiibo data is stored on the flash memory of the ESP8266, so there's no need for additional storage hardware.  The whole thing is managed using any web browser!

The library that runs most of the amiibo functions is a port of [amiitool](https://github.com/socram8888/amiitool) for the ESP8266 using the Arduino environment.

## Features
* Save/back up existing amiibo
* Create blank amiibos without any existing dumps (requires encryption keys)
* Create new ammibos using data files (requires encryption keys)
* View your amiibo collection using any web browser (even on your phone!)


## Getting Started
### Hardware
Wifiibo requires the following hardware:
* ESP8266 (Wemos D1 Mini from Aliexpress: ~$3)
* PN532 NFC board (**RECOMMENDED**) (PN532 V3 board from Aliexpress: ~$5) 


Remember to flip the DIP switches on the PN532 board to SPI mode (ch1: OFF, ch2: ON).


Wifiibo also uses modified versions of the following libraries; they are included with Wifiibo, so you don't need to download them separately (they also use different library names so they will coexist with existing copies):
* [Adafruit_PN532](https://github.com/adafruit/Adafruit-PN532)
* [rfid](https://github.com/miguelbalboa/rfid)
* [mbedtls](https://tls.mbed.org/)
 
## References & Credits
Wifiibo mostly uses software & libraries written by others.  The following resources were used to develop Wifiibo:
* [Kostia Plays](https://games.kel.mn/en/create-amiibo-clones-with-arduino/) - Tons of info on how to read & write amiibos by @konstantin-kelemen
* [amiitool](https://github.com/socram8888/amiitool) - Software to encrypt/decrypt amiibo data by @socram8888
* [GBATemp](http://wiki.gbatemp.net/wiki/Amiibo) - amiibo data structure information
* [3dBrew](https://www.3dbrew.org/wiki/Amiibo) - amiibo/NTAG215 data structure information
* [amiibo API](https://github.com/N3evin/AmiiboAPI/) - Online amiibo database & index by @N3vin

