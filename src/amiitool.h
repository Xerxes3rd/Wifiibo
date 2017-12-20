#ifndef AMIITOOL_H
#define AMIITOOL_H

#include <string.h>
#include "amiibo.h"
//#define USE_SDFAT

#ifdef __XTENSA__
#define FS_NO_GLOBALS //allow spiffs to coexist with SD card, define BEFORE including FS.h
#include "FS.h"
#endif
#ifdef USE_SDFAT
#include <SdFat.h>
#endif
#include <NFCInterface.h>

#define AMIIBO_KEY_FILE_SIZE 160
#define NTAG215_SIZE 540
#define NTAG215_PAGESIZE 4
#define AMIIBO_NAME_LEN 20 // Actually 10 UTF-16 chars
#define AMIIBO_PAGES 135
#define AMIIBO_DYNAMIC_LOCK_PAGE 130
#define AMIIBO_STATIC_LOCK_PAGE 2

// These offsets are in the decrypted data
#define AMIIBO_NAME_OFFSET 			0x003C		// Offset in decrypted data
#define AMIIBO_DEC_CHARDATA_OFFSET 	0x01DC		// Offset in decrypted data
#define AMIIBO_ENC_CHARDATA_OFFSET	0x0054		// Offset in encrypted data
#define AMIIBO_DEC_OWNERMII_NAME_OFFSET	0x0066
#define AMIIBO_CHARNUM_MASK			0xFF
#define AMIIBO_HEAD_LEN				4
#define AMIIBO_TAIL_LEN				4

#define NFC_PAGE_READ_TRIES 10

typedef struct
{
	char amiiboName[AMIIBO_NAME_LEN*6+1];
	char amiiboOwnerMiiName[AMIIBO_NAME_LEN*6+1];
	unsigned short amiiboCharacterNumber;
	byte amiiboVariation;
	byte amiiboForm;
	unsigned short amiiboNumber;
	byte amiiboSet;
	uint8_t amiiboHead[AMIIBO_HEAD_LEN];
	uint8_t amiiboTail[AMIIBO_TAIL_LEN];
	char amiiboHeadChar[AMIIBO_HEAD_LEN*2+1];
	char amiiboTailChar[AMIIBO_TAIL_LEN*2+1];
} amiiboInfoStruct;

typedef void (*StatusMessageCallback)(char *);
typedef void (*ProgressPercentCallback)(int);

class amiitool
{
	public:
		nfc3d_amiibo_keys amiiboKeys;

		uint8_t original[NTAG215_SIZE];
		uint8_t modified[NFC3D_AMIIBO_SIZE];

		uint8_t plain_base[NFC3D_AMIIBO_SIZE];
		uint8_t plain_save[NFC3D_AMIIBO_SIZE];
		
		amiiboInfoStruct amiiboInfo;
		uint32_t nfcVersionData;
		char nfcChip[8];
		char nfcChipFWVer[8];

		amiitool(char *keyFileName
#ifdef USE_SDFAT
		, SdFat *sdFat = NULL);
#else
		);
#endif
		bool tryLoadKey();
#ifdef __XTENSA__
		bool loadKeySPIFFS(const char * keyfilename);
#endif
		bool loadKeySD(const char * keyfilename);
		bool isKeyLoaded();
#ifdef __XTENSA__
		static bool isSPIFFSFilePossiblyAmiibo(fs::File *f);
		int loadFileSPIFFS(fs::File *f, bool lenient);
#endif
		bool loadFileFromData(uint8_t * filedata, int size, bool lenient);
		void unloadFile();
		int encryptLoadedFile(uint8_t * uid);
		int decryptLoadedFile(bool lenient);
		bool isEncrypted(uint8_t * filedata);
		bool decryptedFileValid(uint8_t * filedata);
		void readIDFields(uint8_t *data, unsigned short offset, amiiboInfoStruct *info);
		bool initNFC(NFCInterface *nfc);
		bool readTag(StatusMessageCallback statusReport, ProgressPercentCallback progressPercentReport);
		void cancelNFCRead();
		bool writeTag(StatusMessageCallback statusReport, ProgressPercentCallback progressPercentReport);
		void cancelNFCWrite();
		
		static void printData(uint8_t *data, int len, int valsPerCol, bool headers, bool formatForC);
		static void readUTF16BEStr(uint8_t *startByte, int stringLen, char *outStr, bool byteSwap);
		
	private:
		bool keyloaded;
		bool fileloaded;
		bool nfcstarted;
		char *keyfilename;
		volatile bool cancelRead;
		volatile bool cancelWrite;
		NFCInterface *nfc;
#ifdef USE_SDFAT
		SdFat *sd;
#endif
		
		int readDecryptedFields();
};

#endif