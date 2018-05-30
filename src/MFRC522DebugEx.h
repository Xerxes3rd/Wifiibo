#ifndef MFRC522DebugEx_h
#define MFRC522DebugEx_h

#include "MFRC522Ex.h"

class MFRC522DebugEx {
private:
	
public:
	// Get human readable code and type
	static const __FlashStringHelper *PICC_GetTypeName(MFRC522Ex::PICC_Type type);
	static const __FlashStringHelper *GetStatusCodeName(MFRC522Ex::StatusCode code);
};
#endif // MFRC522Debug_h
