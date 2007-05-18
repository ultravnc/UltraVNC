#if !defined(_WINVNC_VNCDESKTOPTHREAD)
#define _WINVNC_VNCDESKTOPTHREAD
#include "stdhdrs.h"
#include "vncServer.h"
#include "vncKeymap.h"
#include "vncDesktop.h"
#include "vncService.h"
#include "mmsystem.h"


typedef struct _CURSORINFO
{
    DWORD   cbSize;
    DWORD   flags;
    HCURSOR hCursor;
    POINT   ptScreenPos;
} MyCURSORINFO, *PMyCURSORINFO, *LPMyCURSORINFO;
// The desktop handler thread
// This handles the messages posted by RFBLib to the vncDesktop window
typedef BOOL (WINAPI *_GetCursorInfo)(PMyCURSORINFO pci);
extern bool g_DesktopThread_running;

class vncDesktopThread : public omni_thread
{
public:
	vncDesktopThread() {
		m_returnsig = NULL;
		user32 = LoadLibrary("user32.dll");
		MyGetCursorInfo=NULL;
		if (user32) MyGetCursorInfo=(_GetCursorInfo )GetProcAddress(user32, "GetCursorInfo");
		g_DesktopThread_running=true;

		m_lLastMouseMoveTime = 0L;
		m_lLastUpdateTime = 0L;
	};
protected:
	~vncDesktopThread() {
		if (m_returnsig != NULL) delete m_returnsig;
		if (user32) FreeLibrary(user32);
		g_DesktopThread_running=false;
	};
public:
	virtual BOOL Init(vncDesktop *desktop, vncServer *server);
	virtual void *run_undetached(void *arg);
	virtual void ReturnVal(BOOL result);
	void PollWindow(rfb::Region2D &rgn, HWND hwnd);
	// Modif rdv@2002 - v1.1.x - videodriver
	virtual BOOL handle_driver_changes(rfb::Region2D &rgncache,rfb::UpdateTracker &tracker);
	virtual void copy_bitmaps_to_buffer(ULONG i,rfb::Region2D &rgncache,rfb::UpdateTracker &tracker);

protected:
	vncServer *m_server;
	vncDesktop *m_desktop;

	omni_mutex m_returnLock;
	omni_condition *m_returnsig;
	BOOL m_return;
	BOOL m_returnset;
	bool m_screen_moved;
	bool lastsend;
	HMODULE user32;
	_GetCursorInfo MyGetCursorInfo;
	bool XRichCursorEnabled;
	DWORD newtick,oldtick;

	DWORD m_lLastUpdateTime;
	DWORD m_lLastMouseMoveTime;
	void SessionFix();

};
#endif