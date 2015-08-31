#ifdef _Gii
#include "vnctouch.h"


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

/////////////////////////////////////////////////////
//RFB extention
// #define MF_flag 0x400000 // pointer move
// #define UF_flag 0x200000 // pointer up
//+-- + -- + -- + -- + -- + -- + -- + -- + -- + -------- - +------------ + ---------- - +
//| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22  .. 8 | 7 .. 4   | 3 .. 0    |
//+-- + -- + -- + -- + -- + -- + -- + -- + -- + -------- - +------------ + ---------- - +
//| PF | R | IF | S1 | S2 | RT | PR | TI | HC  | |Reserved|Pix-Format| ID Format |
//+-- + -- + -- + -- + -- + -- + -- + -- + -- + -------- - +------------ + ---------- - +
//
/////////////////////////////////////////////////////
/*
VALUATOR_EVENT for Multitouch Devices is :

Bytes          Type[Value]         Description
......................................................
1              U8        16 + 4 * count  event - size
1              U8        12 or 13           event - type
2              U16                             padding
4              U32                            device - origin
4              U32                            first(Number of touchevents)
4              U32                            count(Number of values) // need to be ignored, see remark
4 * count   S32 array                    value(32Bit data type)


Der valuator event enthält einen konstanen Anteil bestehend aus :
event - size, event.type, padding, device - origin, first, count
Danach kommt der variable Anteil mit den Daten der Touchevents
Die Anzahl der Touchevents entspricht valuator.first.
Die Gesamtanzahl der Touchdaten(values) entspricht valuator.count.
Ein Wert(value) hat eine Länge von 4 Byte(32Bit)
Das Wertefeld(value - array)  hat eine Länge von(valuator.count * 4) Bytes
Innerhalb des Wertefeldes(value - array) können mehrere Touchevents fortlaufend enthalten
sein gemäß valuator.first.
Die Anzahl der Werte für einen Touchevent berechnet sich aus der Gesamtanzahl der Werte
geteilt durch die Anzahl der Touchevents(valuator.count / valuator.first).
////////////////////////////////////////////////////////////////////////////////
//UVNC: REMARK
//There is an issue when not all pointers have the same size.
//valuator.count / valuator.first expext that all arrays are the same while they are actual
//defined with (can be missing)...
//The server need to ignore the count number and calculate the size based on the flags
////////////////////////////////////////////////////////////////////////////////
Wertefeld für einen Touchevent:
VALUE-ARRAY is

Bytes          Type      [Value]         Description
......................................................
4              U32                               Formatflag
4              U32                               Touch-ID
4              U32                               Touch-Position
4              U32   (can be missing)    Touch Aerea (in case of flag S1)
4              U32   (can be missing)    Touch Pressure (in case of flag PR)
4              U32   (can be missing)    Timestamp millisec  (in case of flag TI)
4              U32   (can be missing)    Timestamp microsec Low (in case of flag HC)
4              U32   (can be missing)    Timestamp microsec High (in case of flag HC)

Für jeden einzelnen Touchevent ist immer ein Wertefeld der Länge: (valuator.count / valuator.first) vorhanden.
Wenn das ID-Format des zugehörigen Touchevents eine 1 enthält, dann darf das Wertefeld weiter verarbeitet werden.
Enthält das ID-Format -15 (0xF), dann dürfen die weiteren Daten nicht mehr verarbeitet werden und der InjectEvent ist damit beendet.
*/
/*
Detailbeschreibung der Touchwerte

DWORD 1:    (Formatflag)

+--+--+--+--+--+--+--+--+--+---------+------------+-----------+
|31|30|29|28|27|26|25|24|23| 22 .. 8 | 7 .. 4     | 3 .. 0    |
+--+--+--+--+--+--+--+--+--+---------+------------+-----------+
|PF|R |IF|S1|S2|RT|PR|TI|HC| R       | Pix-Format | ID Format |
+--+--+--+--+--+--+--+--+--+---------+------------+-----------+

PF         - 'P'ressed 'F'lag : es wird übertragen, ob der Touchpunkt gedrückt oder losgelassen wurde.
IF         - pr'I'mary 'F'lag : es wird übertragen, ob der Touchpunkt der primäre Touchpunkt ist.
S1         - 'S'ize Flag : es werden zusätzlich zur Position auch die Daten der Touchfläche übertragen.
Die Touchfläche wird (derzeit) als symmetrische Ellipse übertragen
S2         - reserviert für asymetrische Ellipse, wird momentan nicht unterstützt und muss 0 sein.
RT         - 'R'ec't'angle: Die Touchfläche wurde vom Treiber als Rechteck, und nicht als Ellipse ermittelt.
PR         - 'Pr'essure Flag: Der Druck, welcher auf die Touchfläche ausgeübt wird, wird mit übertragen.
TI         - 'Ti'mestamp: Der Zeitstempel zu dem das Ereignis am Touchdevice entstanden ist, wird mit übertragen.
HC         - 'H'igh Performance 'C'ounter
R          - reservierte Flags für zukünftige Nutzung, müssen derzeit 0 sein
Pix-Format - Format der Pixeldaten:
Als Kombination zwischen Länge und Format
+--------+-------+
| 7 .. 6 | 5..4  |
+--------+-------+
| Format | Länge |
+--------+-------+
Länge 0: sollte zur Erkennung von Übertragungsfehlern nicht verwendet werden
Länge 1: 1 DWORD
--> Format       0 : 16bit signed für x gefolgt von 16bit signed für y gemeinsam in einem DWORD
Formate > 1..3 : noch nicht definiert und dürfen derzeit nicht benutzt werden.
Länge 2: 2 DWORD
--> Format       0 : 32bit signed für x gefolgt von 32bit signed für y in jeweils einem DWORD
Formate > 1..3 : noch nicht definiert und dürfen derzeit nicht benutzt werden.
Länge 3: 4 DWORD
--> Format       0 : 64bit signed für x gefolgt von 64bit signed für y in jeweils 2 DWORDs
Formate > 1..3 : noch nicht definiert und dürfen derzeit nicht benutzt werden.

ID Format  - Format des vorzeichenbehafteter Identifier des jeweiligen "Fingers"
0 : sollte zur Erkennung von Übertragungsfehlern nicht verwendet werden
1 : 32-bit ID (1 DWORD)
15 : Clear Message --> kein Touchpunkt mehr vorhanden
alle anderen Werte sind für zukünftige Erweiterungen reserviert und dürfen derzeit nicht benutzt werden.



Die folgenden Felder werden nur bei Bedarf übertragen. Wenn die ID Format ungleich -1 ist, muss direkt danach die ID gefolgt von den Koordinaten im
entsprechendem Format übertragen werden.
Alle folgenden DWORDs werden nur übertragen, falls das entsprechende Bit im ersten DWORD gesetzt ist.
Die Reihenfolge der dabei zu übertragenden DWORDs muss immer der hier definierten ensprechen, auch wenn dazwischenliegende DWORDs nicht
übertragen werden.



Die Interpretation der Daten der Daten bei ID-Format = 1

DWORD 2:  Touch ID
+---------+
| 31 .. 0 |
+---------+
| ID      |
+---------+
((idFormat > 0) && (idFormat < 15) && (pixFormat == 1))

DWORD 3:

Koordinaten: (wenn 0 < ID Format < 15 && Pix-Format = 1)
+----------+----------+
| 31 .. 16 | 15..0    |
+----------+----------+
| x-pos    | y-pos    |
+----------+----------+
x-pos  - x Koordinate absolut 16bit (-32768 .. 32768 Pixel)
y-pos  - y Koordinate absolut 16bit (-32768 .. 32768 Pixel)
Koordinaten: (wenn 0 < ID Format < 15 && Pix-Format != 1)
noch nicht definiert!


weitere DWORDS dürfen nur bei den jeweils gesetzten Flags mit folgender fallender Priorität interpretiert werden: (S1 | S2 | RT), PR, TI, HC
Wenn Flag 'S1' gesetzt: Fläche (Ellipse) des Touchpunktes, wobei x-pos und y-pos den Mittelpunkt darstellen.
+-------------+----------+----------+
| 31 .. 24    | 23 .. 12 | 11..0    |
+-------------+----------+----------+
| orientation | major    | minor    |
+-------------+----------+----------+
major : lange Halbachse der Ellipse in Pixel
minor : kurze Halbachse der Ellipse in Pixel
orientation : -180° .. + 180° // Werte müssen noch definiert werden!!!
/// Vorschlag: S1: 0 = senkrechte Lage oben, 63 waagerechte Lage rechts,
/// -64 waagerechte Lage links, 127 senkrechte Lage unten,
/// alles andere entsprechend orientation*180°/128
/// Alternative: Microsoft sieht einen Wertebereich von 0..359 vor, dann müssten
/// wir die Daten auf 2 DWORDs aufteilen, oder die Touchfläche auf 11-bit
/// pro Richtung begrenzen.
Asymetrische Ellipsen werden von uns (noch) nicht unterstützt. Dafür ist aber bereits S2 mit der selben Daten-Struktur wie S1 reserviert.

Wenn Flag 'PR' gesetzt: Druck des Touchpunktes
+----------------+----------------+
| 31 .. 16       | 15 .. 0        |
+----------------+----------------+
| R              | pressure       |
+----------------+----------------+
pressure: 0       - Druckangabe wird vom Treiber nicht unterstützt
1..1024 - normierte Druckangabe

Wenn Flag 'TI' gesetzt: Zeitstempel des Touchereignisses in Millisekunden
+----------------+
| 31 .. 0        |
+----------------+
| to be defined  |
+----------------+

Wenn Flag 'HC' gesetzt: Zeitstempel des Touchereignisses in Mikrosekunden
+----------------+  +----------------+
| 31 .. 0        |  | 31 .. 0        |
+----------------+  +----------------+
| to be defined  |  | to be defined  |
+----------------+  +----------------+

*/



