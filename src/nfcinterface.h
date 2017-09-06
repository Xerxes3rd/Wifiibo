#ifndef NFCINTERFACE_H_INCLUDED
#define NFCINTERFACE_H_INCLUDED

class NFCInterface
{
	public:
		NFCInterface(){}
		virtual void begin()=0;
		virtual uint32_t getFirmwareVersion()=0;
		virtual bool SAMConfig()=0;
		virtual bool readID(uint8_t * uid, uint8_t * uidLength, uint16_t timeout)=0;
		virtual uint8_t ntag2xx_ReadPage (uint8_t page, uint8_t * buffer)=0;
		virtual uint8_t ntag2xx_WritePage (uint8_t page, uint8_t * data)=0;
};

#endif