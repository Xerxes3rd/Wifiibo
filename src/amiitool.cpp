#include "amiitool.h"

void amiitool::printData(uint8_t *data, int len, int valsPerCol, bool headers, bool formatForC)
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

void printSha1(const unsigned char * data, size_t len)
{
  uint8_t sha1_out[20];
  mbedtls_sha1(data, len, sha1_out);
  Serial.print("SHA1 Sum: ");
  for (int i = 0; i < 20; i++)
  {
    Serial.print(sha1_out[i], HEX);
  }
  Serial.println("");
}

bool keyValid(nfc3d_amiibo_keys * amiiboKeys)
{
  if ((amiiboKeys->data.magicBytesSize > 16) ||
    (amiiboKeys->tag.magicBytesSize > 16)) {
    return false;
  }

  return true;
}

#ifdef __XTENSA__
bool nfc3d_amiibo_load_keys_SPIFFS(nfc3d_amiibo_keys * amiiboKeys, const char * path) {
  fs::File f = SPIFFS.open(path, "r");
  if (!f) {
    return false;
  }

  if (!f.readBytes((char*)amiiboKeys, sizeof(*amiiboKeys))) {
    f.close();
    return false;
  }
  f.close();

  return keyValid(amiiboKeys);
}
#endif

#ifdef USE_SDFAT
bool nfc3d_amiibo_load_keys_SD(SdFat *sd, nfc3d_amiibo_keys * amiiboKeys, const char * path) {
  File f = sd->open(path, FILE_READ);
  if (!f) {
    return false;
  }

  if (!f.readBytes((char*)amiiboKeys, sizeof(*amiiboKeys))) {
    f.close();
    return false;
  }
  f.close();

  return keyValid(amiiboKeys);
}
#endif

#ifdef USE_SDFAT
amiitool::amiitool(char *keyFileName, SdFat *sdFat)
#else
amiitool::amiitool(char *keyFileName)
#endif
{
	cancelRead = false;
	nfcstarted = false;
	keyloaded = false;
	keyfilename = keyFileName;
#ifdef USE_SDFAT
	sd = sdFat;
#endif
}

bool amiitool::tryLoadKey()
{
  if (!isKeyLoaded())
  {
#ifdef __XTENSA__
    if (loadKeySPIFFS(keyfilename))
    {
		printSha1((unsigned char*)&amiiboKeys, sizeof(amiiboKeys));
		return true;
    }
#endif

#ifdef USE_SDFAT
	if (loadKeySD(keyfilename))
	{
		return true;
	}
    else
    {
		return false;
    }
#endif
  }
  
  return keyloaded;
}

#ifdef __XTENSA__
bool amiitool::loadKeySPIFFS(const char * keyfilename)
{
	keyloaded = nfc3d_amiibo_load_keys_SPIFFS(&amiiboKeys, keyfilename);
	return keyloaded;
}
#endif

#ifdef USE_SDFAT
bool amiitool::loadKeySD(const char * keyfilename)
{
	if (sd == NULL)
	{
		keyloaded = false;
	}
	else
	{
		keyloaded = nfc3d_amiibo_load_keys_SD(sd, &amiiboKeys, keyfilename);
	}
	return keyloaded;
}
#endif

bool amiitool::isKeyLoaded()
{
	return keyloaded;
}

#ifdef __XTENSA__
bool amiitool::isSPIFFSFilePossiblyAmiibo(fs::File *f)
{
	if (f && ((f->size() >= NFC3D_AMIIBO_SIZE_SMALL) && (f->size() <= NFC3D_AMIIBO_SIZE_HASH)))
		return true;
	
	return false;
}