#define TOUCH_MASK_NONE 0x00000000
#define POINTER_FLAG_NONE 0x00000000
#define TOUCH_MASK_CONTACTAREA 0x00000001 //TOUCHINPUTMASKF_CONTACTAREA
#define POINTER_FLAG_INRANGE 0x00000002 //TOUCHEVENTF_INRANGE
#define POINTER_FLAG_INCONTACT 0x00000004 //TOUCHEVENTF_INRANGE
#define POINTER_FLAG_PRIMARY 0x00002000 //TOUCHEVENTF_PRIMARY
#define POINTER_FLAG_DOWN 0x00010000 //TOUCHEVENTF_DOWN
#define POINTER_FLAG_UPDATE 0x00020000 //TOUCHEVENTF_MOVE
#define POINTER_FLAG_UP 0x00040000 //TOUCHEVENTF_UP

vnctouch::vnctouch()
{
	MAXPOINTS = 0;
	pMyTouchInfo = NULL;
	idLookup = NULL;
	point_down = NULL;
	gii_deviceOrigin = 0;
	gettimeofday(&start_time, NULL);
	IsTouchActivated = false;
}
vnctouch::~vnctouch()
{
	if (pMyTouchInfo)delete [] pMyTouchInfo;
	if (idLookup) delete []idLookup;
	if (point_down) delete[]point_down;
}

