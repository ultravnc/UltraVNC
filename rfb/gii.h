#ifndef GII_H
#define GII_H
/*
 * gii.h - header file for the GII pseudo encoding
 *
 */

/* TODO: sinnvolle werte ? */
#define rfbGIIMinVersion	1
#define rfbGIIMaxVersion	1

/* pseudo encoding */
#define rfbEncodingGII		   0xFFFFFECF

/* message types */
#define rfbGIIMessage 253

/* message sub-types */
#define _rfbGIIEventInjection 0
#define _rfbGIIVersionMessage 1
#define _rfbGIIDeviceCreation 2
#define _rfbGIIDeviceDestruction 3 

#define rfbGIIBigEndian 128

#define rfbGIIEventInjection (rfbGIIBigEndian | _rfbGIIEventInjection)
#define rfbGIIVersionMessage (rfbGIIBigEndian | _rfbGIIVersionMessage)
#define rfbGIIDeviceCreation (rfbGIIBigEndian | _rfbGIIDeviceCreation)
#define rfbGIIDeviceDestruction (rfbGIIBigEndian | _rfbGIIDeviceDestruction) 

/* event masks */
#define rfbGIIevMaskValuatorAbsolute 0x00002000

/* valuator flags */
#define VAL_MT_PRESSED	0x8000
#define VAL_MT_PRIMARY	0x2000
#define VAL_MT_SIZE	0x1000
#define VAL_MT_S2	0x0800
#define VAL_MT_RECT	0x0400
#define VAL_MT_PRESSURE	0x0200
#define VAL_MT_TIME	0x0100
#define VAL_MT_HIRES	0x0080

/* format flags */
#define VAL_MT_PIXFMT_16	0x0010
#define VAL_MT_IDFMT_32		0x0001
#define VAL_MT_IDFMT_CLEAR	0x000F

#define rfbGIIEndianType 128

typedef struct {
	uint8_t messageType;
	uint8_t subType;
	uint16_t length;
	/* followed by sub-type specific data */
} rfbGIIMsgHeader; 

#define sz_rfbGIIMsgHeader 4

typedef struct {
	rfbGIIMsgHeader header;
	uint16_t maxVersion;
	uint16_t minVersion;
} rfbGIIServerVersionMsg; 

#define sz_rfbGIIServerVersionMsg (4 + sz_rfbGIIMsgHeader)

typedef struct {
	rfbGIIMsgHeader header;
	uint16_t version;
} rfbGIIClientVersionMsg; 

#define sz_rfbGIIClientVersionMsg (2 + sz_rfbGIIMsgHeader)

typedef struct {
	rfbGIIMsgHeader header;
	uint32_t deviceOrigin;
} rfbGIIServerDeviceCreationMsg; 

#define sz_rfbGIIServerDeviceCreationMsg (4 + sz_rfbGIIMsgHeader)

typedef struct {
	rfbGIIMsgHeader header;
	uint8_t deviceName[31];
	uint8_t DNTerm;
	uint32_t vendorID;
	uint32_t productID;
	uint32_t eventMask;
	uint32_t numRegisters;
	uint32_t numValuators;
	uint32_t numButtons;
	/* valuators */
} rfbGIIClientDeviceCreationMsg; 

#define sz_rfbGIIClientDeviceCreationMsg (56 + sz_rfbGIIMsgHeader)

typedef struct {
	uint32_t index;
	uint8_t longName[74];
	uint8_t LNTerm;
	uint8_t shortName[4];
	uint8_t SNTerm;
	uint32_t rangeMin;
	uint32_t rangeCenter;
	uint32_t rangeMax;
	uint32_t SIUnit;
	uint32_t SIAdd;
	uint32_t SIMul;
	uint32_t SIDiv;
	uint32_t SIShift;
} rfbGIIValuatorMsg; 

#define sz_rfbGIIValuatorMsg 116

typedef struct {
	rfbGIIClientDeviceCreationMsg dc;
	rfbGIIValuatorMsg v;
} rfbGIIMTCreationMsg;

#define sz_rfbGIIMTCreationMsg (sz_rfbGIIClientDeviceCreationMsg + sz_rfbGIIValuatorMsg)

typedef struct {
	rfbGIIMsgHeader header;
	uint8_t eventSize;
	uint8_t eventType;
	uint16_t padding;
	uint32_t deviceOrigin;
	uint32_t first;
	uint32_t count;
	/* value array */
} rfbGIIValuatorEventMsg;

#define sz_rfbGIIValuatorEventMsg (sz_rfbGIIMsgHeader + 16)

typedef union {
	rfbGIIMsgHeader h; 
	rfbGIIServerVersionMsg sv; 
	rfbGIIClientVersionMsg cv; 
	rfbGIIServerDeviceCreationMsg sdc;
} rfbGIIMsg;

typedef struct {
	uint16_t f1;
	uint8_t	 f2;
	uint8_t	 fmt;
} rfbGIIValuatorFlags;

#define sz_rfbGIIValuatorFlags 4

typedef struct {
	int32_t id; 
} rfbGIIValuatorID;

#define sz_rfbGIIValuatorID 4

typedef struct {
	uint16_t x; 
	uint16_t y;
} rfbGIIValuatorPos;

#define sz_rfbGIIValuatorPos 4

typedef struct {
	int32_t msec; 
} rfbGIIValuatorTime;

#define sz_rfbGIIValuatorTime 4

typedef struct {
	int64_t usec; 
} rfbGIIValuatorHPTime;

#define sz_rfbGIIValuatorHPTime 8

#endif /* GII_H */