int amiitool::loadFileSPIFFS(fs::File *f, bool lenient)
{
	int retval = -1;
	fileloaded = false;

	if (f == NULL) {
		return -1;
	}
	
	if (f->size() == NTAG215_SIZE)
	{
		if (f->readBytes((char*)original, NTAG215_SIZE))
		{
			retval = loadFileFromData(original, NTAG215_SIZE, lenient);
		}
		else
		{
			retval = -2;
		}
		
		f->close();
	}
	else if ((f->size() >= NFC3D_AMIIBO_SIZE) && (f->size() <= NFC3D_AMIIBO_SIZE_HASH))
	{
		if (f->readBytes((char*)original, NFC3D_AMIIBO_SIZE))
		{
			retval = loadFileFromData(original, NFC3D_AMIIBO_SIZE, lenient);
		}
		else
		{
			retval = -3;
		}
	}
	else if ((f->size() >= NFC3D_AMIIBO_SIZE_SMALL) && (f->size() < NFC3D_AMIIBO_SIZE))
	{
		memset((void*)original, 0, NFC3D_AMIIBO_SIZE);
		if (f->readBytes((char*)original, f->size()))
		{
			retval = loadFileFromData(original, NFC3D_AMIIBO_SIZE, lenient);
		}
		else
		{
			retval = -4;
		}
	}
	else
	{
		retval = -5;
	}
	
	if (retval < 0)
		return retval;
	
	//fileloaded = retval;
	
	//if (isEncrypted(original))
	//	decryptLoadedFile(false);
	
	return fileloaded;
}
#endif

void amiitool::generateBlankAmiibo(uint8_t * amiiboID)
{
	fileloaded = false;
	memset(modified, 0, NFC3D_AMIIBO_SIZE);
	
	const uint16_t InternalByte = 0x0001;
	modified[InternalByte] = 0x48;

	const uint16_t CCLoc = 0x0004;
	uint8_t ccBytes[4] = { 0xF1, 0x10, 0xFF, 0xEE };
	memcpy(modified+CCLoc, ccBytes, 4);

	const uint16_t A5Loc = 0x0028;
	modified[A5Loc] = 0xA5;

	const uint16_t CFG01Loc = 0x020C;
	uint8_t cfg01Bytes[8] = { 0x00, 0x00, 0x00, 0x04, 0x5F, 0x00, 0x00, 0x00 };
	memcpy(modified+CFG01Loc, cfg01Bytes, 8);

	const uint16_t StaticLockLoc = 0x0002;
	uint8_t staticLockBytes[2] = { 0x0F, 0xE0 };
	memcpy(modified+StaticLockLoc, staticLockBytes, 2);

	const uint16_t DynLockLoc = 0x0208;
	uint8_t dynLockBytes[4] = { 0x01, 0x00, 0x0F, 0xBD };
	memcpy(modified+DynLockLoc, dynLockBytes, 4);
	
	const uint16_t AmiiboIDLoc = 0x01DC;
	memcpy(modified+AmiiboIDLoc, amiiboID, 8);
	
	fileloaded = true;
}

void amiitool::generateRandomUID(uint8_t * uid, uint8_t * sizeLen)
{
	uid[0] = 0x04; // Manufacturer code for NXP
	
	for (int i = 1; i < 7; i++)
	{
		uid[i] = random(0, 256);
	}
	
	if (uid[4] == 0x88) // 0x88 is reserved, and indicates a 7-byte UID
		uid[4] = 0x89;

	// Force BCC1 to be 0
	//uid[6] = uid[3] ^ uid[4] ^ uid[5];
	
	*sizeLen = 7;
}

bool amiitool::loadFileFromData(uint8_t * filedata, int size, bool lenient)
{
	fileloaded = false;
	if (size == NTAG215_SIZE)
	{
		if (isEncrypted(filedata))
		{
			if (original != filedata)
			{
				memcpy(original, filedata, NTAG215_SIZE);
			}
			if (decryptLoadedFile(lenient) >= 0)
			{
				fileloaded = true;
			}
		}
		else
		{
			memcpy(modified, filedata, NFC3D_AMIIBO_SIZE);
			fileloaded = true;
		}
	}
	else if (size == NFC3D_AMIIBO_SIZE)
	{
		if (isEncrypted(filedata))
		{
			// Error
			fileloaded = false;
		}
		else
		{
			memcpy(modified, filedata, NFC3D_AMIIBO_SIZE);
			fileloaded = true;
		}
	}
	
	return fileloaded;
}