void vnctouch::Set_ClientConnect(ClientConnection *IN_cc)
{
	cc = IN_cc;
}

void vnctouch::AddFlag(DWORD *flag, DWORD value)
{
	*flag |= value;
}

bool vnctouch::IsFlagSet(DWORD flag, DWORD value)
{
	return (flag & value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Time functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////
int vnctouch::gettimeofday(struct timeval* p, void* tz /* IGNORED */)
{
	union {
		long long ns100; /*time since 1 Jan 1601 in 100ns units */
		FILETIME ft;
	} now;

	GetSystemTimeAsFileTime(&(now.ft));
	p->tv_usec = (long)((now.ns100 / 10LL) % 1000000LL);
	p->tv_sec = (long)((now.ns100 - (116444736000000000LL)) / 10000000LL);
	return 0;
}


uint64_t swap64ifle(uint64_t arg) {
	uint32_t *ptr = (uint32_t*) &arg;
	uint32_t tmp = Swap32IfLE(ptr[0]);
	ptr[0] = Swap32IfLE(ptr[1]);
	ptr[1] = tmp;
	return arg;
}
void vnctouch::rfb_gii_init_timestamps(DWORD *time_msec, DWORD64 *time_usec)
{
	//int endianTest = 1;
	struct timeval t_tmp;
	gettimeofday(&t_tmp, NULL);
	*time_msec = Swap32IfLE(((t_tmp.tv_sec - start_time.tv_sec) * 1000) + (t_tmp.tv_usec / 1000));
	DWORD64 t = (((t_tmp.tv_sec - start_time.tv_sec) * 1000000) + t_tmp.tv_usec);
	t = swap64ifle(t);
	*time_usec = swap64ifle((int64_t) (((t_tmp.tv_sec - start_time.tv_sec) * 1000000) + t_tmp.tv_usec));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Convert dwId ( random value ) to a 0-maxpoints number.
//////////////////////////////////////////////////////////////////////////////////////////////////////////
int vnctouch::GetContactIndex(int dwID){
	for (int i = 0; i < MAXPOINTS; i++){
		if (idLookup[i] == -1){
			idLookup[i] = dwID;
			point_down[i] = true;
			return i;
		}
		else if (idLookup[i] == dwID)
		{
			point_down[i] = true;
			return i;
		}
	}
//cleaup table
	for (int i = 0; i < MAXPOINTS; i++){
		if (point_down[i]==false){
			idLookup[i] = -1;
		}
	}
//check empty spot
	for (int i = 0; i < MAXPOINTS; i++){
		if (idLookup[i] == -1){
			idLookup[i] = dwID;
			point_down[i] = true;
			return i;
		}
	}

	return -1;
}

bool vnctouch::All_Points_Up()
{
	if (point_down)
	for (int i = 0; i < MAXPOINTS; i++){
		if (point_down[i]) return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Convert position to remote coordinates, and return onscreen
//////////////////////////////////////////////////////////////////////////////////////////////////////////
int vnctouch::scale_coordinates(int x, int y, int *nx, int *ny)
{
	int x_scaled = (x + cc->m_hScrollPos) * cc->m_opts.m_scale_den / cc->m_opts.m_scale_num;
	if (cc->m_opts.m_Directx) x_scaled = (x)*  cc->m_si.framebufferWidth / cc->m_cliwidth;
	*nx = x_scaled;

	int y_scaled = (y + cc->m_vScrollPos) * cc->m_opts.m_scale_den / cc->m_opts.m_scale_num;
	if (cc->m_opts.m_Directx)
	{
		if (cc->m_opts.m_ShowToolbar) y_scaled = (y)* cc->m_si.framebufferHeight / (cc->m_cliheight - cc->m_TBr.bottom);
		else y_scaled = (y)*cc->m_si.framebufferHeight / cc->m_cliheight;
	}
	*ny = y_scaled;
	if (x_scaled < 0 || x_scaled > cc->m_si.framebufferWidth) return 0;
	if (y_scaled < 0 || y_scaled > cc->m_si.framebufferHeight) return 0;
	return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//Init Vars
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void vnctouch::Initialize_Vars()
{
	if (pMyTouchInfo != NULL)delete pMyTouchInfo;
	pMyTouchInfo = new MyTouchINfo[MAXPOINTS];
	idLookup = new int[MAXPOINTS];
	point_down = new bool[MAXPOINTS];
	for (int i = 0; i < MAXPOINTS; i++){
		pMyTouchInfo[i].X = -1;
		pMyTouchInfo[i].Y = -1;
		pMyTouchInfo[i].TouchId = 999999999;
		pMyTouchInfo[i].ContactWidth = -1;
		pMyTouchInfo[i].ContactHeight = -1;
		idLookup[i] = -1;
		point_down[i] = false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// We need to register to accept wm_touch in the same thread
// Using a settimer, this is done with the WM_IME in the correct thread via the wndproc
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void vnctouch::Activate_touch(HWND hWnd)
{	
	if (IsTouchActivated) return;
	int value = GetSystemMetrics(SM_DIGITIZER);
	if ((value  & NID_MULTI_INPUT) && (value & NID_READY)) MAXPOINTS = GetSystemMetrics(SM_MAXIMUMTOUCHES);
#ifdef _DEBUG
	char			szText[256];

	if ((value  & NID_MULTI_INPUT) && (value & NID_READY))
	{
		sprintf(szText, " Multitouch found and ready \n");
		if (value & NID_INTEGRATED_TOUCH){ sprintf(szText, "Multitouch found and ready, Integrated touch with maxtouches=%i \n", MAXPOINTS); }
	}
	OutputDebugString(szText);
#endif
	//RegisterTouchWindow(m_hwndcn, 0);
	//The registering need to be in the same thread as the window.
	//We use a timer function to trigger the activation in the wndproc.	
	SetTimer(hWnd, TOUCH_REGISTER_TIMER, 100, NULL);
	Sleep(1000);
	Initialize_Vars();
	IsTouchActivated = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Handle server to viewer rfbGIIMessage
//////////////////////////////////////////////////////////////////////////////////////////////////////////
int vnctouch::_handle_gii_message(HWND hwnd)
{
	//int subtype;
	int gii_bigendian;
	rfbGIIMsg msg;

	msg.h.messageType = rfbGIIMessage;
	cc->ReadExact((char*)&msg.h.subType, 1);
	gii_bigendian = msg.h.subType & rfbGIIEndianType;
	msg.h.subType |= rfbGIIEndianType;

	switch (msg.h.subType)
	{
	case rfbGIIVersionMessage:
		cc->ReadExact(((char*)&msg.sv) + 2, sz_rfbGIIServerVersionMsg - 2);
		if (_handle_gii_version_message(&msg, gii_bigendian))
			_handle_gii_device_creation();
		break;
	case rfbGIIDeviceCreation:
		cc->ReadExact(((char*)&msg.sdc) + 2, sz_rfbGIIServerDeviceCreationMsg - 2);
		gii_deviceOrigin = (gii_bigendian) ? Swap32IfLE(msg.sdc.deviceOrigin) : msg.sdc.deviceOrigin;
		if (gii_deviceOrigin) Activate_touch(hwnd);
		break;
	default:
		/* invalid gii type */
		break;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tell server to create device, we use the number of buttons
// numButtons= max supported touch points
//////////////////////////////////////////////////////////////////////////////////////////////////////////
int vnctouch::_handle_gii_device_creation(void)
{
	rfbGIIMTCreationMsg msg;
	//rfbGIIServerDeviceCreationMsg retMsg;

	msg.dc.header.messageType = rfbGIIMessage;
	msg.dc.header.subType = rfbGIIDeviceCreation;
	// TODO: richtiger wert fuer length !!!
	// numValuators und laenge dynamisch machen
	msg.dc.header.length = Swap16IfLE(172);
	memset(msg.dc.deviceName, 0, 31);
	strcpy((char*)msg.dc.deviceName, "TCVNC-MT");
	msg.dc.DNTerm = 0;
	msg.dc.vendorID = Swap32IfLE(0x0908);
	msg.dc.productID = Swap32IfLE(0x000b);
	msg.dc.eventMask = Swap32IfLE(rfbGIIevMaskValuatorAbsolute);
	msg.dc.numRegisters = 0;
	msg.dc.numValuators = Swap32IfLE(1);

	//NO HIGHER VERSION NEEEDED, numbuttons is not used in v1

	// A touchscreen doesn't have buttons, so we use this parameter to pass the max supported points
	//if (serverVersion==2)
	//{
		int value = GetSystemMetrics(SM_DIGITIZER);
		if ((value  & NID_MULTI_INPUT) && (value & NID_READY)) MAXPOINTS = GetSystemMetrics(SM_MAXIMUMTOUCHES);
		msg.dc.numButtons = MAXPOINTS;
		msg.dc.numButtons = Swap32IfLE(msg.dc.numButtons);
	//}
	//else msg.dc.numButtons = 0;

	msg.v.index = 0;
	memset(msg.v.longName, 0, 74);
	strcpy((char*)msg.v.longName, "TCVNC Multitouch Device");
	msg.v.LNTerm = 0;
	memset(msg.v.shortName, 0, 4);
	strcpy((char*)msg.v.shortName, "TMD");
	msg.v.SNTerm = 0;
	msg.v.rangeMin = 0;
	msg.v.rangeCenter = 0;
	msg.v.rangeMax = 0;
	msg.v.SIUnit = 0;
	msg.v.SIAdd = 0;
	msg.v.SIMul = 0;
	msg.v.SIDiv = 0;
	msg.v.SIShift = 0;
	cc->WriteExact((char*)&msg, sz_rfbGIIMTCreationMsg);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Find highest supported common version
//////////////////////////////////////////////////////////////////////////////////////////////////////////
int vnctouch::_find_gii_version(uint16_t min, uint16_t max)
{
	int i;

	for (i = max; i >= min; --i) {
		if (i <= rfbGIIMaxVersion &&
			i >= rfbGIIMinVersion)
			return i;
	}
	return -1;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Send negotiated version
//////////////////////////////////////////////////////////////////////////////////////////////////////////
int vnctouch::_handle_gii_version_message(rfbGIIMsg *msg, int gii_bigendian)
{
	int min, max;
	rfbGIIClientVersionMsg cmsg;
	cmsg.header.messageType = rfbGIIMessage;
	cmsg.header.subType = rfbGIIVersionMessage;
	cmsg.header.length = Swap16IfLE(sizeof(CARD16));

	min = (gii_bigendian) ? Swap16IfLE(msg->sv.minVersion) : msg->sv.minVersion;
	max = (gii_bigendian) ? Swap16IfLE(msg->sv.maxVersion) : msg->sv.maxVersion;
	if (min > rfbGIIMaxVersion ||
		max < rfbGIIMinVersion)
		return 0;
	if ((cmsg.version = _find_gii_version(min, max)) == -1) {
		cc->m_opts.m_giienable = 0;
		return 0;
	}
	serverVersion = cmsg.version;
	cmsg.version = Swap16IfLE(cmsg.version);
	cc->WriteExact((char*)&cmsg, sz_rfbGIIClientVersionMsg);
	cc->m_opts.m_giienable = 1;
	return 1;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////
// empty_event
//////////////////////////////////////////////////////////////////////////////////////////////////////////
int  vnctouch::rfb_send_gii_mt_empty_event(rfbGIIValuatorEventMsg *val, DWORD time_msec, DWORD64 time_usec)
{
	DWORD flag = 0;
	AddFlag(&flag, HC_flag);
	AddFlag(&flag, TI_flag);
	AddFlag(&flag, LANGE_16_flag);
	AddFlag(&flag, IDFORMAT_CLEAR);
	flag = Swap32IfLE(flag);
	val->header.length = Swap16IfLE(32);// not used
	val->eventSize = (sz_rfbGIIValuatorEventMsg + 12);// not used
	val->count = Swap32IfLE(4); //(DWORD+DWORD+LONG)
	cc->WriteExact((char*)val, sz_rfbGIIValuatorEventMsg);
	cc->WriteExact((char*)&flag, sizeof(DWORD)); 
	cc->WriteExact((char*) &time_msec, sizeof(DWORD));
	cc->WriteExact((char*) &time_usec, sizeof(LONG));
	return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Defaults
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void vnctouch::rfb_gii_init_valuator(rfbGIIValuatorEventMsg *val, int cnt)
{
	val->header.messageType = rfbGIIMessage;
	val->header.subType = rfbGIIEventInjection;
	val->header.length = Swap16IfLE(16 + (6 * 4 * cnt)); // not used
	val->eventSize = (16 + (6 * 4 * cnt));// not used
	val->eventType = 12;// not used
	val->padding = 0;
	val->deviceOrigin = Swap32IfLE(gii_deviceOrigin);
	val->first = Swap32IfLE(cnt); //# points
	val->count = Swap32IfLE(6 * cnt);  //ERROR size is not always 6
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//switch x and y coordinates
// UVNC windows: our touch "driver" does not switch touch coordinates
// MOUSE coordinates are switched by windows!!
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void vnctouch::doczka_vertical()
{
	int i = 0;
	if (cc->m_si.framebufferWidth < cc->m_si.framebufferHeight)
	{
		//switch x and y coordinates
		// UVNC windows: our touch "driver" does not switch touch coordinates
		// MOUSE coordinates are switched by windows!!
		int tmppos;
		for (i = 0; i < MAXPOINTS; ++i) {
			tmppos = pMyTouchInfo[i].X;
			pMyTouchInfo[i].X = pMyTouchInfo[i].Y;
			pMyTouchInfo[i].Y = cc->m_si.framebufferHeight - tmppos;
		}
	}
}

int vnctouch::rfb_send_gii_mt_event()
{
	int i, x, y;
	int num_cts = 0;
	rfbGIIValuatorEventMsg ValuatorEventMsg;
	DWORD ValuatorFlag = 0;
	DWORD ValuatorTouch_Id = 0;
	DWORD ValuatorTouch_Position16XY = 0;
	DWORD ValuatorTouch_Area = 0;
	DWORD ValuatorTouch_Pressure = 0;
	DWORD ValuatorTime_msec = 0;
	DWORD64 ValuatorTime_usec = 0;

	for (i = 0; i<MAXPOINTS; ++i) {
		if (pMyTouchInfo[i].TouchId != 999999999 && !scale_coordinates(pMyTouchInfo[i].X, pMyTouchInfo[i].Y, &x, &y)) {
			pMyTouchInfo[i].TouchId = 999999999;
		}
		if (pMyTouchInfo[i].TouchId != 999999999) {
			++num_cts;
		}
	}

	doczka_vertical();

	rfb_gii_init_valuator(&ValuatorEventMsg, 1);
	rfb_gii_init_timestamps(&ValuatorTime_msec, &ValuatorTime_usec);

	if (num_cts == 0){
		ValuatorEventMsg.first = Swap32IfLE(1);
		return rfb_send_gii_mt_empty_event(&ValuatorEventMsg, ValuatorTime_msec, ValuatorTime_usec);
	}
	rfb_gii_init_valuator(&ValuatorEventMsg, num_cts);
	cc->WriteExact((char*) &ValuatorEventMsg, sz_rfbGIIValuatorEventMsg);

	
	//Send array of DWORDS
	for (i = 0; i<MAXPOINTS; ++i) {						
			if (pMyTouchInfo[i].TouchId != 999999999){
				ValuatorFlag = 0;
				AddFlag(&ValuatorFlag, HC_flag);
				AddFlag(&ValuatorFlag, TI_flag);
				AddFlag(&ValuatorFlag, LANGE_16_flag);
				AddFlag(&ValuatorFlag, IDFORMAT_32);
				//if (IsFlagSet(pMyTouchInfo[i].pointerflag, POINTER_FLAG_UP)) AddFlag(&ValuatorFlag, UF_flag);
				if (IsFlagSet(pMyTouchInfo[i].pointerflag, POINTER_FLAG_DOWN)) { /*AddFlag(&ValuatorFlag, DF_flag);*/ AddFlag(&ValuatorFlag, PF_flag); }
				if (IsFlagSet(pMyTouchInfo[i].pointerflag, POINTER_FLAG_UPDATE)) { /*AddFlag(&ValuatorFlag, MF_flag);*/ AddFlag(&ValuatorFlag, PF_flag); }
				if (IsFlagSet(pMyTouchInfo[i].pointerflag, POINTER_FLAG_PRIMARY)) AddFlag(&ValuatorFlag, IF_flag);
				if (IsFlagSet(pMyTouchInfo[i].touchmask, TOUCH_MASK_CONTACTAREA)) AddFlag(&ValuatorFlag, S1_flag);
				//if (IsFlagSet(pMyTouchInfo[i].pointerflag, POINTER_FLAG_INRANGE)) AddFlag(&ValuatorFlag, IR_flag);
				ValuatorFlag = Swap32IfLE(ValuatorFlag);
				cc->WriteExact((char*) &ValuatorFlag, sizeof(DWORD));
				ValuatorFlag = Swap32IfLE(ValuatorFlag); // revert back for reading
				ValuatorTouch_Id = Swap32IfLE(pMyTouchInfo[i].TouchId);
				cc->WriteExact((char*) &ValuatorTouch_Id, sizeof(DWORD));
				scale_coordinates(pMyTouchInfo[i].X, pMyTouchInfo[i].Y, &x, &y);
				if (IsFlagSet(ValuatorFlag, LANGE_16_flag)){
					ValuatorTouch_Position16XY = Swap32IfLE((x << 16) | (y));
					cc->WriteExact((char*) &ValuatorTouch_Position16XY, sizeof(DWORD));
				}
				if (IsFlagSet(ValuatorFlag, S1_flag)) {
					ValuatorTouch_Area = Swap32IfLE((pMyTouchInfo[i].ContactWidth << 16) | (pMyTouchInfo[i].ContactHeight));
					cc->WriteExact((char*) &ValuatorTouch_Area, sizeof(DWORD));
				}
				if (IsFlagSet(ValuatorFlag, PR_flag)){
					ValuatorTouch_Pressure = Swap32IfLE(ValuatorTouch_Pressure);
					cc->WriteExact((char*) &ValuatorTouch_Pressure, sizeof(DWORD));
				}				
				if (IsFlagSet(ValuatorFlag, TI_flag)){
						cc->WriteExact((char*) &ValuatorTime_msec, sizeof(DWORD));
					}
				if (IsFlagSet(ValuatorFlag, HC_flag)){
						cc->WriteExact((char*) &ValuatorTime_usec, sizeof(DWORD64));
					}
			}
		}
	return 1;
}

void vnctouch::OnTouch(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	if (pMyTouchInfo == NULL) return; //not yet initialized
	int index = -1;
	POINT ptInput;
	BOOL bHandled = FALSE;
	UINT cInputs = LOWORD(wParam);
	PTOUCHINPUT pInputs = new TOUCHINPUT[cInputs];
	for (int i = 0; i< MAXPOINTS; i++){
		pMyTouchInfo[i].X = -1;
		pMyTouchInfo[i].Y = -1;
		pMyTouchInfo[i].TouchId = 999999999;
		pMyTouchInfo[i].ContactWidth = -1;
		pMyTouchInfo[i].ContactHeight = -1;
	}


	if (pInputs)
	{
		if (GetTouchInputInfo((HTOUCHINPUT)lParam, cInputs, pInputs, sizeof(TOUCHINPUT))){
			for (UINT i = 0; i < cInputs; i++){
				TOUCHINPUT ti = pInputs[i];
				index = GetContactIndex(ti.dwID);
				if (ti.dwID != 0 && index < MAXPOINTS && index != -1){
					ptInput.x = TOUCH_COORD_TO_PIXEL(ti.x);
					ptInput.y = TOUCH_COORD_TO_PIXEL(ti.y);
					ScreenToClient(hWnd, &ptInput);

					//clear entry
					pMyTouchInfo[index].X = ptInput.x;
					pMyTouchInfo[index].Y = ptInput.y;
					pMyTouchInfo[index].TouchId = index;
					pMyTouchInfo[index].touchmask = TOUCH_MASK_NONE;
					pMyTouchInfo[index].pointerflag = POINTER_FLAG_NONE;
					if (ti.dwMask & TOUCHINPUTMASKF_CONTACTAREA)
					{
						pMyTouchInfo[index].touchmask = TOUCH_MASK_CONTACTAREA;
						pMyTouchInfo[index].ContactWidth = ti.cxContact;
						pMyTouchInfo[index].ContactHeight = ti.cyContact;
					}
					if (ti.dwFlags& TOUCHEVENTF_INRANGE)
						pMyTouchInfo[index].pointerflag |= POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT;
					if (ti.dwFlags& TOUCHEVENTF_PRIMARY) 
						pMyTouchInfo[index].pointerflag |= POINTER_FLAG_PRIMARY;
					if (ti.dwFlags& TOUCHEVENTF_DOWN)
						pMyTouchInfo[index].pointerflag |= POINTER_FLAG_DOWN;
					if (ti.dwFlags& TOUCHEVENTF_MOVE)
						pMyTouchInfo[index].pointerflag |= POINTER_FLAG_UPDATE;
					if (ti.dwFlags& TOUCHEVENTF_UP)
					{
						pMyTouchInfo[index].pointerflag |= POINTER_FLAG_UP;
						//point_down[index] = false;
					}
#ifdef _DEBUG
					char			szText[256];
					sprintf(szText, "cInputs %i index %i %i %i\n", cInputs, index, pMyTouchInfo[index].pointerflag, ti.dwID);
					OutputDebugString(szText);
#endif					
				}
			}
			rfb_send_gii_mt_event();
			for (UINT i = 0; i < cInputs; i++){
				if (pMyTouchInfo[index].pointerflag & POINTER_FLAG_UP) point_down[index] = false;
			}
			bHandled = TRUE;
		}
	}

	if (bHandled) CloseTouchInputHandle((HTOUCHINPUT) lParam);		
	if (pInputs) delete [] pInputs;
	return;
}
#endif














