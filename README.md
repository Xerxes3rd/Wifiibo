# Wifiibo - Wifi amiibo Management Tool
Wifiibo is a tool for managing an amiibo collection.  Using an ESP8266 (with the Arduino development environment) and an NFC reader, you can read & store existing amiibos, create new amiibos using amiibo data files, and view your amiibo data.  All amiibo data is stored on the flash memory of the ESP8266, so there's no need for additional storage hardware.  The whole thing is managed using any web browser!

The library that runs most of the amiibo functions is a port of [amiitool](https://github.com/socram8888/amiitool) for the ESP8266 using the Arduino environment.

## Features
* Save/back up existing amiibo
* Create new ammibos using data files (requires encryption keys)
* View your amiibo collection using any web browser (even on your phone!)

## Screenshots
Main Page:  
![Main Page](/screenshots/main.png?=raw=true "Main Page")
Filtering displayed amiibo:  
![Filters](/screenshots/filter.png?=raw=true "Filters")
Read an existing amiibo tag/figure:  
![Read Amiibo](/screenshots/read.png?=raw=true "Read amiibo")
Upload saved amiibo data:  
![Upload Amiibo](/screenshots/upload.png?=raw=true "Upload amiibo")

## Getting Started
### Hardware
Wifiibo requires the following hardware:
* ESP8266 (Wemos D1 Mini from Aliexpress: ~$3)
* PN532 NFC board (PN532 V3 board from Aliexpress: ~$5) or MFRC522 Board (MFRC522 RFID Reader module from Aliexpress: ~$5)

Remember to flip the DIP switches on the PN532 board to SPI mode (ch1: OFF, ch2: ON).

Make the following connections between the ESP8266 and the NFC board:

PN532 Pin | ESP Pin
--------|----------
MOSI | D7
MISO | D6
SCK | D5
SS | D4
VCC | +5V
GND | GND

MFRC-522 Pin | ESP Pin
--------|----------
SDA | D2
SCK | D5
MOSI | D7
MISO | D6
GND | GND
RST | D3
3.3v | 3.3v

### Flashing
If you already have the ESP toolchain (or at least utilities) installed, running the following command from a terminal/command prompt will flash the ESP:

`esptool.py --port <serial_port> write_flash -fm dio 0x00000 <path/to/Wifiibo_1.35.ino.d1_mini.bin>`

On Windows, the serial port will be something like `COM5`, on Linux it will be something like `/dev/ttyS1` or `/dev/ttyUSB0`, and on Mac something like `/dev/cu.usbserial-141130`.

If you don't have the ESP tools installed, but you have python (Linux,Mac), you can try to install them by running the following command from a terminal:
`pip install esptool`

### Compiling
Wifiibo depends on a number of additional Arduino libraries (which are needed if you want to compile Wifiibo):
* [EspAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP) 
* [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
* [ArduinoJson](https://github.com/bblanchon/ArduinoJson)
  * Use version 5; version 6 has a new API that is not yet supported

Wifiibo also uses modified versions of the following libraries; they are included with Wifiibo, so you don't need to download them separately (they also use different library names so they will coexist with existing copies):
* [Adafruit_PN532](https://github.com/adafruit/Adafruit-PN532)
* [rfid](https://github.com/miguelbalboa/rfid)
* [mbedtls](https://tls.mbed.org/)
  
#### IDE
In order to compile the Wifiibo software, you'll need the Arduino IDE.  Ensure you have the ESP8266 board support package version 2.5.0 installed (refer to [the main ESP8266 Arduino page](https://github.com/esp8266/Arduino) for instructions on setting it up).  Clone this repository into the 'libraries' folder in your Arduino sketch folder.  Open the Arduino IDE, and you'll find Wifiibo under the "Examples" menu.  After selecting your ESP8266 board in the IDE, click the Upload button to flash Wifiibo to the ESP8266.  Hint: After the inital upload, you can place existing amiibo dumps and the retail key file into the sketch Data directory, and use the "ESP8266 Sketch Data Upload" option in the "Tools" menu to upload them all at once.

## Using Wifiibo
When Wifiibo starts, if your ESP8266 isn't connected to Wifi, it will start up its own hotspot called "Wifiibo."  Connect to the hotspot using your phone or computer, then open a web browser and go to http://192.168.4.1 to access Wifiibo.  Click on the "Configure Wifi" button, enter your Wifi network credentials, and click submit.

Once connected to Wifi, you can access Wifiibo by going to http://wifiibo.local in your broswer.  Alternatively, some routers will allow you to go to [http://wifiibo.](http://wifiibo.) (note the '.' at the end).  Tip: Once you get to the Wifiibo page, you can click on the IP in the top-right corner of the page.  Using the IP directly seems to be faster on some Windows computers.

In order for Wifiibo to read or write any amiibo tags, it needs the amiibo encryption keys.  Click on the red button labeled "Upload keys" for further instructions.  Alternatively, if you already have a keys file, you can upload it using the "Upload amiibo" button (the keys file must be named "key_retail.bin").  Once you have successfully imported the amiibo encryption keys, the red "Upload keys" button will disappear.

Using Wifiibo is fairly straightforward.  amiibo data is pulled from the excellent [amiibo API](https://github.com/N3evin/AmiiboAPI/) by @N3vin.
In order to write amiibo information to a tag, the tag must be type NTAG215 and must be blank.

### Updating
If new software releases are made, they will show up in the "releases" folder.  To easily update your Wifiibo, download the latest release binary, then go to http://wifiibo.local/update.  Select the "bin" file you downloaded, and click the "Update" button.  When updating is complete, the page will change to "OK."  Go back to http://wifiibo.local and verify that the version number in the upper-right corner of the screen has been updated.

## Enclosure
If you have access to a 3D printer, you can print an enclosure for Wifiibo.  The enclosure is in two halves, which press-fit together.  The lid piece will need to be printed upside-down and requires support material for the "inset" on the top.  The Wemos D1 Mini board can be press-fit into the base, and the NFC board can be screwed into the standoffs in the lid.  I made the standoffs slightly smaller than the #4-40 screws I used, so I tapped the mounts using an inexpensive tapping tool.  Alternatively, there's another lid version that allows the NFC board to be pressfit into the lid.

Enclosure (created in OpenSCAD):  
![Enclosure Rendering](/screenshots/enclosure-render.png?=raw=true "Enclosure Rendering")

Assembled Enclsoure (Open):  
![Enclosure Assembled Open](/screenshots/enclosure-open.png?=raw=true "Enclosure Assembled (Open)")

Assembled Enclosure (Closed):  
![Enclosure Assembled Closed](/screenshots/enclosure-closed.png?=raw=true "Enclosure Assembled (Closed)")

## Developer Notes
* The main Wifiibo page is called amiitool.htm.  There's a python script & accompanying executable that's used to gzip the page, then convert the binary into a C-style header file so the page is embedded in the firmware.  This way, any future firmware updates will contain the updated web page (otherwise users would have to upload it separately).
* The main page (amiibool.htm) is pretty messy

## References & Credits
Wifiibo mostly uses software & libraries written by others.  The following resources were used to develop Wifiibo:
* [Kostia Plays](https://games.kel.mn/en/create-amiibo-clones-with-arduino/) - Tons of info on how to read & write amiibos by @konstantin-kelemen
* [amiitool](https://github.com/socram8888/amiitool) - Software to encrypt/decrypt amiibo data by @socram8888
* [GBATemp](http://wiki.gbatemp.net/wiki/Amiibo) - amiibo data structure information
* [3dBrew](https://www.3dbrew.org/wiki/Amiibo) - amiibo/NTAG215 data structure information
* [amiibo API](https://github.com/N3evin/AmiiboAPI/) - Online amiibo database & index by @N3vin

### Possible Enhancements
* ~~MFRC-522 NFC chip support~~ Complete!
  * ~~Pull requests welcome; ensure it implements the NFCInterface pure virtual class.~~
* Card emulation support using PN532
  * Not sure if this is possible- the documentation on the PN532 states that in emulation mode, only 4-byte IDs are supported.  Might be able to supply raw page data to get it to work.
* SD card support
  * The amiitool library has some SD support (using the SdFat library for long filename support), but it has not been tested.
* The Wifi scan results JSON info in the main program might need to be changed from static to dynamic to handle a large number of results
* Bulk upload & download (via ZIP file)
  * This should be handled in the JavaScript frontend, since the ESP8266 doesn't have enough memory to handle it