void amiitool::unloadFile()
{
	fileloaded = false;
}

int amiitool::encryptLoadedFile(uint8_t * uid)
{
	uint8_t bcc0;
	uint8_t bcc1;
	
	uint8_t pw[4];
	uint8_t newuid[8];
	
	if (!fileloaded)
		return -1;
	
	bcc0 = 0x88 ^ uid[0] ^ uid[1] ^ uid[2];
	bcc1 = uid[3] ^ uid[4] ^ uid[5] ^ uid[6];
	
	newuid[0] = uid[0];
	newuid[1] = uid[1];
	newuid[2] = uid[2];
	newuid[3] = bcc0;
	newuid[4] = uid[3];
	newuid[5] = uid[4];
	newuid[6] = uid[5];
	newuid[7] = uid[6];
	
	pw[0] = 0xAA ^ uid[1] ^ uid[3];
	pw[1] = 0x55 ^ uid[2] ^ uid[4];
	pw[2] = 0xAA ^ uid[3] ^ uid[5];
	pw[3] = 0x55 ^ uid[4] ^ uid[6];
	
	// Modify the UID record
	memcpy(modified+0x01D4, newuid, 8);
	
	// Add password
	memcpy(modified+0x0214, pw, 4);
	
	// Set defaults
	memset(modified+0x0208, 0, 3);
	memcpy(modified, &bcc1, 1);
	memset(modified+0x0002, 0, 2);
	
	nfc3d_amiibo_pack(&amiiboKeys, modified, original);
	
	memset(original+0x000A, 0, 2);
	memset(original+0x0208, 0, 3);

	// Clear the last 8 bytes- they can't be written to on NTAG215s
	//memset(modified+0x0214, 0, 8);
	
	return 0;
}

void amiitool::readUTF16BEStr(uint8_t *startByte, int stringLen, char *outStr, bool byteSwap)
{
	for (int i = 0; i < stringLen; i++)
	{
		uint8_t byte0 = *(startByte+(i*2)+0);
		uint8_t byte1 = *(startByte+(i*2)+1);

		if ((byte0 == 0x00) && (byte1 == 0x00))
		  break;

		int ind = i*6;
		outStr[ind+0] = '\\';
		outStr[ind+1] = 'u';
		sprintf(outStr+ind+2, "%02x", byteSwap ? byte1 : byte0);
		sprintf(outStr+ind+4, "%02x", byteSwap ? byte0 : byte1);
	}
}

int amiitool::readDecryptedFields()
{
	amiiboInfo.amiiboName[0] = '\0';
	amiiboInfo.amiiboOwnerMiiName[0] = '\0';
	
	if ((((modified[AMIIBO_DEC_FLAGS_BYTE_OFFSET] >> 4) & 0x01) == 0x01) &&
		(((modified[AMIIBO_DEC_FLAGS_BYTE_OFFSET] >> 5) & 0x01) == 0x01))
	{
		readUTF16BEStr(modified+AMIIBO_NAME_OFFSET, AMIIBO_NAME_LEN/2, amiiboInfo.amiiboName, false);
		readUTF16BEStr(modified+AMIIBO_DEC_OWNERMII_NAME_OFFSET, AMIIBO_NAME_LEN/2, amiiboInfo.amiiboOwnerMiiName, true);
	}
	
	readIDFields(modified, AMIIBO_DEC_CHARDATA_OFFSET, &amiiboInfo);
	return 0;
}

