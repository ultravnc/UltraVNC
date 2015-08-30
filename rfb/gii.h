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


/////////////////////////////////////////////////////
//RFB extention
// #define MF_flag 0x400000 // pointer move
// #define UF_flag 0x200000 // pointer up
//+-- + -- + -- + -- + -- + -- + -- + -- + -- + -------- - +------------ + ---------- - +
//| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22  .. 8 | 7 .. 4   | 3 .. 0    |
//+-- + -- + -- + -- + -- + -- + -- + -- + -- + -------- - +------------ + ---------- - +
//| PF | R | IF | S1 | S2 | RT | PR | TI | HC  | Reserved|Pix-Format| ID Format |
//+-- + -- + -- + -- + -- + -- + -- + -- + -- + -------- - +------------ + ---------- - +
//
/////////////////////////////////////////////////////
#define PF_flag 0x80000000// 'P'ressed 'F'lag : es wird übertragen, ob der Touchpunkt gedrückt oder losgelassen wurde. 
#define R1_flag 0x40000000 //Reserved 1
#define IF_flag 0x20000000//pr'I'mary 'F'lag : es wird übertragen, ob der Touchpunkt der primäre Touchpunkt ist.
#define S1_flag 0x10000000//'S'ize Flag : es werden zusätzlich zur Position auch die Daten der Touchfläche übertragen. Die Touchfläche wird(derzeit) als symmetrische Ellipse übertragen
#define S2_flag 0x8000000//reserviert für asymetrische Ellipse, wird momentan nicht unterstützt und muss 0 sein.
#define RT_flag 0x4000000//'R'ec't'angle : Die Touchfläche wurde vom Treiber als Rechteck, und nicht als Ellipse ermittelt.
#define PR_flag 0x2000000//'Pr'essure Flag : Der Druck, welcher auf die Touchfläche ausgeübt wird, wird mit übertragen.
#define TI_flag 0x1000000//'Ti'mestamp : Der Zeitstempel zu dem das Ereignis am Touchdevice entstanden ist, wird mit übertragen.
#define HC_flag 0x800000//'H'igh Performance 'C'ounter

// Reserverd   0x40000-0x100
#define FORMAT_0_flag 0x0
#define LANGE_16_flag 0x10 //16bit signed für x gefolgt von 16bit signed für y gemeinsam in einem DWORD
#define LANGE_32_flag 0x20 //32bit signed für x gefolgt von 32bit signed für y in jeweils einem DWORD
#define LANGE_64_flag 0x30 //64bit signed für x gefolgt von 64bit signed für y in jeweils 2 DWORD
#define IDFORMAT_32  0x1 //32bit ID
#define IDFORMAT_CLEAR 0xF // No more touch points

// just define else we need increase the winver setting
#define POINTER_FLAG_NONE               0x00000000 // Default
#define POINTER_FLAG_NEW                0x00000001 // New pointer
#define POINTER_FLAG_INRANGE            0x00000002 // Pointer has not departed
#define POINTER_FLAG_INCONTACT          0x00000004 // Pointer is in contact
#define POINTER_FLAG_FIRSTBUTTON        0x00000010 // Primary action
#define POINTER_FLAG_SECONDBUTTON       0x00000020 // Secondary action
#define POINTER_FLAG_THIRDBUTTON        0x00000040 // Third button
#define POINTER_FLAG_FOURTHBUTTON       0x00000080 // Fourth button
#define POINTER_FLAG_FIFTHBUTTON        0x00000100 // Fifth button
#define POINTER_FLAG_PRIMARY            0x00002000 // Pointer is primary
#define POINTER_FLAG_CONFIDENCE         0x00004000 // Pointer is considered unlikely to be accidental
#define POINTER_FLAG_CANCELED           0x00008000 // Pointer is departing in an abnormal manner
#define POINTER_FLAG_DOWN               0x00010000 // Pointer transitioned to down state (made contact)
#define POINTER_FLAG_UPDATE             0x00020000 // Pointer update
#define POINTER_FLAG_UP                 0x00040000 // Pointer transitioned from down state (broke contact)
#define POINTER_FLAG_WHEEL              0x00080000 // Vertical wheel
#define POINTER_FLAG_HWHEEL             0x00100000 // Horizontal wheel
#define POINTER_FLAG_CAPTURECHANGED     0x00200000 // Lost capture
#define POINTER_FLAG_HASTRANSFORM       0x00400000 // Input has a transform associated with it

#endif /* GII_H */
