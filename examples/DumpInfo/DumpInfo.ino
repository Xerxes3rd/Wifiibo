/**************************************************************************/
/*! 
    @file     iso14443a_uid.pde
    @author   Adafruit Industries
	@license  BSD (see license.txt)

    This example will attempt to connect to an ISO14443A
    card or tag and retrieve some basic information about it
    that can be used to determine what type of card it is.   
   
    Note that you need the baud rate to be 115200 because we need to print
	out the data and read from the card at the same time!

This is an example sketch for the Adafruit PN532 NFC/RFID breakout boards
This library works with the Adafruit NFC breakout 
  ----> https://www.adafruit.com/products/364
 
Check out the links above for our tutorials and wiring diagrams 
These chips use SPI or I2C to communicate.

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

*/
/**************************************************************************/
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532Ex.h>

// If using the breakout with SPI, define the pins for SPI communication.
#define PN532_SCK  (2)
#define PN532_MOSI (3)
#define PN532_SS   (D4)
#define PN532_MISO (5)

// If using the breakout or shield with I2C, define just the pins connected
// to the IRQ and reset lines.  Use the values below (2, 3) for the shield!
#define PN532_IRQ   (2)
#define PN532_RESET (3)  // Not connected by default on the NFC Shield

// Uncomment just _one_ line below depending on how your breakout or shield
// is connected to the Arduino:

// Use this line for a breakout with a SPI connection:
//Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

// Use this line for a breakout with a hardware SPI connection.  Note that
// the PN532 SCK, MOSI, and MISO pins need to be connected to the Arduino's
// hardware SPI SCK, MOSI, and MISO pins.  On an Arduino Uno these are
// SCK = 13, MOSI = 11, MISO = 12.  The SS line can be any digital IO pin.
Adafruit_PN532Ex nfc(PN532_SS);

// Or use this line for a breakout or shield with an I2C connection:
//Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

#if defined(ARDUINO_ARCH_SAMD)
// for Zero, output on USB Serial console, remove line below if using programming port to program the Zero!
// also change #define in Adafruit_PN532.cpp library file
   #define Serial SerialUSB
#endif

static bool tryFixCard = true;

void printData(uint8_t *data, int len, int valsPerCol, bool headers, bool formatForC)
{
  char temp[16] = {0};

  if (headers)
  {
    Serial.print("Offset(h)  ");
    for (int i = 0; i < valsPerCol; i++)
    {
      sprintf(temp, "%02X ", i);
      Serial.print(temp);
    }
    Serial.println("");
  }
  
  for (int i = 0; i < len; i+=valsPerCol)
  {
    if (headers)
    {
	  if (formatForC)
		sprintf(temp, " 0x%08X, ", i);  
	  else
        sprintf(temp, " %08X  ", i);
      Serial.print(temp);
    }
    for (int j = 0; j < valsPerCol; j++)
    {
      if (i+j < len)
      {
        sprintf(temp, "%02X ", data[i+j]);
        Serial.print(temp);
      }
    }
    Serial.println("");
  }
}

void printHex(int num, int precision) {
     char tmp[16];
     char format[128];

     sprintf(format, "0x%%.%dX", precision);

     sprintf(tmp, format, num);
     Serial.print(tmp);
}

void setup(void) {
  #ifndef ESP8266
    while (!Serial); // for Leonardo/Micro/Zero
  #endif
  Serial.begin(115200);
  Serial.println("Hello!");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // Set the max number of retry attempts to read from a card
  // This prevents us from waiting forever for a card, which is
  // the default behaviour of the PN532.
  //nfc.setPassiveActivationRetries(0xFF);
  
  // configure board to read RFID tags
  nfc.SAMConfig();
  
  Serial.println("Waiting for an ISO14443A card");
}

void loop(void) {
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };	// Buffer to store the returned UID
  uint8_t uidLength;				// Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  uint8_t data_fa[] = {0x00, 0x04, 0x04, 0x02};
  uint8_t data_fb[] = {0x01, 0x00, 0x11, 0x03};
  uint8_t data_fc[] = {0x01, 0x00, 0x00, 0x00};
  
  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  
  if (success) {
    Serial.println("Found a card!");
    Serial.print("UID Length: ");
    Serial.print(uidLength, DEC);
    Serial.println(" bytes");
    Serial.print("UID Value: ");
    for (uint8_t i=0; i < uidLength; i++) 
    {
      Serial.print(" 0x");Serial.print(uid[i], HEX); 
    }
    Serial.println("");

    uint8_t data[32];
    for (uint16_t i = 0x0000; i <= 0x00FF; i++)
    {
        success = nfc.ntag2xx_ReadPage((uint8_t)i, data);
        if (success)
        {
            Serial.print("Read page ");
            Serial.print(i);
            Serial.print("(");
            printHex(i, 2);
            Serial.print("): ");
            printData(data, 4, 16, false, false);

            if (tryFixCard)
            {
                if (i == 0x00FA)
                {
                    if ((data[0] != data_fa[0]) ||
                        (data[1] != data_fa[1]) ||
                        (data[2] != data_fa[2]) ||
                        (data[3] != data_fa[3]))
                    {
                        Serial.print("Setting data on page 0xFA...");
                        success = nfc.ntag2xx_WritePage((uint8_t)i, data_fa);
                        Serial.println(success ? "done." : "failure.");
                    }
                }
                else if (i == 0x00FB)
                {
                    if ((data[0] != data_fb[0]) ||
                        (data[1] != data_fb[1]) ||
                        (data[2] != data_fb[2]) ||
                        (data[3] != data_fb[3]))
                    {
                        Serial.print("Setting data on page 0xFB...");
                        success = nfc.ntag2xx_WritePage((uint8_t)i, data_fb);
                        Serial.println(success ? "done." : "failure.");
                    }
                }
                else if (i == 0x00FC)
                {
                    if ((data[0] != data_fc[0]) ||
                        (data[1] != data_fc[1]) ||
                        (data[2] != data_fc[2]) ||
                        (data[3] != data_fc[3]))
                    {
                        Serial.print("Setting data on page 0xFC...");
                        success = nfc.ntag2xx_WritePage((uint8_t)i, data_fc);
                        Serial.println(success ? "done." : "failure.");
                    }
                }
            }
        }
        else
        {
            Serial.print("Couldn't read page ");
            Serial.print(i);
            Serial.print("(0x");
            Serial.print(i, HEX);
            Serial.print("): ");
            break;
        }
    }
	// Wait 1 second before continuing
	delay(1000);
  }
  else
  {
    // PN532 probably timed out waiting for a card
    Serial.println("Timed out waiting for a card");
  }
}