void amiitool::readIDFields(uint8_t *data, unsigned short offset, amiiboInfoStruct *info)
{
	memcpy(info->amiiboHead, data+offset, AMIIBO_HEAD_LEN);
	memcpy(info->amiiboTail, data+offset+AMIIBO_HEAD_LEN, AMIIBO_TAIL_LEN);
	for (int i = 0; i < AMIIBO_HEAD_LEN; i++)
		sprintf(info->amiiboHeadChar+(i*2), "%02X", info->amiiboHead[i]);
	info->amiiboHeadChar[AMIIBO_HEAD_LEN*2+1] = '\0';
	
	for (int i = 0; i < AMIIBO_TAIL_LEN; i++)
		sprintf(info->amiiboTailChar+(i*2), "%02X", info->amiiboTail[i]);
	info->amiiboTailChar[AMIIBO_TAIL_LEN*2+1] = '\0';
	
	info->amiiboCharacterNumber = ((*(data+offset+0) & AMIIBO_CHARNUM_MASK) << 8) | *(data+offset+1);
	info->amiiboVariation = *(data+offset+2);
	info->amiiboForm = *(data+offset+3);
	info->amiiboNumber = (*(data+offset+4) << 8) | *(data+offset+5);
	info->amiiboSet = *(data+offset+6);
}

int amiitool::decryptLoadedFile(bool lenient)
{
	if (amiitool::tryLoadKey())
	{
		//printData(original, NTAG215_SIZE, 16, true, false);
		if (!nfc3d_amiibo_unpack(&amiiboKeys, original, modified))
			if (!lenient)
				return -2;
		//printData(modified, NFC3D_AMIIBO_SIZE, 16, false, false);
		readDecryptedFields();
		return 0;
	}
	else
	{
		return -1;
	}
}

bool amiitool::isEncrypted(uint8_t * filedata)
{
	return !memcmp(filedata+0x0C, "\xF1\x10\xFF\xEE", 4);
}

bool amiitool::decryptedFileValid(uint8_t * filedata)
{
	return !memcmp(filedata+0x04, "\xF1\x10\xFF\xEE", 4);
}

bool amiitool::initNFC(NFCInterface *nfcObj)
{
  nfc = nfcObj;
  nfc->begin();

  nfcVersionData = nfc->getFirmwareVersion();
  if ((nfcVersionData == 0x00) || (nfcVersionData == 0xFF)) {
	return false;
  }
  
  nfc->getChipTypeString(nfcChip);
  nfc->getFirmwareVersionString(nfcChipFWVer);

  // configure board to read RFID tags
  nfc->SAMConfig();
  nfcstarted = true;
  return true;
}

void SendStatusMessage(StatusMessageCallback statusReport, char * message)
{
	yield();
	if (statusReport != NULL) 
		statusReport(message);
	yield();
}

void ReportProgressPercent(ProgressPercentCallback progressPercentReport, int pct)
{
	yield();
	if (progressPercentReport != NULL) 
		progressPercentReport(pct);
	yield();
}

