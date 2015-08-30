#ifdef _Gii
#ifndef VNCTOUCH_H__
#define VNCTOUCH_H__

#include "stdhdrs.h"
#include "vncviewer.h"
#include "ClientConnection.h"


struct MyTouchINfo
{
	DWORD TouchId;
	DWORD pointerflag;
	DWORD touchmask;
	int X;
	int Y;	
	int ContactWidth;
	int ContactHeight;
};

class vnctouch
{
public:
	vnctouch();
	~vnctouch();	
	void OnTouch(HWND hWnd, WPARAM wParam, LPARAM lParam);
	int _handle_gii_message(HWND hwnd);
	void Set_ClientConnect(ClientConnection *IN_cc);
	bool All_Points_Up();

private:
	void Activate_touch(HWND hWnd);
	int GetContactIndex(int dwID);
	int rfb_send_gii_mt_event();
	int scale_coordinates(int x, int y, int *nx, int *ny);
	int gettimeofday(struct timeval* p, void* tz /* IGNORED */);
	void rfb_gii_init_timestamps(DWORD *time_msec, DWORD64 *time_usec);
	void doczka_vertical();

	int _find_gii_version(uint16_t min, uint16_t max);
	int _handle_gii_device_creation(void);
	int _handle_gii_version_message(rfbGIIMsg *msg, int gii_bigendian);	
	void rfb_gii_init_valuator(rfbGIIValuatorEventMsg *val, int cnt);
	int  rfb_send_gii_mt_empty_event(rfbGIIValuatorEventMsg *val, DWORD time_msec, DWORD64 time_usec);
	
	void Initialize_Vars();
	void AddFlag(DWORD *flag, DWORD value);
	bool IsFlagSet(DWORD flag, DWORD value);

	MyTouchINfo *pMyTouchInfo;
	int *idLookup;
	bool *point_down;
	int MAXPOINTS;
	ClientConnection *cc;
	uint32_t gii_deviceOrigin;
	struct timeval start_time;
	bool IsTouchActivated;
	uint16_t serverVersion;
};
#endif
#endif