bool amiitool::readTag(StatusMessageCallback statusReport, ProgressPercentCallback progressPercentReport) {
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  const float pctScaleVal = 100.0/((float)NTAG215_SIZE/4.0);
	uint8_t success;
  uint8_t uidLength;                        // Length of the UID
  int8_t tries;
  bool retval = false;
  cancelRead = false;

  if (!tryLoadKey()) {
    SendStatusMessage(statusReport, "readTag error: key not loaded.");
		return false;
  }
  
  if (!nfcstarted) {
		SendStatusMessage(statusReport, "readTag error: NFC hardware not available");
		return false;
  }

  SendStatusMessage(statusReport, "Place amiibo on the reader.");
  // Wait for an NTAG215 card.  When one is found 'uid' will be populated with
  // the UID, and uidLength will indicate the size of the UUID (normally 7)
  do
  {
		success = nfc->readID(uid, &uidLength, 500);
  } while (!success && !cancelRead);
  
  if (cancelRead)
  {
		SendStatusMessage(statusReport, "Connected.");
		cancelRead = false;
  }
  
  if (success) {
		//Serial.println("Tag UID read.");
    //Serial.println();
    //Serial.print(F("Tag UID: "));
    //PrintHexShort(uid, uidLength);
    //Serial.println();
		const uint32_t reportInvervalMS = 500;
    int pct = 0;
    ReportProgressPercent(progressPercentReport, pct);
    uint32_t lastTimer = millis();

    if (uidLength == 7)
    {     
      uint8_t data[32];

      SendStatusMessage(statusReport, "Starting dump...");

      for (uint8_t i = 0; i < NTAG215_SIZE/4; i++) 
      {
        tries = NFC_PAGE_READ_TRIES;
        while (tries > 0)
        {
          success = nfc->ntag2xx_ReadPage(i, data);
  
          if (success) 
          {
            // Dump the page data
            //PrintHexShort(data, 4);
            original[(i*4)+0] = data[0];
            original[(i*4)+1] = data[1];
            original[(i*4)+2] = data[2];
            original[(i*4)+3] = data[3];
            break;
          }
          else if (tries == 1)
          {
            char txt[32];
            sprintf(txt, "Unable to read page: %d", i);
            SendStatusMessage(statusReport, txt);
          }
          tries--;
          yield();
        }

        int newPct = (int)(pctScaleVal*(float)i);
        if ((millis() - lastTimer) > reportInvervalMS)
        //if (newPct > pct+9)
        {
          pct = newPct;
          ReportProgressPercent(progressPercentReport, pct);
          lastTimer = millis();
        }

				yield();
        
        if (!success)
          break;
      }

      ReportProgressPercent(progressPercentReport, 100);
    }
    else
    {
      SendStatusMessage(statusReport, "This doesn't seem to be an amiibo.");
      success = false;
    }
	
    if (success)
    {  
			//Serial.println("Tag dump:");
			//printData(original, NTAG215_SIZE, 4, false, true);
			yield();
      retval = loadFileFromData(original, NTAG215_SIZE, false);
			yield();
      
      if (!retval)
      {
        SendStatusMessage(statusReport, "Decryption error.");
      }
      else
      {
        //Serial.println("Decrypted tag data:");
        //printData(modified, NFC3D_AMIIBO_SIZE, 16, true, false);
        SendStatusMessage(statusReport, "Tag read successfully.");
      }
    }
  }
  else
  {
		if (cancelRead)
			SendStatusMessage(statusReport, "NFC read cancelled.");
		else
			SendStatusMessage(statusReport, "NFC read timeout.");
  }

	yield();
  cancelRead = false;
  nfc->ntag2xx_FinishedReading();
  yield();

  return retval;
}

void amiitool::cancelNFCRead() {
	cancelRead = true;
}

bool nfc__ntag2xx_WritePage(NFCInterface *nfc, uint8_t page, uint8_t * data)
{
	return nfc->ntag2xx_WritePage(page, data);
	//return true; 
}

bool amiitool::isCardRewritable()
{
	uint8_t pagebytes[] = {0, 0, 0, 0};
	uint8_t ZeroDynamicLockBytes[] = {0x00, 0x00, 0x00, 0x00};
	uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
	uint8_t uidLength;                        // Length of the UID
	bool retval = false;
	bool dynLockClear = false;
	bool uidChangeable = false;
	uint8_t origByte0 = 0x00;
	const uint8_t byte0Test = 0x05;

	// TODO: Try this code with non-rewritable tags
	if (nfc->ntag2xx_ReadPage(AMIIBO_DYNAMIC_LOCK_PAGE, pagebytes)) 
	{
		yield();
		if ((pagebytes[0] != 0) &&
			(pagebytes[1] != 0) &&
			(pagebytes[2] != 0) &&
			(pagebytes[3] != 0))
		{
			// Dynamic lock not clear, attempt to clear it
			if (nfc__ntag2xx_WritePage(nfc, AMIIBO_DYNAMIC_LOCK_PAGE, ZeroDynamicLockBytes))
			{
				yield();
				if (nfc->ntag2xx_ReadPage(AMIIBO_DYNAMIC_LOCK_PAGE, pagebytes)) 
				{
					yield();
					if ((pagebytes[0] == 0) &&
						(pagebytes[1] == 0) &&
						(pagebytes[2] == 0) &&
						(pagebytes[3] == 0))
					{
						if (nfc__ntag2xx_WritePage(nfc, AMIIBO_STATIC_LOCK_PAGE, ZeroDynamicLockBytes))
						{
							yield();
							uidChangeable = true;
						}
					}
				}
			}
		}
	}

/*
	// This code doesn't play nicely with non-rewritable tags
	if (nfc->ntag2xx_ReadPage(0, pagebytes))
	{
		origByte0 = pagebytes[0];
		pagebytes[0] = byte0Test;
		if (nfc__ntag2xx_WritePage(nfc, 0, pagebytes))
		{
			if (nfc->ntag2xx_ReadPage(0, pagebytes))
			{
				if (pagebytes[0] == byte0Test)
				{
					uidChangeable = true;
					pagebytes[0] = origByte0;
					nfc__ntag2xx_WritePage(nfc, 0, pagebytes);
				}
				else
				{
					Serial.println("isCardRewritable: Page 0 bytes don't match attempted write; tag is not rewritable");
				}
			}
			else
			{
				Serial.println("isCardRewritable: Unable to read page 0 after writing");
			}
		}
		else
		{
			Serial.println("isCardRewritable: Unable to write page 0, tag is not rewritable");
		}
	}
	else
	{
		Serial.println("isCardRewritable: Unable to read page 0");
	}
*/

	return uidChangeable;
}

bool amiitool::verifyPage(int pagenum, uint8_t * pagebytes)
{
	bool retval = false;
	bool success = false;

	uint8_t pagebytesTemp[] = {0, 0, 0, 0};

	Serial.print("Verifying page ");
	Serial.print(pagenum);
	Serial.print(": ");
	success = nfc->ntag2xx_ReadPage(pagenum, pagebytesTemp);

	if (success)
	{
		if ((*(pagebytes+0) != pagebytesTemp[0]) ||
			(*(pagebytes+1) != pagebytesTemp[1]) ||
			(*(pagebytes+2) != pagebytesTemp[2]) ||
			(*(pagebytes+3) != pagebytesTemp[3]))
		{
			Serial.print("mismatch: should be: ");
			printData(pagebytes, 4, 16, false, false);
			Serial.print(" but read: ");
			printData(pagebytesTemp, 4, 16, false, false);
			Serial.println("");
		}
		else
		{
			Serial.println("OK.");
			retval = true;
		}
	}
	else
	{
		Serial.println("Error reading page.");
	}

	return retval;
}

bool amiitool::writeTag(StatusMessageCallback statusReport, ProgressPercentCallback progressPercentReport)
{
	uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // Buffer to store the returned UID
	byte pagebytes[] = {0, 0, 0, 0};
	const float pctScaleVal = 100.0 / ((float)NTAG215_SIZE / 4.0);
	int pct = 0;
	uint8_t uidLength;										 // Length of the UID
	uint8_t success;
	int8_t tries;
	bool retval = false;
	//uint8_t currentPage = 3;
	uint8_t maxPage = 135;	
	bool cardLocked = false;
	bool cardRewritable = false;
	uint8_t temp = 0;
	cancelWrite = false;

	if (!tryLoadKey())
	{
		SendStatusMessage(statusReport, "writeTag error: key not loaded.");
		return false;
	}

	if (!nfcstarted)
	{
		SendStatusMessage(statusReport, "writeTag error: NFC hardware not available");
		return false;
	}

	SendStatusMessage(statusReport, "Place tag on the reader.");

	do
	{
		success = nfc->readID(uid, &uidLength, 500);
	} while (!success && !cancelWrite);

	if (cancelWrite)
	{
		SendStatusMessage(statusReport, "Connected.");
		cancelWrite = false;
	}

	if (success)
	{
		//Serial.println();
		//Serial.print(F("Tag UID: "));
		//amiitool::printData(uid, uidLength, 16, false, false);
		//PrintHexShort(uid, uidLength);
		//Serial.println();

		//cardRewritable = isCardRewritable(); // TODO: Difficult to check for rewritable cards, and they don't seem to work anyway
		if (cardRewritable)
		{
			SendStatusMessage(statusReport, "Rewritable card detected, generating random UID.");
			//return false;
			generateRandomUID(uid, &uidLength);
		}

		pct = 0;
		ReportProgressPercent(progressPercentReport, pct);

		if ((uidLength != 7) && !cardRewritable)
		{
			SendStatusMessage(statusReport, "Invalid tag UID.");
			return false;
		}
		else
		{
			Serial.print("Tag UID: ");
			for (int bN = 0; bN < 7; bN++)
			{
				Serial.print(uid[bN], HEX);
			}
			Serial.println("\n");
		}

		// Check the static & dynamic lock bytes if this card isn't a rewritable type
		if (!cardRewritable)
		{
			if (nfc->ntag2xx_ReadPage(AMIIBO_DYNAMIC_LOCK_PAGE, pagebytes))
			{
				if ((pagebytes[0] == DynamicLockBytes[0]) &&
						(pagebytes[1] == DynamicLockBytes[1]) &&
						(pagebytes[2] == DynamicLockBytes[2]))
				{
					SendStatusMessage(statusReport, "Cannot write amiibo: amiibo is locked (dynamic lock).");
					cardLocked = true;
				}
			}
			else
			{
				SendStatusMessage(statusReport, "Unable to read dynamic lock bytes.");
				cardLocked = true;
			}

			if (!cardLocked && !cardRewritable)
			{
				if (nfc->ntag2xx_ReadPage(AMIIBO_STATIC_LOCK_PAGE, pagebytes))
				{
					if ((pagebytes[2] == StaticLockBytes[2]) &&
							(pagebytes[3] == StaticLockBytes[3]))
					{
						SendStatusMessage(statusReport, "Cannot write amiibo: amiibo is locked (static lock).");
						cardLocked = true;
					}
				}
				else
				{
					SendStatusMessage(statusReport, "Unable to read static lock bytes.");
					cardLocked = true;
				}
			}
		}

		if (!cardLocked)
		{
			if (encryptLoadedFile(uid) < 0)
			{
				SendStatusMessage(statusReport, "Error encrypting file.");
				return false;
			}

			SendStatusMessage(statusReport, "Writing tag data...");

			if (cardRewritable)
			{
				for (volatile int8_t currentPage = 3; currentPage >= 0; currentPage--)
				{
					tries = NFC_PAGE_READ_TRIES;
					for (volatile uint8_t pagebyte = 0; pagebyte < NTAG215_PAGESIZE; pagebyte++)
					{
						pagebytes[pagebyte] = original[(currentPage * NTAG215_PAGESIZE) + pagebyte];
					}

					while (tries > 0)
					{
						yield();
						success = nfc__ntag2xx_WritePage(nfc, (uint8_t)currentPage, pagebytes);

						if (success)
						{
							//Serial.print("writeTag: Wrote page ");
							//Serial.println(currentPage);
							Serial.print("Wrote page ");
							Serial.print(currentPage);
							Serial.print(", bytes:");
							printData(pagebytes, 4, 16, false, false);
							yield();
							//success = verifyPage(currentPage, pagebytes);
							//yield();
							break;
						}
						else if (tries == 1)
						{
							char txt[32];
							sprintf(txt, "Unable to write page: %d, please try again.", currentPage);
							SendStatusMessage(statusReport, txt);
							//Serial.print("Page ");
							//Serial.print(currentPage);
							//Serial.print(" bytes:");
							//printData(pagebytes, 4, 16, false, false);
							break;
						}
						else
						{
							Serial.print("Error writing page ");
							Serial.println(currentPage);
						}
						tries--;
					}

					if (!success)
						break;
				}
			}

			for (volatile uint8_t currentPage = cardRewritable ? 4 : 3; currentPage < AMIIBO_PAGES; currentPage++)
			{
				tries = NFC_PAGE_READ_TRIES;
				for (volatile uint8_t pagebyte = 0; pagebyte < NTAG215_PAGESIZE; pagebyte++)
				{
					pagebytes[pagebyte] = original[(currentPage * NTAG215_PAGESIZE) + pagebyte];
				}

				if (currentPage >= AMIIBO_PAGES)
				{
					//Serial.println("Break, page count too high.");
					break;
				}

				while (tries > 0)
				{
					yield();
					success = nfc__ntag2xx_WritePage(nfc, currentPage, pagebytes);

					if (success)
					{
						//Serial.print("writeTag: Wrote page ");
						//Serial.println(currentPage);
						Serial.print("Wrote page ");
						Serial.print(currentPage);
						Serial.print(", bytes:");
						printData(pagebytes, 4, 16, false, false);
						yield();
						//success = verifyPage(currentPage, pagebytes);
						//yield();
						break;
					}
					else if (tries == 1)
					{
						char txt[32];
						sprintf(txt, "Unable to write page: %d, please try again.", currentPage);
						SendStatusMessage(statusReport, txt);
						//Serial.print("Page ");
						//Serial.print(currentPage);
						//Serial.print(" bytes:");
						//printData(pagebytes, 4, 16, false, false);
						break;
					}
					else
					{
						//Serial.print("writeTag: Error writing page ");
						//Serial.println(currentPage);
					}
					tries--;
				}

				int newPct = (int)(pctScaleVal * (float)currentPage);
				if (newPct > pct + 9)
				{
					pct = newPct;
					ReportProgressPercent(progressPercentReport, pct);
				}

				if (!success)
					break;
			}

			ReportProgressPercent(progressPercentReport, 100);

			if (success)
			{
				if (!nfc__ntag2xx_WritePage(nfc, AMIIBO_DYNAMIC_LOCK_PAGE, (uint8_t *)DynamicLockBytes))
				{
					SendStatusMessage(statusReport, "Failed to write static lock bytes, please try again.");
					return false;
				}
				else
				{
					Serial.print("Wrote page (dynamic lock) ");
					Serial.print(AMIIBO_DYNAMIC_LOCK_PAGE);
					Serial.print(", bytes:");
					printData((uint8_t *)DynamicLockBytes, 4, 16, false, false);
					yield();
				}
				
				
				//Serial.println("Wrote dynamic lock page.");
				pagebytes[0] = cardRewritable ? (uid[3] ^ uid[4] ^ uid[5] ^ uid[6]) : 0x00;
				pagebytes[1] = cardRewritable ? 0x48 : 0x00;
				pagebytes[2] = StaticLockBytes[2];
				pagebytes[3] = StaticLockBytes[3];
				if (!nfc__ntag2xx_WritePage(nfc, AMIIBO_STATIC_LOCK_PAGE, pagebytes))
				{
					SendStatusMessage(statusReport, "Failed to write dynamic lock bytes, please try again.");
					return false;
				}
				else
				{
					Serial.print("Wrote page (static lock) ");
					Serial.print(AMIIBO_STATIC_LOCK_PAGE);
					Serial.print(", bytes:");
					printData(pagebytes, 4, 16, false, false);
					yield();
				}

				retval = true;
			}
		}
	}

	return retval;
}

void amiitool::cancelNFCWrite() {
	cancelWrite = true;
}

NFCInterface* amiitool::getNFC() {
	return nfc;
}

bool amiitool::isNFCStarted() {
	return nfcstarted;
}
