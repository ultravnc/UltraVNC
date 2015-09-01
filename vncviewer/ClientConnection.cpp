/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2013 UltraVNC Team Members. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
// If the source code for the program is not available from the place from
// which you received this file, check 
// http://www.uvnc.com/
//
////////////////////////////////////////////////////////////////////////////

#include "stdhdrs.h"

#include "vncviewer.h"

#ifdef UNDER_CE
#include "omnithreadce.h"
#define SD_BOTH 0x02
#else
#include "omnithread/omnithread.h"
#endif

#include "ClientConnection.h"
#include "SessionDialog.h"
#include "AuthDialog.h"
#include "AboutBox.h"
#include "LowLevelHook.h"

#include "Exception.h"
extern "C" {
	#include "vncauth.h"
}

#include <rdr/FdInStream.h>
#include <rdr/ZlibInStream.h>
#include <rdr/ZlibOutStream.h>
#ifdef _XZ
#include <rdr/xzInStream.h>
#endif
#include <rdr/MemInStream.h>
#include <rdr/xzOutStream.h>
#include <rdr/MemOutStream.h>
#include <rdr/Exception.h>

#include <rfb/dh.h>

#include <DSMPlugin/DSMPlugin.h> // sf@2002
#include "common/win32_helpers.h"
#include "display.h"

// [v1.0.2-jp1 fix]
#pragma comment(lib, "imm32.lib")

#define INITIALNETBUFSIZE 4096
#ifdef _XZ
#define MAX_ENCODINGS (LASTENCODING+15)
#else
#define MAX_ENCODINGS (LASTENCODING+10)
#endif
#define VWR_WND_CLASS_NAME _T("VNCviewer")
#define VWR_WND_CLASS_NAME_VIEWER _T("VNCviewerwindow")
#define SESSION_MRU_KEY_NAME _T("Software\\ORL\\VNCviewer\\MRU")

const UINT FileTransferSendPacketMessage = RegisterWindowMessage("UltraVNC.Viewer.FileTransferSendPacketMessage");
extern bool g_passwordfailed;
bool havetobekilled=false;
bool forcedexit=false;
const UINT RebuildToolbarMessage = RegisterWindowMessage("UltraVNC.Viewer.RebuildToolbar");
extern bool g_ConnectionLossAlreadyReported;
extern bool paintbuzy;
#define FAILED(hr) (((HRESULT)(hr)) < 0)

/*
 * Macro to compare pixel formats.
 */

#define PF_EQ(x,y)							\
	((x.bitsPerPixel == y.bitsPerPixel) &&				\
	 (x.depth == y.depth) &&					\
	 ((x.bigEndian == y.bigEndian) || (x.bitsPerPixel == 8)) &&	\
	 (x.trueColour == y.trueColour) &&				\
	 (!x.trueColour || ((x.redMax == y.redMax) &&			\
			    (x.greenMax == y.greenMax) &&		\
			    (x.blueMax == y.blueMax) &&			\
			    (x.redShift == y.redShift) &&		\
			    (x.greenShift == y.greenShift) &&		\
			    (x.blueShift == y.blueShift))))

const rfbPixelFormat vnc8bitFormat			= {8,8,0,1,7,7,3,0,3,6, 0, 0}; // 256 colors
const rfbPixelFormat vnc8bitFormat_64		= {8,6,0,1,3,3,3,4,2,0, 0, 0} ;	// 64 colors
const rfbPixelFormat vnc8bitFormat_8		= {8,3,0,1,1,1,1,2,1,0, 0, 0} ;	// 8 colors
const rfbPixelFormat vnc8bitFormat_8Grey	= {8,8,0,1,7,7,3,0,3,6, 1, 0} ;	// 8 colors-Dark Scale
const rfbPixelFormat vnc8bitFormat_4Grey	= {8,6,0,1,3,3,3,4,2,0, 1, 0} ;	// 4 colors-Grey Scale
const rfbPixelFormat vnc8bitFormat_2Grey	= {8,3,0,1,1,1,1,2,1,0, 1, 0} ;	// 2 colors-Grey Scale

const rfbPixelFormat vnc16bitFormat			= {16,16,0,1,63,31,31,0,6,11, 0, 0};

#define KEYMAP_LALT_FLAG        (KEYMAP_LALT     << 28)
#define KEYMAP_RALT_FLAG        (KEYMAP_RALT     << 28)
#define KEYMAP_RCONTROL_FLAG    (KEYMAP_RCONTROL << 28)

//static LRESULT CALLBACK ClientConnection::WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
extern HWND currentHWND;
extern char sz_L1[64];
extern char sz_L2[64];
extern char sz_L3[64];
extern char sz_L4[64];
extern char sz_L5[64];
extern char sz_L6[64];
extern char sz_L7[64];
extern char sz_L8[64];
extern char sz_L9[64];
extern char sz_L10[64];
extern char sz_L11[64];
extern char sz_L12[64];
extern char sz_L13[64];
extern char sz_L14[64];
extern char sz_L15[64];
extern char sz_L16[64];
extern char sz_L17[64];
extern char sz_L18[64];
extern char sz_L19[64];
extern char sz_L20[64];
extern char sz_L21[64];
extern char sz_L22[64];
extern char sz_L23[64];
extern char sz_L24[64];
extern char sz_L25[64];
extern char sz_L26[64];
extern char sz_L27[64];
extern char sz_L28[64];
extern char sz_L29[64];
extern char sz_L30[64];
extern char sz_L31[64];
extern char sz_L32[64];
extern char sz_L33[64];
extern char sz_L34[64];
extern char sz_L35[64];
extern char sz_L36[64];
extern char sz_L37[64];
extern char sz_L38[64];
extern char sz_L39[64];
extern char sz_L40[64];
extern char sz_L41[64];
extern char sz_L42[64];
extern char sz_L43[64];
extern char sz_L44[64];
extern char sz_L45[64];
extern char sz_L46[64];
extern char sz_L47[64];
extern char sz_L48[64];
extern char sz_L49[64];
extern char sz_L50[64];
extern char sz_L51[128];
extern char sz_L52[64];
extern char sz_L53[64];
extern char sz_L54[64];
extern char sz_L55[64];
extern char sz_L56[64];
extern char sz_L57[64];
extern char sz_L58[64];
extern char sz_L59[64];
extern char sz_L60[64];
extern char sz_L61[64];
extern char sz_L62[64];
extern char sz_L63[64];
extern char sz_L64[64];
extern char sz_L65[64];
extern char sz_L66[64];
extern char sz_L67[64];
extern char sz_L68[64];
extern char sz_L69[64];
extern char sz_L70[64];
extern char sz_L71[64];
extern char sz_L72[64];
extern char sz_L73[64];
extern char sz_L74[64];
extern char sz_L75[64];
extern char sz_L76[64];
extern char sz_L77[128];
extern char sz_L78[64];
extern char sz_L79[64];
extern char sz_L80[64];
extern char sz_L81[128];
extern char sz_L82[64];
extern char sz_L83[64];
extern char sz_L84[64];
extern char sz_L85[64];
extern char sz_L86[64];
extern char sz_L87[64];
extern char sz_L88[64];
extern char sz_L89[64];
extern char sz_L90[64];
extern char sz_L91[64];
extern char sz_L92[64];
extern char sz_L93[64];
extern char sz_L94[64];

extern char sz_F1[64];
extern char sz_F5[128];
extern char sz_F6[64];
extern bool command_line;

// *************************************************************************
//  A Client connection involves two threads - the main one which sets up
//  connections and processes window messages and inputs, and a
//  client-specific one which receives, decodes and draws output data
//  from the remote server.
//  This first section contains bits which are generally called by the main
//  program thread.
// *************************************************************************
// adzm - 2010-07 - Extended clipboard
//ClientConnection::ClientConnection()
//	: fis(0), zis(0), m_clipboard(ClipboardSettings::defaultViewerCaps)
//{
//	m_keymap = NULL;
//	m_keymapJap = NULL;
//	//adzm - 2009-06-21
//	m_pPluginInterface = NULL;
//	//adzm 2010-05-10
//	m_pIntegratedPluginInterface = NULL;
//	SB_HORZ_BOOL=true;
//	SB_VERT_BOOL=true;
//}

BOOL CALLBACK DialogProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);

// adzm - 2010-07 - Extended clipboard
ClientConnection::ClientConnection(VNCviewerApp *pApp)
	: fis(0), zis(0), m_clipboard(ClipboardSettings::defaultViewerCaps)
{
	Init(pApp);
}

// adzm - 2010-07 - Extended clipboard
ClientConnection::ClientConnection(VNCviewerApp *pApp, SOCKET sock)
  : fis(0), zis(0), m_clipboard(ClipboardSettings::defaultViewerCaps)
{
	Init(pApp);
    if (m_opts.autoDetect)
	{
      m_opts.m_Use8Bit = rfbPFFullColors; //true;
	}
	m_sock = sock;
	m_serverInitiated = true;
	struct sockaddr_in svraddr;
	int sasize = sizeof(svraddr);
	if (getpeername(sock, (struct sockaddr *) &svraddr,
		&sasize) != SOCKET_ERROR) {
		_stprintf(m_host, _T("%d.%d.%d.%d"),
			svraddr.sin_addr.S_un.S_un_b.s_b1,
			svraddr.sin_addr.S_un.S_un_b.s_b2,
			svraddr.sin_addr.S_un.S_un_b.s_b3,
			svraddr.sin_addr.S_un.S_un_b.s_b4);
		m_port = svraddr.sin_port;
	} else {
		_tcscpy(m_host,sz_L1);
		m_port = 0;
	};
}

// adzm - 2010-07 - Extended clipboard
ClientConnection::ClientConnection(VNCviewerApp *pApp, LPTSTR host, int port)
  : fis(0), zis(0), m_clipboard(ClipboardSettings::defaultViewerCaps)
{
	Init(pApp);
    if (m_opts.autoDetect)
	{
		m_opts.m_Use8Bit = rfbPFFullColors; //true;
	}
	_tcsncpy(m_host, host, MAX_HOST_NAME_LEN);
	m_port = port;
	_tcsncpy(m_proxyhost,m_opts.m_proxyhost, MAX_HOST_NAME_LEN);
	m_proxyport=m_opts.m_proxyport;
	m_fUseProxy = m_opts.m_fUseProxy;
}

void ClientConnection::Init(VNCviewerApp *pApp)
{
	new_ultra_server=false;
	Pressed_Cancel=false;
	saved_set=false;
	m_hwndcn = 0;
	m_desktopName = NULL;
	m_desktopName_viewonly = NULL;
	m_port = -1;
	m_proxyport = -1;
//	m_proxy = 0;
	m_serverInitiated = false;
	m_netbuf = NULL;
	m_netbufsize = 0;
	m_zlibbuf = NULL;
	m_zlibbufsize = 0;
	m_decompStreamInited = false;
	// adzm - 2010-07 - Fix clipboard hangs
	m_hwndNextViewer = (HWND)INVALID_HANDLE_VALUE;
	m_pApp = pApp;
	m_dormant = 0;
	m_hBitmapDC = NULL;
	//m_hBitmap = NULL;
	m_hPalette = NULL;
	m_encPasswd[0] = '\0';
	m_encPasswdMs[0] = '\0'; // act: add mspasswd storage
	m_ms_user[0] = '\0';    // act: add msuser storage
	m_cmdlnUser[0] = '\0'; // act: add user option on command line
	m_clearPasswd[0] = '\0'; // Modif sf@2002
	// static window
	m_BytesSend=0;
	m_BytesRead=0;

	m_keymap = new KeyMap;
	m_keymapJap = new KeyMapJap;

	// We take the initial conn options from the application defaults
	m_opts = m_pApp->m_options;

	// Pass the connection option(s) to module(s)
    m_keymap->SetKeyMapOption1(false);
    m_keymap->SetKeyMapOption2(true);

	m_sock = INVALID_SOCKET;
	//adzm 2010-09
	m_nQueueBufferLength = 0;
	//adzm 2010-08-01
	m_LastSentTick = 0;
	m_bKillThread = false;
	m_threadStarted = true;
	m_running = false;
	m_pendingFormatChange = false;

	// sf@2002 - v1.1.2 - Data Stream Modification Plugin handling
	m_nTO = 1;
	m_pDSMPlugin = new CDSMPlugin();
	m_fUsePlugin = false;
	m_fUseProxy = false;
	m_pNetRectBuf = NULL;
	m_fReadFromNetRectBuf = false;  //
	m_nNetRectBufOffset = 0;
	m_nReadSize = 0;
	m_nNetRectBufSize = 0;
	m_pZRLENetRectBuf = NULL;
	m_fReadFromZRLENetRectBuf = false;  //
	m_nZRLENetRectBufOffset = 0;
	m_nZRLEReadSize = 0;
	m_nZRLENetRectBufSize = 0;
#ifdef _XZ
	m_pXZNetRectBuf = NULL;
	m_fReadFromXZNetRectBuf = false;  // 
	m_nXZNetRectBufOffset = 0;
	m_nXZReadSize = 0;
	m_nXZNetRectBufSize = 0;
#endif

	//adzm - 2009-06-21
	m_pPluginInterface = NULL;
	//adzm 2010-05-10
	m_pIntegratedPluginInterface = NULL;

	// ZlibHex
	m_decompStreamInited = false;
	m_decompStreamRaw.total_in = ZLIBHEX_DECOMP_UNINITED;
	m_decompStreamEncoded.total_in = ZLIBHEX_DECOMP_UNINITED;

	// Initialise a few fields that will be properly set when the
	// connection has been negotiated
	m_fullwinwidth = m_fullwinheight = 0;
	m_si.framebufferWidth = m_si.framebufferHeight = 0;

	m_hScrollPos = 0; m_vScrollPos = 0;

	m_waitingOnEmulateTimer = false;
	m_emulatingMiddleButton = false;

    oldPointerX = oldPointerY = oldButtonMask = 0;

	// Create a buffer for various network operations
	CheckBufferSize(INITIALNETBUFSIZE);

	m_pApp->RegisterConnection(this);

    kbitsPerSecond = 0;
	avg_kbitsPerSecond = 0;
	m_lLastChangeTimeTimeout = 0;
	m_lLastChangeTime = timeGetTime();
						   // timeGetTime Fisr time switch is unstable... wait 30 seconds
						  // 0 because we want the first encoding switching to occur quickly
	                     // (in Auto mode, ZRLE is used: pointless over a LAN)

	m_fScalingDone = false;

    zis = new rdr::ZlibInStream;
#ifdef _XZ
	xzis = new rdr::xzInStream;
#endif

	// tight cusorhandling
	prevCursorSet = false;
	rcCursorX = 0;
	rcCursorY = 0;

	// Modif sf@2002 - FileTransfer
	m_pFileTransfer = new FileTransfer(m_pApp, this);
	m_filezipbuf = NULL;
	m_filezipbufsize = 0;
	m_filechunkbuf = NULL;
	m_filechunkbufsize = 0;

	// Modif sf@2002 - Text Chat
	m_pTextChat = new TextChat(m_pApp, this);

	// Modif sf@2002 - Scaling
	m_pendingScaleChange = false;
	m_pendingCacheInit = false;
	m_nServerScale = 1;
	m_reconnectcounter = 3;
	m_Is_Listening=0;

	//ms logon
	m_ms_logon_I_legacy=false;

	// sf@2002 - FileTransfer on server
	m_fServerKnowsFileTransfer = false;

	// Auto Mode
	m_nConfig = 0;

	// sf@2002 - Options window flag
	m_fOptionsOpen = false;

	// Tight encoding
	for (int i = 0; i < 4; i++)
		m_tightZlibStreamActive[i] = false;

	m_hwndcn=NULL;
	m_hbands=NULL;
	m_hwndTB=NULL;
	m_hwndTBwin=NULL;
	m_hwndMain=NULL;
	m_hwndStatus=NULL;
	m_TrafficMonitor=NULL;
	m_logo_wnd=NULL;
	m_button_wnd=NULL;
	// m_ToolbarEnable=true;
	m_remote_mouse_disable=false;
	m_SWselect=false;

	EncodingStatusWindow = -1;
	OldEncodingStatusWindow = -2;

	m_nStatusTimer = 0;
//	m_FTtimer = 0;
	skipprompt2=true;
//	flash=NULL;

	m_hmemdc=NULL;
	m_DIBbits=NULL;
	m_DIBbitsCache=NULL;
	m_membitmap=NULL;
	m_BigToolbar=false;
	strcpy(m_proxyhost,"");
	KillEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	newtick=0;
	oldtick=0;

	m_zipbuf=NULL;
	m_filezipbuf=NULL;
	m_filechunkbuf=NULL;
	m_zlibbuf=NULL;
	rcSource=NULL;
	rcMask=NULL;
	zywrle_level = 1;
#ifdef _XZ
	xzyw_level = 1;
	xzyw = 0;
#endif

	m_autoReconnect = m_opts.m_autoReconnect;
	ThreadSocketTimeout=NULL;
	m_statusThread=NULL;
	m_SavedAreaBIB=NULL;
    m_bClosedByUser = false;
    m_server_wants_keepalives = false;
	hbmToolbig = (HBITMAP)LoadImage(m_pApp->m_instance, "tlbarbig.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_LOADMAP3DCOLORS);
	hbmToolsmall = (HBITMAP)LoadImage(m_pApp->m_instance, "tlbarsmall.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_LOADMAP3DCOLORS);
	hbmToolbigX = (HBITMAP)LoadImage(m_pApp->m_instance, "tlbarbigx.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_LOADMAP3DCOLORS);
	hbmToolsmallX = (HBITMAP)LoadImage(m_pApp->m_instance, "tlbarsmallx.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_LOADMAP3DCOLORS);
	rcth=NULL;

	// adzm - 2010-07 - Extended clipboard
	m_hPopupMenuClipboard = NULL;
	m_hPopupMenuDisplay = NULL;
	m_hPopupMenuKeyboard = NULL;
	m_hPopupMenuClipboardFormats = NULL;

	m_keepalive_timer = 0;
	m_idle_timer = 0;
	m_idle_time = 5000;
	m_emulate3ButtonsTimer = 0;
	// adzm 2010-09
	m_flushMouseMoveTimer = 0;

	m_settingClipboardViewer = false;
	LoadClipboardPreferences();

	//adzm 2010-09
	m_fPluginStreamingIn = false;
	m_fPluginStreamingOut = false;

	//adzm 2010-10
	m_PendingMouseMove.dwMinimumMouseMoveInterval = m_opts.m_throttleMouse;
	directx_used=false;

#ifdef _Gii
	mytouch = new vnctouch;
	mytouch->Set_ClientConnect(this);
#endif
}

// helper functions for setting socket timeouts during file transfer
bool ClientConnection::SetSendTimeout(int msecs)
{
    int timeout= msecs < 0 ? m_opts.m_FTTimeout * 1000 : msecs;
	if (setsockopt(m_sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR)
	{
		return false;
	}
	return true;
}

bool ClientConnection::SetRecvTimeout(int msecs)
{
    int timeout= msecs < 0 ? m_opts.m_FTTimeout * 1000 : msecs;
	if (setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR)
	{
		return false;
	}
	return true;
}

//
// Run() creates the connection if necessary, does the initial negotiations
// and then starts the thread running which does the output (update) processing.
// If Run throws an Exception, the caller must delete the ClientConnection object.
//

void ClientConnection::Run()
{
	havetobekilled=false;
	forcedexit=false;
	// Get the host name and port if we haven't got it
	if (m_port == -1)
	{
		GetConnectDetails();
		// sf@2002 - DSM Plugin loading if required
		LoadDSMPlugin(false);
	}
	else
	{
		LoadDSMPlugin(false);
		// sf@2003 - Take command line quickoption into account
		HandleQuickOption();
	}

	// add user option on command line
	if ( (strlen(	m_pApp->m_options.m_cmdlnUser) > 0) && !m_pApp->m_options.m_NoMoreCommandLineUserPassword) // Fix by Act
		strcpy(m_cmdlnUser, m_pApp->m_options.m_cmdlnUser);

	// Modif sf@2002 - bit of a hack...and unsafe
	if ( (strlen(	m_pApp->m_options.m_clearPassword) > 0) && !m_pApp->m_options.m_NoMoreCommandLineUserPassword)
		strcpy(m_clearPasswd, m_pApp->m_options.m_clearPassword);

	if (saved_set)
	{
		saved_set=FALSE;
		Save_Latest_Connection();
	}

	GTGBS_CreateDisplay();
	GTGBS_CreateToolbar();
	CreateDisplay();

	DoConnection(); // sf@2007 - Autoreconnect - Must be done after windows creation, otherwise ReadServerInit does not initialise the title bar...

	//adzm 2009-06-21 - if we are connected now, show the window
	ShowWindow(m_hwndcn, SW_SHOW);

	// adzm - 2010-07 - Fix clipboard hangs - watch the clipboard now that our message pump will not be blocking on connection
	WatchClipboard();

	Createdib();
	SizeWindow();

	// This starts the worker thread.
	// The rest of the processing continues in run_undetached.
	LowLevelHook::Initialize(m_hwndMain);
	start_undetached();

	EndDialog(m_hwndStatus,0);
}

// sf@2007 - Autoreconnect
void ClientConnection::DoConnection()
{
	havetobekilled=true;
	// Connect if we're not already connected
	if (m_sock == INVALID_SOCKET)
		if (strcmp(m_proxyhost,"") !=NULL && m_fUseProxy)
			ConnectProxy();
		else
			Connect();

	SetSocketOptions();

	SetDSMPluginStuff(); // The Plugin is now activated BEFORE the protocol negociation
						 // so ALL the communication data travel through the DSMPlugin

	if (strcmp(m_proxyhost,"")!=NULL && m_fUseProxy)
		NegotiateProxy();

	NegotiateProtocolVersion();

	std::vector<CARD32> current_auth;
	Authenticate(current_auth);

//	if (flash) {flash->Killflash();}
	SendClientInit();

	ReadServerInit();

	CreateLocalFramebuffer();

	SetupPixelFormat();

    SetFormatAndEncodings();

	reconnectcounter=m_reconnectcounter;

	havetobekilled=false;;
}

DWORD WINAPI ReconnectThreadProc(LPVOID lpParameter)
{
	ClientConnection *cc=(ClientConnection*)lpParameter;
	Sleep( cc->m_autoReconnect * 1000 );
	try
	{
		cc->DoConnection();
		cc->m_bKillThread = false;
		cc->m_running = true;
		cc->m_pendingFormatChange=true;
		//adzm 2010-09 - Not sure about this one, but honestly i've never had the reconnect stuff work reliably for me, so I'll just let this
		//call into the internal function rather than deal with messages
		if (cc->m_nServerScale > 1) cc->SendServerScale(cc->m_nServerScale);
		cc->Internal_SendAppropriateFramebufferUpdateRequest();
	}
	catch (Exception &e)
	{
		if( cc->m_autoReconnect == 0)
			e.Report();
		cc->reconnectcounter--;
		if (cc->reconnectcounter<0) cc->reconnectcounter=0;
		//Seems is needed, countdown stop
		PostMessage(cc->m_hwndMain, WM_CLOSE, cc->reconnectcounter, 1);
	}
	return 0;
}

void ClientConnection::Reconnect()
{
	Sleep( m_autoReconnect * 1000 );
	try
	{
		DoConnection();
		m_bKillThread = false;
		m_running = true;

		//adzm 2010-09 - Not sure about this one, but honestly i've never had the reconnect stuff work reliably for me, so I'll just let this
		//call into the internal function rather than deal with messages
		if (m_nServerScale > 1) SendServerScale(m_nServerScale);
		Internal_SendAppropriateFramebufferUpdateRequest();
	}
	catch (Exception &e)
	{
		if( m_autoReconnect == 0)
			e.Report();
		reconnectcounter--;
		if (reconnectcounter<0) reconnectcounter=0;
		PostMessage(m_hwndMain, WM_CLOSE, reconnectcounter, 1);
	}
}

HWND ClientConnection::GTGBS_ShowConnectWindow()
{
	DWORD				  threadID;
	m_statusThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE )ClientConnection::GTGBS_ShowStatusWindow,(LPVOID)this,0,&threadID);
	if (m_statusThread) ResumeThread(m_statusThread);
	return (HWND)0;
}

////////////////////////////////////////////////////////
#include <commctrl.h>
#include <shellapi.h>
#include <lmaccess.h>
#include <lmat.h>
#include <lmalert.h>

void ClientConnection::CreateButtons(BOOL mini,BOOL ultra)
{
	if (ultra)
	{
		int nr_buttons = 14;
		TBADDBITMAP tbab;
		TBBUTTON tbButtons []=
		{
			{0,ID_BUTTON_CAD,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,0},
			{1,ID_BUTTON_FULLSCREEN,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,1},
			{2,ID_BUTTON_PROPERTIES,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,2},
			{3,ID_BUTTON_REFRESH,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,3},
			{4,ID_BUTTON_STRG_ESC,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,4},
			{5,ID_BUTTON_SEP,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,5},
			{6,ID_BUTTON_INFO,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,6},
			{7,ID_BUTTON_END,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,7},
			{8,ID_BUTTON_DBUTTON,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,8},
			{9,ID_BUTTON_DINPUT,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,9},
			{10,ID_BUTTON_FTRANS,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,10},
			{11,ID_BUTTON_SW,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,11},
			{12,ID_BUTTON_DESKTOP,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,12},
			{13,ID_BUTTON_TEXTCHAT,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,13},
		};
		static char *szTips[14] =
		{
				sz_L2,
				sz_L3,
				sz_L4,
				sz_L5,
				sz_L6,
				sz_L7,
				sz_L8,
				sz_L9,
				sz_L10,
				sz_L11,
				sz_L12,
				sz_L13,
				sz_L14,
				sz_L15,
		};
		int stdidx;
		HWND m_hwndTT;
		UINT buttonmap,minibuttonmap;
		int row,col;
		TOOLINFO ti;
		int id=0;
		RECT clr;
		InitCommonControls();
		GetClientRect(m_hwndMain,&clr);
		m_TBr.left=0;
		m_TBr.right=clr.right;
		m_TBr.top=0;
		m_TBr.bottom=28;
		if(hbmToolbig && hbmToolsmall && hbmToolbigX && hbmToolsmallX)
		{
			if (m_remote_mouse_disable)
			{
				if(mini)
					{
						m_hwndTB = CreateToolbarEx(
						m_hwndTBwin
						,WS_CHILD | TBSTYLE_WRAPABLE | WS_VISIBLE |TBSTYLE_TOOLTIPS |CCS_NORESIZE| TBSTYLE_FLAT | TBSTYLE_TRANSPARENT
						,IDR_TOOLBAR
						,nr_buttons
						,NULL
						,(UINT)hbmToolsmallX
						,(LPCTBBUTTON)&tbButtons
						,nr_buttons
						,10
						,10
						,10
						,10
						,sizeof(TBBUTTON));
					}
				else
				{
						m_hwndTB = CreateToolbarEx(
						m_hwndTBwin
						,WS_CHILD | TBSTYLE_WRAPABLE | WS_VISIBLE |TBSTYLE_TOOLTIPS |CCS_NORESIZE | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT
						,IDR_TOOLBAR
						,nr_buttons
						,NULL
						,(UINT)hbmToolbigX
						,(LPCTBBUTTON)&tbButtons
						,nr_buttons
						,20
						,20
						,20
						,20
						,sizeof(TBBUTTON));
				}
			}
			else
			{
				if(mini)
				{
					m_hwndTB = CreateToolbarEx(
						m_hwndTBwin
						,WS_CHILD | TBSTYLE_WRAPABLE | WS_VISIBLE |TBSTYLE_TOOLTIPS |CCS_NORESIZE| TBSTYLE_FLAT | TBSTYLE_TRANSPARENT
						,IDR_TOOLBAR
						,nr_buttons
						,NULL
						,(UINT)hbmToolsmall
						,(LPCTBBUTTON)&tbButtons
						,nr_buttons
						,10
						,10
						,10
						,10
						,sizeof(TBBUTTON));
				}
				else
				{
					m_hwndTB = CreateToolbarEx(
						m_hwndTBwin
						,WS_CHILD | TBSTYLE_WRAPABLE | WS_VISIBLE |TBSTYLE_TOOLTIPS |CCS_NORESIZE | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT
						,IDR_TOOLBAR
						,nr_buttons
						,NULL
						,(UINT)hbmToolbig
						,(LPCTBBUTTON)&tbButtons
						,nr_buttons
						,20
						,20
						,20
						,20
						,sizeof(TBBUTTON));
				}
			}
		}
		else
		{
		//old
		//buttonmap=IDB_BITMAP1;
		//minibuttonmap=IDB_BITMAP7;
		// BW
		//buttonmap=IDB_BITMAP16;
		//minibuttonmap=IDB_BITMAP18;
		// new color
		buttonmap=IDB_BITMAPl;
		minibuttonmap=IDB_BITMAPs;
		if (m_remote_mouse_disable)
					{
						//old
						//buttonmap=IDB_BITMAP8;
						//minibuttonmap=IDB_BITMAP9;
						//BW
						//buttonmap=IDB_BITMAP17;
						//minibuttonmap=IDB_BITMAP15;
						// new color
						buttonmap=IDB_BITMAPlx;
						minibuttonmap=IDB_BITMAPsx;
					}
		if (mini)
		{
			m_hwndTB = CreateToolbarEx(
				m_hwndTBwin
				//,WS_CHILD|WS_BORDER|WS_VISIBLE|TBSTYLE_TOOLTIPS|TBSTYLE_WRAPABLE|TB_AUTOSIZE
				,WS_CHILD | TBSTYLE_WRAPABLE | WS_VISIBLE |TBSTYLE_TOOLTIPS |CCS_NORESIZE| TBSTYLE_FLAT | TBSTYLE_TRANSPARENT
				,IDR_TOOLBAR
				,nr_buttons
				,(HINSTANCE)m_pApp->m_instance
				,minibuttonmap
				,(LPCTBBUTTON)&tbButtons
				,nr_buttons
				,10
				,10
				,10
				,10
				,sizeof(TBBUTTON));
		}
		else
		{
			m_hwndTB = CreateToolbarEx(
				m_hwndTBwin
				//,WS_CHILD|WS_BORDER|WS_VISIBLE|TBSTYLE_TOOLTIPS|TBSTYLE_WRAPABLE|TB_AUTOSIZE
				,WS_CHILD | TBSTYLE_WRAPABLE | WS_VISIBLE |TBSTYLE_TOOLTIPS |CCS_NORESIZE | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT
				,IDR_TOOLBAR
				,nr_buttons
				,(HINSTANCE)m_pApp->m_instance
				,buttonmap
				,(LPCTBBUTTON)&tbButtons
				,nr_buttons
				,20
				,20
				,20
				,20
				,sizeof(TBBUTTON)); 
		}
		}

		tbab.hInst = m_pApp->m_instance;
		tbab.nID = IDB_BITMAPl;
		stdidx = SendMessage(m_hwndTB,TB_ADDBITMAP,6,(LPARAM)&tbab);
		RECT tbrect;
		RECT wrect;
		RECT trect;
		SendMessage(m_hwndTB,TB_SETROWS,(WPARAM) MAKEWPARAM (2, true),(LPARAM) (LPRECT) (&trect));

		GetClientRect(m_hwndTB,&tbrect);
		GetClientRect(m_hwndTBwin,&wrect);

		m_hwndTT = CreateWindow(
			TOOLTIPS_CLASS,
			(LPSTR)NULL,
			TTS_ALWAYSTIP,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			NULL,
			(HMENU)NULL,
			(HINSTANCE)m_pApp->m_instance,
			NULL);
		
        // 6 May 2008 jdp make topmost so they display in fullscreen mode
        ::SetWindowPos(m_hwndTT, HWND_TOPMOST,0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		DWORD buttonWidth = LOWORD(SendMessage(m_hwndTB,TB_GETBUTTONSIZE,(WPARAM)0,(LPARAM)0));
		DWORD buttonHeight = HIWORD(SendMessage(m_hwndTB,TB_GETBUTTONSIZE,(WPARAM)0,(LPARAM)0));

		for (row = 0; row < 1 ; row++ )
			for (col = 0; col < nr_buttons; col++) {
				ti.cbSize = sizeof(TOOLINFO);
				ti.uFlags = 0 ;
				ti.hwnd = m_hwndTB;
				ti.hinst = m_pApp->m_instance;
				ti.uId = (UINT) id;
				ti.lpszText = (LPSTR) szTips[id++];
				ti.rect.left = col * buttonWidth;
				ti.rect.top = row * buttonHeight;
				ti.rect.right = ti.rect.left + buttonWidth;
				ti.rect.bottom = ti.rect.top + buttonHeight;

				SendMessage(m_hwndTT, TTM_ADDTOOL, 0,
                    (LPARAM) (LPTOOLINFO) &ti);
			}
			SendMessage(m_hwndTB,TB_SETTOOLTIPS,(WPARAM)(HWND)m_hwndTT,(LPARAM)0);
			SendMessage(m_hwndTT,TTM_SETTIPBKCOLOR,(WPARAM)(COLORREF)0x00404040,(LPARAM)0);
			SendMessage(m_hwndTT,TTM_SETTIPTEXTCOLOR,(WPARAM)(COLORREF)0x00F5B28D,(LPARAM)0);
			SendMessage(m_hwndTT,TTM_SETDELAYTIME,(WPARAM)(DWORD)TTDT_INITIAL,(LPARAM)(INT) MAKELONG(200,0));           

			ShowWindow(m_hwndTB, SW_SHOW);
			ShowWindow(m_hwndTBwin, SW_SHOW);
	}
	else
	{
		int nr_buttons=9;
		TBADDBITMAP tbab;
		TBBUTTON tbButtons []=
		{
			{0,ID_BUTTON_CAD,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,0},
			{1,ID_BUTTON_FULLSCREEN,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,1},
			{2,ID_BUTTON_PROPERTIES,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,2},
			{3,ID_BUTTON_REFRESH,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,3},
			{4,ID_BUTTON_STRG_ESC,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,4},
			{5,ID_BUTTON_SEP,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,5},
			{6,ID_BUTTON_INFO,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,6},
			{7,ID_BUTTON_END,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,7},
			{8,ID_BUTTON_DBUTTON,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,8},
		};
		static char *szTips[9] =
		{
			sz_L2,
				sz_L3,
				sz_L4,
				sz_L5,
				sz_L6,
				sz_L7,
				sz_L8,
				sz_L9,
				sz_L10,
		};
		int stdidx;
		HWND m_hwndTT;
		int row,col;
		TOOLINFO ti;
		int id=0;
		RECT clr;
		InitCommonControls();
		GetClientRect(m_hwndMain,&clr);
		m_TBr.left=0;
		m_TBr.right=clr.right;
		m_TBr.top=0;
		m_TBr.bottom=28;
		if (mini)
		{
			m_hwndTB = CreateToolbarEx(
				m_hwndTBwin
				//,WS_CHILD|WS_BORDER|WS_VISIBLE|TBSTYLE_TOOLTIPS|TBSTYLE_WRAPABLE|TB_AUTOSIZE
				,WS_CHILD | TBSTYLE_WRAPABLE | WS_VISIBLE |TBSTYLE_TOOLTIPS |CCS_NORESIZE | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT
				,IDR_TOOLBAR
				,nr_buttons
				,(HINSTANCE)m_pApp->m_instance
				,IDB_BITMAPs
				,(LPCTBBUTTON)&tbButtons
				,nr_buttons
				,12
				,8
				,12
				,8
				,sizeof(TBBUTTON));
		}
		else
		{
			m_hwndTB = CreateToolbarEx(
				m_hwndTBwin
				//,WS_CHILD|WS_BORDER|WS_VISIBLE|TBSTYLE_TOOLTIPS|TBSTYLE_WRAPABLE|TB_AUTOSIZE
				,WS_CHILD | TBSTYLE_WRAPABLE | WS_VISIBLE |TBSTYLE_TOOLTIPS |CCS_NORESIZE | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT
				,IDR_TOOLBAR
				,nr_buttons
				,(HINSTANCE)m_pApp->m_instance
				,IDB_BITMAPl
				,(LPCTBBUTTON)&tbButtons
				,nr_buttons
				,20
				,20
				,20
				,20
				,sizeof(TBBUTTON));
		}

		tbab.hInst = m_pApp->m_instance;
		tbab.nID = IDB_BITMAPl;
		stdidx = SendMessage(m_hwndTB,TB_ADDBITMAP,6,(LPARAM)&tbab);
		RECT tbrect;
		RECT wrect;
		RECT trect;
		SendMessage(m_hwndTB,TB_SETROWS,(WPARAM) MAKEWPARAM (2, true),(LPARAM) (LPRECT) (&trect));

		GetClientRect(m_hwndTB,&tbrect);
		GetClientRect(m_hwndTBwin,&wrect);

		m_hwndTT = CreateWindow(
			TOOLTIPS_CLASS,
			(LPSTR)NULL,
			TTS_ALWAYSTIP,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			NULL,
			(HMENU)NULL,
			(HINSTANCE)m_pApp->m_instance,
			NULL);

        // 6 May 2008 jdp make topmost so they display in fullscreen mode
        ::SetWindowPos(m_hwndTT, HWND_TOPMOST,0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

		DWORD buttonWidth = LOWORD(SendMessage(m_hwndTB,TB_GETBUTTONSIZE,(WPARAM)0,(LPARAM)0));
		DWORD buttonHeight = HIWORD(SendMessage(m_hwndTB,TB_GETBUTTONSIZE,(WPARAM)0,(LPARAM)0));

		for (row = 0; row < 1 ; row++ )
			for (col = 0; col < nr_buttons; col++) {
				ti.cbSize = sizeof(TOOLINFO);
				ti.uFlags = 0 ;
				ti.hwnd = m_hwndTB;
				ti.hinst = m_pApp->m_instance;
				ti.uId = (UINT) id;
				ti.lpszText = (LPSTR) szTips[id++];
				ti.rect.left = col * buttonWidth;
				ti.rect.top = row * buttonHeight;
				ti.rect.right = ti.rect.left + buttonWidth;
				ti.rect.bottom = ti.rect.top + buttonHeight;

				SendMessage(m_hwndTT, TTM_ADDTOOL, 0,
                    (LPARAM) (LPTOOLINFO) &ti);
			}
			SendMessage(m_hwndTB,TB_SETTOOLTIPS,(WPARAM)(HWND)m_hwndTT,(LPARAM)0);
			SendMessage(m_hwndTT,TTM_SETTIPBKCOLOR,(WPARAM)(COLORREF)0x0000ff00,(LPARAM)0);
			SendMessage(m_hwndTT,TTM_SETTIPTEXTCOLOR,(WPARAM)(COLORREF)0x00000000,(LPARAM)0);
			SendMessage(m_hwndTT,TTM_SETDELAYTIME,(WPARAM)(DWORD)TTDT_INITIAL,(LPARAM)(INT) MAKELONG(200,0));            

			ShowWindow(m_hwndTB, SW_SHOW);
			ShowWindow(m_hwndTBwin, SW_SHOW);
	}
}

void ClientConnection::RebuildToolbar(HWND hwnd)
{
    if (m_opts.m_ShowToolbar)
    {
        RECT rect;
        GetWindowRect(hwnd, &rect);
        m_winwidth = rect.right - rect.left;
        m_winheight = rect.bottom - rect.top ;
        if (m_winwidth > 140+85+14*24)
        {
            DestroyWindow(m_hwndTB);
            m_BigToolbar=true;
            CreateButtons(false, m_fServerKnowsFileTransfer);
        }
        else
        {
            m_BigToolbar=false;
            DestroyWindow(m_hwndTB);
            CreateButtons(true, m_fServerKnowsFileTransfer);
        }
    }
}

void ClientConnection::GTGBS_CreateToolbar()
{
	RECT clr;
	WNDCLASS wndclass;

	wndclass.style			= 0;
	wndclass.lpfnWndProc	= ClientConnection::WndProcTBwin;
	wndclass.cbClsExtra		= 0;
	wndclass.cbWndExtra		= 0;
	wndclass.hInstance		= m_pApp->m_instance;
	wndclass.hIcon			= LoadIcon(m_pApp->m_instance, MAKEINTRESOURCE(IDR_TRAY));
	switch (m_opts.m_localCursor) {
	case NOCURSOR:
		wndclass.hCursor		= LoadCursor(m_pApp->m_instance, MAKEINTRESOURCE(IDC_NOCURSOR));
		break;
	case NORMALCURSOR:
		wndclass.hCursor		= LoadCursor(NULL, IDC_ARROW);
		break;
	case DOTCURSOR:
	default:
		wndclass.hCursor		= LoadCursor(m_pApp->m_instance, MAKEINTRESOURCE(IDC_DOTCURSOR));
	}
	//wndclass.hbrBackground	= (HBRUSH) GetStockObject(BLACK_BRUSH);
	wndclass.hbrBackground	=   (HBRUSH)(COLOR_BTNFACE+1);
    wndclass.lpszMenuName	= (const TCHAR *) NULL;
	wndclass.lpszClassName	= VWR_WND_CLASS_NAME;

	RegisterClass(&wndclass);

	const DWORD winstyle = WS_CHILD ;

	GetClientRect(m_hwndMain,&clr);
	m_hwndTBwin = CreateWindowEx(
					//WS_EX_TOPMOST  ,
					0,
					VWR_WND_CLASS_NAME,
					_T("VNC ToolBar"),
					winstyle,
					0,
					0,
					clr.right - clr.left,
					28,
					//m_hwnd,                // Parent handle
					m_hwndMain,
					NULL,                // Menu handle
					m_pApp->m_instance,
					NULL);
	helper::SafeSetWindowUserData(m_hwndTBwin, (LONG_PTR)this);
	ShowWindow(m_hwndTBwin, SW_HIDE);
	//////////////////////////////////////////////////
	if ((clr.right-clr.left)>140+85+14*24)
		CreateButtons(false,m_fServerKnowsFileTransfer);
	else
		CreateButtons(true,m_fServerKnowsFileTransfer);
	//////////////////////////////////////////////////
	RECT r;

	GetClientRect(m_hwndTBwin,&r);
	m_TrafficMonitor = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_CLIENTEDGE,
											"Static",
											NULL,
											WS_CHILD | WS_VISIBLE ,
											clr.right - clr.left-45,
											((r.bottom-r.top) / 2) - 8,
											35,
											22,
											m_hwndTBwin,
											NULL,
											m_pApp->m_instance,
											NULL);

	m_bitmapNONE = LoadImage(m_pApp->m_instance,MAKEINTRESOURCE(IDB_STAT_NONE),IMAGE_BITMAP,22,20,LR_SHARED);
	m_bitmapFRONT = LoadImage(m_pApp->m_instance,MAKEINTRESOURCE(IDB_STAT_FRONT),IMAGE_BITMAP,22,20,LR_SHARED);
	m_bitmapBACK= LoadImage(m_pApp->m_instance,MAKEINTRESOURCE(IDB_STAT_BACK),IMAGE_BITMAP,22,20,LR_SHARED);
	HDC hdc = GetDC(m_TrafficMonitor);
	HDC hdcBits;
	hdcBits = CreateCompatibleDC(hdc);
	HGDIOBJ hbrOld = SelectObject(hdcBits,m_bitmapNONE);
	BitBlt(hdc,0,0,22,22,hdcBits,0,0,SRCCOPY);
	SelectObject(hdcBits,hbrOld);
	DeleteDC(hdcBits);
	ReleaseDC(m_TrafficMonitor,hdc);

	///////////////////////////////////////////////////
	m_logo_wnd = CreateWindow(
									"combobox",
									"",
									WS_CHILD | WS_VISIBLE | WS_TABSTOP|CBS_SIMPLE | CBS_AUTOHSCROLL | WS_VSCROLL,
									clr.right - clr.left-45-70,
									4,
									70,
									28,
									m_hwndTBwin,
									(HMENU)9999,
									m_pApp->m_instance,
									NULL);
	m_button_wnd = CreateWindow(
									"button",
									"",
									WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
									clr.right - clr.left-45-70-200,
									4,
									20,
									20,
									m_hwndTBwin,
									(HMENU)9998,
									m_pApp->m_instance,
									NULL);
	TCHAR valname[256];
	MRU *m_pMRU;
	m_pMRU = new MRU(SESSION_MRU_KEY_NAME,26);
	//adzm 2009-06-21 - show the proxy in the 'recent' box
	if (m_fUseProxy && strlen(m_proxyhost) > 0) {
		TCHAR proxyname[MAX_HOST_NAME_LEN];
		_snprintf(proxyname, MAX_HOST_NAME_LEN-1, "%s:%li (%s:%li)", m_host, m_port, m_proxyhost, m_proxyport);
		SendMessage(m_logo_wnd, CB_ADDSTRING, 0, (LPARAM)proxyname);
	}
    for (int i = 0; i < m_pMRU->NumItems(); i++) {
        m_pMRU->GetItem(i, valname, 255);
        int pos = SendMessage(m_logo_wnd, CB_ADDSTRING, 0, (LPARAM) valname);
    }
    SendMessage(m_logo_wnd, CB_SETCURSEL, 0, 0);
	if (m_pMRU) delete m_pMRU;
}
//////////////////////////////////////////////////////////

void ClientConnection::CreateDisplay()
{
#ifdef _WIN32_WCE
	//const DWORD winstyle = WS_VSCROLL | WS_HSCROLL | WS_CAPTION | WS_SYSMENU;
	const DWORD winstyle =  WS_CHILD;
#else
	//const DWORD winstyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME | WS_VSCROLL | WS_HSCROLL;
	const DWORD winstyle = WS_CHILD;
#endif
	RECT Rmain;
	RECT Rtb;
	GetClientRect(m_hwndMain,&Rmain);
	GetClientRect(m_hwndTBwin,&Rtb);

	WNDCLASS wndclass;

	wndclass.style			= 0;
	wndclass.lpfnWndProc	= ClientConnection::WndProchwnd;
	wndclass.cbClsExtra		= 0;
	wndclass.cbWndExtra		= 0;
	wndclass.hInstance		= m_pApp->m_instance;
	wndclass.hIcon			= LoadIcon(m_pApp->m_instance, MAKEINTRESOURCE(IDR_TRAY));
	switch (m_opts.m_localCursor) {
	case NOCURSOR:
		wndclass.hCursor		= LoadCursor(m_pApp->m_instance, MAKEINTRESOURCE(IDC_NOCURSOR));
		break;
	case NORMALCURSOR:
		wndclass.hCursor		= LoadCursor(NULL, IDC_ARROW);
		break;
	case DOTCURSOR:
	default:
		wndclass.hCursor		= LoadCursor(m_pApp->m_instance, MAKEINTRESOURCE(IDC_DOTCURSOR));
	}
	//wndclass.hbrBackground	= (HBRUSH) GetStockObject(BLACK_BRUSH);
	wndclass.hbrBackground	=   NULL;//(HBRUSH)(COLOR_WINDOW+1);
    wndclass.lpszMenuName	= (const TCHAR *) NULL;
	wndclass.lpszClassName	= VWR_WND_CLASS_NAME_VIEWER;

	RegisterClass(&wndclass);

	m_hwndcn = CreateWindow(VWR_WND_CLASS_NAME_VIEWER,
	//m_hwnd = CreateWindow(_T("VNCMDI_Window"),
			      _T("VNCviewer"),
			      winstyle ,
			      0,
			      Rtb.top + Rtb.bottom,
			      CW_USEDEFAULT,       // x-size
			      CW_USEDEFAULT,       // y-size
			      //NULL,                // Parent handle
				  m_hwndMain,
			      NULL,                // Menu handle
			      m_pApp->m_instance,
			      (LPVOID)this);

	//ShowWindow(m_hwnd, SW_HIDE);
	//ShowWindow(m_hwndcn, SW_SHOW);
	//adzm 2009-06-21 - let's not show until connected.

	// record which client created this window
    helper::SafeSetWindowUserData(m_hwndcn, (LONG_PTR)this);

//	SendMessage(m_hwnd,WM_CREATE,0,0);

	// Create a memory DC which we'll use for drawing to
	// the local framebuffer
	m_hBitmapDC = CreateCompatibleDC(NULL);

	// Set a suitable palette up
	if (GetDeviceCaps(m_hBitmapDC, RASTERCAPS) & RC_PALETTE) {
		vnclog.Print(3, _T("Palette-based display - %d entries, %d reserved\n"),
			GetDeviceCaps(m_hBitmapDC, SIZEPALETTE), GetDeviceCaps(m_hBitmapDC, NUMRESERVED));
		BYTE buf[sizeof(LOGPALETTE)+216*sizeof(PALETTEENTRY)];
		LOGPALETTE *plp = (LOGPALETTE *) buf;
		int pepos = 0;
		for (int r = 5; r >= 0; r--) {
			for (int g = 5; g >= 0; g--) {
				for (int b = 5; b >= 0; b--) {
					plp->palPalEntry[pepos].peRed   = r * 255 / 5;
					plp->palPalEntry[pepos].peGreen = g * 255 / 5;
					plp->palPalEntry[pepos].peBlue  = b * 255 / 5;
					plp->palPalEntry[pepos].peFlags  = NULL;
					pepos++;
				}
			}
		}
		plp->palVersion = 0x300;
		plp->palNumEntries = 216;
		m_hPalette = CreatePalette(plp);
	}

	// Add stuff to System menu
	HMENU hsysmenu = GetSystemMenu(m_hwndMain, FALSE);
	if (!m_opts.m_restricted) {
		// Modif sf@2002
		AppendMenu(hsysmenu, MF_SEPARATOR, NULL, NULL);
		AppendMenu(hsysmenu, MF_STRING, ID_FILETRANSFER,	sz_L16);
		AppendMenu(hsysmenu, MF_STRING, ID_TEXTCHAT,	sz_L17);
		AppendMenu(hsysmenu, MF_SEPARATOR, NULL, NULL);
		AppendMenu(hsysmenu, MF_STRING, ID_DBUTTON,	sz_L18);
		// AppendMenu(hsysmenu, MF_STRING, ID_BUTTON,	_T("Show Toolbar Buttons"));
		AppendMenu(hsysmenu, MF_SEPARATOR, NULL, NULL);
		AppendMenu(hsysmenu, MF_STRING, ID_DINPUT,	sz_L19); // disable remote input
		AppendMenu(hsysmenu, MF_STRING, ID_INPUT,	sz_L20); // enable remote input
		// this seems logically placed here
		AppendMenu(hsysmenu, MF_STRING, ID_VIEWONLYTOGGLE,	sz_L93); // Todo: translate

		AppendMenu(hsysmenu, MF_SEPARATOR, NULL, NULL);

#pragma message("TODO - Localize these new menu strings!")

		// adzm - 2010-07 - Extended clipboard

		m_hPopupMenuClipboard = CreatePopupMenu();
		{
			AppendMenu(m_hPopupMenuClipboard, MF_STRING, ID_ENABLE_CLIPBOARD,		"Enable Automatic Synchronization");
			AppendMenu(m_hPopupMenuClipboard, MF_SEPARATOR, NULL, NULL);

			m_hPopupMenuClipboardFormats = CreatePopupMenu();
			{
				AppendMenu(m_hPopupMenuClipboardFormats, MF_STRING, ID_CLIPBOARD_TEXT,		"Text (Unicode)");
				AppendMenu(m_hPopupMenuClipboardFormats, MF_STRING, ID_CLIPBOARD_RTF,		"Rich Text (RTF)");
				AppendMenu(m_hPopupMenuClipboardFormats, MF_STRING, ID_CLIPBOARD_HTML,		"HTML");
				AppendMenu(m_hPopupMenuClipboardFormats, MF_STRING, ID_CLIPBOARD_DIB,		"Image (Send or Request only)");
			}
			AppendMenu(m_hPopupMenuClipboard, MF_POPUP, (UINT_PTR)m_hPopupMenuClipboardFormats, "Automatically Synchronized Formats");
			AppendMenu(m_hPopupMenuClipboard, MF_SEPARATOR, NULL, NULL);
			AppendMenu(m_hPopupMenuClipboard, MF_STRING, ID_CLIPBOARD_SEND,		"Send All Formats Now...");
			AppendMenu(m_hPopupMenuClipboard, MF_STRING, ID_CLIPBOARD_RECV,		"Request All Formats Now...");
		}
		AppendMenu(hsysmenu, MF_POPUP, (UINT_PTR)m_hPopupMenuClipboard, "Clipboard");

		AppendMenu(hsysmenu, MF_SEPARATOR, NULL, NULL);
		m_hPopupMenuDisplay = CreatePopupMenu();
		{
			AppendMenu(m_hPopupMenuDisplay, MF_STRING, ID_FULLSCREEN,		sz_L24);
			AppendMenu(m_hPopupMenuDisplay, MF_SEPARATOR, NULL, NULL);
			AppendMenu(m_hPopupMenuDisplay, MF_STRING, ID_AUTOSCALING,		sz_L25);
			// Modif sf@2002
			AppendMenu(m_hPopupMenuDisplay, MF_STRING, ID_HALFSCREEN,		sz_L26);
			AppendMenu(m_hPopupMenuDisplay, MF_STRING, ID_FUZZYSCREEN,		sz_L27);
			AppendMenu(m_hPopupMenuDisplay, MF_STRING, ID_NORMALSCREEN2,	sz_L28);
			AppendMenu(m_hPopupMenuDisplay, MF_SEPARATOR, NULL, NULL);
			AppendMenu(m_hPopupMenuDisplay, MF_STRING, ID_MAXCOLORS,		sz_L29);
			AppendMenu(m_hPopupMenuDisplay, MF_STRING, ID_256COLORS,		sz_L30);
		}
		AppendMenu(hsysmenu, MF_POPUP, (UINT_PTR)m_hPopupMenuDisplay, "Display");

		AppendMenu(hsysmenu, MF_SEPARATOR, NULL, NULL);
		m_hPopupMenuKeyboard = CreatePopupMenu();
		{
 			AppendMenu(m_hPopupMenuKeyboard, MF_STRING, ID_CONN_CTLALTDEL,	sz_L31);
			AppendMenu(m_hPopupMenuKeyboard, MF_STRING, ID_CONN_CTLESC,		sz_L32);
			AppendMenu(m_hPopupMenuKeyboard, MF_STRING, ID_CONN_CTLDOWN,	sz_L33);
			AppendMenu(m_hPopupMenuKeyboard, MF_STRING, ID_CONN_CTLUP,		sz_L34);
			AppendMenu(m_hPopupMenuKeyboard, MF_STRING, ID_CONN_ALTDOWN,	sz_L35);
			AppendMenu(m_hPopupMenuKeyboard, MF_STRING, ID_CONN_ALTUP,		sz_L36);
		}
		AppendMenu(hsysmenu, MF_POPUP, (UINT_PTR)m_hPopupMenuKeyboard, "Keyboard");

		// group connection options together
		AppendMenu(hsysmenu, MF_SEPARATOR, NULL, NULL);
		AppendMenu(hsysmenu, MF_STRING, IDC_OPTIONBUTTON,	sz_L21);
		AppendMenu(hsysmenu, MF_STRING, ID_CONN_ABOUT,		sz_L22);
		AppendMenu(hsysmenu, MF_STRING, ID_REQUEST_REFRESH,	sz_L23);
		AppendMenu(hsysmenu, MF_SEPARATOR, NULL, NULL);
		AppendMenu(hsysmenu, MF_STRING, ID_NEWCONN,			sz_L37);
		AppendMenu(hsysmenu, MF_STRING | (m_serverInitiated ? MF_GRAYED : 0),
			ID_CONN_SAVE_AS,	sz_L38);
	}
    AppendMenu(hsysmenu, MF_SEPARATOR, NULL, NULL);
	AppendMenu(hsysmenu, MF_STRING, IDD_APP_ABOUT,		sz_L39);
	if (m_opts.m_listening) {
		AppendMenu(hsysmenu, MF_SEPARATOR, NULL, NULL);
		AppendMenu(hsysmenu, MF_STRING, ID_CLOSEDAEMON, sz_L40);
	}
	DrawMenuBar(m_hwndMain);
	TheAccelKeys.SetWindowHandle(m_opts.m_NoHotKeys ? 0 : m_hwndMain);

	// adzm - 2010-07 - Extended clipboard
	UpdateMenuItems();

	// adzm - 2010-07 - Fix clipboard hangs - we don't do this immediately anymore
	//WatchClipboard();

	//Added by: Lars Werner (http://lars.werner.no)
	if(TitleBar.GetSafeHwnd()==NULL)
		TitleBar.Create(m_pApp->m_instance, m_hwndMain);
}

// adzm - 2010-07 - Fix clipboard hangs
void ClientConnection::WatchClipboard()
{
	// Set up clipboard watching
#ifndef _WIN32_WCE
	// We want to know when the clipboard changes, so
	// insert ourselves in the viewer chain. But doing
	// this will cause us to be notified immediately of
	// the current state.
	// We don't want to send that.
	//m_initialClipboardSeen = false;
	m_settingClipboardViewer = true;
	m_hwndNextViewer = SetClipboardViewer(m_hwndcn);
	vnclog.Print(6, "SetClipboardViewer to 0x%08x; next is 0x%08x. Last error 0x%08x", m_hwndcn, m_hwndNextViewer, GetLastError());
	m_settingClipboardViewer = false;
#endif
}

// adzm - 2010-07 - Extended clipboard
void ClientConnection::UpdateMenuItems()
{
	// system menu
	HMENU hsysmenu = GetSystemMenu(m_hwndMain, FALSE);

	CheckMenuItem(hsysmenu,
				  ID_DBUTTON,
				  MF_BYCOMMAND | (m_opts.m_ShowToolbar ? MF_CHECKED :MF_UNCHECKED));

	CheckMenuItem(hsysmenu,
				  ID_VIEWONLYTOGGLE,
				  MF_BYCOMMAND | (m_opts.m_ViewOnly ? MF_CHECKED :MF_UNCHECKED));

	// display menu
	CheckMenuItem(m_hPopupMenuDisplay,
				  ID_AUTOSCALING,
				  MF_BYCOMMAND | (m_opts.m_fAutoScaling ? MF_CHECKED :MF_UNCHECKED));

	// clipboard menu
	CheckMenuItem(m_hPopupMenuClipboard,
				  ID_ENABLE_CLIPBOARD,
				  MF_BYCOMMAND | (m_opts.m_DisableClipboard ? MF_UNCHECKED : MF_CHECKED));
	// don't allow text format to be interacted with
	CheckMenuItem(m_hPopupMenuClipboardFormats,
				  ID_CLIPBOARD_TEXT,
				  MF_BYCOMMAND | MF_CHECKED | MF_GRAYED | MF_DISABLED);

	CheckMenuItem(m_hPopupMenuClipboardFormats,
				  ID_CLIPBOARD_RTF,
				  MF_BYCOMMAND | ( (m_clipboard.settings.m_nLimitRTF > 0) ? MF_CHECKED : MF_UNCHECKED));

	CheckMenuItem(m_hPopupMenuClipboardFormats,
				  ID_CLIPBOARD_HTML,
				  MF_BYCOMMAND | ( (m_clipboard.settings.m_nLimitHTML > 0) ? MF_CHECKED : MF_UNCHECKED));

	CheckMenuItem(m_hPopupMenuClipboardFormats,
				  ID_CLIPBOARD_DIB,
				  MF_BYCOMMAND | MF_UNCHECKED | MF_GRAYED | MF_DISABLED);

	EnableMenuItem(m_hPopupMenuClipboard,
				  ID_ENABLE_CLIPBOARD,
				  m_opts.m_ViewOnly ? MF_DISABLED : MF_ENABLED);

	// don't allow text format to be interacted with
	EnableMenuItem(m_hPopupMenuClipboardFormats,
				  ID_CLIPBOARD_TEXT,
				  MF_DISABLED);
	EnableMenuItem(m_hPopupMenuClipboardFormats,
				  ID_CLIPBOARD_RTF,
				  (m_opts.m_ViewOnly || m_opts.m_DisableClipboard || !m_clipboard.settings.m_bSupportsEx || !(m_clipboard.settings.m_remoteCaps & clipRTF)) ? MF_DISABLED : MF_ENABLED);

	EnableMenuItem(m_hPopupMenuClipboardFormats,
				  ID_CLIPBOARD_HTML,
				  (m_opts.m_ViewOnly || m_opts.m_DisableClipboard || !m_clipboard.settings.m_bSupportsEx || !(m_clipboard.settings.m_remoteCaps & clipHTML)) ? MF_DISABLED : MF_ENABLED);

	EnableMenuItem(m_hPopupMenuClipboardFormats,
				  ID_CLIPBOARD_DIB,
				  MF_DISABLED);

	EnableMenuItem(m_hPopupMenuClipboard,
				  ID_CLIPBOARD_RECV,
				  (!m_clipboard.settings.m_bSupportsEx) ? MF_DISABLED : MF_ENABLED);
}

//
// sf@2002 - DSMPlugin loading and initialization if required
//
void ClientConnection::LoadDSMPlugin(bool fForceReload)
{
	if (m_opts.m_fUseDSMPlugin)
	{
		// sf@2007 - Autoreconnect stuff - Reload/Reset of the plugin
		if (m_pDSMPlugin->IsLoaded() && fForceReload)
		{
			m_pDSMPlugin->UnloadPlugin();
			//unloaded, interface doesn't exist anymore
			m_pPluginInterface = NULL;
			m_pIntegratedPluginInterface = NULL;
			m_pDSMPlugin->SetEnabled(false);
		}

		if (!m_pDSMPlugin->IsLoaded())
		{
			m_pDSMPlugin->LoadPlugin(m_opts.m_szDSMPluginFilename, m_opts.m_listening);
			if (m_pDSMPlugin->IsLoaded())
			{
				if (m_pDSMPlugin->InitPlugin())
				{
					//detect old_plugin
					if (!m_pDSMPlugin->SupportsMultithreaded()) //PGM
					m_opts.m_oldplugin=true; //PGM
					else //PGM
					m_opts.m_oldplugin=false; //PGM

					m_pDSMPlugin->SetEnabled(true);
					m_pDSMPlugin->DescribePlugin();
					/*
					MessageBox(NULL,
					_T(_this->m_pDSMPlugin->DescribePlugin()),
					_T("Plugin Description"), MB_OK | MB_ICONEXCLAMATION );
					*/
				}
				else
				{
					m_pDSMPlugin->SetEnabled(false);
					MessageBox(m_hwndMain,
						sz_F1,
						sz_F6, MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_TOPMOST);
					return;
				}
			}
			else
			{
				m_pDSMPlugin->SetEnabled(false);
				MessageBox(m_hwndMain,
					sz_F5,
					sz_F6, MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_TOPMOST);
				return;
			}
		}
	}
	return;
}

//
// Get & Set the VNC password for the DSMPlugin if necessary
//
void ClientConnection::SetDSMPluginStuff()
{
	//adzm - 2009-06-21
	if (m_pPluginInterface) {
		delete m_pPluginInterface;
		m_pPluginInterface = NULL;
		//adzm 2010-05-10
		m_pIntegratedPluginInterface = NULL;
	}

	if (m_pDSMPlugin->IsEnabled())
	{
		vnclog.Print(0, _T("DSMPlugin enabled\n"));
		char szParams[256+16];
		//strcpy(szParams,m_pDSMPlugin->GetPluginParams());
		// Does the plugin need the VNC password to do its job ?
		if (!_stricmp(m_pDSMPlugin->GetPluginParams(), "VNCPasswordNeeded"))
		{
			// Yes. The user must enter the VNC password
			// He won't be prompted again for password if ms_logon is not used.
			if (strlen(m_clearPasswd) == 0) // Possibly set using -password command line
			{
				AuthDialog ad;
				if (ad.DoDialog(false,m_host,m_port))
				{
					strncpy(m_clearPasswd, ad.m_passwd,254);
				}
			}
			strcpy(szParams, m_clearPasswd);
		}
		else
			strcpy(szParams, "NoPassword");

		// The second parameter tells the plugin the kind of program is using it
		// (in vncviewer : "viewer")
		strcat(szParams, ",");
		strcat(szParams, "viewer");

		// Initialize the DSM Plugin with params
		if (!m_pDSMPlugin->SetPluginParams(NULL, szParams))
		{
			m_pDSMPlugin->SetEnabled(false);
			m_fUsePlugin = false;
			vnclog.Print(0, _T("DSMPlugin cannot be configured\n"));
			throw WarningException(sz_L41,IDS_L41);
		}
		// If all went well
		m_fUsePlugin = true;

		//adzm - 2009-06-21
		if (m_pDSMPlugin->SupportsMultithreaded()) {
			//adzm 2010-05-10
			if (m_pDSMPlugin->SupportsIntegrated()) {
				m_pIntegratedPluginInterface = m_pDSMPlugin->CreateIntegratedPluginInterface();
				m_pPluginInterface = m_pIntegratedPluginInterface;
			} else {
				m_pIntegratedPluginInterface = NULL;
				m_pPluginInterface = m_pDSMPlugin->CreatePluginInterface();
			}
		} else {
			m_pIntegratedPluginInterface = NULL;
			m_pPluginInterface = NULL;
		}
	} else {
		vnclog.Print(0, _T("DSMPlugin not enabled\n"));
	}
}

//
//
//
void ClientConnection::HandleQuickOption()
{
	switch (m_opts.m_quickoption)
	{
	case 1:
		m_opts.m_PreferredEncodings.clear();
		if (new_ultra_server) m_opts.m_PreferredEncodings.push_back(rfbEncodingUltra2);
		else m_opts.m_PreferredEncodings.push_back(rfbEncodingHextile);
		m_opts.m_Use8Bit = rfbPFFullColors; //false;
		m_opts.m_fEnableCache = false;
		m_opts.autoDetect = true;
		break;

	case 2:
		m_opts.m_PreferredEncodings.clear();
		m_opts.m_PreferredEncodings.push_back(rfbEncodingHextile);
		m_opts.m_Use8Bit = rfbPFFullColors; // false; // Max colors
		m_opts.autoDetect = false;
		m_opts.m_fEnableCache = false;
//		m_opts.m_localCursor = NOCURSOR;
		// m_opts.m_requestShapeUpdates = true;
		// m_opts.m_ignoreShapeUpdates = false;
		break;

	case 3:
		m_opts.m_PreferredEncodings.clear();
		m_opts.m_PreferredEncodings.push_back(rfbEncodingZRLE);
		m_opts.m_Use8Bit = rfbPF256Colors; //false;
		m_opts.autoDetect = false;
		m_opts.m_fEnableCache = false;
//		m_opts.m_localCursor = NOCURSOR;
		break;

	case 4:
		m_opts.m_PreferredEncodings.clear();
		m_opts.m_PreferredEncodings.push_back(rfbEncodingZRLE);
		m_opts.m_Use8Bit = rfbPF64Colors; //true;
		m_opts.autoDetect = false;
		m_opts.m_fEnableCache = true;
		break;

	case 5:
		m_opts.m_PreferredEncodings.clear();
		m_opts.m_PreferredEncodings.push_back(rfbEncodingZRLE);
		m_opts.m_Use8Bit = rfbPF8Colors; //true;
		// m_opts.m_scaling = true;
		// m_opts.m_scale_num = 200;
		// m_opts.m_scale_den = 100;
		// m_opts.m_nServerScale = 2;
		m_opts.m_enableJpegCompression = false;
		m_opts.autoDetect = false;
		m_opts.m_fEnableCache = true;
		break;

	case 7:
		m_opts.m_PreferredEncodings.clear();
		m_opts.m_PreferredEncodings.push_back(rfbEncodingUltra);
		m_opts.m_Use8Bit = rfbPFFullColors; //false; // Max colors
		m_opts.autoDetect = false;
		// [v1.0.2-jp2 fix-->]
		m_opts.m_UseEnc[rfbEncodingCopyRect] = false;
		// [<--v1.0.2-jp2 fix]
		m_opts.m_fEnableCache = false;
		m_opts.m_requestShapeUpdates = false;
		m_opts.m_ignoreShapeUpdates = true;
//		m_opts.m_localCursor = NOCURSOR;
		break;

	default: // 0 can be set by noauto command line option. Do not chnage any setting in this case
		/* sf@2005
		m_opts.m_PreferredEncoding = rfbEncodingZRLE;
		m_opts.m_Use8Bit = rfbPF256Colors; //false;
		m_opts.m_fEnableCache = true;
		m_opts.autoDetect = false;
		*/
		break;
	}
}

void ClientConnection::GetConnectDetails()
{
	if (m_opts.m_configSpecified) {
		LoadConnection(m_opts.m_configFilename, false);
	}
	else
	{
		if (!command_line)
		{
            char optionfile[MAX_PATH];
            VNCOptions::GetDefaultOptionsFileName(optionfile);
			if (LoadConnection(optionfile, false)==-1)
				{
					SessionDialog sessdlg(&m_opts, this, m_pDSMPlugin); //sf@2002
					if (!sessdlg.DoDialog())
					{
						throw QuietException(sz_L42);
					}
					_tcsncpy(m_host, sessdlg.m_host_dialog, MAX_HOST_NAME_LEN);
					m_port = sessdlg.m_port;
					_tcsncpy(m_proxyhost, sessdlg.m_proxyhost, MAX_HOST_NAME_LEN);
			//		_tcsncpy(m_remotehost, sessdlg.m_remotehost, MAX_HOST_NAME_LEN);
					m_proxyport = sessdlg.m_proxyport;
					m_fUseProxy = sessdlg.m_fUseProxy;
					if (m_opts.autoDetect)
					{
						m_opts.m_Use8Bit = rfbPFFullColors;
					}
				}
		}
		else
		{
			SessionDialog sessdlg(&m_opts, this, m_pDSMPlugin); //sf@2002
			if (!sessdlg.DoDialog())
					{
						throw QuietException(sz_L42);
					}
			_tcsncpy(m_host, sessdlg.m_host_dialog, MAX_HOST_NAME_LEN);
			m_port = sessdlg.m_port;
			_tcsncpy(m_proxyhost, sessdlg.m_proxyhost, MAX_HOST_NAME_LEN);
	//		_tcsncpy(m_remotehost, sessdlg.m_remotehost, MAX_HOST_NAME_LEN);
			m_proxyport = sessdlg.m_proxyport;
			m_fUseProxy = sessdlg.m_fUseProxy;
			if (m_opts.autoDetect)
			{
				m_opts.m_Use8Bit = rfbPFFullColors;
			}
		}
	}
	// This is a bit of a hack:
	// The config file may set various things in the app-level defaults which
	// we don't want to be used except for the first connection. So we clear them
	// in the app defaults here.
	if (!command_line)
	{
	m_pApp->m_options.m_host_options[0] = '\0';
	m_pApp->m_options.m_port = -1;
	m_pApp->m_options.m_proxyhost[0] = '\0';
	m_pApp->m_options.m_proxyport = -1;
	m_pApp->m_options.m_connectionSpecified = false;
	m_pApp->m_options.m_configSpecified = false;
	}
}
DWORD WINAPI SocketTimeout(LPVOID lpParam)
{
	SOCKET *sock;
	sock=(SOCKET*) lpParam;
	int counter=0;
	while (havetobekilled && !forcedexit)
	{
		Sleep(100);
	}

	if (havetobekilled)
	{
		closesocket(*sock);
		*sock = INVALID_SOCKET;
	}
	return 0;
}
void ClientConnection::Connect()
{
	struct sockaddr_in thataddr;
	int res;
	if (!m_opts.m_NoStatus) GTGBS_ShowConnectWindow();
	if (m_sock!=NULL && m_sock!=INVALID_SOCKET) closesocket(m_sock);
	m_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (m_hwndStatus) SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L43);
	if (m_sock == INVALID_SOCKET) {if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L44);throw WarningException(sz_L44);}
	int one = 1;

	if (m_hwndStatus) SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L45);
	if (m_hwndStatus) UpdateWindow(m_hwndStatus);

	// The host may be specified as a dotted address "a.b.c.d"
	// Try that first
	thataddr.sin_addr.s_addr = inet_addr(m_host);

	// If it wasn't one of those, do gethostbyname
	if (thataddr.sin_addr.s_addr == INADDR_NONE) {
		LPHOSTENT lphost;
		lphost = gethostbyname(m_host);

		if (lphost == NULL)
		{
			//if(myDialog!=0)DestroyWindow(myDialog);
			SetEvent(KillEvent);
			if (m_hwndStatus) SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L46);
			throw WarningException(sz_L46,IDS_L46);
		};
		thataddr.sin_addr.s_addr = ((LPIN_ADDR) lphost->h_addr)->s_addr;
	};

	if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L47);
	if (m_hwndStatus)ShowWindow(m_hwndStatus,SW_SHOW);
	if (m_hwndStatus)UpdateWindow(m_hwndStatus);
	if (m_hwndStatus)SetDlgItemInt(m_hwndStatus,IDC_PORT,m_port,FALSE);
	thataddr.sin_family = AF_INET;
	thataddr.sin_port = htons(m_port);
	///Force break after timeout
	DWORD				  threadID;
	ThreadSocketTimeout = CreateThread(NULL,0,SocketTimeout,(LPVOID)&m_sock,0,&threadID);
	res = connect(m_sock, (LPSOCKADDR) &thataddr, sizeof(thataddr));

	if (res == SOCKET_ERROR)
		{
			int a=WSAGetLastError();
			vnclog.Print(0, _T("socket error %i\n"),a);
			if(a==6)
				Sleep(5000);
			if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L48);
			SetEvent(KillEvent);
			if (!Pressed_Cancel) throw WarningException(sz_L48,IDS_L48);
			else throw QuietException(sz_L48);
		}
	vnclog.Print(0, _T("Connected to %s port %d\n"), m_host, m_port);
	if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L49);
	if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_VNCSERVER,m_host);
	if (m_hwndStatus)ShowWindow(m_hwndStatus,SW_SHOW);
	if (m_hwndStatus)UpdateWindow(m_hwndStatus);
}

void ClientConnection::ConnectProxy()
{
	struct sockaddr_in thataddr;
	int res;
	if (!m_opts.m_NoStatus) GTGBS_ShowConnectWindow();

	m_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L43);
	if (m_sock == INVALID_SOCKET) {if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L44);throw WarningException(sz_L44);}
	int one = 1;

	if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L45);
	if (m_hwndStatus)UpdateWindow(m_hwndStatus);

	// The host may be specified as a dotted address "a.b.c.d"
	// Try that first
	thataddr.sin_addr.s_addr = inet_addr(m_proxyhost);

	// If it wasn't one of those, do gethostbyname
	if (thataddr.sin_addr.s_addr == INADDR_NONE) {
		LPHOSTENT lphost;
		lphost = gethostbyname(m_proxyhost);

		if (lphost == NULL) {
			//if(myDialog!=0)DestroyWindow(myDialog);
			SetEvent(KillEvent);
			if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L46);
			throw WarningException(sz_L46);
		};
		thataddr.sin_addr.s_addr = ((LPIN_ADDR) lphost->h_addr)->s_addr;
	};

	if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L47);
	if (m_hwndStatus)ShowWindow(m_hwndStatus,SW_SHOW);
	if (m_hwndStatus)UpdateWindow(m_hwndStatus);
	if (m_hwndStatus)SetDlgItemInt(m_hwndStatus,IDC_PORT,m_proxyport,FALSE);
	thataddr.sin_family = AF_INET;
	thataddr.sin_port = htons(m_proxyport);

	DWORD				  threadID;
	ThreadSocketTimeout = CreateThread(NULL,0,SocketTimeout,(LPVOID)&m_sock,0,&threadID);

	res = connect(m_sock, (LPSOCKADDR) &thataddr, sizeof(thataddr));
	if (res == SOCKET_ERROR) {if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L48);throw WarningException(sz_L48,IDS_L48);}
	vnclog.Print(0, _T("Connected to %s port %d\n"), m_proxyhost, m_proxyport);
	if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L49);
	if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_VNCSERVER,m_proxyhost);
	if (m_hwndStatus)ShowWindow(m_hwndStatus,SW_SHOW);
	if (m_hwndStatus)UpdateWindow(m_hwndStatus);
}

void ClientConnection::SetSocketOptions()
{
	// Disable Nagle's algorithm
	BOOL nodelayval = TRUE;
	if (setsockopt(m_sock, IPPROTO_TCP, TCP_NODELAY, (const char *) &nodelayval, sizeof(BOOL)))
		throw WarningException(sz_L50);
	
	// adzm 2010-10
	if (fis) {
		delete fis;
		fis = NULL;
	}
    fis = new rdr::FdInStream(m_sock);
	fis->SetDSMMode(m_pDSMPlugin->IsEnabled()); // sf@2003 - Special DSM mode for ZRLE encoding
}

void ClientConnection::NegotiateProtocolVersion()
{
	rfbProtocolVersionMsg pv;

   /* if the connection is immediately closed, don't report anything, so
       that pmw's monitor can make test connections */

	bool fNotEncrypted = false;
	if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L89);
    try
	{
		//ReadExact(pv, sz_rfbProtocolVersionMsg);
		//adzm 2009-06-21
		ReadExactProtocolVersion(pv, sz_rfbProtocolVersionMsg, fNotEncrypted);
	}
	//adzm 2009-07-05
	catch (rdr::EndOfStream& c)
	{
		SetEvent(KillEvent);
		vnclog.Print(0, _T("Error reading protocol version: %s\n"),
                          c.str());
		if (!Pressed_Cancel)
		{
		if (m_fUsePlugin && m_pIntegratedPluginInterface == NULL)
			throw WarningException("Connection failed - Error reading Protocol Version\r\n\r\n"
									"Possible causes:\r\n"
									"- You've forgotten to select a DSMPlugin and the Server uses a DSMPlugin\r\n"
									"- The selected DSMPlugin is not compatible with the one running on the Server\r\n"
									"- The selected DSMPlugin is not correctly configured (also possibly on the Server)\r\n"
									"- The password you've possibly entered is incorrect\r\n"
									"- Another viewer using a DSMPlugin is already connected to the Server (more than one is forbidden)\r\n"
									,1003
									);
		else
			throw WarningException("Connection failed - End of Stream\r\n\r\n"
									"Possible causes:\r\r"
									"- Another user is already listening on this ID\r\n"
									"- Bad connection\r\n",1004
									);
		}
		throw QuietException(c.str());
	}
	catch (Exception &c)
	{
		SetEvent(KillEvent);
		vnclog.Print(0, _T("Error reading protocol version: %s\n"),
                          c.m_info);
		if (!Pressed_Cancel)
		{
		if (m_fUsePlugin && m_pIntegratedPluginInterface == NULL)
			throw WarningException("Connection failed - Error reading Protocol Version\r\n\n\r"
									"Possible causes:\r\n"
									"- You've forgotten to select a DSMPlugin and the Server uses a DSMPlugin\r\n"
									"- The selected DSMPlugin is not compatible with the one running on the Server\r\n"
									"- The selected DSMPlugin is not correctly configured (also possibly on the Server)\r\n"
									"- The password you've possibly entered is incorrect\r\n"
									"- Another viewer using a DSMPlugin is already connected to the Server (more than one is forbidden)\r\n"
									,1003
									);
		else
			throw WarningException("Connection failed - Error reading Protocol Version\r\n\r\n"
									"Possible causes:\r\n"
									"- You've forgotten to select a DSMPlugin and the Server uses a DSMPlugin\r\n"
									"- Viewer and Server are not compatible (they use different RFB protocols)\r\n"
									"- Bad connection\r\n",1004
									);
		}

		throw QuietException(c.m_info);
	}

    pv[sz_rfbProtocolVersionMsg] = 0;

	//adzm 2009-06-21 - warn if we are trying to connect to an unencrypted server. but still allow it if desired.
	//adzm 2010-05-10
	//1096 !m_pIntegratedPluginInterface is NEEDED
	if (m_fUsePlugin && fNotEncrypted && !m_pIntegratedPluginInterface) {
		//adzm 2010-05-12
		if (m_opts.m_fRequireEncryption) {
			throw WarningException("The insecure connection was refused.");
		}
		else
		{
			//SetSocketOptions / fDSMMode?
			m_fUsePlugin = false;

			//adzm - 2009-06-21 - I don't set the plugin to be disabled here, just rely on m_fUsePlugin.

			//adzm - 2009-06-21
			if (m_pPluginInterface) {
				delete m_pPluginInterface;
				m_pPluginInterface = NULL;
				//adzm 2010-05-10
				m_pIntegratedPluginInterface = NULL;
			}
			if (!m_opts.m_fAutoAcceptNoDSM)
			{
				//adzm 2009-07-19 - Auto-accept the connection if it is unencrypted if that option is specified

				int returnvalue=MessageBox(m_hwndMain, "You have specified an encryption plugin, however this connection is unencrypted! Do you want to continue?", "Accept insecure connection", MB_YESNO | MB_ICONEXCLAMATION | MB_TOPMOST);
				if (returnvalue==IDNO)
				{
					throw WarningException("You refused the insecure connection.");
				}
			}
		}
	}

	/*
	// sf@2005 - Cleanup scrambled chars before parsing -> Restore original RFB protocol header first chars
	if (m_fUsePlugin)
	{
		pv[0] = rfbProtocolVersionFormat[0];
		pv[1] = rfbProtocolVersionFormat[1];
		pv[2] = rfbProtocolVersionFormat[2];
		pv[3] = rfbProtocolVersionFormat[3];
	}
	*/

    if (sscanf(pv,rfbProtocolVersionFormat,&m_majorVersion,&m_minorVersion) != 2)
	{
		SetEvent(KillEvent);
		if (m_fUsePlugin && m_pIntegratedPluginInterface == NULL)
			throw WarningException("Connection failed - Invalid protocol !\r\n\r\n"
									"Possible causes:\r\r"
									"- You've forgotten to select a DSMPlugin and the Server uses a DSMPlugin\r\n"
									"- The selected DSMPlugin is not compatible with the one running on the Server\r\n"
									"- The selected DSMPlugin is not correctly configured (also possibly on the Server)\r\n"
									"- The password you've possibly entered is incorrect\r\n"
									"- Another viewer using a DSMPlugin is already connected to the Server (more than one is forbidden)\r\n"

									);
		else
			throw WarningException("Connection failed - Invalid protocol !\r\n\r\n"
									"Possible causes:\r\r"
									"- You've forgotten to select a DSMPlugin and the Server uses a DSMPlugin\r\n"
									"- Viewer and Server are not compatible (they use different RFB protocols)\r\n"
									);
    }

    vnclog.Print(0, _T("RFB server supports protocol version %d.%d\n"),
	    m_majorVersion,m_minorVersion);

	// UltraVNC specific functionnalities
	// - ms logon
	// - FileTransfer (TODO: change Minor version in next eSVNC release so it's compatible with Ultra)
	// Minor = 4 means that server supports FileTransfer and requires ms logon
	// Minor = 6 means that server support FileTransfer and requires normal VNC logon
	if (m_minorVersion == 4)
	{
		m_ms_logon_I_legacy = true;
		m_fServerKnowsFileTransfer = true;
	}
	else if (m_minorVersion == 6) // 6 because 5 already used in TightVNC viewer for some reason
	{
		m_fServerKnowsFileTransfer = true;
	}
	else if (m_minorVersion == 7) // adzm 2010-09 - RFB 3.8
	{
		// adzm2010-10 - RFB3.8 - m_fServerKnowsFileTransfer set during rfbUltraVNC auth
	}
	else if (m_minorVersion == 8) // adzm 2010-09 - RFB 3.8
	{
		// adzm2010-10 - RFB3.8 - m_fServerKnowsFileTransfer set during rfbUltraVNC auth
	}
	// Added for SC so we can do something before actual data transfer start
	else if (m_minorVersion == 14 )
	{
		m_ms_logon_I_legacy = true;
		m_fServerKnowsFileTransfer = true;
	}
	else if ( m_minorVersion == 16)
	{
		m_fServerKnowsFileTransfer = true;
	}
	else if ( m_minorVersion == 18) // adzm 2010-09 - RFB 3.8
	{
		m_fServerKnowsFileTransfer = true;
	}

    else if ((m_majorVersion == 3) && (m_minorVersion < 3)) {
        /* if server is 3.2 we can't use the new authentication */
		vnclog.Print(0, _T("Can't use IDEA authentication\n"));
        /* This will be reported later if authentication is requested*/
	}
	else if ((m_majorVersion == 3) && (m_minorVersion == 3)) {
        /* if server is 3.2 we can't use the new authentication */
		vnclog.Print(0, _T("RFB version 3.3, Legacy \n"));
        /* This will be reported later if authentication is requested*/
    } else {
        /* any other server version, just tell the server what we want */
		m_majorVersion = rfbProtocolMajorVersion;
		m_minorVersion = rfbProtocolMinorVersion; // always 4 for Ultra Viewer
    }

    sprintf(pv,rfbProtocolVersionFormat, m_majorVersion, m_minorVersion);

    WriteExact(pv, sz_rfbProtocolVersionMsg);
	if (m_minorVersion == 14 || m_minorVersion == 16 || m_minorVersion == 18) // adzm 2010-09
	{
		int size;
		ReadExact((char *)&size,sizeof(int));
		char mytext[1025]; //10k
		//block
		if (size<0 || size >1024)
		{
			throw WarningException("Buffer too big, ");
			if (size<0) size=0;
			if (size>1024) size=1024;
		}

		ReadExact(mytext,size);
		mytext[size]=0;

		//adzm 2009-06-21 - auto-accept if specified
		if (!m_opts.m_fAutoAcceptIncoming) {
			int returnvalue=MessageBox(m_hwndMain,   mytext,"Accept Incoming SC Connection", MB_YESNO |  MB_TOPMOST);
			if (returnvalue==IDNO)
			{
				int nummer=0;
				WriteExact((char *)&nummer,sizeof(int));
				throw WarningException("You refused the connection");
			}
			else
			{
				int nummer=1;
				WriteExact((char *)&nummer,sizeof(int));
			}
		} else {
			int nummer=1;
			WriteExact((char *)&nummer,sizeof(int));
		}

		m_minorVersion -= 10;
	}

	vnclog.Print(0, _T("Connected to RFB server, using protocol version %d.%d\n"),
		rfbProtocolMajorVersion, m_minorVersion);

	if (m_minorVersion >= 7 && m_pIntegratedPluginInterface) {
		m_fPluginStreamingIn = true;
		m_fPluginStreamingOut = true;
	}
}

void ClientConnection::NegotiateProxy()
{
	rfbProtocolVersionMsg pv;

   /* if the connection is immediately closed, don't report anything, so
       that pmw's monitor can make test connections */

	if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L89);
    try
	{
		ReadExactProxy(pv, sz_rfbProtocolVersionMsg);
	}
	catch (Exception &c)
	{
		vnclog.Print(0, _T("Error reading protocol version: %s\n"),
                          c.m_info);
		if (m_fUsePlugin)
			throw WarningException("Proxy Connection failed - Error reading Protocol Version\r\n\n\r"
									"Possible causes:\r\r"
									"- You've forgotten to select a DSMPlugin and the Server uses a DSMPlugin\r\n"
									"- The selected DSMPlugin is not compatible with the one running on the Server\r\n"
									"- The selected DSMPlugin is not correctly configured (also possibly on the Server)\r\n"
									"- The password you've possibly entered is incorrect\r\n"
									"- Another viewer using a DSMPlugin is already connected to the Server (more than one is forbidden)\r\n"
									);
		else
			throw WarningException("Proxy Connection failed - Error reading Protocol Version\r\n\n\r"
									"Possible causes:\r\r"
									"- You've forgotten to select a DSMPlugin and the Server uses a DSMPlugin\r\n"
									"- Viewer and Server are not compatible (they use different RFB protocoles)\r\n"
									"- Bad connection\r\n"
									);

		throw QuietException(c.m_info);
	}

    pv[sz_rfbProtocolVersionMsg] = 0;

	/*
	// sf@2005 - Cleanup scrambled chars before parsing -> Restore original RFB protocol header first chars
	if (m_fUsePlugin)
	{
		pv[0] = rfbProtocolVersionFormat[0];
		pv[1] = rfbProtocolVersionFormat[1];
		pv[2] = rfbProtocolVersionFormat[2];
		pv[3] = rfbProtocolVersionFormat[3];
	}
	*/

	if (sscanf(pv,rfbProtocolVersionFormat,&m_majorVersion,&m_minorVersion) != 2)
	{
		if (m_fUsePlugin)
			throw WarningException("Proxy Connection failed - Invalid protocol !\r\n\r\n"
									"Possible causes:\r\r"
									"- You've forgotten to select a DSMPlugin and the Server uses a DSMPlugin\r\n"
									"- The selected DSMPlugin is not compatible with the one running on the Server\r\n"
									"- The selected DSMPlugin is not correctly configured (also possibly on the Server)\r\n"
									"- The password you've possibly entered is incorrect\r\n"
									"- Another viewer using a DSMPlugin is already connected to the Server (more than one is forbidden)\r\n"
									);
		else
			throw WarningException("Proxy Connection failed - Invalid protocol !\r\n\r\n"
									"Possible causes:\r\r"
									"- You've forgotten to select a DSMPlugin and the Server uses a DSMPlugin\r\n"
									"- Viewer and Server are not compatible (they use different RFB protocols)\r\n"
									);
    }

    vnclog.Print(0, _T("Connected to proxy \n"),
	    m_majorVersion,m_minorVersion);

	if (m_majorVersion==0 && m_minorVersion==0)
	{
	TCHAR tmphost[MAX_HOST_NAME_LEN];
	TCHAR tmphost2[256];
	//adzm 2010-09
	::ZeroMemory(tmphost, sizeof(tmphost));
	::ZeroMemory(tmphost2, sizeof(tmphost2));
	_tcscpy(tmphost,m_host);
	if (strcmp(tmphost,"")!=NULL)
	{
	_tcscat(tmphost,":");
	_tcscat(tmphost,_itoa(m_port,tmphost2,10));
	}
    WriteExactProxy(tmphost,MAX_HOST_NAME_LEN);

	vnclog.Print(0, _T("Connected to RFB server, using protocol version %d.%d\n"),
		rfbProtocolMajorVersion, m_minorVersion);
	}
}

void ClientConnection::Authenticate(std::vector<CARD32>& current_auth)
{
	CARD32 authScheme = rfbInvalidAuth;

	// adzm 2010-09
	if (m_minorVersion < 7) {
		ReadExact((char *)&authScheme, sizeof(authScheme));
		authScheme = Swap32IfLE(authScheme);

		// adzm 2010-10 - TRanslate legacy constants into new 3.8-era constants
		switch (authScheme) {
			case rfbLegacy_SecureVNCPlugin:
				authScheme = rfbUltraVNC_SecureVNCPluginAuth_new;
				break;
			case rfbLegacy_MsLogon:
				authScheme = rfbUltraVNC_MsLogonIIAuth;
				break;
		}
	} else {
		CARD8 authAllowedLength = 0;
		ReadExact((char *)&authAllowedLength, sizeof(authAllowedLength));
		if (authAllowedLength == 0) {
			authScheme = rfbConnFailed;
		} else {
			CARD8 authAllowed[256];
			ReadExact((char *)authAllowed, authAllowedLength);

			std::vector<CARD8> auth_supported;
			for (int i = 0; i < authAllowedLength; i++) {
				if (std::find(current_auth.begin(), current_auth.end(), (CARD32)authAllowed[i]) != current_auth.end()) {
					// only once max per scheme
					continue;
				}

				switch (authAllowed[i]) {
				case rfbUltraVNC:
				case rfbUltraVNC_SecureVNCPluginAuth:
				case rfbUltraVNC_SecureVNCPluginAuth_new:
				case rfbUltraVNC_SCPrompt: // adzm 2010-10
				case rfbUltraVNC_SessionSelect:
				case rfbUltraVNC_MsLogonIIAuth:
				case rfbVncAuth:
				case rfbNoAuth:
					auth_supported.push_back(authAllowed[i]);
					break;
				}
			}

			if (!auth_supported.empty()) {
				std::vector<CARD8> auth_priority;
				auth_priority.push_back(rfbUltraVNC);
				auth_priority.push_back(rfbUltraVNC_SecureVNCPluginAuth_new);
				auth_priority.push_back(rfbUltraVNC_SecureVNCPluginAuth);				
				auth_priority.push_back(rfbUltraVNC_SCPrompt); // adzm 2010-10
				auth_priority.push_back(rfbUltraVNC_SessionSelect);
				auth_priority.push_back(rfbUltraVNC_MsLogonIIAuth);
				auth_priority.push_back(rfbVncAuth);
				auth_priority.push_back(rfbNoAuth);

				for (std::vector<CARD8>::iterator best_auth_it = auth_priority.begin(); best_auth_it != auth_priority.end(); best_auth_it++) {
					if (std::find(auth_supported.begin(), auth_supported.end(), (CARD32)(*best_auth_it)) != auth_supported.end()) {
						authScheme = *best_auth_it;
						break;
					}
				}
			}

			if (authScheme == rfbInvalidAuth) {
				throw WarningException("No supported authentication methods!");
			}

			CARD8 authSchemeMsg = (CARD8)authScheme;
			WriteExact((char *)&authSchemeMsg, sizeof(authSchemeMsg));
		}
	}

	AuthenticateServer(authScheme, current_auth);
}

void ClientConnection::AuthenticateServer(CARD32 authScheme, std::vector<CARD32>& current_auth)
{
	if (current_auth.size() > 5) {
		vnclog.Print(0, _T("Cannot layer more than two authentication schemes\n"), authScheme);
		throw ErrorException("Cannot layer more than two authentication schemes\n");
	}

	CARD32 reasonLen;
	CARD32 authResult;

	if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L90);

	bool bSecureVNCPluginActive = std::find(current_auth.begin(), current_auth.end(), rfbUltraVNC_SecureVNCPluginAuth) != current_auth.end();
	if (!bSecureVNCPluginActive) bSecureVNCPluginActive = std::find(current_auth.begin(), current_auth.end(), rfbUltraVNC_SecureVNCPluginAuth_new) != current_auth.end();

	if (!bSecureVNCPluginActive && m_fUsePlugin && m_pIntegratedPluginInterface && authScheme != rfbConnFailed && authScheme != rfbUltraVNC_SecureVNCPluginAuth  && authScheme != rfbUltraVNC_SecureVNCPluginAuth_new && authScheme != rfbUltraVNC)
	{
		//adzm 2010-05-12
		if (m_opts.m_fRequireEncryption) {
			throw WarningException("The insecure connection was refused.");
		}
		else
		{
			//SetSocketOptions / fDSMMode?
			m_fUsePlugin = false;

			//adzm - 2009-06-21 - I don't set the plugin to be disabled here, just rely on m_fUsePlugin.

			//adzm - 2009-06-21
			if (m_pPluginInterface) {
				delete m_pPluginInterface;
				m_pPluginInterface = NULL;
				//adzm 2010-05-10
				m_pIntegratedPluginInterface = NULL;
			}

			//adzm 2009-07-19 - Auto-accept the connection if it is unencrypted if that option is specified
			if (!m_opts.m_fAutoAcceptNoDSM) {
				int returnvalue=MessageBox(m_hwndMain, "You have specified an encryption plugin, however this connection is unencrypted! Do you want to continue?", "Accept insecure connection", MB_YESNO | MB_ICONEXCLAMATION | MB_TOPMOST);
				if (returnvalue==IDNO)
				{
					throw WarningException("You refused the insecure connection.");
				}
			}
		}
	}

	switch(authScheme)
	{
	case rfbUltraVNC:
		new_ultra_server=true;
		m_fServerKnowsFileTransfer = true;
		HandleQuickOption();
		break;
	case rfbUltraVNC_SecureVNCPluginAuth_new:
		if (bSecureVNCPluginActive) {
			vnclog.Print(0, _T("Cannot layer multiple SecureVNC plugin authentication schemes\n"), authScheme);
			throw WarningException("Cannot layer multiple SecureVNC plugin authentication schemes\n");
		}
		AuthSecureVNCPlugin();
		break;
	case rfbUltraVNC_SecureVNCPluginAuth:
		if (bSecureVNCPluginActive) {
			vnclog.Print(0, _T("Cannot layer multiple SecureVNC plugin authentication schemes\n"), authScheme);
			throw WarningException("Cannot layer multiple SecureVNC plugin authentication schemes\n");
		}
		AuthSecureVNCPlugin_old();
		break;
	case rfbUltraVNC_MsLogonIIAuth:
		AuthMsLogonII();
		break;
	case rfbUltraVNC_MsLogonIAuth:
		m_ms_logon_I_legacy = true;
	case rfbVncAuth:
		if (m_ms_logon_I_legacy) {
			AuthMsLogonI();
		} else {
			AuthVnc();
		}
		break;
	case rfbUltraVNC_SCPrompt:
		AuthSCPrompt();
		break;
	case rfbUltraVNC_SessionSelect:
		AuthSessionSelect();
		break;
	case rfbNoAuth:
		if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L92);
		vnclog.Print(0, _T("No authentication needed\n"));
//		current_auth.push_back(authScheme);
		if (m_minorVersion < 8)
			{
				current_auth.push_back(authScheme);
				return;
			}
		break;
	case rfbConnFailed:
		ReadExact((char *)&reasonLen, 4);
		reasonLen = Swap32IfLE(reasonLen);

		CheckBufferSize(reasonLen+1);
		ReadString(m_netbuf, reasonLen);

		vnclog.Print(0, _T("RFB connection failed, reason: %s\n"), m_netbuf);
		if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L91);
		throw WarningException(m_netbuf);
		break;
	default:
		vnclog.Print(0, _T("RFB connection failed, unknown authentication method: %lu\n"), authScheme);
		throw ErrorException("Unknown authentication method!");
		break;
	}

	current_auth.push_back(authScheme);

	// Read the authentication response
	ReadExact((char *)&authResult, sizeof(authResult));
	authResult = Swap32IfLE(authResult);

	if (m_pIntegratedPluginInterface && authScheme == rfbUltraVNC_SecureVNCPluginAuth) {
		m_pIntegratedPluginInterface->SetHandshakeComplete();
		if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_PLUGIN_STATUS,m_pIntegratedPluginInterface->DescribeCurrentSettings());
	}	

	switch (authResult)
	{
	case rfbVncAuthOK:
		if (m_hwndStatus)vnclog.Print(0, _T("VNC authentication succeeded\n"));
		if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L55);

		g_passwordfailed=false;
		break;
	case rfbVncAuthFailed:
	case rfbVncAuthFailedEx:
		vnclog.Print(0, _T("VNC authentication failed!"));
		if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L56);
		if (m_minorVersion >= 7 || authResult == rfbVncAuthFailedEx) {
			vnclog.Print(0, _T("VNC authentication failed! Extended information available."));
			//adzm 2010-05-11 - Send an explanatory message for the failure (if any)
			ReadExact((char *)&reasonLen, 4);
			reasonLen = Swap32IfLE(reasonLen);

			CheckBufferSize(reasonLen+1);
			ReadString(m_netbuf, reasonLen);

			vnclog.Print(0, _T("VNC authentication failed! Extended information: %s\n"), m_netbuf);
			if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,m_netbuf);
			throw WarningException(m_netbuf);
		} else {
			vnclog.Print(0, _T("VNC authentication failed!"));
			SetEvent(KillEvent);
			throw WarningException(sz_L57,IDS_L57);
		}
		break;
	case rfbVncAuthTooMany:
		SetEvent(KillEvent);
		throw WarningException(
			sz_L58);
		break;
	case rfbLegacy_MsLogon:
		if (m_minorVersion >= 7) {
			vnclog.Print(0, _T("Invalid auth response for protocol version.\n"));
			throw ErrorException("Invalid auth response");
		}
		if ((authScheme != rfbUltraVNC_SecureVNCPluginAuth) || !m_pIntegratedPluginInterface) {
			vnclog.Print(0, _T("Invalid auth response response\n"));
			throw ErrorException("Invalid auth response");
		}
		//adzm 2010-05-10
		AuthMsLogonII();
		break;
	case rfbVncAuthContinue:
		if (m_minorVersion < 7) {
			vnclog.Print(0, _T("Invalid auth continue response for protocol version.\n"));
			throw ErrorException("Invalid auth continue response");
		}
		if (current_auth.size() > 5) { // arbitrary
			vnclog.Print(0, _T("Cannot layer more than six authentication schemes\n"), authScheme);
			throw ErrorException("Cannot layer more than six authentication schemes\n");
		}
		Authenticate(current_auth);
		break;
	default:
		vnclog.Print(0, _T("Unknown VNC authentication result: %d\n"),
			(int)authResult);
//				if (flash) {flash->Killflash();}
		throw ErrorException(sz_L59,IDS_L59);
		break;
	}

	return;
}

void ClientConnection::AuthSecureVNCPlugin()
{
	if (m_pIntegratedPluginInterface==NULL) {
		if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,"SecureVNCPlugin authentication failed (SecureVNCPlugin interface available)");
		SetEvent(KillEvent);
		throw ErrorException("SecureVNCPlugin authentication failed (no plugin interface available)");
	}

	char passwd[256];
	passwd[0] = '\0';

	if (strlen(m_clearPasswd)>0)
	{
		strcpy(passwd, m_clearPasswd);
	}
	else if (strlen((const char *) m_encPasswd)>0)
	{  char * pw = vncDecryptPasswd(m_encPasswd);
		strcpy(passwd, pw);
		free(pw);
	}
	m_pIntegratedPluginInterface->SetPasswordData(NULL, 0);

	int nSequenceNumber = 0;
	bool bExpectChallenge = true;
	bool bTriedNoPassword = false;
	bool bSuccess = false;
	bool bCancel = false;
	char passphraseused=0;
	do {
		WORD wChallengeLength = 0;

		ReadExact((char*)&wChallengeLength, sizeof(wChallengeLength));

		BYTE* pChallengeData = new BYTE[wChallengeLength];

		ReadExact((char*)pChallengeData, wChallengeLength);
		ReadExact(&passphraseused, 1);

		bool bPasswordOK = false;
		bool bPassphraseRequired = false;
		bSuccess = false;
		int counter=0;
		do {
			bSuccess = m_pIntegratedPluginInterface->HandleChallenge(pChallengeData, wChallengeLength, nSequenceNumber, bPasswordOK, bPassphraseRequired);
			if (bSuccess && !bPasswordOK)
			{
				if (!bTriedNoPassword && strlen(passwd) > 0) {
					m_pIntegratedPluginInterface->SetPasswordData((const BYTE*)passwd, strlen(passwd));
					bTriedNoPassword = true;
				} 
			}
			counter++;
			if (counter>3) bSuccess=false;
		} while (bSuccess && !bPasswordOK && !bCancel);

		delete[] pChallengeData;

		if (bSuccess && !bCancel) {
			BYTE* pResponseData = NULL;
			int nResponseLength = 0;

			m_pIntegratedPluginInterface->GetResponse(pResponseData, nResponseLength, nSequenceNumber, bExpectChallenge);

			WORD wResponseLength = (WORD)nResponseLength;

			WriteExact((char*)&wResponseLength, sizeof(wResponseLength));

			WriteExact((char*)pResponseData, nResponseLength);

			if (m_pIntegratedPluginInterface) {
				m_pIntegratedPluginInterface->SetHandshakeComplete();
				if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_PLUGIN_STATUS,m_pIntegratedPluginInterface->DescribeCurrentSettings());
				}

			if (passphraseused!=2)
			{
				WORD lengt=0;

				AuthDialog ad;
					//adzm 2010-05-12 - passphrase
				ad.m_bPassphraseMode = (bool)passphraseused;
				bPassphraseRequired= (bool)passphraseused;
				if (strlen(passwd)>0)
				{
					//password was passed via commandline
					lengt=strlen(passwd);
				}
				else
				{
					if (ad.DoDialog(false,m_host,m_port))
						{
							strncpy(passwd, ad.m_passwd,254);
							if (!bPassphraseRequired && strlen(passwd) > 8) {
								passwd[8] = '\0';
							}
							lengt=strlen(passwd);												
						}
					else
						{
							bCancel = true; // cancel
						}	
				}


				WriteExact((char*)&lengt, sizeof(lengt));
				WriteExact((char*)passwd, lengt);
				vncEncryptPasswd(m_encPasswd, passwd);
			}

			m_pIntegratedPluginInterface->FreeMemory(pResponseData);
		}

		nSequenceNumber++;
	} while (bExpectChallenge && bSuccess);

	if (bCancel) {
		// cancelled
		if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,"Authentication cancelled");
		SetEvent(KillEvent);
		throw ErrorException("Authentication cancelled");
	} else if (!bSuccess) {
		// other failure
		if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,m_pIntegratedPluginInterface->GetLastErrorString());
		SetEvent(KillEvent);
		throw ErrorException(m_pIntegratedPluginInterface->GetLastErrorString());
	}
}


void ClientConnection::AuthSecureVNCPlugin_old()
{
	if (!m_pIntegratedPluginInterface) {
		if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,"SecureVNCPlugin authentication failed (SecureVNCPlugin interface available)");
		SetEvent(KillEvent);
		throw ErrorException("SecureVNCPlugin authentication failed (no plugin interface available)");
	}

	char passwd[256];
	passwd[0] = '\0';

	if (strlen(m_clearPasswd)>0)
	{
		strcpy(passwd, m_clearPasswd);
	}
	else if (strlen((const char *) m_encPasswd)>0)
	{  char * pw = vncDecryptPasswd(m_encPasswd);
		strcpy(passwd, pw);
		free(pw);
	}
	m_pIntegratedPluginInterface->SetPasswordData(NULL, 0);

	int nSequenceNumber = 0;
	bool bExpectChallenge = true;
	bool bTriedNoPassword = false;
	bool bSuccess = false;
	bool bCancel = false;
	do {
		WORD wChallengeLength = 0;

		ReadExact((char*)&wChallengeLength, sizeof(wChallengeLength));

		BYTE* pChallengeData = new BYTE[wChallengeLength];

		ReadExact((char*)pChallengeData, wChallengeLength);

		bool bPasswordOK = false;
		bool bPassphraseRequired = false;
		bSuccess = false;
		do {
			bSuccess = m_pIntegratedPluginInterface->HandleChallenge(pChallengeData, wChallengeLength, nSequenceNumber, bPasswordOK, bPassphraseRequired);
			if (bSuccess && !bPasswordOK)
			{
				if (!bTriedNoPassword && strlen(passwd) > 0) {
					m_pIntegratedPluginInterface->SetPasswordData((const BYTE*)passwd, strlen(passwd));
					bTriedNoPassword = true;
				} 
				else {
					bTriedNoPassword = true;
					if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,"WARNING: Server should be upgraded");
					Sleep(3000);
					if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,"Using the vncpasswd as encryption key");
					Sleep(3000);
					if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,"is not save.  Password can be hacked !!");
					AuthDialog ad;
					//adzm 2010-05-12 - passphrase
					ad.m_bPassphraseMode = bPassphraseRequired;

					if (ad.DoDialog(false,false,true))
					{
						strncpy(passwd, ad.m_passwd,254);
						if (!bPassphraseRequired && strlen(passwd) > 8) {
							passwd[8] = '\0';
						}

						m_pIntegratedPluginInterface->SetPasswordData((const BYTE*)passwd, strlen(passwd));
					}
					else
					{
						bCancel = true; // cancel
					}
				}
			}
		} while (bSuccess && !bPasswordOK && !bCancel);

		delete[] pChallengeData;

		if (bSuccess && !bCancel) {
			BYTE* pResponseData = NULL;
			int nResponseLength = 0;

			m_pIntegratedPluginInterface->GetResponse(pResponseData, nResponseLength, nSequenceNumber, bExpectChallenge);

			WORD wResponseLength = (WORD)nResponseLength;

			WriteExact((char*)&wResponseLength, sizeof(wResponseLength));

			WriteExact((char*)pResponseData, nResponseLength);

			if (m_pIntegratedPluginInterface) {
				if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_PLUGIN_STATUS,"Encryption initialized.");
			}

			m_pIntegratedPluginInterface->FreeMemory(pResponseData);
		}

		nSequenceNumber++;
	} while (bExpectChallenge && bSuccess);

	if (bCancel) {
		// cancelled
		if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,"Authentication cancelled");
		SetEvent(KillEvent);
		throw ErrorException("Authentication cancelled");
	} else if (!bSuccess) {
		// other failure
		if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,m_pIntegratedPluginInterface->GetLastErrorString());
		SetEvent(KillEvent);
		throw ErrorException(m_pIntegratedPluginInterface->GetLastErrorString());
	}
}

// marscha@2006: Try to better hide the windows password.
// I know that this is no breakthrough in modern cryptography.
// It's just a patch/kludge/workaround.
void ClientConnection::AuthMsLogonII()
{
	char gen[8], mod[8], pub[8], resp[8];
	char user[256], passwd[64];
	unsigned char key[8];

//act: clearPasswd must NOT be cleared, because of use of user command line
//line commented:	memset(m_clearPasswd, 0, sizeof(m_clearPasswd)); // ??

	ReadExact(gen, sizeof(gen));
	ReadExact(mod, sizeof(mod));
	ReadExact(resp, sizeof(resp));

	DH dh(bytesToInt64(gen), bytesToInt64(mod));
	int64ToBytes(dh.createInterKey(), pub);

	WriteExact(pub, sizeof(pub));

	int64ToBytes(dh.createEncryptionKey(bytesToInt64(resp)), (char*) key);
	vnclog.Print(100, _T("After DH: g=%I64u, m=%I64u, i=%I64u, key=%I64u\n"),
	  bytesToInt64(gen), bytesToInt64(mod), bytesToInt64(pub), bytesToInt64((char*) key));
	// get username and passwd
	// m_clearPasswd is not from commandline only, but also filled by dsm
	// Need to be both &&

	if ((strlen(m_cmdlnUser)>0)&& (strlen(	m_pApp->m_options.m_clearPassword) > 0) && !m_pApp->m_options.m_NoMoreCommandLineUserPassword)
    {
		vnclog.Print(0, _T("Command line MS-Logon.\n"));
#ifndef UNDER_CE
		strcpy(m_clearPasswd, m_pApp->m_options.m_clearPassword);
		strncpy(passwd, m_clearPasswd, 64);
		strncpy(user, m_cmdlnUser, 254);
		//strncpy(domain, ad.m_domain, 254);
#else
		vncWc2Mb(passwd, m_clearPasswd, 64);
		vncWc2Mb(user, m_cmdlnUser, 256);
		//vncWc2Mb(domain, ad.m_domain, 256);
#endif
		vncEncryptPasswdMs(m_encPasswdMs, passwd);
		strcpy(m_ms_user, user);
	}
	else if (strlen((const char *) m_encPasswdMs)>0)
	{  char * pw = vncDecryptPasswdMs(m_encPasswdMs);
	   strcpy(passwd, pw);
	   free(pw);
	   strcpy(user, m_ms_user);
	}
	else
	{
	AuthDialog ad;
	// adzm 2010-10 - RFB3.8 - the 'mslogon' param woudl always be true here
	if (ad.DoDialog(true, m_host, m_port, true)) {
#ifndef UNDER_CE
		strncpy(passwd, ad.m_passwd, 64);
		strncpy(user, ad.m_user, 254);
		//strncpy(domain, ad.m_domain, 254);
#else
		vncWc2Mb(passwd, ad.m_passwd, 64);
		vncWc2Mb(user, ad.m_user, 256);
		//vncWc2Mb(domain, ad.m_domain, 256);
#endif
		vncEncryptPasswdMs(m_encPasswdMs, passwd);
		strcpy(m_ms_user, user);
	} else {
		throw QuietException(sz_L54);
	}
	//user = domain + "\\" + user;
	}
	vncEncryptBytes2((unsigned char*) user, sizeof(user), key);
	vncEncryptBytes2((unsigned char*) passwd, sizeof(passwd), key);

	WriteExactQueue(user, sizeof(user));
	WriteExact(passwd, sizeof(passwd));
}

void ClientConnection::AuthMsLogonI()
{
	if (!m_ms_logon_I_legacy) {
		vnclog.Print(0, _T("AuthMsLogonI should not be called!\n"));
		throw WarningException("AuthMsLogonI should not be called!\n");
	}

    CARD8 challenge[CHALLENGESIZE];
	CARD8 challengems[CHALLENGESIZEMS];

	// rdv@2002 - v1.1.x
	char passwd[256];
	char domain[256];
	char user[256];

	memset(passwd, 0, sizeof(char)*256);
	memset(domain, 0, sizeof(char)*256);
	memset(user, 0, sizeof(char)*256);

	// We NOT ignore the clear password in case of ms_logon !
	// finally done !! : Add ms_user & ms_password command line params
	// act: add user option on command line
	if (m_ms_logon_I_legacy)
	{	// mslogon required
		// if user cmd line option is not specified, cmd line passwd must be cleared
		// the same if user is provided and not password
		if (strlen(m_cmdlnUser)>0)
		{	if (strlen(m_clearPasswd)>0)
			{  //user and password are not empty
			    strcpy(user, m_cmdlnUser);
				strcpy(passwd, m_clearPasswd);
			}
			else memset(m_cmdlnUser, 0, sizeof(m_cmdlnUser)); // user without password
		}
		else
			memset(m_clearPasswd, 0, sizeof(m_clearPasswd));
	}

	// Was the password already specified in a config file or entered for DSMPlugin ?
	// Modif sf@2002 - A clear password can be transmitted via the vncviewer command line
	if (strlen(m_clearPasswd)>0)
	{
		strcpy(passwd, m_clearPasswd);
	    if (m_ms_logon_I_legacy) strcpy(user, m_cmdlnUser);
	}
	else if (strlen((const char *) m_encPasswd)>0)
	{  char * pw = vncDecryptPasswd(m_encPasswd);
		strcpy(passwd, pw);
		free(pw);
	}
	else if (strlen((const char *) m_encPasswdMs)>0)
	{  char * pw = vncDecryptPasswdMs(m_encPasswdMs);
	   strcpy(passwd, pw);
	   free(pw);
	   strcpy(user, m_ms_user);
	}
	else
	{
		AuthDialog ad;
		///////////////ppppppppppppppppppppppppppppppppppppppppp // adzm 2010-10 - what?
		if (ad.DoDialog(true,m_host, m_port))
		{
//					flash = new BmpFlasher;
			strncpy(passwd, ad.m_passwd,254);
			strncpy(user, ad.m_user,254);
			strncpy(domain, ad.m_domain,254);
			if (strlen(user)==0 ||!m_ms_logon_I_legacy)//need longer passwd for ms
				{
					if (strlen(passwd) == 0) {
//								if (flash) {flash->Killflash();}
						vnclog.Print(0, _T("Password had zero length\n"));
						throw WarningException(sz_L53);
					}
					if (strlen(passwd) > 8) {
						passwd[8] = '\0';
					}
				}
			if (m_ms_logon_I_legacy)
			{
				vncEncryptPasswdMs(m_encPasswdMs, passwd);
				strcpy(m_ms_user, user);
			}
		}
		else
		{
//					if (flash) {flash->Killflash();}
			throw QuietException(sz_L54);
		}
	}

	// sf@2002
	// m_ms_logon = false;
	if (m_ms_logon_I_legacy) ReadExact((char *)challengems, CHALLENGESIZEMS);
	ReadExact((char *)challenge, CHALLENGESIZE);

	// MS logon
	if (m_ms_logon_I_legacy)
	{
		int i=0;
		for (i=0;i<32;i++)
		{
			challengems[i]=m_encPasswdMs[i]^challengems[i];
		}
		WriteExact((char *) user, sizeof(char)*256);
		WriteExact((char *) domain, sizeof(char)*256);
		WriteExact((char *) challengems, CHALLENGESIZEMS);
		vncEncryptBytes(challenge, passwd);

		/* Lose the plain-text password from memory */
		int nLen = (int)strlen(passwd);
		for ( i=0; i< nLen; i++) {
			passwd[i] = '\0';
		}

		WriteExact((char *) challenge, CHALLENGESIZE);
	}
}

void ClientConnection::AuthVnc()
{
	CARD8 challenge[CHALLENGESIZE];
	CARD32 authResult=rfbVncAuthFailed;
	if ((m_majorVersion == 3) && (m_minorVersion < 3))
	{
		/* if server is 3.2 we can't use the new authentication */
		vnclog.Print(0, _T("Can't use IDEA authentication\n"));
		MessageBox(m_hwndMain,sz_L51, sz_L52, MB_OK | MB_ICONSTOP | MB_SETFOREGROUND | MB_TOPMOST);
		throw WarningException("Can't use IDEA authentication any more!");
	}
	// rdv@2002 - v1.1.x
	char passwd[256];
	memset(passwd, 0, sizeof(char)*256);
	// Was the password already specified in a config file or entered for DSMPlugin ?
	// Modif sf@2002 - A clear password can be transmitted via the vncviewer command line
	if (strlen(m_clearPasswd)>0)
	{
		strcpy(passwd, m_clearPasswd);
	}
	else if (strlen((const char *) m_encPasswd)>0)
	{
		char * pw = vncDecryptPasswd(m_encPasswd);
		strcpy(passwd, pw);
		free(pw);
	}
	else
	{
		AuthDialog ad;
		if (ad.DoDialog(false, m_host, m_port))
		{
			strncpy(passwd, ad.m_passwd,254);
			if (strlen(passwd) == 0)
			{
				vnclog.Print(0, _T("Password had zero length\n"));
				throw WarningException(sz_L53);
			}
			if (strlen(passwd) > 8)
			{
				passwd[8] = '\0';
			}
			vncEncryptPasswd(m_encPasswd, passwd);
		}
		else
		{
			throw QuietException(sz_L54);
		}
	}
	ReadExact((char *)challenge, CHALLENGESIZE);
	vncEncryptBytes(challenge, passwd);
	/* Lose the plain-text password from memory */
	int nLen = (int)strlen(passwd);
	for (int i=0; i< nLen; i++)
	{
		passwd[i] = '\0';
	}
	WriteExact((char *) challenge, CHALLENGESIZE);
}

void ClientConnection::AuthSCPrompt()
{
	if (m_minorVersion < 7) {
		vnclog.Print(0, _T("Invalid auth continue response for protocol version.\n"));
		throw ErrorException("Invalid auth continue response");
	}
	/// SHOW Messagebox
	int size;
	ReadExact((char *)&size,sizeof(int));
	char mytext[1025]; //10k
	//block
	if (size<0 || size >1024)
	{
		throw WarningException("Buffer too big, ");
		if (size<0) size=0;
		if (size>1024) size=1024;
	}

	ReadExact(mytext,size);
	mytext[size]=0;

	//adzm 2009-06-21 - auto-accept if specified
	int accepted = 0;
	if (!m_opts.m_fAutoAcceptIncoming) {
		int returnvalue=MessageBox(m_hwndMain,   mytext,"Accept Incoming SC Connection", MB_YESNO |  MB_TOPMOST);
		if (returnvalue != IDNO)
		{
			accepted = 1;
		}
	} else {
		accepted = 1;
	}

	WriteExact((char *)&accepted,sizeof(accepted));
}

void ClientConnection::AuthSessionSelect()
{
	if (m_minorVersion < 7) {
		vnclog.Print(0, _T("Invalid auth continue response for protocol version.\n"));
		throw ErrorException("Invalid auth continue response");
	}

	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwICC = ICC_LISTVIEW_CLASSES|ICC_INTERNET_CLASSES;
	InitCtrls.dwSize = sizeof(INITCOMMONCONTROLSEX);
	BOOL bRet = InitCommonControlsEx(&InitCtrls);
	int tt=DialogBoxParam(m_pApp->m_instance, MAKEINTRESOURCE(IDD_SESSIONSELECTOR), NULL, (DLGPROC)DialogProc,(LONG_PTR)this);
	WriteExact((char *)&tt,sizeof(int));
}

void ClientConnection::SendClientInit()
{
    rfbClientInitMsg ci;
	// adzm 2010-09
	ci.flags = 0;
	if (m_opts.m_Shared) {
		ci.flags |= clientInitShared;
	}

    WriteExact((char *)&ci, sz_rfbClientInitMsg); // sf@2002 - RSM Plugin
}

void ClientConnection::ReadServerInit()
{
    ReadExact((char *)&m_si, sz_rfbServerInitMsg);

    m_si.framebufferWidth = Swap16IfLE(m_si.framebufferWidth);
    m_si.framebufferHeight = Swap16IfLE(m_si.framebufferHeight);
    m_si.format.redMax = Swap16IfLE(m_si.format.redMax);
    m_si.format.greenMax = Swap16IfLE(m_si.format.greenMax);
    m_si.format.blueMax = Swap16IfLE(m_si.format.blueMax);
    m_si.nameLength = Swap32IfLE(m_si.nameLength);

    m_desktopName = new TCHAR[m_si.nameLength + 4 + 256];
	m_desktopName_viewonly = new TCHAR[m_si.nameLength + 4 + 256+16];

#ifdef UNDER_CE
    char *deskNameBuf = new char[m_si.nameLength + 4];

	ReadString(deskNameBuf, m_si.nameLength);

	MultiByteToWideChar( CP_ACP,   MB_PRECOMPOSED,
			     deskNameBuf, m_si.nameLength,
			     m_desktopName, m_si.nameLength+1);
    delete deskNameBuf;
#else
    ReadString(m_desktopName, m_si.nameLength);
#endif
    // TCHAR tcDummy [MAX_PATH * 3];

	// sprintf(tcDummy,"%s ",m_desktopName);
	strcat(m_desktopName, " ");

	strcpy(m_desktopName_viewonly,m_desktopName);
	strcat(m_desktopName_viewonly,"viewonly");

	if (m_opts.m_ViewOnly) SetWindowText(m_hwndMain, m_desktopName_viewonly);
	else SetWindowText(m_hwndMain, m_desktopName);

	vnclog.Print(0, _T("Desktop name \"%s\"\n"),m_desktopName);
	vnclog.Print(1, _T("Geometry %d x %d depth %d\n"),
		m_si.framebufferWidth, m_si.framebufferHeight, m_si.format.depth );

	//SetWindowText(m_hwndMain, m_desktopName);
	//adzm 2009-06-21 - if we decide to connect even though it is unencrypted, do not show the plugin info
	if (m_pDSMPlugin->IsEnabled() && m_fUsePlugin)
	{
			char szMess[255];
			memset(szMess, 0, 255);
			sprintf(szMess, "--- UltraVNC Viewer + %s-v%s by %s ",
					m_pDSMPlugin->GetPluginName(),
					m_pDSMPlugin->GetPluginVersion(),
					m_pDSMPlugin->GetPluginAuthor()
					);
			strcat(m_desktopName, szMess);
	}
	strcpy(m_desktopName_viewonly,m_desktopName);
	strcat(m_desktopName_viewonly,"viewonly");

	if (m_opts.m_ViewOnly) SetWindowText(m_hwndMain, m_desktopName_viewonly);
	else SetWindowText(m_hwndMain, m_desktopName);
	SizeWindow();
}

void ClientConnection::SizeWindow()
{
	// Find how large the desktop work area is
	RECT workrect;
	tempdisplayclass tdc;
	tdc.Init();
	workrect.left=tdc.monarray[m_opts.m_selected_screen].wl;
	workrect.right=tdc.monarray[m_opts.m_selected_screen].wr;
	workrect.top=tdc.monarray[m_opts.m_selected_screen].wt;
	workrect.bottom=tdc.monarray[m_opts.m_selected_screen].wb;
	//SystemParametersInfo(SPI_GETWORKAREA, 0, &workrect, 0);
	int workwidth = workrect.right -  workrect.left;
	int workheight = workrect.bottom - workrect.top;
	vnclog.Print(2, _T("Screen work area is %d x %d\n"), workwidth, workheight);

	int widthwindow,heightwindow;

	// sf@2003 - AutoScaling
	if (m_opts.m_fAutoScaling && !m_fScalingDone)
	{
		// We save the scales values coming from options
		m_opts.m_saved_scale_num = m_opts.m_scale_num;
		m_opts.m_saved_scale_den = m_opts.m_scale_den;
		m_opts.m_saved_scaling = m_opts.m_scaling;

		RECT myrect,myclrect;
		GetWindowRect(m_hwndMain,&myrect);
		GetClientRect(m_hwndMain,&myclrect);
		int dx=(myrect.right-myrect.left)-(myclrect.right-myclrect.left);
		int dy=(myrect.bottom-myrect.top)-(myclrect.bottom-myclrect.top);

		//case one, does the scaling fit the window include borders?
		int clientwidth=m_si.framebufferWidth*m_opts.m_scale_den/m_opts.m_scale_num;
		int clientheight=m_si.framebufferHeight*m_opts.m_scale_den/m_opts.m_scale_num;
		{
			// we change the scaling to fit the window
			// max windows size including borders etc..
			int horizontalRatio= (int) (((workwidth-dx)*100)/m_si.framebufferWidth);
			int verticalRatio = (int) (((workheight-dy*2)*100)/m_si.framebufferHeight);
			int Ratio= min(verticalRatio, horizontalRatio);
			widthwindow=m_si.framebufferWidth*Ratio/100;
			heightwindow=m_si.framebufferHeight*Ratio/100;
			m_opts.m_scale_num =Ratio;
			m_opts.m_scale_den = 100;
			m_opts.m_scaling = true;
			m_fScalingDone = true;
		}
	}

	if (!m_opts.m_fAutoScaling && m_fScalingDone)
	{
		// Restore scale values to the original options values
		m_opts.m_scale_num = m_opts.m_saved_scale_num;
		m_opts.m_scale_den = m_opts.m_saved_scale_den;
		m_opts.m_scaling = m_opts.m_saved_scaling;
		m_fScalingDone = false;
	}

	// Size the window.
	// Let's find out how big a window would be needed to display the
	// whole desktop (assuming no scrollbars).

	RECT fullwinrect;

	if (m_opts.m_scaling)
		SetRect(&fullwinrect, 0, 0,
				m_si.framebufferWidth * m_opts.m_scale_num / m_opts.m_scale_den,
				m_si.framebufferHeight * m_opts.m_scale_num / m_opts.m_scale_den);
	else
		SetRect(&fullwinrect, 0, 0, m_si.framebufferWidth, m_si.framebufferHeight);

	AdjustWindowRectEx(&fullwinrect,
			   GetWindowLong(m_hwndcn, GWL_STYLE) & ~WS_VSCROLL & ~WS_HSCROLL,
			   FALSE, GetWindowLong(m_hwndcn, GWL_EXSTYLE));

	m_fullwinwidth = fullwinrect.right - fullwinrect.left;
	m_fullwinheight = (fullwinrect.bottom - fullwinrect.top);

	m_winwidth  = min(m_fullwinwidth,  workwidth);
	m_winheight = min(m_fullwinheight, workheight);

	//SetWindowPos(m_hwnd, HWND_TOP,
	if (m_opts.m_ShowToolbar)
		SetWindowPos(m_hwndcn, m_hwndTBwin, 0, m_TBr.bottom, m_winwidth, m_winheight, SWP_SHOWWINDOW);
	else
	{
		SetWindowPos(m_hwndcn, m_hwndTBwin, 0, 0, m_winwidth, m_winheight, SWP_SHOWWINDOW);
		SetWindowPos(m_hwndTBwin, NULL ,0, 0, 0, 0, SWP_HIDEWINDOW);
	}

   // Hauptfenster positionieren
	AdjustWindowRectEx(&fullwinrect,
					   GetWindowLong(m_hwndMain, GWL_STYLE) & ~WS_VSCROLL & ~WS_HSCROLL,
					   FALSE, GetWindowLong(m_hwndMain, GWL_EXSTYLE));

	m_fullwinwidth = fullwinrect.right - fullwinrect.left;
	m_fullwinheight = (fullwinrect.bottom - fullwinrect.top);

	//m_winwidth  = min(m_fullwinwidth+16,  workwidth);
	m_winwidth  = min(m_fullwinwidth,  workwidth);
	//m_winheight = min(m_fullwinheight+m_TBr.bottom + m_TBr.top+16 , workheight);
	if (m_opts.m_ShowToolbar)
		m_winheight = min(m_fullwinheight + m_TBr.bottom + m_TBr.top , workheight);
	else
		m_winheight = min(m_fullwinheight, workheight);
	int aa=GetSystemMetrics(SM_CXBORDER)+GetSystemMetrics(SM_CXHSCROLL);
	int bb=tdc.monarray[1].wr-tdc.monarray[1].wl+aa;
	if (m_opts.m_selected_screen==0 && (m_fullwinwidth <= bb )) //fit on primary
		// -20 for border
	{
		SetWindowPos(m_hwndMain, HWND_TOP,
				tdc.monarray[1].wl + ((tdc.monarray[1].wr-tdc.monarray[1].wl)-m_winwidth) / 2,
				tdc.monarray[1].wt + ((tdc.monarray[1].wb-tdc.monarray[1].wt)-m_winheight) / 2,
				m_winwidth, m_winheight, SWP_SHOWWINDOW);
	}
	else
	{
	SetWindowPos(m_hwndMain, HWND_TOP,
				workrect.left + (workwidth-m_winwidth) / 2,
				workrect.top + (workheight-m_winheight) / 2,
				m_winwidth, m_winheight, SWP_SHOWWINDOW);
	}

	SetForegroundWindow(m_hwndMain);

	if (m_opts.m_ShowToolbar)
		MoveWindow(m_hwndTBwin, 0, 0, workwidth, m_TBr.bottom - m_TBr.top, TRUE);

	if (m_opts.m_ShowToolbar)
		MoveWindow(m_hwndTB, 0, 0, m_winwidth-200, m_TBr.bottom - m_TBr.top, TRUE);

	if (m_opts.m_ShowToolbar)
		ShowWindow(m_hwndTB, SW_SHOW);
	else
		ShowWindow(m_hwndTB, SW_HIDE);

	if (m_opts.m_ShowToolbar)
		ShowWindow(m_hwndTBwin, SW_SHOW);
	else
		ShowWindow(m_hwndTB, SW_HIDE);
}

// We keep a local copy of the whole screen.  This is not strictly necessary
// for VNC, but makes scrolling & deiconifying much smoother.

void ClientConnection::CreateLocalFramebuffer()
{
	omni_mutex_lock l(m_bitmapdcMutex);
}

void ClientConnection::SetupPixelFormat() {
	// Have we requested a reduction to 8-bit?
    if (m_opts.m_Use8Bit)
	{
		switch (m_opts.m_Use8Bit)
		{
		case rfbPF256Colors:
			m_myFormat = vnc8bitFormat;
			break;
		case rfbPF64Colors:
			m_myFormat = vnc8bitFormat_64;
			break;
		case rfbPF8Colors:
			m_myFormat = vnc8bitFormat_8;
			break;
		case rfbPF8GreyColors:
			m_myFormat = vnc8bitFormat_8Grey;
			break;
		case rfbPF4GreyColors:
			m_myFormat = vnc8bitFormat_4Grey;
			break;
		case rfbPF2GreyColors:
			m_myFormat = vnc8bitFormat_2Grey;
			break;
		}
		vnclog.Print(2, _T("Requesting 8-bit truecolour\n"));
		// We don't support colormaps so we'll ask the server to convert
    }
	else if (!m_si.format.trueColour)
	{
        // We'll just request a standard 16-bit truecolor
        vnclog.Print(2, _T("Requesting 16-bit truecolour\n"));
        m_myFormat = vnc16bitFormat;
    }
	else
	{
		// Normally we just use the sever's format suggestion
		m_myFormat = m_si.format;
        m_myFormat.bigEndian = 0; // except always little endian

		// It's silly requesting more bits than our current display has, but
		// in fact it doesn't usually amount to much on the network.
		// Windows doesn't support 8-bit truecolour.
		// If our display is palette-based, we want more than 8 bit anyway,
		// unless we're going to start doing palette stuff at the server.
		// So the main use would be a 24-bit true-colour desktop being viewed
		// on a 16-bit true-colour display, and unless you have lots of images
		// and hence lots of raw-encoded stuff, the size of the pixel is not
		// going to make much difference.
		//   We therefore don't bother with any restrictions, but here's the
		// start of the code if we wanted to do it.

		if (false) {
			// Get a DC for the root window
			TempDC hrootdc(NULL);
			int localBitsPerPixel = GetDeviceCaps(hrootdc, BITSPIXEL);
			int localRasterCaps	  = GetDeviceCaps(hrootdc, RASTERCAPS);
			vnclog.Print(2, _T("Memory DC has depth of %d and %s pallete-based.\n"),
				localBitsPerPixel, (localRasterCaps & RC_PALETTE) ? "is" : "is not");

			// If we're using truecolor, and the server has more bits than we do
			if ( (localBitsPerPixel > m_myFormat.depth) &&
				! (localRasterCaps & RC_PALETTE)) {
				m_myFormat.depth = localBitsPerPixel;

				// create a bitmap compatible with the current display
				// call GetDIBits twice to get the colour info.
				// set colour masks and shifts
			}
		}
	}

	// The endian will be set before sending
}

void ClientConnection::SetFormatAndEncodings()
{
	// Set pixel format to myFormat

	rfbSetPixelFormatMsg spf;

    spf.type = rfbSetPixelFormat;
    spf.format = m_myFormat;
    spf.format.redMax = Swap16IfLE(spf.format.redMax);
    spf.format.greenMax = Swap16IfLE(spf.format.greenMax);
    spf.format.blueMax = Swap16IfLE(spf.format.blueMax);

    WriteExactQueue((char *)&spf, sz_rfbSetPixelFormatMsg, rfbSetPixelFormat);

    // The number of bytes required to hold at least one pixel.
	m_minPixelBytes = (m_myFormat.bitsPerPixel + 7) >> 3;

	// Set encodings
    char buf[sz_rfbSetEncodingsMsg + MAX_ENCODINGS * 4];
	int aa = sz_rfbSetEncodingsMsg + MAX_ENCODINGS * 4;
    rfbSetEncodingsMsg *se = (rfbSetEncodingsMsg *)buf;
    CARD32 *encs = (CARD32 *)(&buf[sz_rfbSetEncodingsMsg]);
    int len = 0;

    se->type = rfbSetEncodings;
    se->nEncodings = 0;

	bool useCompressLevel = false;
	int i = 0;
	//no u2 supported, even when it was selected
	//
	if (!new_ultra_server)
	{
		for (std::vector<int>::iterator it = m_opts.m_PreferredEncodings.begin(); it != m_opts.m_PreferredEncodings.end(); ++it) {
			if (*it == rfbEncodingUltra2) {
				*it = rfbEncodingZRLE;
			}
		}
	}
	// Put the preferred encoding first, and change it if the
	// preferred encoding is not actually usable.
	std::vector<int> preferred_encodings = m_opts.m_PreferredEncodings;

	if (preferred_encodings.end() != std::find(preferred_encodings.begin(), preferred_encodings.end(), rfbEncodingZYWRLE)) {
		zywrle = 1;
	} else {
		zywrle = 0;
	}
#ifdef _XZ
	if (preferred_encodings.end() != std::find(preferred_encodings.begin(), preferred_encodings.end(), rfbEncodingXZYW)) {
		xzyw = 1;
	} else {
		xzyw = 0;
	}
#endif


	// Now we go through and put in all the other encodings in order.
	// We do rather assume that the most recent encoding is the most
	// desirable!
	for (i = LASTENCODING; i >= rfbEncodingRaw; i--)
	{
		if (m_opts.m_UseEnc[i] && preferred_encodings.end() == std::find(preferred_encodings.begin(), preferred_encodings.end(), i)) {
			preferred_encodings.push_back(i);
		}
	}
	
	for (std::vector<int>::iterator it = preferred_encodings.begin(); it != preferred_encodings.end(); it++) {
		if (*it == rfbEncodingZlib ||
			*it == rfbEncodingTight ||
			*it == rfbEncodingZlibHex 
#ifdef _XZ
			||*it == rfbEncodingXZ ||
			*it == rfbEncodingXZYW
#endif
			)
		{
			useCompressLevel = true;
		}

		encs[se->nEncodings++] = Swap32IfLE(*it);
	}

	// Tight - Request desired compression level if applicable
	if ( useCompressLevel && m_opts.m_useCompressLevel &&
		 m_opts.m_compressLevel >= 0 &&
		 m_opts.m_compressLevel <= 9) {
		encs[se->nEncodings++] = Swap32IfLE( rfbEncodingCompressLevel0 +
											 m_opts.m_compressLevel );
	}

	// Tight - Request cursor shape updates if enabled by user
	if (m_opts.m_requestShapeUpdates) {
		//encs[se->nEncodings++] = Swap32IfLE(rfbEncodingXCursor);
		encs[se->nEncodings++] = Swap32IfLE(rfbEncodingRichCursor);
		if (!m_opts.m_ignoreShapeUpdates)
			encs[se->nEncodings++] = Swap32IfLE(rfbEncodingPointerPos); // marscha PointerPos
	}

	// Tight - Request JPEG quality level if JPEG compression was enabled by user
	if ( m_opts.m_enableJpegCompression &&
		 m_opts.m_jpegQualityLevel >= 0 &&
		 m_opts.m_jpegQualityLevel <= 9) {
		encs[se->nEncodings++] = Swap32IfLE( rfbEncodingQualityLevel0 +
											 m_opts.m_jpegQualityLevel );
	}

    // Modif rdv@2002
	//Tell the server that we support the special Zlibencoding
	encs[se->nEncodings++] = Swap32IfLE(rfbEncodingXOREnable);

	// Tight - LastRect - SINGLE WINDOW
	encs[se->nEncodings++] = Swap32IfLE(rfbEncodingLastRect);
	encs[se->nEncodings++] = Swap32IfLE(rfbEncodingNewFBSize);

	// Modif sf@2002
	if (m_opts.m_fEnableCache)
	{
		encs[se->nEncodings++] = Swap32IfLE(rfbEncodingCacheEnable);
		// vnclog.Print(0, _T("Cache: Enable Cache sent to Server\n"));
	}

    // len = sz_rfbSetEncodingsMsg + se->nEncodings * 4;

    encs[se->nEncodings++] = Swap32IfLE(rfbEncodingServerState);
    encs[se->nEncodings++] = Swap32IfLE(rfbEncodingEnableKeepAlive);
	encs[se->nEncodings++] = Swap32IfLE(rfbEncodingEnableIdleTime);
    encs[se->nEncodings++] = Swap32IfLE(rfbEncodingFTProtocolVersion);
	encs[se->nEncodings++] = Swap32IfLE(rfbEncodingpseudoSession);

	// adzm - 2010-07 - Extended clipboard
	encs[se->nEncodings++] = Swap32IfLE(rfbEncodingExtendedClipboard);
	// all multithreaded versions of the plugins support streaming
	if (m_fUsePlugin && m_pDSMPlugin && m_pDSMPlugin->IsEnabled() && m_pDSMPlugin->SupportsMultithreaded()) {
		encs[se->nEncodings++] = Swap32IfLE(rfbEncodingPluginStreaming);
	}

#ifdef _Gii
	if (m_opts.m_giienable)
		encs[se->nEncodings++] = Swap32IfLE(rfbEncodingGII);
#endif


    // sf@2002 - DSM Plugin
	int nEncodings = se->nEncodings;
	se->nEncodings = Swap16IfLE(se->nEncodings);
	// WriteExact((char *)buf, len);
	int bb = sz_rfbSetEncodingsMsg + sizeof(CARD32) * nEncodings;
	WriteExact((char *)buf, sz_rfbSetEncodingsMsg + sizeof(CARD32) * nEncodings, rfbSetEncodings);
}
void ClientConnection::Createdib()
{
	omni_mutex_lock l(m_bitmapdcMutex);
	TempDC hdc(m_hwndcn);
	BitmapInfo bi;
	UINT iUsage;
    memset(&bi, 0, sizeof(bi));

	iUsage = m_myFormat.trueColour ? DIB_RGB_COLORS : DIB_PAL_COLORS;
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biBitCount = m_myFormat.bitsPerPixel;
    bi.bmiHeader.biSizeImage = (m_myFormat.bitsPerPixel / 8) * m_si.framebufferWidth * m_si.framebufferHeight;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biWidth = m_si.framebufferWidth;
    bi.bmiHeader.biHeight = -m_si.framebufferHeight;
    bi.bmiHeader.biCompression = (m_myFormat.bitsPerPixel > 8) ? BI_BITFIELDS : BI_RGB;
    bi.mask.red = m_myFormat.redMax << m_myFormat.redShift;
    bi.mask.green = m_myFormat.greenMax << m_myFormat.greenShift;
    bi.mask.blue = m_myFormat.blueMax << m_myFormat.blueShift;

	if (directx_used)
		{
			directx_output.DestroyD3D();
			directx_used=false;
		}
	if (m_hmemdc != NULL) {DeleteDC(m_hmemdc);m_hmemdc = NULL;m_DIBbits=NULL;}
	if (m_membitmap != NULL) {DeleteObject(m_membitmap);m_membitmap= NULL;}
	m_hmemdc = CreateCompatibleDC(m_hBitmapDC);
	m_membitmap = CreateDIBSection(m_hmemdc, (BITMAPINFO*)&bi.bmiHeader, iUsage, &m_DIBbits, NULL, 0);
	memset((char*)m_DIBbits,128,bi.bmiHeader.biSizeImage);

	ObjectSelector bb(m_hmemdc, m_membitmap);

	if (m_myFormat.bitsPerPixel==8 && m_myFormat.trueColour)
	{
		struct Colour {
		int r, g, b;
		};
		Colour rgbQ[256];
		 for (int i=0; i < (1<<(m_myFormat.depth)); i++) {
			rgbQ[i].b = ((((i >> m_myFormat.blueShift) & m_myFormat.blueMax) * 65535) + m_myFormat.blueMax/2) / m_myFormat.blueMax;
			rgbQ[i].g = ((((i >> m_myFormat.greenShift) & m_myFormat.greenMax) * 65535) + m_myFormat.greenMax/2) / m_myFormat.greenMax;
			rgbQ[i].r = ((((i >> m_myFormat.redShift) & m_myFormat.redMax) * 65535) + m_myFormat.redMax/2) / m_myFormat.redMax;
		 }

	for (int ii=0; ii<256; ii++)
	{
		bi.color[ii].rgbRed      = rgbQ[ii].r >> 8;
		bi.color[ii].rgbGreen    = rgbQ[ii].g >> 8;
		bi.color[ii].rgbBlue     = rgbQ[ii].b >> 8;
		bi.color[ii].rgbReserved = 0;
	}
	SetDIBColorTable(m_hmemdc, 0, 256, bi.color);
	}
	if (m_opts.m_fEnableCache)
	{
		if (m_DIBbitsCache != NULL) delete [] m_DIBbitsCache;
		int Pitch=m_si.framebufferWidth*m_myFormat.bitsPerPixel/8;
		if (Pitch % 4)Pitch += 4 - Pitch % 4;

		m_DIBbitsCache= new BYTE[Pitch*m_si.framebufferHeight];
		vnclog.Print(0, _T("Cache: Cache buffer bitmap creation\n"));
	}
	if (m_opts.m_Directx && (m_myFormat.bitsPerPixel==32 || m_myFormat.bitsPerPixel==16))
	if (!FAILED(directx_output.InitD3D(m_hwndcn,m_hwndMain, m_si.framebufferWidth, m_si.framebufferHeight, false,m_myFormat.bitsPerPixel,m_myFormat.redShift)))
			{
				if (directx_output.m_directxformat.bitsPerPixel ==m_myFormat.bitsPerPixel)
					{
						directx_used=true;
						m_myFormat.redShift=(CARD8)directx_output.m_directxformat.redShift;
						m_myFormat.greenShift=(CARD8)directx_output.m_directxformat.greenShift;
						m_myFormat.blueShift=(CARD8)directx_output.m_directxformat.blueShift;

						if (m_hmemdc != NULL) {DeleteDC(m_hmemdc);m_hmemdc = NULL;m_DIBbits=NULL;}
						if (m_membitmap != NULL) {DeleteObject(m_membitmap);m_membitmap= NULL;}
					}
				else
					{
						directx_output.DestroyD3D();
						directx_used=false;
					}
				//TEST WITHOUT DIRECTX
				//directx_output.DestroyD3D();
				//directx_used = false;
			}
}
// Closing down the connection.
// Close the socket, kill the thread.
void ClientConnection::KillThread()
{
	m_bKillThread = true;
	m_running = false;

	if (m_sock != INVALID_SOCKET) {
		fis->Update_socket();
		shutdown(m_sock, SD_BOTH);
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		//adzm 2010-09
		m_nQueueBufferLength = 0;
	}
	WaitForSingleObject(KillEvent, 6000);
}

// sf@2007 - AutoReconnect
// When we want to autoreconnect we don't really kill the working thread
// and we keep intact most of the current clientconnection data
// We close the socket and suspend the thread for a while (socket polling)
// We also need to cleanup some parts, especially those dealing with data streams
void ClientConnection::SuspendThread()
{
	m_bKillThread = true;
	m_running = false;

	// Reinit encoders stuff
	delete(zis);
	zis = new rdr::ZlibInStream;

#ifdef _XZ
	delete(xzis);
	xzis = new rdr::xzInStream;
#endif
	for (int i = 0; i < 4; i++)
		m_tightZlibStreamActive[i] = false;

	// Reinit DSM stuff
	m_nTO = 1;
	LoadDSMPlugin(true);
	// WHat is this doing here ???
	// m_fUseProxy = false;  << repeater block after reconnect+	

	delete[] m_pNetRectBuf;
	m_pNetRectBuf = NULL;
	m_fReadFromNetRectBuf = false;
	m_nNetRectBufOffset = 0;
	m_nReadSize = 0;
	m_nNetRectBufSize = 0;
	delete[] m_pZRLENetRectBuf;
	m_pZRLENetRectBuf = NULL;
	m_fReadFromZRLENetRectBuf = false;
	m_nZRLENetRectBufOffset = 0;
	m_nZRLEReadSize = 0;
	m_nZRLENetRectBufSize = 0;
#ifdef _XZ
	delete[] m_pXZNetRectBuf;
	m_pXZNetRectBuf = NULL;
	m_fReadFromXZNetRectBuf = false;
	m_nXZNetRectBufOffset = 0;
	m_nXZReadSize = 0;
	m_nXZNetRectBufSize = 0;
#endif

	if (m_sock != INVALID_SOCKET)
	{
		shutdown(m_sock, SD_BOTH);
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		//adzm 2010-09
		m_nQueueBufferLength = 0;
	}
	m_fPluginStreamingIn = false;
	m_fPluginStreamingOut = false;
}
void
ClientConnection::CloseWindows()
{
	SetEvent(KillEvent);
	if (m_hwndcn)SendMessage(m_hwndcn, WM_CLOSE, 0, 1);
	SetEvent(KillEvent);
	if (m_hwndMain) SendMessage(m_hwndMain, WM_CLOSE, 0, 1);
}
ClientConnection::~ClientConnection()
{	
	if (m_hwndStatus)
		EndDialog(m_hwndStatus,0);
	Sleep(500);
	if (m_pNetRectBuf != NULL)
		delete [] m_pNetRectBuf;
	LowLevelHook::Release();

	// Modif sf@2002 - FileTransfer
	if (m_pFileTransfer)
		delete(m_pFileTransfer);

	// Modif sf@2002 - Text Chat
	if (m_pTextChat)
		delete(m_pTextChat);

	//adzm - 2009-06-21
	if (m_pPluginInterface) {
		delete m_pPluginInterface;
		m_pPluginInterface = NULL;
		//adzm 2010-05-10
		m_pIntegratedPluginInterface = NULL;
	}

	// Modif sf@2002 - DSMPlugin handling
	if (m_pDSMPlugin != NULL)
		delete(m_pDSMPlugin);

    if (zis)
      delete zis;
#ifdef _XZ
	if (xzis)
		delete(xzis);
#endif
    if (fis)
      delete fis;

	if (m_pZRLENetRectBuf != NULL)
		delete [] m_pZRLENetRectBuf;
#ifdef _XZ
	if (m_pXZNetRectBuf != NULL)
		delete [] m_pXZNetRectBuf;
#endif


	if (m_sock != INVALID_SOCKET) {
		shutdown(m_sock, SD_BOTH);
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		//adzm 2010-09
		m_nQueueBufferLength = 0;
	}

	if (m_desktopName != NULL) delete [] m_desktopName;
	if (m_desktopName_viewonly != NULL) delete [] m_desktopName_viewonly;
	delete [] m_netbuf;

	if (m_DIBbitsCache!=NULL) delete []m_DIBbitsCache;
	m_DIBbitsCache=NULL;

	if (m_hBitmapDC != NULL)
		DeleteDC(m_hBitmapDC);
	if (m_hBitmapDC != NULL)
		DeleteObject(m_hBitmapDC);
//	if (m_hBitmap != NULL)
//		DeleteObject(m_hBitmap);

	if (m_hPalette != NULL)
		DeleteObject(m_hPalette);

	if (directx_used)
		{
			directx_used=false;
			directx_output.DestroyD3D();
		}

	if (m_hmemdc != NULL) {DeleteDC(m_hmemdc);m_hmemdc = NULL;m_DIBbits=NULL;}
	if (m_membitmap != NULL) {DeleteObject(m_membitmap);m_membitmap = NULL;}
//	if (flash) delete flash;
	m_pApp->DeregisterConnection(this);
	if (m_zipbuf!=NULL)
		delete [] m_zipbuf;
	if (m_filezipbuf!=NULL)
		delete [] m_filezipbuf;
	if (m_filechunkbuf!=NULL)
		delete [] m_filechunkbuf;
	if (m_zlibbuf!=NULL)
		delete [] m_zlibbuf;
	if (m_hwndTBwin!= 0)
		DestroyWindow(m_hwndTBwin);
	if (rcSource!=NULL)
		delete[] rcSource;
	if (rcMask!=NULL)
		delete[] rcMask;
	if (KillEvent) CloseHandle(KillEvent);
	if (ThreadSocketTimeout)
	{
	havetobekilled=false; //force SocketTimeout thread to quit
	WaitForSingleObject(ThreadSocketTimeout,5000);
	CloseHandle(ThreadSocketTimeout);
	}
	if (m_statusThread) CloseHandle(m_statusThread);
	if (m_SavedAreaBIB) delete [] m_SavedAreaBIB;
	m_SavedAreaBIB=NULL;
	if (m_keymap) {
        delete m_keymap;
        m_keymap = NULL;
    }
	if (m_keymapJap) {
        delete m_keymapJap;
        m_keymapJap = NULL;
    }
#ifdef _Gii
	if (mytouch)
	{
		delete mytouch;
		mytouch = NULL;
	}
#endif
}

// You can specify a dx & dy outside the limits; the return value will
// tell you whether it actually scrolled.
bool ClientConnection::ScrollScreen(int dx, int dy)
{
	dx = max(dx, -m_hScrollPos);
	dx = min(dx, m_hScrollMax-(m_cliwidth)-m_hScrollPos);
	dy = max(dy, -m_vScrollPos);
	dy = min(dy, m_vScrollMax-(m_cliheight)-m_vScrollPos);
	if (dx || dy) {
		m_hScrollPos += dx;
		m_vScrollPos += dy;
		RECT clirect;
		RECT Rtb;
		GetClientRect(m_hwndMain, &clirect);
		if (m_opts.m_ShowToolbar)
			GetClientRect(m_hwndTBwin, &Rtb);
		else
			{
				Rtb.top=0;
				Rtb.bottom=0;
			}

		clirect.top += Rtb.top;
		clirect.bottom += Rtb.bottom;
		ScrollWindowEx(m_hwndcn, -dx, -dy, NULL, &clirect, NULL, NULL,  SW_INVALIDATE);
		UpdateScrollbars();
		UpdateWindow(m_hwndcn);

		return true;
	}
	return false;
}

// ProcessPointerEvent handles the delicate case of emulating 3 buttons
// on a two button mouse, then passes events off to SubProcessPointerEvent.
inline bool ClientConnection::ProcessPointerEvent(int x, int y, DWORD keyflags, UINT msg)
{
	//adzm 2010-09 - Throttle mousemove events
	if (msg == WM_MOUSEMOVE) {
		bool bMouseKeyDown = (keyflags & (MK_LBUTTON|MK_MBUTTON|MK_RBUTTON|MK_XBUTTON1|MK_XBUTTON2)) != 0;
		if (m_PendingMouseMove.ShouldThrottle(bMouseKeyDown)) {
			m_PendingMouseMove.x = x;
			m_PendingMouseMove.y = y;
			m_PendingMouseMove.keyflags = keyflags;
			m_PendingMouseMove.bValid = true;

			m_flushMouseMoveTimer = SetTimer(m_hwndcn, IDT_FLUSHMOUSEMOVETIMER, m_PendingMouseMove.dwMinimumMouseMoveInterval, NULL);

			// this is the only time we will return false!
			return false;
		} else {
			m_PendingMouseMove.bValid = false;
			m_PendingMouseMove.dwLastSentMouseMove = GetTickCount();
		}
	} else {
		//adzm 2010-09 - If we are sending an input, ensure the mouse is moved to the last known spot before sending
		//the input message
		FlushThrottledMouseMove();
	}
	if (m_opts.m_Emul3Buttons) {
		// XXX To be done:
		// If this is a left or right press, the user may be
		// about to press the other button to emulate a middle press.
		// We need to start a timer, and if it expires without any
		// further presses, then we send the button press.
		// If a press of the other button, or any release, comes in
		// before timer has expired, we cancel timer & take different action.
		if (m_waitingOnEmulateTimer)
		{
			if (msg == WM_LBUTTONUP || msg == WM_RBUTTONUP ||
				abs(x - m_emulateButtonPressedX) > m_opts.m_Emul3Fuzz ||
				abs(y - m_emulateButtonPressedY) > m_opts.m_Emul3Fuzz)
			{
				// if button released or we moved too far then cancel.
				// First let the remote know where the button was down
				SubProcessPointerEvent(
					m_emulateButtonPressedX,
					m_emulateButtonPressedY,
					m_emulateKeyFlags);
				// Then tell it where we are now
				SubProcessPointerEvent(x, y, keyflags);
			}
			else if (
				(msg == WM_LBUTTONDOWN && (m_emulateKeyFlags & MK_RBUTTON))
				|| (msg == WM_RBUTTONDOWN && (m_emulateKeyFlags & MK_LBUTTON)))
			{
				// Triggered an emulate; remove left and right buttons, put
				// in middle one.
				DWORD emulatekeys = keyflags & ~(MK_LBUTTON|MK_RBUTTON);
				emulatekeys |= MK_MBUTTON;
				SubProcessPointerEvent(x, y, emulatekeys);

				m_emulatingMiddleButton = true;
			}
			else
			{
				// handle movement normally & don't kill timer.
				// just remove the pressed button from the mask.
				DWORD keymask = m_emulateKeyFlags & (MK_LBUTTON|MK_RBUTTON);
				DWORD emulatekeys = keyflags & ~keymask;
				SubProcessPointerEvent(x, y, emulatekeys);
				return true;
			}

			// if we reached here, we don't need the timer anymore.
			KillTimer(m_hwndcn, m_emulate3ButtonsTimer);
			m_waitingOnEmulateTimer = false;
		}
		else if (m_emulatingMiddleButton)
		{
			if ((keyflags & MK_LBUTTON) == 0 && (keyflags & MK_RBUTTON) == 0)
			{
				// We finish emulation only when both buttons come back up.
				m_emulatingMiddleButton = false;
				SubProcessPointerEvent(x, y, keyflags);
			}
			else
			{
				// keep emulating.
				DWORD emulatekeys = keyflags & ~(MK_LBUTTON|MK_RBUTTON);
				emulatekeys |= MK_MBUTTON;
				SubProcessPointerEvent(x, y, emulatekeys);
			}
		}
		else
		{
			// Start considering emulation if we've pressed a button
			// and the other isn't pressed.
			if ( (msg == WM_LBUTTONDOWN && !(keyflags & MK_RBUTTON))
				|| (msg == WM_RBUTTONDOWN && !(keyflags & MK_LBUTTON)))
			{
				// Start timer for emulation.
				m_emulate3ButtonsTimer =
					SetTimer(
					m_hwndcn,
					IDT_EMULATE3BUTTONSTIMER,
					m_opts.m_Emul3Timeout,
					NULL);

				if (!m_emulate3ButtonsTimer)
				{
					vnclog.Print(0, _T("Failed to create timer for emulating 3 buttons"));
					PostMessage(m_hwndMain, WM_CLOSE, 0, 1);
					return true;
				}

				m_waitingOnEmulateTimer = true;

				// Note that we don't send the event here; we're batching it for
				// later.
				m_emulateKeyFlags = keyflags;
				m_emulateButtonPressedX = x;
				m_emulateButtonPressedY = y;
			}
			else
			{
				// just send event noramlly
				SubProcessPointerEvent(x, y, keyflags);
			}
		}
	}
	else
	{
		SubProcessPointerEvent(x, y, keyflags);
	}

	return true;
}

//adzm 2010-09 - Ensure the mouse is moved to the last known spot
bool ClientConnection::FlushThrottledMouseMove()
{
	if (m_flushMouseMoveTimer != 0) {
		KillTimer(m_hwndcn, m_flushMouseMoveTimer);
		m_flushMouseMoveTimer = 0;
	}
	if (m_PendingMouseMove.bValid) {
		m_PendingMouseMove.bValid = false;
		m_PendingMouseMove.dwLastSentMouseMove = GetTickCount();

		SubProcessPointerEvent(m_PendingMouseMove.x, m_PendingMouseMove.y, m_PendingMouseMove.keyflags);

		return true;
	}

	return false;
}

// SubProcessPointerEvent takes windows positions and flags and converts
// them into VNC ones.

inline void ClientConnection::SubProcessPointerEvent(int x, int y, DWORD keyflags)
{
	int mask;

	if (m_opts.m_SwapMouse) {
		mask = ( ((keyflags & MK_LBUTTON) ? rfbButton1Mask : 0) |
				 ((keyflags & MK_MBUTTON) ? rfbButton3Mask : 0) |
				 ((keyflags & MK_RBUTTON) ? rfbButton2Mask : 0) );
	} else {
		mask = ( ((keyflags & MK_LBUTTON) ? rfbButton1Mask : 0) |
				 ((keyflags & MK_MBUTTON) ? rfbButton2Mask : 0) |
				 ((keyflags & MK_RBUTTON) ? rfbButton3Mask : 0) );
	}

	if ((short)HIWORD(keyflags) > 0) {
		mask |= rfbButton4Mask;
	} else if ((short)HIWORD(keyflags) < 0) {
		mask |= rfbButton5Mask;
	}

	try {
		int x_scaled =
			(x + m_hScrollPos) * m_opts.m_scale_den / m_opts.m_scale_num;
		int y_scaled =
			(y + m_vScrollPos) * m_opts.m_scale_den / m_opts.m_scale_num;

		if (m_opts.m_Directx)
		{
			x_scaled = (x ) *  m_si.framebufferWidth /m_cliwidth ;
			if(m_opts.m_ShowToolbar) y_scaled =(y) * m_si.framebufferHeight / (m_cliheight-m_TBr.bottom) ;
			else y_scaled =(y) * m_si.framebufferHeight / m_cliheight ;
		}

		SendPointerEvent(x_scaled, y_scaled, mask);

		if ((short)HIWORD(keyflags) != 0) {
			// Immediately send a "button-up" after mouse wheel event.
			mask &= !(rfbButton4Mask | rfbButton5Mask);
			SendPointerEvent(x_scaled, y_scaled, mask);
		}
	} catch (Exception &e) {
		if( m_autoReconnect ==0)
			e.Report();
		PostMessage(m_hwndMain, WM_CLOSE, reconnectcounter, 1);
	}
}

//
// RealVNC 335 method
//
inline void ClientConnection::ProcessMouseWheel(int delta)
{
  int wheelMask = rfbWheelUpMask;
  if (delta < 0) {
    wheelMask = rfbWheelDownMask;
    delta = -delta;
  }
  while (delta > 0) {
    SendPointerEvent(oldPointerX, oldPointerY, oldButtonMask | wheelMask);
    SendPointerEvent(oldPointerX, oldPointerY, oldButtonMask & ~wheelMask);
    delta -= 120;
  }
}

//
// SendPointerEvent.
//

inline void
ClientConnection::SendPointerEvent(int x, int y, int buttonMask)
{
	if (m_pFileTransfer->m_fFileTransferRunning && ( m_pFileTransfer->m_fVisible || m_pFileTransfer->UsingOldProtocol())) return;
	if (m_pTextChat->m_fTextChatRunning && m_pTextChat->m_fVisible) return;

	//omni_mutex_lock l(m_UpdateMutex);

	/*
	newtick=GetTickCount();
	if ((newtick-oldtick)<100) return;
	oldtick=newtick;
	*/

	rfbPointerEventMsg pe;

    oldPointerX = x;
    oldPointerY = y;
    oldButtonMask = buttonMask;
    pe.type = rfbPointerEvent;
    pe.buttonMask = buttonMask;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
	// tight cursor handling
	SoftCursorMove(x, y);
    pe.x = Swap16IfLE(x);
    pe.y = Swap16IfLE(y);
	//adzm 2010-09
	WriteExactQueue_timeout((char *)&pe, sz_rfbPointerEventMsg, rfbPointerEvent,5); // sf@2002 - For DSM Plugin
}

//
// ProcessKeyEvent
//
// Normally a single Windows key event will map onto a single RFB
// key message, but this is not always the case.  Much of the stuff
// here is to handle AltGr (=Ctrl-Alt) on international keyboards.
// Example cases:
//
//    We want Ctrl-F to be sent as:
//      Ctrl-Down, F-Down, F-Up, Ctrl-Up.
//    because there is no keysym for ctrl-f, and because the ctrl
//    will already have been sent by the time we get the F.
//
//    On German keyboards, @ is produced using AltGr-Q, which is
//    Ctrl-Alt-Q.  But @ is a valid keysym in its own right, and when
//    a German user types this combination, he doesn't mean Ctrl-@.
//    So for this we will send, in total:
//
//      Ctrl-Down, Alt-Down,
//                 (when we get the AltGr pressed)
//
//      Alt-Up, Ctrl-Up, @-Down, Ctrl-Down, Alt-Down
//                 (when we discover that this is @ being pressed)
//
//      Alt-Up, Ctrl-Up, @-Up, Ctrl-Down, Alt-Down
//                 (when we discover that this is @ being released)
//
//      Alt-Up, Ctrl-Up
//                 (when the AltGr is released)

inline void ClientConnection::ProcessKeyEvent(int virtKey, DWORD keyData)
{
    bool down = ((keyData & 0x80000000l) == 0);

    // if virtkey found in mapping table, send X equivalent
    // else
    //   try to convert directly to ascii
    //   if result is in range supported by X keysyms,
    //      raise any modifiers, send it, then restore mods
    //   else
    //      calculate what the ascii would be without mods
    //      send that

#ifdef _DEBUG
#ifdef UNDER_CE
	char *keyname="";
#else
    char keyname[32];
    if (GetKeyNameText(  keyData,keyname, 31)) {
//        vnclog.Print(4, _T("Process key: %s (keyData %04x): virtkey %04x "), keyname, keyData,virtkey);
//		if (virtkey==0x00dd)
//			vnclog.Print(4, _T("Process key: %s (keyData %04x): virtkey %04x "), keyname, keyData,virtkey);
    };
#endif
#endif

	if (m_opts.m_JapKeyboard==0 && virtKey!=69)
	{
		try {
			m_keymap->PCtoX(virtKey, keyData, this);
		} catch (Exception &e) {
			if( m_autoReconnect ==0)
				e.Report();
			PostMessage(m_hwndMain, WM_CLOSE, reconnectcounter, 1);
		}
	}
	else
	{
		try {
			KeyActionSpec kas = m_keymapJap->PCtoX(virtKey, keyData);

			if (kas.releaseModifiers & KEYMAP_LCONTROL) {
				SendKeyEvent(XK_Control_L, false );
				vnclog.Print(5, _T("fake L Ctrl raised\n"));
			}
			if (kas.releaseModifiers & KEYMAP_LALT) {
				SendKeyEvent(XK_Alt_L, false );
				vnclog.Print(5, _T("fake L Alt raised\n"));
			}
			if (kas.releaseModifiers & KEYMAP_RCONTROL) {
				SendKeyEvent(XK_Control_R, false );
				vnclog.Print(5, _T("fake R Ctrl raised\n"));
			}
			if (kas.releaseModifiers & KEYMAP_RALT) {
				SendKeyEvent(XK_Alt_R, false );
				vnclog.Print(5, _T("fake R Alt raised\n"));
			}

			for (int i = 0; kas.keycodes[i] != XK_VoidSymbol && i < MaxKeysPerKey; i++) {
				SendKeyEvent(kas.keycodes[i], down );
				//vnclog.Print(4, _T("Sent keysym %04x (%s)\n"),
				//	kas.keycodes[i], down ? _T("press") : _T("release"));
			}

			if (kas.releaseModifiers & KEYMAP_RALT) {
				SendKeyEvent(XK_Alt_R, true );
				vnclog.Print(5, _T("fake R Alt pressed\n"));
			}
			if (kas.releaseModifiers & KEYMAP_RCONTROL) {
				SendKeyEvent(XK_Control_R, true );
				vnclog.Print(5, _T("fake R Ctrl pressed\n"));
			}
			if (kas.releaseModifiers & KEYMAP_LALT) {
				SendKeyEvent(XK_Alt_L, false );
				vnclog.Print(5, _T("fake L Alt pressed\n"));
			}
			if (kas.releaseModifiers & KEYMAP_LCONTROL) {
				SendKeyEvent(XK_Control_L, false );
				vnclog.Print(5, _T("fake L Ctrl pressed\n"));
			}
		} catch (Exception &e) {
			if( m_autoReconnect ==0)
				e.Report();
			PostMessage(m_hwndMain, WM_CLOSE, 4, 1);
		}
	}
}

//
// SendKeyEvent
//

inline void
ClientConnection::SendKeyEvent(CARD32 key, bool down)
{
	if (m_pFileTransfer->m_fFileTransferRunning && ( m_pFileTransfer->m_fVisible || m_pFileTransfer->UsingOldProtocol())) return;
	if (m_pTextChat->m_fTextChatRunning && m_pTextChat->m_fVisible) return;

    rfbKeyEventMsg ke;

    ke.type = rfbKeyEvent;
    ke.down = down ? 1 : 0;
    ke.key = Swap32IfLE(key);
	//adzm 2010-09
    WriteExactQueue_timeout((char *)&ke, sz_rfbKeyEventMsg, rfbKeyEvent,5);
    //vnclog.Print(0, _T("SendKeyEvent: key = x%04x status = %s ke.key=%d\n"), key,
      //  down ? _T("down") : _T("up"),ke.key);
}

#ifndef UNDER_CE
//
// SendClientCutText
//

void ClientConnection::SendClientCutText(char *str, int len)
{
	// adzm - 2010-07 - Extended clipboard
	if (m_pFileTransfer->m_fFileTransferRunning && ( m_pFileTransfer->m_fVisible || m_pFileTransfer->UsingOldProtocol())) {
		vnclog.Print(6, _T("Ignoring SendClientCutText due to in-progress file transfer\n"));
		return;
	}
	if (m_pTextChat->m_fTextChatRunning && m_pTextChat->m_fVisible) {
		vnclog.Print(6, _T("Ignoring SendClientCutText due to in-progress text chat\n"));
		return;
	}

	omni_mutex_lock l(m_clipMutex);

	std::string strStr(str);

	if (!m_clipboard.m_strLastCutText.empty()) {
		if (m_clipboard.m_strLastCutText.compare(strStr) == 0) {
			vnclog.Print(6, _T("Ignoring SendClientCutText due to identical data\n"));
			return;
		}

		m_clipboard.m_strLastCutText = "";
	}

	m_clipboard.m_strLastCutText = strStr;

	rfbClientCutTextMsg cct;

	cct.type = rfbClientCutText;
	cct.length = Swap32IfLE(len);
	//adzm 2010-09
	WriteExactQueue((char *)&cct, sz_rfbClientCutTextMsg, rfbClientCutText);
	WriteExact(str, len);
	vnclog.Print(6, _T("Sent %d bytes of clipboard\n"), len);
}
#endif

// Copy any updated areas from the bitmap onto the screen.

inline void ClientConnection::DoBlit()
{
//	if (m_hBitmap == NULL) return;
	if (!m_running) return;

	// No other threads can use bitmap DC
	//omni_mutex_lock l(m_bitmapdcMutex);

	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(m_hwndcn, &ps);

	if (directx_used)
	{
		directx_output.paint();
	}
	else
	{
		// Select and realize hPalette
	//	PaletteSelector p(hdc, m_hPalette);
	//	ObjectSelector b(m_hBitmapDC, m_hBitmap);

		if (m_opts.m_delay) {
			// Display the area to be updated for debugging purposes
			/*
			COLORREF oldbgcol = SetBkColor(hdc, RGB(0,0,0));
			::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &ps.rcPaint, NULL, 0, NULL);
			SetBkColor(hdc,oldbgcol);
			*/
			// adzm - 2010-07 - Works better this way
			::FillRect(hdc, &ps.rcPaint, ::GetSysColorBrush(COLOR_DESKTOP));
			::Sleep(m_pApp->m_options.m_delay);
		}

		if (m_opts.m_scaling || (directx_used == false && m_opts.m_Directx == true))
		{

			if ((directx_used == false && m_opts.m_Directx == true))
			{

				RECT myrect, myclrect;
				GetClientRect(m_hwndMain, &myclrect);
				int w = myclrect.right - myclrect.left;
				int h = myclrect.bottom - myclrect.top;

				if (m_opts.m_ShowToolbar) h = h - m_TBr.bottom;

				float horizontalRatio = (float)w / (float)m_si.framebufferWidth;
				float verticalRatio = (float)h / (float)m_si.framebufferHeight;


				SetStretchBltMode(hdc, HALFTONE);
				SetBrushOrgEx(hdc, 0, 0, NULL);
				{
					if (m_hmemdc)
					{
						ObjectSelector bb(m_hmemdc, m_membitmap);
						StretchBlt(
							hdc,
							ps.rcPaint.left,
							ps.rcPaint.top,
							ps.rcPaint.right - ps.rcPaint.left,
							ps.rcPaint.bottom - ps.rcPaint.top,
							m_hmemdc,
							(ps.rcPaint.left + m_hScrollPos)     / horizontalRatio,
							(ps.rcPaint.top + m_vScrollPos)      / verticalRatio,
							(ps.rcPaint.right - ps.rcPaint.left) / horizontalRatio,
							(ps.rcPaint.bottom - ps.rcPaint.top) / verticalRatio,
							SRCCOPY);
					}
				}
			}
			else
			{

				int n = m_opts.m_scale_num;
				int d = m_opts.m_scale_den;

				SetStretchBltMode(hdc, HALFTONE);
				SetBrushOrgEx(hdc, 0, 0, NULL);
				{
					if (m_hmemdc)
					{
						ObjectSelector bb(m_hmemdc, m_membitmap);
						StretchBlt(
							hdc,
							ps.rcPaint.left,
							ps.rcPaint.top,
							ps.rcPaint.right - ps.rcPaint.left,
							ps.rcPaint.bottom - ps.rcPaint.top,
							m_hmemdc,
							(ps.rcPaint.left + m_hScrollPos)     * d / n,
							(ps.rcPaint.top + m_vScrollPos)      * d / n,
							(ps.rcPaint.right - ps.rcPaint.left) * d / n,
							(ps.rcPaint.bottom - ps.rcPaint.top) * d / n,
							SRCCOPY);
					}
				}
			}
		}
		else
		{
			if (m_hmemdc)
			{
				ObjectSelector bb(m_hmemdc, m_membitmap);
				BitBlt(hdc, ps.rcPaint.left, ps.rcPaint.top,
				ps.rcPaint.right-ps.rcPaint.left, ps.rcPaint.bottom-ps.rcPaint.top,
				m_hmemdc, ps.rcPaint.left+m_hScrollPos, ps.rcPaint.top+m_vScrollPos, SRCCOPY);
			}
		}
	}
	EndPaint(m_hwndcn, &ps);
}

inline void ClientConnection::UpdateScrollbars()
{
	// We don't update the actual scrollbar info in full-screen mode
	// because it causes them to flicker.
	bool setInfo = !InFullScreenMode();

	SCROLLINFO scri;
	scri.cbSize = sizeof(scri);
	scri.fMask = SIF_ALL;
	scri.nMin = 0;
	scri.nMax = m_hScrollMax;
	scri.nPage= m_cliwidth;
	scri.nPos = m_hScrollPos;

	if (setInfo)
		SetScrollInfo(m_hwndMain, SB_HORZ, &scri, TRUE);

	scri.cbSize = sizeof(scri);
	scri.fMask = SIF_ALL;
	scri.nMin = 0;

	scri.nMax = m_vScrollMax ;
	scri.nPage= m_cliheight;
	scri.nPos = m_vScrollPos;

	if (setInfo)
		SetScrollInfo(m_hwndMain, SB_VERT, &scri, TRUE);
}

void ClientConnection::ShowConnInfo()
{
	TCHAR buf[2048];
#ifndef UNDER_CE
	char kbdname[9];
	GetKeyboardLayoutName(kbdname);
#else
	TCHAR *kbdname = _T("(n/a)");
#endif
	TCHAR num[16];
	_stprintf(
		buf,
		_T("Connected to: %s\n\r\n\r")
		_T("Host: %s  Port: %d\n\r")
		_T("%s %s  %s\n\r\n\r")
		_T("Desktop geometry: %d x %d x %d\n\r")
		_T("Using depth: %d\n\r")
		_T("Line speed estimate: %d kbit/s\n")
		_T("Current protocol version: %d.%d\n\r\n\r")
		_T("Current keyboard name: %s\n\r\n\r")
		_T("Using Plugin : %s - %s\n\r\n\r"), // sf@2002 - v1.1.2
		m_desktopName, m_host, m_port,
		strcmp(m_proxyhost,"") ? m_proxyhost : "",
		strcmp(m_proxyhost,"") ? "Port" : "",
		strcmp(m_proxyhost,"") ? _itoa(m_proxyport, num, 10) : "",
		m_si.framebufferWidth, m_si.framebufferHeight,
                m_si.format.depth,
		m_myFormat.depth, kbitsPerSecond,
		m_majorVersion, m_minorVersion,
		kbdname,
		m_pDSMPlugin->IsEnabled() ? m_pDSMPlugin->GetPluginName() : "",
		m_pDSMPlugin->IsEnabled() ? m_pDSMPlugin->GetPluginVersion() : "");
	MessageBox(m_hwndMain, buf, _T("VNC connection info"), MB_ICONINFORMATION | MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
}

// ********************************************************************
//  Methods after this point are generally called by the worker thread.
//  They finish the initialisation, then chiefly read data from the server.
// ********************************************************************

void* ClientConnection::run_undetached(void* arg) {
	vnclog.Print(9,_T("Update-processing thread started\n"));

	m_threadStarted = true;

	// Modif sf@2002 - Server Scaling
	m_nServerScale = m_opts.m_nServerScale;

	m_reconnectcounter = m_opts.m_reconnectcounter;
	m_autoReconnect = m_opts.m_autoReconnect;
	if (m_Is_Listening)m_reconnectcounter=0;
	reconnectcounter = m_reconnectcounter;

	if (m_nServerScale > 1) SendServerScale(m_nServerScale);

	//adzm 2010-09 - all socket writes must remain on a single thread
	SendFullFramebufferUpdateRequest(false);

	SizeWindow();
	RealiseFullScreenMode();
	if (!InFullScreenMode()) SizeWindow();

	m_running = true;
	UpdateWindow(m_hwndcn);

	// sf@2002 - Attempt to speed up the thing
	// omni_thread::set_priority(omni_thread::PRIORITY_LOW);

    rdr::U8 msgType=0;

	// sf@2007 - AutoReconnect
	// Error, value can be set 0 by gui in that case you get a gray screen
	if (m_autoReconnect==0) m_autoReconnect=1;
	initialupdate_counter=0;
	while (m_autoReconnect > 0)
	{
		try
		{
			while (!m_bKillThread)
			{
				// sf@2002 - DSM Plugin
				if (!m_fUsePlugin)
				{
					msgType = fis->readU8();
					m_nTO = 1; // Read the rest of the rfb message (normal case)
				}
				else if (m_pDSMPlugin->IsEnabled())
				{
					// Read the additional type char sent by the DSM Plugin (server)
					// We need it to know the type of rfb message that follows
					// because we can't see the type inside the transformed rfb message.
					ReadExact((char *)&msgType, sizeof(msgType));
					// adzm 2010-09
					if (m_fPluginStreamingIn)
					{
						m_nTO = 1; // we'll need to read the whole transformed rfb message that follows
					}
					else
					{
						m_nTO = 0; // we'll need to read the whole transformed rfb message that follows
					}
				}

				switch (msgType)
				{
#ifdef _Gii
				case rfbGIIMessage:
					mytouch->_handle_gii_message(m_hwndcn);
					break;
#endif
                case rfbKeepAlive:
                    m_server_wants_keepalives = true;
#if defined(_DEBUG)
                    {
                        static time_t lastrecv = 0;
                        time_t now = time(&now);
                        time_t delta = now - lastrecv;
                        lastrecv = now;
                        char msg[255];
                        sprintf(msg, "keepalive received %I64i seconds since last one\n", delta);
                        OutputDebugString(msg);
                    }
#endif
                    if (sz_rfbKeepAliveMsg > 1)
                    {
                  	rfbKeepAliveMsg kp;
                	ReadExact(((char *) &kp)+m_nTO, sz_rfbKeepAliveMsg-m_nTO);
                    }
                    break;
				case rfbRequestSession:
					break;
				case rfbFramebufferUpdate:
					ReadScreenUpdate();
					break;

				case rfbSetColourMapEntries:
					vnclog.Print(3, _T("rfbSetColourMapEntries read but not supported\n") );
					throw WarningException(sz_L63);
					break;

				case rfbBell:
					ReadBell();
					break;

				case rfbServerCutText:
					ReadServerCutText();
					break;

				// Modif sf@2002 - FileTransfer
				// File Transfer Message
				case rfbFileTransfer:
					{
					// vnclog.Print(0, _T("rfbFileTransfer\n") );
					// m_pFileTransfer->ProcessFileTransferMsg();
					// sf@2005 - FileTransfer rfbMessage and screen updates must be sent/received
					// by the same thread
					SendMessage(m_hwndMain, FileTransferSendPacketMessage, 1, 0);
					}
					break;

				// Modif sf@2002 - Text Chat
				case rfbTextChat:
					m_pTextChat->ProcessTextChatMsg();
					break;

				// Modif sf@2002 - Server Scaling
				// Server Scaled screen buffer size has changed, so we resize
				// the viewer window
				case rfbResizeFrameBuffer:
				{
					ClearCache();
					rfbResizeFrameBufferMsg rsmsg;
					ReadExact(((char*)&rsmsg) + m_nTO, sz_rfbResizeFrameBufferMsg - m_nTO);

					m_si.framebufferWidth = Swap16IfLE(rsmsg.framebufferWidth);
					m_si.framebufferHeight = Swap16IfLE(rsmsg.framebufferHeigth);

					CreateLocalFramebuffer();
					// SendFullFramebufferUpdateRequest();
					Createdib();
					m_pendingScaleChange = true;
					m_pendingFormatChange = true;
					//adzm 2010-09 - this could send in the middle of other sends by the ui thread! We can still use SendMessage to ensure
					//it blocks until processed.
					//SendAppropriateFramebufferUpdateRequest();
					//SendMessage(m_hwndcn, WM_REGIONUPDATED, NULL, NULL);
					//adzm 2010-09 - This function delegates the processing to the UI thread now, basically doing the same as the sendmessage above.
					SendAppropriateFramebufferUpdateRequest(false);

					SizeWindow();
					InvalidateRect(m_hwndcn, NULL, TRUE);
					RealiseFullScreenMode();
					break;
				}

                case rfbServerState:
                {
                    ReadServerState();
                    ::PostMessage(m_hwndMain, RebuildToolbarMessage, 0, 0);
                }
                    break;

				// adzm 2010-09 - Notify streaming DSM plugin support
                case rfbNotifyPluginStreaming:
                    if (sz_rfbNotifyPluginStreamingMsg > 1)
                    {
                  	rfbNotifyPluginStreamingMsg nspm;
                	ReadExact(((char *) &nspm)+m_nTO, sz_rfbNotifyPluginStreamingMsg-m_nTO);
                    }
					m_fPluginStreamingIn = true;
					PostMessage(m_hwndcn, WM_NOTIFYPLUGINSTREAMING, NULL, NULL);
                    break;
				default:
						  vnclog.Print(3, _T("Unknown message type x%02x\n"), msgType );
						  throw WarningException(sz_L64);
						  break;

				/*
				default:
					log.Print(3, _T("Unknown message type x%02x\n"), msgType );
					throw WarningException("Unhandled message type received!\n");
				*/
				}
				// yield();
			}
		}
		catch (Exception &)
		{
			// sf@2002
			// m_pFileTransfer->m_fFileTransferRunning = false;
			// m_pTextChat->m_fTextChatRunning = false;
			if (m_pPluginInterface) {
				delete m_pPluginInterface;
				m_pPluginInterface = NULL;
				//adzm 2010-05-10
				m_pIntegratedPluginInterface = NULL;
			}
			m_bKillThread = true;
			PostMessage(m_hwndMain, WM_CLOSE, reconnectcounter, 1);
		}
		catch (rdr::Exception& e)
		{
			vnclog.Print(0,"rdr::Exception (1): %s\n",e.str());
			// m_pFileTransfer->m_fFileTransferRunning = false;
			// m_pTextChat->m_fTextChatRunning = false;
			// throw QuietException(e.str());
			if ((strcmp(e.str(),"rdr::EndOfStream: read")==NULL) && !m_bClosedByUser)
			{
				WarningException w(sz_L94,200);
               // w.Report();
			}
			else if ((strcmp(e.str(),"rdr::SystemException: read: Unknown error (10054)")==NULL) && !m_bClosedByUser)
			{
				WarningException w(sz_L94,200);
			}
            else if (!(/*m_pFileTransfer->m_fFileTransferRunning || m_pTextChat->m_fTextChatRunning ||*/ m_bClosedByUser))
            {
                WarningException w(sz_L69);
                w.Report();
                g_ConnectionLossAlreadyReported = true;
            }
			m_bKillThread = true;
			PostMessage(m_hwndMain, WM_CLOSE, reconnectcounter, 1);
		}

		Sleep(0);
		Sleep(2000);
	}

	vnclog.Print(4, _T("Update-processing thread finishing\n") );
	SetEvent(KillEvent);
		// sf@2002
	m_pFileTransfer->m_fFileTransferRunning = false;
	m_pTextChat->m_fTextChatRunning = false;
	return this;
}

//
// Requesting screen updates from the server
//

void ClientConnection::Internal_SendFramebufferUpdateRequest(int x, int y, int w, int h, bool incremental)
{
	if (m_pFileTransfer->m_fFileTransferRunning && ( m_pFileTransfer->m_fVisible || m_pFileTransfer->UsingOldProtocol())) return;
	if (m_pTextChat->m_fTextChatRunning && m_pTextChat->m_fVisible) return;

	//omni_mutex_lock l(m_UpdateMutex);

    rfbFramebufferUpdateRequestMsg fur;

	// vnclog.Print(0, _T("Request %s update x=%d,y=%d,w=%d,h=%d\n"), incremental ? _T("incremental") : _T("full"),x,y,w,h);

    fur.type = rfbFramebufferUpdateRequest;
    fur.incremental = incremental ? 1 : 0;
    fur.x = Swap16IfLE(x);
    fur.y = Swap16IfLE(y);
    fur.w = Swap16IfLE(w);
    fur.h = Swap16IfLE(h);

	//vnclog.Print(10, _T("Request %s update\n"), incremental ? _T("incremental") : _T("full"));
    WriteExact_timeout((char *)&fur, sz_rfbFramebufferUpdateRequestMsg, rfbFramebufferUpdateRequest,5);
}

void ClientConnection::HandleFramebufferUpdateRequest(WPARAM wParam, LPARAM lParam)
{
	switch (wParam) {
		case 0x00000001:
			// Incremental
			Internal_SendIncrementalFramebufferUpdateRequest();
			break;
		case 0xFFFFFFFF:
			// Appropriate:
			Internal_SendAppropriateFramebufferUpdateRequest();
			break;
		case 0x00000000:
		default:
			// Full
			Internal_SendFullFramebufferUpdateRequest();
			break;
	}
}

void ClientConnection::SendIncrementalFramebufferUpdateRequest(bool bAsync)
{
	SendFramebufferUpdateRequest(0x00000001, bAsync);
}

void ClientConnection::SendFullFramebufferUpdateRequest(bool bAsync)
{
	SendFramebufferUpdateRequest(0x00000000, bAsync);
}

void ClientConnection::SendAppropriateFramebufferUpdateRequest(bool bAsync)
{
	SendFramebufferUpdateRequest(0xFFFFFFFF, bAsync);
}

void ClientConnection::SendFramebufferUpdateRequest(WPARAM requestType, bool bAsync)
{
	if (bAsync) {
		PostMessage(m_hwndcn, WM_REQUESTUPDATE, (WPARAM)requestType, (LPARAM)0);
	} else {
		SendMessage(m_hwndcn, WM_REQUESTUPDATE, (WPARAM)requestType, (LPARAM)0);
	}
}

void ClientConnection::Internal_SendIncrementalFramebufferUpdateRequest()
{
    Internal_SendFramebufferUpdateRequest(0, 0, m_si.framebufferWidth,
					m_si.framebufferHeight, true);
}

void ClientConnection::Internal_SendFullFramebufferUpdateRequest()
{
    Internal_SendFramebufferUpdateRequest(0, 0, m_si.framebufferWidth,
					m_si.framebufferHeight, false);
}


void ClientConnection::Internal_SendAppropriateFramebufferUpdateRequest()
{
	if (m_pendingFormatChange)
	{
		if (prevCursorSet) {
		if (m_SavedAreaBIB) delete[] m_SavedAreaBIB;
		m_SavedAreaBIB=NULL;
		if ( rcSource) delete[] rcSource;
		rcSource=NULL;
		if (rcMask) delete[] rcMask;
		rcMask=NULL;
		prevCursorSet = false;
		}
		vnclog.Print(3, _T("Requesting new pixel format\n") );
		rfbPixelFormat oldFormat = m_myFormat;
		SetupPixelFormat();
		SetFormatAndEncodings();
		Createdib();
		m_pendingFormatChange = false;

		// Cache init/reinit - A SetFormatAndEncoding() implies a cache reinit on server side
		// Cache enabled, so it's going to be reallocated/reinited on server side
		omni_mutex_lock l(m_bitmapdcMutex);//m_cursorMutex);
		if (m_opts.m_fEnableCache)
		{
			// create viewer cache buffer if necessary
			if (m_DIBbitsCache == NULL)
			{
				int Pitch=m_si.framebufferWidth*m_myFormat.bitsPerPixel/8;
				if (Pitch % 4)Pitch += 4 - Pitch % 4;

				m_DIBbitsCache= new BYTE[Pitch*m_si.framebufferHeight];
			}
			ClearCache(); // Clear the cache
			m_pendingCacheInit = true; // Order full update to synchronize both sides caches
		}
		else
		{
			if (m_DIBbitsCache!=NULL) delete []m_DIBbitsCache;
			m_DIBbitsCache=NULL;
		}
		if (m_SavedAreaBIB) delete [] m_SavedAreaBIB;
		m_SavedAreaBIB=NULL;

		// If the pixel format has changed, or cache, or scale request whole screen
		if (true)//!PF_EQ(m_myFormat, oldFormat) || m_pendingCacheInit || m_pendingScaleChange)
		{
			Internal_SendFullFramebufferUpdateRequest();
		}
		else
		{
			Internal_SendIncrementalFramebufferUpdateRequest();
		}
		m_pendingScaleChange = false;
		m_pendingCacheInit = false;
	}
	else
	{
		if (m_dormant!=1)
			Internal_SendIncrementalFramebufferUpdateRequest();
		if (m_dormant == 2) m_dormant = 1;
	}
}

//
// Modif sf@2002 - Server Scaling
//
bool ClientConnection::SendServerScale(int nScale)
{
    rfbSetScaleMsg ssc;
    int len = 0;

    ssc.type = rfbSetScale;
    ssc.scale = /*(unsigned short)*/ nScale;

    WriteExact((char*)&ssc, sz_rfbSetScaleMsg, rfbSetScale);

    return true;
}

//
// Modif rdv@2002 - Set Server input
//
bool ClientConnection::SendServerInput(BOOL enabled)
{
    rfbSetServerInputMsg sim;
    int len = 0;

    sim.type = rfbSetServerInput;
    sim.status = enabled;

    WriteExact((char*)&sim, sz_rfbSetServerInputMsg, rfbSetServerInput);

    return true;
}

//
// Modif rdv@2002 - Single window
//
bool ClientConnection::SendSW(int x, int y)
{
    rfbSetSWMsg sw;
    int len = 0;
	if (x==9999 && y==9999)
	{
		sw.type = rfbSetSW;
		sw.x = Swap16IfLE(1);
		sw.y = Swap16IfLE(1);
	}
	else
	{
		int x_scaled =
			(x + m_hScrollPos) * m_opts.m_scale_den / m_opts.m_scale_num;
		int y_scaled =
			(y + m_vScrollPos) * m_opts.m_scale_den / m_opts.m_scale_num;

		sw.type = rfbSetSW;
		sw.x = Swap16IfLE(x_scaled);
		sw.y = Swap16IfLE(y_scaled);
	}

    WriteExact_timeout((char*)&sw, sz_rfbSetSWMsg, rfbSetSW,5);
	m_SWselect=false;
    return true;
}

// A ScreenUpdate message has been received
inline void ClientConnection::ReadScreenUpdate()
{
	//adzm 2010-07-04
	bool bSentUpdateRequest = false;
	if (m_opts.m_preemptiveUpdates && !m_pendingFormatChange) {
		bSentUpdateRequest = true;
		//PostMessage(m_hwndcn, WM_REGIONUPDATED, NULL, NULL);
		//adzm 2010-09 - We can simply call SendAppropriateFramebufferUpdateRequest now, with a true bAsync param so the request is posted rather than sent.
		SendAppropriateFramebufferUpdateRequest(true);
	}

	HDC hdcX,hdcBits;

	bool fTimingAlreadyStopped = false;
	fis->startTiming();

	rfbFramebufferUpdateMsg sut;
	ReadExact(((char *) &sut)+m_nTO, sz_rfbFramebufferUpdateMsg-m_nTO);
    sut.nRects = Swap16IfLE(sut.nRects);
	HRGN UpdateRegion=CreateRectRgn(0,0,0,0);
	bool Recover_from_sync=false;

    //if (sut.nRects == 0) return;  XXX tjr removed this - is this OK?

	for (UINT iCurrentRect=0; iCurrentRect < sut.nRects; iCurrentRect++)
	{
		rfbFramebufferUpdateRectHeader surh;
		ReadExact((char *) &surh, sz_rfbFramebufferUpdateRectHeader);
		surh.r.x = Swap16IfLE(surh.r.x);
		surh.r.y = Swap16IfLE(surh.r.y);
		surh.r.w = Swap16IfLE(surh.r.w);
		surh.r.h = Swap16IfLE(surh.r.h);
		surh.encoding = Swap32IfLE(surh.encoding);
		// vnclog.Print(0, _T("%d %d\n"), i,sut.nRects);
		//vnclog.Print(0, _T("encoding %d\n"), surh.encoding);

		// Tight - If lastrect we must quit this loop (nRects = 0xFFFF)
		if (surh.encoding == rfbEncodingLastRect)
			break;

		if (surh.encoding == rfbEncodingNewFBSize)
		{
			ReadNewFBSize(&surh);
			break;
		}

		// Tight cursor handling
		if ( surh.encoding == rfbEncodingXCursor ||
			surh.encoding == rfbEncodingRichCursor )
		{
			ReadCursorShape(&surh);
			continue;
		}

        // marscha PointerPos
		if (surh.encoding == rfbEncodingPointerPos) {
			//vnclog.Print(0, _T("reading cursorPos (%d,%d)\n"), surh.r.x, surh.r.y);
			ReadCursorPos(&surh);
			continue;
		}

		if (surh.encoding !=rfbEncodingNewFBSize && surh.encoding != rfbEncodingCacheZip && surh.encoding != rfbEncodingSolMonoZip && surh.encoding !=rfbEncodingUltraZip)
			SoftCursorLockArea(surh.r.x, surh.r.y, surh.r.w, surh.r.h);

		// Modif sf@2002 - DSM Plugin
		// With DSM, all rects contents (excepted caches) are buffered into memory in one shot
		// then they will be read in this buffer by the "regular" Read*Type*Rect() functions
		// adzm 2010-09
		if (m_fUsePlugin && !m_fPluginStreamingIn && m_pDSMPlugin->IsEnabled())
		{
			if (!m_fReadFromNetRectBuf)
			{
				switch (surh.encoding)
				{
				case rfbEncodingRaw:
				case rfbEncodingRRE:
				case rfbEncodingCoRRE:
				case rfbEncodingHextile:
				case rfbEncodingUltra:
				case rfbEncodingUltra2:
				case rfbEncodingZlib:
				case rfbEncodingXOR_Zlib:
				case rfbEncodingXORMultiColor_Zlib:
				case rfbEncodingXORMonoColor_Zlib:
				case rfbEncodingSolidColor:
				case rfbEncodingTight:
				case rfbEncodingZlibHex:
					{
						// Get the size of the rectangle data buffer
						ReadExact((char*)&(m_nReadSize), sizeof(CARD32));
						m_nReadSize = Swap32IfLE(m_nReadSize);
						// Read the whole  rect buffer and put the result in m_netbuf
						CheckNetRectBufferSize((int)(m_nReadSize));
						CheckBufferSize((int)(m_nReadSize)); // sf@2003
						ReadExact((char*)(m_pNetRectBuf), (int)(m_nReadSize));
						// Tell the following ReadExact() function calls to Read Data from memory
						m_nNetRectBufOffset = 0;
						m_fReadFromNetRectBuf = true;
					}
					break;
				}
			}
		}

		// adzm 2010-09
		if (m_fUsePlugin && m_pDSMPlugin->IsEnabled())
		{
			// ZRLE special case
			if (!fis->GetReadFromMemoryBuffer())
			{
				if ((surh.encoding == rfbEncodingZYWRLE)||(surh.encoding == rfbEncodingZRLE))
				{
					if (m_minorVersion==6 || m_minorVersion==4 || m_minorVersion==16 || m_minorVersion==14 || m_opts.m_oldplugin)
					{
						ReadExact((char*)&(m_nZRLEReadSize), sizeof(CARD32));
						m_nZRLEReadSize = Swap32IfLE(m_nZRLEReadSize);

						CheckZRLENetRectBufferSize((int)(m_nZRLEReadSize));
						CheckBufferSize((int)(m_nZRLEReadSize)); // sf@2003
						ReadExact((char*)(m_pZRLENetRectBuf), (int)(m_nZRLEReadSize));
						fis->SetReadFromMemoryBuffer(m_nZRLEReadSize, (char*)(m_pZRLENetRectBuf));
					}
					else
					{
					// Get the size of the rectangle data buffer
					ReadExact((char*)&(m_nZRLEReadSize), sizeof(CARD32));
					m_nZRLEReadSize = Swap32IfLE(m_nZRLEReadSize);
					int tempsize=m_nZRLEReadSize;
					// Read the whole  rect buffer and put the result in m_netbuf
					CheckZRLENetRectBufferSize((int)(m_nZRLEReadSize)+4);
					CheckBufferSize((int)(m_nZRLEReadSize)+4); // sf@2003
					ReadExact((char*)(m_pZRLENetRectBuf+4), (int)(m_nZRLEReadSize));
					// Tell the following Read() function calls to Read Data from memory
					tempsize = Swap32IfLE(tempsize);
					memcpy(m_pZRLENetRectBuf,(char*)&tempsize,4);
					fis->SetReadFromMemoryBuffer(m_nZRLEReadSize+4, (char*)(m_pZRLENetRectBuf));
					}
				}
			}
		}

		RECT cacherect;
		if (m_opts.m_fEnableCache)
		{
			cacherect.left=surh.r.x;
			cacherect.right=surh.r.x+surh.r.w;
			cacherect.top=surh.r.y;
			cacherect.bottom=surh.r.y+surh.r.h;
		}

		if (m_TrafficMonitor)
		{
			HDC hdcX,hdcBits;
			hdcX = GetDC(m_TrafficMonitor);
			hdcBits = CreateCompatibleDC(hdcX);
			HGDIOBJ hbrOld=SelectObject(hdcBits,m_bitmapBACK);
			BitBlt(hdcX,4,2,22,20,hdcBits,0,0,SRCCOPY);
			SelectObject(hdcBits, hbrOld);
			DeleteDC(hdcBits);
			ReleaseDC(m_TrafficMonitor,hdcX);
		}

		// sf@2004
		// adzm 2010-09
		if (m_fUsePlugin && !m_fPluginStreamingIn && m_pDSMPlugin->IsEnabled() && (m_fReadFromNetRectBuf || fis->GetReadFromMemoryBuffer()))
		{
			fis->stopTiming();
			kbitsPerSecond = fis->kbitsPerSecond();
			if (avg_kbitsPerSecond==0) avg_kbitsPerSecond=kbitsPerSecond;
			else
			{
			if (kbitsPerSecond>= avg_kbitsPerSecond/2 && kbitsPerSecond<=avg_kbitsPerSecond*3/2)
				avg_kbitsPerSecond=(avg_kbitsPerSecond+kbitsPerSecond)/2;
			else avg_kbitsPerSecond=(3*avg_kbitsPerSecond+kbitsPerSecond)/4;
			}
			fTimingAlreadyStopped = true;
		}
			initialupdate_counter++;
			// vnclog.Print(0, _T("known encoding %d - not supported!\n"), surh.encoding);
		switch (surh.encoding)
		{
		case rfbEncodingHextile:
			if (directx_used) m_DIBbits=directx_output.Preupdate((unsigned char *)m_DIBbits);
			SaveArea(cacherect);
			ReadHextileRect(&surh);
			EncodingStatusWindow=rfbEncodingHextile;
			break;
		case rfbEncodingUltra:
			if (directx_used) m_DIBbits=directx_output.Preupdate((unsigned char *)m_DIBbits);
			ReadUltraRect(&surh);
			EncodingStatusWindow=rfbEncodingUltra;
			break;
		case rfbEncodingUltra2:
			if (directx_used) m_DIBbits=directx_output.Preupdate((unsigned char *)m_DIBbits);
			ReadUltra2Rect(&surh);
			EncodingStatusWindow=rfbEncodingUltra2;
			break;
		case rfbEncodingUltraZip:
			if (directx_used) m_DIBbits=directx_output.Preupdate((unsigned char *)m_DIBbits);
			ReadUltraZip(&surh,&UpdateRegion);
			break;
		case rfbEncodingRaw:
			if (directx_used) m_DIBbits=directx_output.Preupdate((unsigned char *)m_DIBbits);
			SaveArea(cacherect);
			ReadRawRect(&surh);
			EncodingStatusWindow=rfbEncodingRaw;
			break;
		case rfbEncodingCopyRect:
			if (directx_used) m_DIBbits=directx_output.Preupdate((unsigned char *)m_DIBbits);
			ReadCopyRect(&surh);
			break;
		case rfbEncodingCache:
			if (directx_used) m_DIBbits=directx_output.Preupdate((unsigned char *)m_DIBbits);
			ReadCacheRect(&surh);
			break;
		case rfbEncodingCacheZip:
			if (directx_used) m_DIBbits=directx_output.Preupdate((unsigned char *)m_DIBbits);
			ReadCacheZip(&surh,&UpdateRegion);
			break;
		case rfbEncodingSolMonoZip:
			if (directx_used) m_DIBbits=directx_output.Preupdate((unsigned char *)m_DIBbits);
			ReadSolMonoZip(&surh,&UpdateRegion);
			break;
		case rfbEncodingRRE:
			if (directx_used) m_DIBbits=directx_output.Preupdate((unsigned char *)m_DIBbits);
			SaveArea(cacherect);
			ReadRRERect(&surh);
			EncodingStatusWindow=rfbEncodingRRE;
			break;
		case rfbEncodingCoRRE:
			if (directx_used) m_DIBbits=directx_output.Preupdate((unsigned char *)m_DIBbits);
			SaveArea(cacherect);
			ReadCoRRERect(&surh);
			EncodingStatusWindow=rfbEncodingCoRRE;
			break;
		case rfbEncodingZlib:
			if (directx_used) m_DIBbits=directx_output.Preupdate((unsigned char *)m_DIBbits);
			SaveArea(cacherect);
			ReadZlibRect(&surh,0);
			EncodingStatusWindow=rfbEncodingZlib;
			break;
		case rfbEncodingZlibHex:
			if (directx_used) m_DIBbits=directx_output.Preupdate((unsigned char *)m_DIBbits);
			SaveArea(cacherect);
			ReadZlibHexRect(&surh);
			EncodingStatusWindow=rfbEncodingZlibHex;
			break;
		case rfbEncodingXOR_Zlib:
			if (directx_used) m_DIBbits=directx_output.Preupdate((unsigned char *)m_DIBbits);
			SaveArea(cacherect);
			ReadZlibRect(&surh,1);
			break;
		case rfbEncodingXORMultiColor_Zlib:
			if (directx_used) m_DIBbits=directx_output.Preupdate((unsigned char *)m_DIBbits);
			SaveArea(cacherect);
			ReadZlibRect(&surh,2);
			break;
		case rfbEncodingXORMonoColor_Zlib:
			if (directx_used) m_DIBbits=directx_output.Preupdate((unsigned char *)m_DIBbits);
			SaveArea(cacherect);
			ReadZlibRect(&surh,3);
			break;
		case rfbEncodingSolidColor:
			if (directx_used) m_DIBbits=directx_output.Preupdate((unsigned char *)m_DIBbits);
			SaveArea(cacherect);
			ReadSolidRect(&surh);
			break;
		case rfbEncodingZRLE:
			if (directx_used) m_DIBbits=directx_output.Preupdate((unsigned char *)m_DIBbits);
			zywrle = 0;
		case rfbEncodingZYWRLE:
			if (directx_used) m_DIBbits=directx_output.Preupdate((unsigned char *)m_DIBbits);
			SaveArea(cacherect);
			zrleDecode(surh.r.x, surh.r.y, surh.r.w, surh.r.h);
			EncodingStatusWindow=zywrle ? rfbEncodingZYWRLE : rfbEncodingZRLE;
			break;
#ifdef _XZ
		case rfbEncodingXZ:
			xzyw = 0;
		case rfbEncodingXZYW:			
			EncodingStatusWindow=xzyw ? rfbEncodingXZYW : rfbEncodingXZ;
			{
				CheckBufferSize(0x20000);
				int nAllRects = (surh.r.x << 16) | surh.r.y;
				int nDataLength = (surh.r.w << 16) | surh.r.h;

				if (!fis->GetReadFromMemoryBuffer())
				{
					m_nXZReadSize = nDataLength;
					assert(m_nXZReadSize > 0);
					CheckXZNetRectBufferSize((int)m_nXZReadSize);
					CheckBufferSize((int)m_nXZReadSize+4);
					ReadExact((char*)(m_pXZNetRectBuf), (int)(m_nXZReadSize));
					fis->SetReadFromMemoryBuffer(m_nXZReadSize, (char*)(m_pXZNetRectBuf));
				}

				xzis->setUnderlying(fis, nDataLength);

				std::vector<rfbRectangle> rects;

				for (int iRect = 0; iRect < nAllRects; iRect++) {
					rfbRectangle rectangle;
					xzis->readBytes(&rectangle, sz_rfbRectangle);

					rectangle.x = Swap16IfLE(rectangle.x);
					rectangle.y = Swap16IfLE(rectangle.y);
					rectangle.w = Swap16IfLE(rectangle.w);
					rectangle.h = Swap16IfLE(rectangle.h);	

					rects.push_back(rectangle);
				}

				for (std::vector<rfbRectangle>::const_iterator it = rects.begin(); it != rects.end(); it++) {
					const rfbRectangle& rect(*it);

					RECT invalid_rect;
					invalid_rect.left   = rect.x;
					invalid_rect.top    = rect.y;
					invalid_rect.right  = rect.x + rect.w ;
					invalid_rect.bottom = rect.y + rect.h; 
					
					SoftCursorLockArea(rect.x, rect.y, rect.w, rect.h);

					SaveArea(invalid_rect);
					
					xzDecode(rect.x, rect.y, rect.w, rect.h);

					InvalidateScreenRect(&invalid_rect); 

					SoftCursorUnlockScreen();
				}

				iCurrentRect += rects.size() - 1;
			}
			break;
#endif
		case rfbEncodingTight:
			if (directx_used) m_DIBbits=directx_output.Preupdate((unsigned char *)m_DIBbits);
			SaveArea(cacherect);
			ReadTightRect(&surh);
			EncodingStatusWindow=rfbEncodingTight;
			break;
		default:
			// vnclog.Print(0, _T("Unknown encoding %d - not supported!\n"), surh.encoding);
			// Try to empty buffer...
			// so next update should be back in sync
			BYTE * buffer;
			int ii=0;
			while (TRUE)
			{
				int aantal=fis->Check_if_buffer_has_data();
				if (aantal>0) buffer = new BYTE [aantal];
				if (aantal>0)
				{
					ii=0;
					ReadExact(((char *) buffer), aantal);
					delete [] buffer;
					Sleep(5);
				}
				else if (aantal==0)
				{
					if (ii==5) break;
					Sleep(200);
					ii++;
				}
				else break;
			}
			vnclog.Print(0, _T("Buffer cleared, sync should be back OK..Continue \n"));
			Recover_from_sync=true;
			break;
		}
		if (Recover_from_sync)
		{
		    Recover_from_sync=false;
			break;
		}

		//Todo: surh.encoding != rfbEncodingXZ && surh.encoding != rfbEncodingXZYW && 
		if (surh.encoding !=rfbEncodingNewFBSize && surh.encoding != rfbEncodingCacheZip && surh.encoding != rfbEncodingSolMonoZip && surh.encoding != rfbEncodingUltraZip)
		{
			RECT rect;
			rect.left   = surh.r.x;
			rect.top    = surh.r.y;
			rect.right  = surh.r.x + surh.r.w ;
			rect.bottom = surh.r.y + surh.r.h;

			if (!m_opts.m_Directx)InvalidateRegion(&rect,&UpdateRegion);
				//InvalidateScreenRect(&rect);
		}
		else if (surh.encoding !=rfbEncodingNewFBSize)
		{
			/*if (!directx_used)
				InvalidateRgn(m_hwndcn, UpdateRegion, FALSE);
			HRGN tempregion=CreateRectRgn(0,0,0,0);
			CombineRgn(UpdateRegion,UpdateRegion,tempregion,RGN_AND);
			DeleteObject(tempregion);*/
		}

		if (m_TrafficMonitor)
		{
			hdcX = GetDC(m_TrafficMonitor);
			hdcBits = CreateCompatibleDC(hdcX);
			HGDIOBJ hbrOld=SelectObject(hdcBits,m_bitmapNONE);
			BitBlt(hdcX,4,2,22,20,hdcBits,0,0,SRCCOPY);
			SelectObject(hdcBits,hbrOld);
			DeleteDC(hdcBits);
			ReleaseDC(m_TrafficMonitor,hdcX);
		}

		SoftCursorUnlockScreen();
	}

	if (m_opts.m_Directx)
	{
		InvalidateRect(m_hwndcn, NULL, TRUE);
	}
	else
	{
		InvalidateRgn(m_hwndcn, UpdateRegion, FALSE);
		HRGN tempregion=CreateRectRgn(0,0,0,0);
		CombineRgn(UpdateRegion,UpdateRegion,tempregion,RGN_AND);
		DeleteObject(tempregion);
	}

	if (!fTimingAlreadyStopped)
	{
		fis->stopTiming();
		kbitsPerSecond = fis->kbitsPerSecond();
		if (avg_kbitsPerSecond==0) avg_kbitsPerSecond=kbitsPerSecond;
		else
		{
		if (kbitsPerSecond>= avg_kbitsPerSecond/2 && kbitsPerSecond<=avg_kbitsPerSecond*3/2) avg_kbitsPerSecond=(avg_kbitsPerSecond+kbitsPerSecond)/2;
		else avg_kbitsPerSecond=(3*avg_kbitsPerSecond+kbitsPerSecond)/4;
		}
	}

	// sf@2002
	// We only change the preferred encoding if FileTransfer is not running and if
	// the last encoding change occured more than 30s ago
	if (avg_kbitsPerSecond !=0 && m_opts.autoDetect && !m_pFileTransfer->m_fFileTransferRunning && (timeGetTime() - m_lLastChangeTime) > m_lLastChangeTimeTimeout)
	{
		//Beep(1000,1000);
		m_lLastChangeTimeTimeout=60000;  // set to 1 minutes
		int nOldServerScale = m_nServerScale;

		for (std::vector<int>::iterator it = m_opts.m_PreferredEncodings.begin(); it != m_opts.m_PreferredEncodings.end(); ++it) {

			int encoding = *it;
			break;
		}

		if (avg_kbitsPerSecond > 10000 && (m_nConfig != 1))
		{
			m_nConfig = 1;
			int encoding = 0;
			for (std::vector<int>::iterator it = m_opts.m_PreferredEncodings.begin(); it != m_opts.m_PreferredEncodings.end(); ++it) {
				encoding = *it;
				break;
			}
			m_opts.m_PreferredEncodings.clear();
			if (new_ultra_server) m_opts.m_PreferredEncodings.push_back(rfbEncodingUltra2);
			else m_opts.m_PreferredEncodings.push_back(rfbEncodingHextile);
			//m_opts.m_Use8Bit = rfbPFFullColors;			
			if (new_ultra_server && encoding == rfbEncodingUltra2 && m_opts.m_fEnableCache == false){}
			else if (encoding == rfbEncodingHextile && m_opts.m_fEnableCache == false){}
			else m_pendingFormatChange = true;

			m_opts.m_fEnableCache = false;
			m_lLastChangeTime = timeGetTime();
		}
		else if (avg_kbitsPerSecond < 10000 && avg_kbitsPerSecond > 256 && (m_nConfig != 2))
		{
			m_nConfig = 1;			
			if (new_ultra_server) m_opts.m_PreferredEncodings.push_back(rfbEncodingUltra2);
			else m_opts.m_PreferredEncodings.push_back(rfbEncodingZRLE); //rfbEncodingZlibHex;
			//m_opts.m_Use8Bit = rfbPFFullColors; // Max colors
			m_opts.m_fEnableCache = false;
			m_pendingFormatChange = true;
			m_lLastChangeTime = timeGetTime();
		}
		// Medium connection speed
		else if (avg_kbitsPerSecond < 256 && avg_kbitsPerSecond > 128 && (m_nConfig != 2))
		{
			m_nConfig = 2;
			m_opts.m_PreferredEncodings.clear();
			m_opts.m_PreferredEncodings.push_back(rfbEncodingZRLE); //rfbEncodingZlibHex;
			//m_opts.m_Use8Bit = rfbPF256Colors;
			m_opts.m_fEnableCache = false;
			m_pendingFormatChange = true;
			m_lLastChangeTime = timeGetTime();
		}
		// Modem (including cable modem) connection speed
		else if (avg_kbitsPerSecond < 128 && avg_kbitsPerSecond > 19 && (m_nConfig != 3))
		{
			m_nConfig = 3;
			m_opts.m_PreferredEncodings.clear();
			m_opts.m_PreferredEncodings.push_back(rfbEncodingTight); // rfbEncodingZRLE;
			//m_opts.m_Use8Bit = rfbPF64Colors;
			m_opts.m_fEnableCache = false;
			m_pendingFormatChange = true;
			m_lLastChangeTime = timeGetTime();
		}

		// Slow Modem connection speed
		// Not sure it's a good thing in Auto mode...because in some cases
		// (CTRL-ALT-DEL, initial screen loading, connection short hangups...)
		// the speed can be momentary VERY slow. The fast fuzzy/normal modes switching
		// can be quite disturbing and useless in these situations.
		else if (avg_kbitsPerSecond < 19 && avg_kbitsPerSecond > 5 && (m_nConfig != 4))
		{
			m_nConfig = 4;
			m_opts.m_PreferredEncodings.clear();
			m_opts.m_PreferredEncodings.push_back(rfbEncodingTight); //rfbEncodingZRLE;
			//m_opts.m_Use8Bit = rfbPF8Colors;
			m_opts.m_fEnableCache = false;
			m_pendingFormatChange = true;
		}
	}

	// Inform the other thread that an update is needed.

	//adzm 2010-07-04
	if (!bSentUpdateRequest) {
		//adzm 2010-09 - We can simply call SendAppropriateFramebufferUpdateRequest now, with a true bAsync param so the request is posted rather than sent.
		SendAppropriateFramebufferUpdateRequest(true);
	}
	DeleteObject(UpdateRegion);
}

void ClientConnection::SetDormant(int newstate)
{
	vnclog.Print(5, _T("%s dormant mode\n"), newstate ? _T("Entering") : _T("Leaving"));
	m_dormant = newstate;
	if (m_dormant!=1 && m_running) {
		//adzm 2010-09
		SendIncrementalFramebufferUpdateRequest(false);
	}
}

// The server has copied some text to the clipboard - put it
// in the local clipboard too.

void ClientConnection::ReadServerCutText()
{
	rfbServerCutTextMsg sctm;
	vnclog.Print(6, _T("Read remote clipboard change\n"));
	ReadExact(((char *) &sctm)+m_nTO, sz_rfbServerCutTextMsg-m_nTO);
	int len = Swap32IfLE(sctm.length);

	// adzm - 2010-07 - Extended clipboard
	if (!m_clipboard.settings.m_bSupportsEx && len < 0) {
		m_clipboard.settings.m_bSupportsEx = true;
	}

	if (len < 0 && m_clipboard.settings.m_bSupportsEx) {
		vnclog.Print(6, _T("Read remote extended clipboard change\n"));
		len = abs(len);

		ExtendedClipboardDataMessage extendedClipboardDataMessage;

		extendedClipboardDataMessage.EnsureBufferLength(len, false);

		ReadExact((char*)extendedClipboardDataMessage.GetBuffer(), len);

		DWORD action = extendedClipboardDataMessage.GetFlags() & clipActionMask;

		// clipCaps may be combined with other actions
		if (action & clipCaps) {
			action = clipCaps;
		}

		switch(action) {
			case clipCaps:
				{
					omni_mutex_lock l(m_clipMutex);
					m_clipboard.settings.HandleCapsPacket(extendedClipboardDataMessage, false);

					//adzm 2010-09
					//UpdateRemoteClipboardCaps();
					PostMessage(m_hwndcn, WM_UPDATEREMOTECLIPBOARDCAPS, NULL, NULL);
				}
				break;
			case clipProvide:
				{
					{
						if (!m_opts.m_DisableClipboard && !m_opts.m_ViewOnly) {
							omni_mutex_lock l(m_clipMutex);

							ClipboardData newClipboard;

							if (newClipboard.Restore(m_hwndcn, extendedClipboardDataMessage)) {
								vnclog.Print(6, _T("Successfuly restored new clipboard data\n"));
								m_clipboard.m_crc = newClipboard.m_crc;
								m_clipboard.m_notifiedRemoteFormats = 0;
							} else {
								vnclog.Print(6, _T("Failed to set new clipboard data\n"));
							}
						}
					}
				}
				break;
			case clipNotify:
				{
					omni_mutex_lock l(m_clipMutex);
					m_clipboard.m_notifiedRemoteFormats = (extendedClipboardDataMessage.GetFlags() & clipFormatMask);
				}
				break;
			case clipRequest:
				// no need for server to request anything at the moment.
				break;
			case clipPeek:		// irrelevant coming from server
			default:
				// unsupported or not implemented
				break;
		}
	} else {
		if (len < 0) {
			vnclog.Print(6, _T("Invalid clipboard data!\n"));
			return;
		}
		CheckBufferSize(len);
		if (len == 0) {
			m_netbuf[0] = '\0';
		} else {
			ReadString(m_netbuf, len);
		}
		UpdateLocalClipboard(m_netbuf, len);
	}
}

void ClientConnection::ReadBell()
{
	rfbBellMsg bm;
	ReadExact(((char *) &bm)+m_nTO, sz_rfbBellMsg-m_nTO);

	#ifdef UNDER_CE
	MessageBeep( MB_OK );
	#else

	if (! ::PlaySound("VNCViewerBell", NULL,
		SND_APPLICATION | SND_ALIAS | SND_NODEFAULT | SND_ASYNC) ) {
		::Beep(440, 125);
	}
	#endif
	if (m_opts.m_DeiconifyOnBell) {
		if (IsIconic(m_hwndcn)) {
			SetDormant(false);
			ShowWindow(m_hwndcn, SW_SHOWNORMAL);
		}
	}
	vnclog.Print(6, _T("Bell!\n"));
}
void ClientConnection::ReadServerState()
{
    rfbServerStateMsg ss;
    memset(&ss, 0, sizeof ss);

    vnclog.Print(1, _T("ServerState- reading %u bytes\n"), sz_rfbServerStateMsg-m_nTO);

    ReadExact(((char *) &ss)+m_nTO, sz_rfbServerStateMsg-m_nTO);
    CARD32 state = Swap32IfLE(ss.state);
    CARD32 value = Swap32IfLE(ss.value);

    vnclog.Print(1, _T("ServerState (%u, %u)\n"), state, value);
    switch (state)
    {
    case rfbServerRemoteInputsState:
        m_remote_mouse_disable = (value == rfbServerState_Disabled) ? true : false;
        vnclog.Print(1, _T("New input state %u"), m_remote_mouse_disable);
        break;

    case rfbKeepAliveInterval:
        m_opts.m_keepAliveInterval = value;
        if (m_opts.m_keepAliveInterval >= (m_opts.m_FTTimeout - KEEPALIVE_HEADROOM))
          m_opts.m_keepAliveInterval = (m_opts.m_FTTimeout  - KEEPALIVE_HEADROOM);
        vnclog.Print(1, _T("New keepalive interval %u"), m_opts.m_keepAliveInterval);
		m_keepalive_timer = 1011;
		KillTimer(m_hwndcn, m_keepalive_timer);
		if (m_opts.m_keepAliveInterval > 0) {
			SetTimer(m_hwndcn, m_keepalive_timer, m_opts.m_keepAliveInterval * 1000, NULL);
		} else {
			m_keepalive_timer = 0;
		}
        break;

	case rfbIdleInputTimeout:
		m_opts.m_IdleInterval = value;		
		vnclog.Print(1, _T("New IdleTiler interval %u"), m_opts.m_IdleInterval);
		m_idle_timer = 1012;
		m_idle_time = value * 1000;
		KillTimer(m_hwndcn, m_idle_timer);
		KillTimer(m_hwndcn, 1013);
		if (m_opts.m_IdleInterval > 0) {
			SetTimer(m_hwndcn, m_idle_timer, m_idle_time, NULL);			
		}
		else {
			m_idle_timer = 0;
		}
		break;

    default:
        vnclog.Print(1, _T("Ignoring unsupported state %u"), state);
        break;
    }
}

// General utilities -------------------------------------------------

// Reads the number of bytes specified into the buffer given
void ClientConnection::ReadExact(char *inbuf, int wanted)
{
	if (wanted == 0) {
		return;
	}
	//omni_mutex_lock l(m_readMutex);
	// Status window and connection activity updates
	// We comment this because it just takes too much time to the viewer thread
	/*
	HDC hdcX,hdcBits;
	if (m_TrafficMonitor)
	{
		hdcX = GetDC(m_TrafficMonitor);
		hdcBits = CreateCompatibleDC(hdcX);
		SelectObject(hdcBits,m_bitmapBACK);
		BitBlt(hdcX,1,1,22,20,hdcBits,0,0,SRCCOPY);
		DeleteDC(hdcBits);
		ReleaseDC(m_TrafficMonitor,hdcX);
	}
	*/

	// m_BytesRead += wanted;
	/*
	m_BytesRead = fis->GetBytesRead();
	SetDlgItemInt(m_hwndStatus, IDC_RECEIVED, m_BytesRead, false);
	SetDlgItemInt(m_hwndStatus, IDC_SPEED, kbitsPerSecond, false);
	*/
	try
	{
		// sf@2002 - DSM Plugin
		if (m_fUsePlugin)
		{
			if (m_pDSMPlugin->IsEnabled())
			{
				//omni_mutex_lock l(m_pDSMPlugin->m_RestMutex);
				//adzm - 2009-06-21
				omni_mutex_conditional_lock l(m_pDSMPlugin->m_RestMutex, m_pPluginInterface ? false : true);

				// If we must read already restored data from memory
				if (m_fReadFromNetRectBuf)
				{
					memcpy(inbuf, m_pNetRectBuf + m_nNetRectBufOffset, wanted);
					m_nNetRectBufOffset += wanted;
					if (m_nNetRectBufOffset == m_nReadSize)
					{
						// Next ReadExact calls should read the socket
						m_fReadFromNetRectBuf = false;
						m_nNetRectBufOffset = 0;
					}
				}
				// Read restored data from ZRLE mem netbuffer
				else if (fis->GetReadFromMemoryBuffer())
				{
					fis->readBytes(inbuf, wanted);
				}
				else // read tansformed data from the socket (normal case)
				{
					// Get the DSMPlugin destination buffer where to put transformed incoming data
					// The number of bytes to read calculated from bufflen is given back in nTransDataLen
					int nTransDataLen = 0;
					BYTE* pTransBuffer = RestoreBufferStep1(NULL, wanted, &nTransDataLen);
					if (pTransBuffer == NULL)
					{
						// m_pDSMPlugin->RestoreBufferUnlock();
						throw WarningException(sz_L65);
					}

					// Read bytes directly into Plugin Dest rest. buffer
					fis->readBytes(pTransBuffer, nTransDataLen);

					// Ask plugin to restore data from its local rest. buffer into inbuf
					int nRestDataLen = 0;
					RestoreBufferStep2((BYTE*)inbuf, nTransDataLen, &nRestDataLen);

					// Check if we actually get the real original data length
					if (nRestDataLen != wanted)
					{
						throw WarningException(sz_L66);
					}
				}
			}
			else
			{
				fis->readBytes(inbuf, wanted);
			}
		}
		else
		{
			fis->readBytes(inbuf, wanted);
		}
	}
	catch (rdr::Exception& e)
	{
		vnclog.Print(0, "rdr::Exception (2): %s\n",e.str());
		if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L67);
		throw ErrorException(sz_L69);
	}

	// Too slow !
	/*
	if (m_TrafficMonitor)
	{
		hdcX = GetDC(m_TrafficMonitor);
		hdcBits = CreateCompatibleDC(hdcX);
		SelectObject(hdcBits,m_bitmapNONE);
		BitBlt(hdcX,1,1,22,20,hdcBits,0,0,SRCCOPY);
		DeleteDC(hdcBits);
		ReleaseDC(m_TrafficMonitor,hdcX);
	}
	*/
}

//adzm 2009-06-21
void ClientConnection::ReadExactProtocolVersion(char *inbuf, int wanted, bool& fNotEncrypted)
{
	fNotEncrypted = false;
	//omni_mutex_lock l(m_readMutex);
	// Status window and connection activity updates
	// We comment this because it just takes too much time to the viewer thread
	/*
	HDC hdcX,hdcBits;
	if (m_TrafficMonitor)
	{
		hdcX = GetDC(m_TrafficMonitor);
		hdcBits = CreateCompatibleDC(hdcX);
		SelectObject(hdcBits,m_bitmapBACK);
		BitBlt(hdcX,1,1,22,20,hdcBits,0,0,SRCCOPY);
		DeleteDC(hdcBits);
		ReleaseDC(m_TrafficMonitor,hdcX);
	}
	*/

	// m_BytesRead += wanted;
	/*
	m_BytesRead = fis->GetBytesRead();
	SetDlgItemInt(m_hwndStatus, IDC_RECEIVED, m_BytesRead, false);
	SetDlgItemInt(m_hwndStatus, IDC_SPEED, kbitsPerSecond, false);
	*/
	try
	{
		// sf@2002 - DSM Plugin
		if (m_fUsePlugin)
		{
			if (m_pDSMPlugin->IsEnabled())
			{
				//omni_mutex_lock l(m_pDSMPlugin->m_RestMutex);
				//adzm - 2009-06-21
				omni_mutex_conditional_lock l(m_pDSMPlugin->m_RestMutex, m_pPluginInterface ? false : true);

				// If we must read already restored data from memory
				if (m_fReadFromNetRectBuf)
				{
					memcpy(inbuf, m_pNetRectBuf + m_nNetRectBufOffset, wanted);
					m_nNetRectBufOffset += wanted;
					if (m_nNetRectBufOffset == m_nReadSize)
					{
						// Next ReadExact calls should read the socket
						m_fReadFromNetRectBuf = false;
						m_nNetRectBufOffset = 0;
					}
				}
				// Read restored data from ZRLE mem netbuffer
				else if (fis->GetReadFromMemoryBuffer())
				{
					fis->readBytes(inbuf, wanted);
				}
				else // read tansformed data from the socket (normal case)
				{
					//adzm 2009-06-21
					// first just read 4 bytes and see if they are 'RFB '.
					// if so, then this does not appear to be encrypted.
					char testBuffer[4];
					fis->readBytes(testBuffer, 4);
					if (memcmp(testBuffer, "RFB ", 4) == 0) {
						// not encrypted!
						fNotEncrypted = true;
						memcpy(inbuf, testBuffer, 4);
						fis->readBytes(inbuf+4, wanted-4);
						return;
					}

					//adzm 2010-05-11
					// if we have an encrypted start, and we are currently using an integrated inteface, reset
					// and load the classic interface instead.
					if (m_pIntegratedPluginInterface) {
						// release the integrated interface
						delete m_pIntegratedPluginInterface;
						m_pIntegratedPluginInterface = NULL;
						m_pPluginInterface = NULL;

						// and load the classic interface
						m_pPluginInterface = m_pDSMPlugin->CreatePluginInterface();
					}

					// Get the DSMPlugin destination buffer where to put transformed incoming data
					// The number of bytes to read calculated from bufflen is given back in nTransDataLen
					int nTransDataLen = 0;
					BYTE* pTransBuffer = RestoreBufferStep1(NULL, wanted, &nTransDataLen);
					if (pTransBuffer == NULL)
					{
						// m_pDSMPlugin->RestoreBufferUnlock();
						throw WarningException(sz_L65);
					}

					// Read bytes directly into Plugin Dest rest. buffer
					//adzm 2009-06-21 - we already got 4 bytes
					memcpy(pTransBuffer, testBuffer, 4);
					fis->readBytes(pTransBuffer+4, nTransDataLen-4);

					// Ask plugin to restore data from its local rest. buffer into inbuf
					int nRestDataLen = 0;
					RestoreBufferStep2((BYTE*)inbuf, nTransDataLen, &nRestDataLen);

					// Check if we actually get the real original data length
					if (nRestDataLen != wanted)
					{
						throw WarningException(sz_L66);
					}
				}
			}
			else
			{
				fis->readBytes(inbuf, wanted);
			}
		}
		else
		{
			fis->readBytes(inbuf, wanted);
		}
	}
	//adzm 2009-07-05
	catch (rdr::EndOfStream& e)
	{
		throw e;
	}
	catch (rdr::Exception& e)
	{
		vnclog.Print(0, "rdr::Exception (2): %s\n",e.str());
		if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L67);
		throw ErrorException(sz_L69);
	}

	// Too slow !
	/*
	if (m_TrafficMonitor)
	{
		hdcX = GetDC(m_TrafficMonitor);
		hdcBits = CreateCompatibleDC(hdcX);
		SelectObject(hdcBits,m_bitmapNONE);
		BitBlt(hdcX,1,1,22,20,hdcBits,0,0,SRCCOPY);
		DeleteDC(hdcBits);
		ReleaseDC(m_TrafficMonitor,hdcX);
	}
	*/
}

void ClientConnection::ReadExactProxy(char *inbuf, int wanted)
{
	if (wanted == 0) {
		return;
	}
	//omni_mutex_lock l(m_readMutex);
	// Status window and connection activity updates
	// We comment this because it just takes too much time to the viewer thread

	try
	{
		{
			fis->readBytes(inbuf, wanted);
		}
	}
	catch (rdr::Exception& e)
	{
		vnclog.Print(0, "rdr::Exception (2): %s\n",e.str());
		if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L67);
		throw QuietException(e.str());
	}
}

// Read the number of bytes and return them zero terminated in the buffer
/*inline*/ void ClientConnection::ReadString(char *buf, int length)
{
	if (length > 0)
		ReadExact(buf, length);
	buf[length] = '\0';
    vnclog.Print(10, _T("Read a %d-byte string\n"), length);
}
//NON BLOCKING
int
ClientConnection::Send(const char *buff, const unsigned int bufflen,int timeout)
{
	struct fd_set write_fds;
	struct timeval tm;
	int count;
	int aa=0;

	FD_ZERO(&write_fds);
	FD_SET((unsigned int)m_sock, &write_fds);
	tm.tv_sec = timeout;
	tm.tv_usec = 0;
	count = select(m_sock + 1, NULL, &write_fds, NULL, &tm);

	if (count == 0)
		return 0; //timeout
	if (count < 0 || count > 1)
		return -1;

	//adzm 2010-08-01
	m_LastSentTick = GetTickCount();

	if (FD_ISSET((unsigned int)m_sock, &write_fds)) aa=send(m_sock, buff, bufflen, 0);
	return aa;
}

//
// sf@2002 - DSM Plugin
//
void ClientConnection::WriteTransformed(char *buf, int bytes, CARD8 msgType, bool bQueue)
{
	// adzm 2010-09
	if (!m_fUsePlugin || m_fPluginStreamingOut)
	{
		//adzm 2010-09
		WriteTransformed(buf, bytes, bQueue);
	}
	else if (m_pDSMPlugin->IsEnabled())
	{
		// Send the transformed message type first
		//adzm 2010-09
		WriteTransformed((char*)&msgType, sizeof(msgType), bQueue);
		// Then send the transformed rfb message content
		//adzm 2010-09
		WriteTransformed(buf, bytes, bQueue);
	}
}

//adzm 2010-09
void ClientConnection::WriteExact(char *buf, int bytes, CARD8 msgType)
{
	WriteTransformed(buf, bytes, msgType, false);
}

//adzm 2010-09
void ClientConnection::WriteTransformed_timeout(char *buf, int bytes, CARD8 msgType,int timeout, bool bQueue)
{
	// adzm 2010-09
	if (!m_fUsePlugin || m_fPluginStreamingOut)
	{
		WriteTransformed_timeout(buf, bytes,timeout, bQueue);
	}
	else if (m_pDSMPlugin->IsEnabled())
	{
		// Send the transformed message type first
		WriteTransformed_timeout((char*)&msgType, sizeof(msgType),timeout, bQueue);
		// Then send the transformed rfb message content
		WriteTransformed_timeout(buf, bytes,timeout, bQueue);
	}
}

//adzm 2010-09
void ClientConnection::WriteExact_timeout(char *buf, int bytes, CARD8 msgType,int timeout)
{
	WriteTransformed_timeout(buf, bytes, msgType, timeout, false);
}

//adzm 2010-09
// Sends the number of bytes specified from the buffer
void ClientConnection::Write(char *buf, int bytes, bool bQueue, bool bTimeout, int timeout)
{
	omni_mutex_lock l(m_writeMutex);

	if (bytes == 0 && (bQueue || (!bQueue && m_nQueueBufferLength == 0))) return;

	// this will adjust buf and bytes to be < G_SENDBUFFER
	FlushOutstandingWriteQueue(buf, bytes, bTimeout, timeout);

	// append buf to any remaining data in the queue, since we know that m_nQueueBufferLength + bytes < G_SENDBUFFER
	if (bytes > 0) {
		memcpy(m_QueueBuffer + m_nQueueBufferLength, buf, bytes);
		m_nQueueBufferLength += bytes;
	}

	if (!bQueue) {
		int i = 0;
		int j = 0;
		while (i < m_nQueueBufferLength)
		{
			//adzm 2010-08-01
			m_LastSentTick = GetTickCount();
			if (bTimeout) {
				j = Send(m_QueueBuffer+i, m_nQueueBufferLength-i, timeout);
			} else {
				j = send(m_sock, m_QueueBuffer+i, m_nQueueBufferLength-i, 0);
			}
			if (j == SOCKET_ERROR || j==0)
			{
				LPVOID lpMsgBuf;
				int err = ::GetLastError();
				FormatMessage(
					FORMAT_MESSAGE_ALLOCATE_BUFFER |
					FORMAT_MESSAGE_FROM_SYSTEM |
					FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
					err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					(LPTSTR) &lpMsgBuf, 0, NULL ); // Process any inserts in lpMsgBuf.
				vnclog.Print(1, _T("Socket error %d: %s\n"), err, lpMsgBuf);
				LocalFree( lpMsgBuf );
				m_running = false;

				//throw WarningException(sz_L69);
			}
			i += j;
			m_BytesSend += j;
		}

		m_nQueueBufferLength = 0;
	}
}

// Sends the number of bytes specified from the buffer
void ClientConnection::FlushOutstandingWriteQueue(char*& buf2, int& bytes2, bool bTimeout, int timeout)
{
	omni_mutex_lock l(m_writeMutex);

	DWORD nNewSize = m_nQueueBufferLength + bytes2;

	while (nNewSize >= G_SENDBUFFER) {
		//adzm 2010-08-01
		m_LastSentTick = GetTickCount();

		int bufferFill = G_SENDBUFFER - m_nQueueBufferLength;

		// add anything from buf2 to the queued packet
		memcpy(m_QueueBuffer + m_nQueueBufferLength, buf2, bufferFill);

		// adjust buf2
		buf2 += bufferFill;
		bytes2 -= bufferFill;

		m_nQueueBufferLength = G_SENDBUFFER;

		int sent = 0;
		if (bTimeout) {
			sent = Send(m_QueueBuffer, G_SENDBUFFER, timeout);
		} else {
			sent = send(m_sock, m_QueueBuffer, G_SENDBUFFER, 0);
		}
		if (sent == SOCKET_ERROR || sent==0)
		{
			LPVOID lpMsgBuf;
			int err = ::GetLastError();
			FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
				err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf, 0, NULL ); // Process any inserts in lpMsgBuf.
			vnclog.Print(1, _T("Socket error %d: %s\n"), err, lpMsgBuf);
			LocalFree( lpMsgBuf );
			m_running = false;

			//throw WarningException(sz_L69);
		}

		// adjust our stats
		m_BytesSend += sent;

		// adjust the current queue
		nNewSize -= sent;
		m_nQueueBufferLength -= sent;
		// if not everything, move to the beginning of the queue
		if ((G_SENDBUFFER - sent) != 0) {
			memcpy(m_QueueBuffer, m_QueueBuffer + sent, G_SENDBUFFER - sent);
		}
	}
}

void ClientConnection::FlushWriteQueue(bool bTimeout, int timeout)
{
	Write(NULL, 0, false, bTimeout, timeout);
}

// Sends the number of bytes specified from the buffer
void ClientConnection::WriteTransformed(char *buf, int bytes, bool bQueue)
{
	if (bytes == 0) return;

	omni_mutex_lock l(m_writeMutex);
	//vnclog.Print(10, _T("  writing %d bytes\n"), bytes);

	// sf@2002 - DSM Plugin
	char *pBuffer = buf;
	if (m_fUsePlugin)
	{
		if (m_pDSMPlugin->IsEnabled())
		{
			int nTransDataLen = 0;
			pBuffer = (char*)TransformBuffer((BYTE*)buf, bytes, &nTransDataLen);
			if (pBuffer == NULL || (bytes > 0 && nTransDataLen == 0))
				throw WarningException(sz_L68);
			bytes = nTransDataLen;
		}
	}

	//adzm 2010-09
	Write(pBuffer, bytes, bQueue);
}

//adzm 2010-09
void ClientConnection::WriteExact(char *buf, int bytes)
{
	WriteTransformed(buf, bytes, false);
}

//adzm 2010-09
void ClientConnection::WriteQueue(char *buf, int bytes)
{
	Write(buf, bytes, true);
}

void ClientConnection::WriteExactQueue(char *buf, int bytes)
{
	WriteTransformed(buf, bytes, true);
}

void ClientConnection::WriteExactQueue(char *buf, int bytes, CARD8 msgType)
{
	WriteTransformed(buf, bytes, msgType, true);
}

void ClientConnection::WriteExactQueue_timeout(char *buf, int bytes,int timeout)
{
	WriteTransformed_timeout(buf, bytes, timeout, true);
}

void ClientConnection::WriteExactQueue_timeout(char *buf, int bytes, CARD8 msgType,int timeout)
{
	WriteTransformed_timeout(buf, bytes, msgType, timeout, true);
}

// Sends the number of bytes specified from the buffer
//adzm 2010-09
void ClientConnection::WriteTransformed_timeout(char *buf, int bytes,int timeout, bool bQueue)
{
	if (bytes == 0) return;

	omni_mutex_lock l(m_writeMutex);
	//vnclog.Print(10, _T("  writing %d bytes\n"), bytes);

	// sf@2002 - DSM Plugin
	char *pBuffer = buf;
	if (m_fUsePlugin)
	{
		if (m_pDSMPlugin->IsEnabled())
		{
			int nTransDataLen = 0;
			pBuffer = (char*)TransformBuffer((BYTE*)buf, bytes, &nTransDataLen);
			if (pBuffer == NULL || (bytes > 0 && nTransDataLen == 0))
				throw WarningException(sz_L68);
			bytes = nTransDataLen;
		}
	}

	Write_timeout(pBuffer, bytes, timeout, bQueue);
}

//adzm 2010-09
void ClientConnection::WriteExact_timeout(char *buf, int bytes, int timeout)
{
	WriteTransformed_timeout(buf, bytes, timeout, false);
}

//adzm 2010-09
void ClientConnection::Write_timeout(char *buf, int bytes,int timeout, bool bQueue)
{
	Write(buf, bytes, bQueue, true, timeout);
}

// Sends the number of bytes specified from the buffer
void ClientConnection::WriteExactProxy(char *buf, int bytes)
{
	//adzm 2010-09 - just call this function, it is named a bit more clearly than this one
	Write(buf, bytes, false);
}

// Security fix for uvnc 1.0.5 and 1.0.2 (should be ok for all version...)
// Replace the corresponding functions with the following fixed ones in vncviewer\ClientConnection.cpp file

// Makes sure netbuf is at least as big as the specified size.
// Note that netbuf itself may change as a result of this call.
// Throws an exception on failure.
void ClientConnection::CheckBufferSize(int bufsize)
{
	// sf@2009 - Sanity check
	if (bufsize < 0 || bufsize > 104857600) // 100 MBytes max
	{
		throw ErrorException(sz_L70);
	}

	if (m_netbufsize > bufsize) return;

	omni_mutex_lock l(m_bufferMutex);

	char *newbuf = new char[bufsize+256];
	if (newbuf == NULL) {
		throw ErrorException(sz_L70);
	}

	// Only if we're successful...

	if (m_netbuf != NULL)
		delete [] m_netbuf;
	m_netbuf = newbuf;
	m_netbufsize=bufsize + 256;
	vnclog.Print(4, _T("bufsize expanded to %d\n"), m_netbufsize);
}

// Makes sure zipbuf is at least as big as the specified size.
// Note that zlibbuf itself may change as a result of this call.
// Throws an exception on failure.
// sf@2002
void ClientConnection::CheckZipBufferSize(int bufsize)
{
	// sf@2009 - Sanity check
	if (bufsize < 0 || bufsize > 104857600) // 100 MBytes max
	{
		throw ErrorException(sz_L71);
	}

	unsigned char *newbuf;

	if (m_zipbufsize > bufsize) return;

	omni_mutex_lock l(m_ZipBufferMutex);

	newbuf = (unsigned char *)new char[bufsize + 256];
	if (newbuf == NULL) {
		throw ErrorException(sz_L71);
	}

	// Only if we're successful...

	if (m_zipbuf != NULL)
		delete [] m_zipbuf;
	m_zipbuf = newbuf;
	m_zipbufsize = bufsize + 256;
	vnclog.Print(4, _T("zipbufsize expanded to %d\n"), m_zipbufsize);
}

void ClientConnection::CheckFileZipBufferSize(int bufsize)
{
	// sf@2009 - Sanity check
	if (bufsize < 0 || bufsize > 104857600) // 100 MBytes max
	{
		throw ErrorException(sz_L71);
	}

	unsigned char *newbuf;

	if (m_filezipbufsize > bufsize) return;

	omni_mutex_lock l(m_FileZipBufferMutex);

	newbuf = (unsigned char *)new char[bufsize + 256];
	if (newbuf == NULL) {
		throw ErrorException(sz_L71);
	}

	// Only if we're successful...

	if (m_filezipbuf != NULL)
		delete [] m_filezipbuf;
	m_filezipbuf = newbuf;
	m_filezipbufsize = bufsize + 256;
	vnclog.Print(4, _T("zipbufsize expanded to %d\n"), m_filezipbufsize);
}

void ClientConnection::CheckFileChunkBufferSize(int bufsize)
{
	// sf@2009 - Sanity check
	if (bufsize < 0 || bufsize > 104857600) // 100 MBytes max
	{
		throw ErrorException(sz_L71);
	}

	unsigned char *newbuf;

	if (m_filechunkbufsize > bufsize) return;

	omni_mutex_lock l(m_FileChunkBufferMutex);

	newbuf = (unsigned char *)new char[bufsize + 256];
	if (newbuf == NULL) {
		throw ErrorException(sz_L71);
	}

	if (m_filechunkbuf != NULL)
		delete [] m_filechunkbuf;
	m_filechunkbuf = newbuf;
	m_filechunkbufsize = bufsize + 256;
	vnclog.Print(4, _T("m_filechunkbufsize expanded to %d\n"), m_filechunkbufsize);
}

// Processing NewFBSize pseudo-rectangle. Create new framebuffer of
// the size specified in pfburh->r.w and pfburh->r.h, and change the
// window size correspondingly.
//
void ClientConnection::ReadNewFBSize(rfbFramebufferUpdateRectHeader *pfburh)
{
	//adzm 2010-09 - Clear the cache first, since it tries to clear based on m_si which is changing!
	ClearCache();

	// [v1.0.2-jp1 fix]
	//m_si.framebufferWidth = pfburh->r.w;
	//m_si.framebufferHeight = pfburh->r.h;
	m_si.framebufferWidth = pfburh->r.w / m_nServerScale;
	m_si.framebufferHeight = pfburh->r.h / m_nServerScale;

	CreateLocalFramebuffer();
	//adzm 2010-09 - all socket writes must remain on a single thread, so do a synchronous update request
    SendFullFramebufferUpdateRequest(false);
	Createdib();
	m_pendingScaleChange = true;
	m_pendingFormatChange = true;
	//adzm 2010-09 - all socket writes must remain on a single thread, so do a synchronous update request
	SendAppropriateFramebufferUpdateRequest(false);
	SizeWindow();
	InvalidateRect(m_hwndcn, NULL, TRUE);
	RealiseFullScreenMode();
}

//
// sf@2002 - DSMPlugin
//

//
// Tell the plugin to do its transformation on the source data buffer
// Return: pointer on the new transformed buffer (allocated by the plugin)
// nTransformedDataLen is the number of bytes contained in the transformed buffer
//adzm - 2009-06-21
BYTE* ClientConnection::TransformBuffer(BYTE* pDataBuffer, int nDataLen, int* pnTransformedDataLen)
{
	if (m_pPluginInterface) {
		return m_pPluginInterface->TransformBuffer(pDataBuffer, nDataLen, pnTransformedDataLen);
	} else {
		return m_pDSMPlugin->TransformBuffer(pDataBuffer, nDataLen, pnTransformedDataLen);
	}
}

// - If pRestoredDataBuffer = NULL, the plugin check its local buffer and return the pointer
// - Otherwise, restore data contained in its rest. buffer and put the result in pRestoredDataBuffer
//   pnRestoredDataLen is the number bytes put in pRestoredDataBuffers
//adzm - 2009-06-21
BYTE* ClientConnection::RestoreBufferStep1(BYTE* pRestoredDataBuffer, int nDataLen, int* pnRestoredDataLen)
{
	if (m_pPluginInterface) {
		return m_pPluginInterface->RestoreBuffer(pRestoredDataBuffer, nDataLen, pnRestoredDataLen);
	} else {
		return m_pDSMPlugin->RestoreBufferStep1(pRestoredDataBuffer, nDataLen, pnRestoredDataLen);
	}
}

//adzm - 2009-06-21
BYTE* ClientConnection::RestoreBufferStep2(BYTE* pRestoredDataBuffer, int nDataLen, int* pnRestoredDataLen)
{
	if (m_pPluginInterface) {
		return m_pPluginInterface->RestoreBuffer(pRestoredDataBuffer, nDataLen, pnRestoredDataLen);
	} else {
		return m_pDSMPlugin->RestoreBufferStep2(pRestoredDataBuffer, nDataLen, pnRestoredDataLen);
	}
}

//
// Ensures that the temporary "alignement" buffer in large enough
//
inline void ClientConnection::CheckNetRectBufferSize(int nBufSize)
{
	if (m_nNetRectBufSize > nBufSize) return;

	omni_mutex_lock l(m_NetRectBufferMutex);

	BYTE *newbuf = new BYTE[nBufSize + 256];
	if (newbuf == NULL)
	{
		// Error
	}
	if (m_pNetRectBuf != NULL)
		delete [] m_pNetRectBuf;

	m_pNetRectBuf = newbuf;
	m_nNetRectBufSize = nBufSize + 256;
}

//
// Ensures that the temporary "alignement" buffer in large enough
//
inline void ClientConnection::CheckZRLENetRectBufferSize(int nBufSize)
{
	if (m_nZRLENetRectBufSize > nBufSize) return;

	omni_mutex_lock l(m_ZRLENetRectBufferMutex);

	BYTE *newbuf = new BYTE[nBufSize + 256];
	if (newbuf == NULL)
	{
		// Error
	}
	if (m_pZRLENetRectBuf != NULL)
		delete [] m_pZRLENetRectBuf;

	m_pZRLENetRectBuf = newbuf;
	m_nZRLENetRectBufSize = nBufSize + 256;
}

#ifdef _XZ
inline void ClientConnection::CheckXZNetRectBufferSize(int nBufSize)
{
	if (m_nXZNetRectBufSize > nBufSize) return;

	omni_mutex_lock l(m_XZNetRectBufferMutex);

	BYTE *newbuf = new BYTE[nBufSize + 256];
	if (newbuf == NULL) 
	{
		// Error
	}
	if (m_pXZNetRectBuf != NULL)
		delete [] m_pXZNetRectBuf;

	m_pXZNetRectBuf = newbuf;
	m_nXZNetRectBufSize = nBufSize + 256;
}
#endif

//
// Format file size so it is user friendly to read
//
void ClientConnection::GetFriendlySizeString(__int64 Size, char* szText)
{
	szText[0] = '\0';
	if( Size > (1024*1024*1024) )
	{
		__int64 lRest = (Size % (1024*1024*1024));
		Size /= (1024*1024*1024);
		wsprintf(szText,"%u.%4.4lu Gb", (unsigned long)Size, (unsigned long)((__int64)(lRest) * 10000 / 1024 / 1024 / 1024));
	}
	else if( Size > (1024*1024) )
	{
		unsigned long lRest = (Size % (1024*1024));
		Size /= (1024*1024);
		wsprintf(szText,"%u.%3.3lu Mb", (unsigned long)Size, (unsigned long)((__int64)(lRest) * 1000 / 1024 / 1024));
	}
	else if ( Size > 1024 )
	{
		unsigned long lRest = Size % (1024);
		Size /= 1024;
		wsprintf(szText,"%u.%2.2lu Kb", (unsigned long)Size, lRest * 100 / 1024);
	}
	else
	{
		wsprintf(szText,"%u bytes", (unsigned long)Size);
	}
}

//
// sf@2002
//
void ClientConnection::UpdateStatusFields()
{
	char szText[256];

	// Bytes Received
	m_BytesRead = fis->GetBytesRead();
	GetFriendlySizeString(m_BytesRead, szText);
	// SetDlgItemInt(m_hwndStatus, IDC_RECEIVED, m_BytesRead, false);
	if (m_hwndStatus)SetDlgItemText(m_hwndStatus, IDC_RECEIVED, szText);

	// Bytes Sent
	GetFriendlySizeString(m_BytesSend, szText);
	if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_SEND, szText);

	// Speed
	if (m_hwndStatus)SetDlgItemInt(m_hwndStatus, IDC_SPEED, avg_kbitsPerSecond, false);

	// Encoder
	if (m_fStatusOpen) // It's called by the status window timer... fixme
	{
		if (EncodingStatusWindow!=OldEncodingStatusWindow)
		{
			OldEncodingStatusWindow = EncodingStatusWindow;
			switch (EncodingStatusWindow)
			{
			case rfbEncodingRaw:
				if (m_hwndStatus)SetDlgItemText(m_hwndStatus, IDC_ENCODER, m_opts.m_fEnableCache ? "Raw, Cache" : "Raw");
				break;
			case rfbEncodingRRE:
				if (m_hwndStatus)SetDlgItemText(m_hwndStatus, IDC_ENCODER, m_opts.m_fEnableCache ? "RRE, Cache" : "RRE");
				break;
			case rfbEncodingCoRRE:
				if (m_hwndStatus)SetDlgItemText(m_hwndStatus, IDC_ENCODER, m_opts.m_fEnableCache ? "CoRRE, Cache" : "CoRRE");
				break;
			case rfbEncodingHextile:
				if (m_hwndStatus)SetDlgItemText(m_hwndStatus, IDC_ENCODER, m_opts.m_fEnableCache ? "Hextile, Cache" : "Hextile");
				break;
			case rfbEncodingUltra:
				if (m_hwndStatus)SetDlgItemText(m_hwndStatus, IDC_ENCODER, m_opts.m_fEnableCache ? "Ultra, Cache" : "Ultra");
				break;
			case rfbEncodingUltra2:
				if (m_hwndStatus)SetDlgItemText(m_hwndStatus, IDC_ENCODER, m_opts.m_fEnableCache ? "Ultra2, Cache" : "Ultra2");
				break;
			case rfbEncodingZlib:
				if (m_hwndStatus)SetDlgItemText(m_hwndStatus, IDC_ENCODER, m_opts.m_fEnableCache ? "XORZlib, Cache" : "XORZlib");
				break;
  			case rfbEncodingZRLE:
				if (m_hwndStatus)SetDlgItemText(m_hwndStatus, IDC_ENCODER, m_opts.m_fEnableCache ? "ZRLE, Cache" :"ZRLE");
  				break;
  			case rfbEncodingZYWRLE:
				if (m_hwndStatus)SetDlgItemText(m_hwndStatus, IDC_ENCODER, m_opts.m_fEnableCache ? "ZYWRLE, Cache" :"ZYWRLE");
  				break;
#ifdef _XZ
  			case rfbEncodingXZ:		
				if (m_hwndStatus)SetDlgItemText(m_hwndStatus, IDC_ENCODER, m_opts.m_fEnableCache ? "XZ, Cache" :"XZ");
  				break;
  			case rfbEncodingXZYW:		
				if (m_hwndStatus)SetDlgItemText(m_hwndStatus, IDC_ENCODER, m_opts.m_fEnableCache ? "XZYW, Cache" :"XZYW");
  				break;
#endif
			case rfbEncodingTight:
				if (m_hwndStatus)SetDlgItemText(m_hwndStatus, IDC_ENCODER, m_opts.m_fEnableCache ? "Tight, Cache" : "Tight");
				break;
			case rfbEncodingZlibHex:
				if (m_hwndStatus)SetDlgItemText(m_hwndStatus, IDC_ENCODER, m_opts.m_fEnableCache ? "ZlibHex, Cache" : "ZlibHex");
				break;
			}
		}
	}
	else
		OldEncodingStatusWindow = -1;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
////////////////////////////////////////////////

void ClientConnection::GTGBS_ScrollToolbar(int dx, int dy)
{
/*	dx = max(dx, -m_hScrollPos);
	dx = min(dx, m_hScrollMax-(m_cliwidth)-m_hScrollPos);
	dy = max(dy, -m_vScrollPos);
	dy = min(dy, m_vScrollMax-(m_cliheight)-m_vScrollPos);
	if (dx || dy) {
		RECT clirect;
		GetClientRect(m_hwndTBwin, &clirect);
		ScrollWindowEx(m_hwndTBwin, dx, dy, NULL, &clirect, NULL, NULL, SW_ERASE );
		DoBlit();
	}
*/
}

void ClientConnection::GTGBS_CreateDisplay()
{
	// Das eigendliche HauptFenster erstellen,
	// welches das VNC-Fenster und die Toolbar enthält
	WNDCLASS wndclass;

	wndclass.style			= 0;
	wndclass.lpfnWndProc	= ClientConnection::WndProc;
	wndclass.cbClsExtra		= 0;
	wndclass.cbWndExtra		= 0;
	wndclass.hInstance		= m_pApp->m_instance;
	wndclass.hIcon			= LoadIcon(m_pApp->m_instance, MAKEINTRESOURCE(IDR_TRAY));
	switch (m_opts.m_localCursor) {
	case NOCURSOR:
		wndclass.hCursor		= LoadCursor(m_pApp->m_instance, MAKEINTRESOURCE(IDC_NOCURSOR));
		break;
	case NORMALCURSOR:
		wndclass.hCursor		= LoadCursor(NULL, IDC_ARROW);
		break;
	case DOTCURSOR:
	default:
		wndclass.hCursor		= LoadCursor(m_pApp->m_instance, MAKEINTRESOURCE(IDC_DOTCURSOR));
	}
	wndclass.hbrBackground	= (HBRUSH) GetStockObject(BLACK_BRUSH);
    wndclass.lpszMenuName	= (const TCHAR *) NULL;
	wndclass.lpszClassName	= _T("VNCMDI_Window");

	RegisterClass(&wndclass);

#ifdef _WIN32_WCE
	const DWORD winstyle = WS_VSCROLL | WS_HSCROLL | WS_CAPTION | WS_SYSMENU;
#else
	const DWORD winstyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
	  WS_MINIMIZEBOX |WS_MAXIMIZEBOX | WS_THICKFRAME | WS_VSCROLL | WS_HSCROLL;
#endif

	m_hwndMain = CreateWindow(_T("VNCMDI_Window"),
			  _T("VNCviewer"),
			  winstyle,
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  //CW_USEDEFAULT,
			  //CW_USEDEFAULT,
			  320,200,
			  NULL,                // Parent handle
			  NULL,                // Menu handle
			  m_pApp->m_instance,
			  (LPVOID)this);
	helper::SafeSetWindowUserData(m_hwndMain, (LONG_PTR)this);

	/*LONG lExStyle = GetWindowLong(m_hwndMain, GWL_EXSTYLE);
	lExStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
	SetWindowLong(m_hwndMain, GWL_EXSTYLE, lExStyle);
	LONG lStyle = GetWindowLong(m_hwndMain, GWL_STYLE);
	lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
	SetWindowLong(m_hwndMain, GWL_STYLE, lStyle);
	SetWindowPos(m_hwndMain, NULL, 0,0,0,0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);*/
	// [v1.0.2-jp1 fix]
	ImmAssociateContext(m_hwndMain, NULL);    
}

//
//
//
LRESULT CALLBACK ClientConnection::GTGBS_ShowStatusWindow(LPVOID lpParameter)
{
	ClientConnection *_this = (ClientConnection*)lpParameter;

	 _this->m_fStatusOpen = true;
	DialogBoxParam(_this->m_pApp->m_instance,MAKEINTRESOURCE(IDD_STATUS),NULL,(DLGPROC)ClientConnection::GTGBS_StatusProc,(LONG_PTR)_this);
	// _this->m_fStatusOpen = false;
	return 0;
}

//
//
//
LRESULT CALLBACK ClientConnection::GTGBS_StatusProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    ClientConnection* _this = helper::SafeGetWindowUserData<ClientConnection>(hwnd);

	switch (iMsg)
	{
	case WM_INITDIALOG:
		{
			// sf@2002 - Make the window always on top
			RECT Rect;
			GetWindowRect(hwnd, &Rect);
			SetWindowPos(hwnd,
				HWND_TOPMOST,
				Rect.left,
				Rect.top,
				Rect.right - Rect.left,
				Rect.bottom - Rect.top,
				SWP_SHOWWINDOW);

			char wt[MAX_PATH];
			ClientConnection *_this = (ClientConnection *)lParam;
            helper::SafeSetWindowUserData(hwnd, lParam);

			SetDlgItemInt(hwnd,IDC_RECEIVED,_this->m_BytesRead,false);
			SetDlgItemInt(hwnd,IDC_SEND,_this->m_BytesSend,false);

			if (_this->m_host != NULL) {
				SetDlgItemText(hwnd,IDC_VNCSERVER,_this->m_host);
				sprintf(wt,"%s %s",sz_L72,_this->m_host);
				SetWindowText(hwnd,wt);
			} else {
				SetDlgItemText(hwnd,IDC_VNCSERVER,_T(""));
				SetWindowText(hwnd,sz_L73);
			}

			if(_this->m_port != NULL)
				SetDlgItemInt(hwnd,IDC_PORT,_this->m_port,FALSE);
			else
				SetDlgItemText(hwnd,IDC_PORT,_T(""));

			if(_this->m_sock != NULL )
			{
				if (_this->m_pDSMPlugin->IsEnabled())
				{
					char szMess[255];
					memset(szMess, 0, 255);
					sprintf(szMess, "%s (%s-v%s)",
							sz_L49,
							_this->m_pDSMPlugin->GetPluginName(),
							_this->m_pDSMPlugin->GetPluginVersion()
							);
					SetDlgItemText(hwnd,IDC_STATUS, szMess);

					//adzm 2010-05-10
					if (_this->m_pIntegratedPluginInterface) {
						SetDlgItemText(hwnd,IDC_PLUGIN_STATUS,_this->m_pIntegratedPluginInterface->DescribeCurrentSettings());
					} else if (_this->m_pPluginInterface) {
						SetDlgItemText(hwnd,IDC_PLUGIN_STATUS,"(plugin default encryption)");
					} else {
						if (_this->m_opts.m_oldplugin)
						{
							if (_stricmp(_this->m_pDSMPlugin->GetPluginParams(), "NoPassword")==0)
								SetDlgItemText(hwnd,IDC_PLUGIN_STATUS,"(old plugin, rc4.key, encryption)");
							else
								SetDlgItemText(hwnd,IDC_PLUGIN_STATUS,"(old plugin, encryption)");
						}
						else SetDlgItemText(hwnd,IDC_PLUGIN_STATUS,"(no encryption)");
					}

					//Problem, old plugins don't have xxPluginInterface
					// they always say no encryption
				}
				else
					SetDlgItemText(hwnd,IDC_STATUS,sz_L49);
			}
			else
			{
				SetDlgItemText(hwnd,IDC_STATUS,sz_L74);
			}

			//CentreWindow(hwnd);
			ShowWindow(hwnd,SW_SHOW);
			_this->m_hwndStatus = hwnd;
			if (_this->m_running) {
				//Normaler status
				ShowWindow(GetDlgItem(hwnd,IDQUIT),SW_HIDE);
				ShowWindow(GetDlgItem(hwnd,IDCLOSE),SW_SHOW);
				// sf@2002
				if (!_this->m_nStatusTimer)
					_this->m_nStatusTimer = SetTimer( hwnd, 3333, 1000, NULL);
			} else {
				//Verbindungsaufbau status
				ShowWindow(GetDlgItem(hwnd,IDQUIT),SW_SHOW);
				ShowWindow(GetDlgItem(hwnd,IDCLOSE),SW_HIDE);
				SetDlgItemText(hwnd,IDC_STATUS,sz_L43);
				HMENU hMenu = GetSystemMenu(hwnd,0);
				EnableMenuItem(hMenu,SC_CLOSE,MF_BYCOMMAND | MF_GRAYED);
			}

			return TRUE;
		}
	case WM_CLOSE:
		{
			EndDialog(hwnd, TRUE);
			return TRUE;
		}
	case WM_COMMAND:
		{
			if (LOWORD(wParam) == IDCLOSE) {
				EndDialog(hwnd, TRUE);
			}
			if (LOWORD(wParam) == IDQUIT) {
				forcedexit=true;
				_this->Pressed_Cancel=true;
				EndDialog(hwnd, TRUE);
			}

			/*if (LOWORD(wParam) == IDQUIT) {
				PostQuitMessage(0);
				ClientConnection *_this = (ClientConnection *) GetWindowLong(hwnd, GWL_USERDATA);
				_this->KillThread();

				//EndDialog(hwnd, TRUE);
			}*/
			return TRUE;
		}

		// sf@2002 - Every timer tic, we update the status values (speed, Sent, received, Encoder)
	case WM_TIMER:
		{
			_this->UpdateStatusFields();
			return TRUE;
		}

	case WM_DESTROY:
		{
			// sf@2002 - Destroy the status timer... TODO: improve this
			if (_this->m_nStatusTimer != 0)
			{
				KillTimer(hwnd, _this->m_nStatusTimer);
				_this->m_nStatusTimer = 0;
			}
			_this->OldEncodingStatusWindow = -1;
			_this->m_fStatusOpen = false;
			return TRUE;
		}
	}
	return FALSE;
}

//
//
//
LRESULT CALLBACK ClientConnection::GTGBS_SendCustomKey_proc(HWND Dlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg)
	{
	case WM_INITDIALOG:
		{
			SetFocus(GetDlgItem(Dlg,IDC_CUSTOM_KEY));
			ShowWindow(Dlg, SW_SHOW);
			// Window always on top
			RECT Rect;
			GetWindowRect(Dlg, &Rect);
			SetWindowPos(Dlg,
				HWND_TOPMOST,
				Rect.left,
				Rect.top,
				Rect.right - Rect.left,
				Rect.bottom - Rect.top,
				SWP_SHOWWINDOW);
			return TRUE;
		}
	case WM_CLOSE:
		{
			EndDialog(Dlg, 0);
			return TRUE;
		}
	case WM_COMMAND:
		{
			BOOL Okay;
			UINT Key;
			UINT STRG=0;
			UINT ALT=0;
			UINT ALTGR=0;
			if (LOWORD(wParam) == IDCANCEL) {
				EndDialog(Dlg, 0);
			}
			if (LOWORD(wParam) == IDOK) {
				if (SendMessage(GetDlgItem(Dlg,IDC_STRG), BM_GETCHECK, 0, 0) == BST_CHECKED)
					STRG=1;

				if (SendMessage(GetDlgItem(Dlg,IDC_ALT), BM_GETCHECK, 0, 0) == BST_CHECKED)
					ALT=1;

				if (SendMessage(GetDlgItem(Dlg,IDC_ALTGR), BM_GETCHECK, 0, 0) == BST_CHECKED)
					ALTGR=1;

				Key = GetDlgItemInt(Dlg,IDC_CUSTOM_KEY,&Okay,FALSE);

				if (ALT!=0)
					Key |=KEYMAP_LALT_FLAG;
				if (ALTGR != 0)
					Key |= KEYMAP_RALT_FLAG;
				if (STRG != 0)
					Key |= KEYMAP_RCONTROL_FLAG;

				if (Okay)
					{
                    helper::SafeSetMsgResult(Dlg, Key);
					EndDialog(Dlg, Key);
					}
				else
					EndDialog(Dlg, 0);
			}
		}
	}
	return 0;
}

//
// Process windows messages
//
LRESULT CALLBACK ClientConnection::WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
     ClientConnection *_this= (ClientConnection*)helper::SafeGetWindowUserData<ClientConnection>(hwnd);
	 if( iMsg==WM_CREATE )
		 {
		 _this = (ClientConnection*)((CREATESTRUCT*)lParam)->lpCreateParams;
		 helper::SafeSetWindowUserData(hwnd, (LONG_PTR)_this);
		 //SetWindowLongPtr( hwnd, GWLP_USERDATA, (LONG_PTR)_this );
		 }



	// This is a static method, so we don't know which instantiation we're
	// dealing with.  But we've stored a 'pseudo-this' in the window data.
//    ClientConnection *_this = helper::SafeGetWindowUserData<ClientConnection>(hwnd);

	if (_this == NULL)
		return DefWindowProc(hwnd, iMsg, wParam, lParam);

	// HWND parent;

	{
		// Main Window
		// if ( hwnd == _this->m_hwndMain)
		{
			switch (iMsg)
			{
//			case WM_TIMER:
//				KillTimer(hwnd,_this->m_FTtimer);
//				_this->m_FTtimer=0;
//				_this->m_pFileTransfer->SendFileChunk();
//				break;
			case WM_SYSCHAR:
				return true;
			case WM_SYSCOMMAND:
				{
					switch (LOWORD(wParam))
					{
					case ID_SW:
						if (!_this->m_SWselect)
						{
							_this->m_SWselect=true;
						}
						break;

					case ID_DESKTOP:
						if (!_this->m_SWselect)
						{
							//multimon switch
							//_this->m_SWselect=true;
							_this->SendSW(9999,9999);
						}
						break;

					// Toggle toolbar & toolbar menu option
					case ID_DBUTTON:
						_this->m_opts.m_ShowToolbar = !_this->m_opts.m_ShowToolbar;
						_this->SizeWindow();
						_this->SetFullScreenMode(_this->InFullScreenMode());
						// adzm - 2010-07 - Extended clipboard
						//_this->UpdateMenuItems(); // Handled in WM_INITMENUPOPUP
						break;

					/*
					case ID_BUTTON:
						_this->m_opts.m_ShowToolbar=true;
						_this->SizeWindow();
						_this->SetFullScreenMode(_this->InFullScreenMode());
						break;
					*/

					case ID_AUTOSCALING:
						_this->m_opts.m_fAutoScaling = !_this->m_opts.m_fAutoScaling;
						_this->SizeWindow();
						InvalidateRect(hwnd, NULL, TRUE);
						_this->RealiseFullScreenMode();
						// adzm - 2010-07 - Extended clipboard
						//_this->UpdateMenuItems(); // Handled in WM_INITMENUPOPUP
						break;

					case ID_DINPUT:
						_this->m_remote_mouse_disable = true;
						if (_this->m_opts.m_ShowToolbar)
						{
                            // 24 March 2008 jdp
                            _this->RebuildToolbar(hwnd);
							SendMessage(hwnd,WM_SIZE,(WPARAM)ID_DINPUT,(LPARAM)0);
						}
						if (_this->m_opts.m_ViewOnly) return 0;
						_this->SendServerInput(true);
						break;

					case ID_INPUT:
						_this->m_remote_mouse_disable = false;
						if (_this->m_opts.m_ShowToolbar)
						{
                            // 24 March 2008 jdp
                            _this->RebuildToolbar(hwnd);
							SendMessage(hwnd,WM_SIZE,(WPARAM)ID_DINPUT,(LPARAM)0);
						}
						if (_this->m_opts.m_ViewOnly) return 0;
						_this->SendServerInput(false);
						break;

					case SC_MINIMIZE:
						_this->SetDormant(true);
						if (_this->m_hwndStatus)ShowWindow(_this->m_hwndStatus,SW_MINIMIZE);
						break;

					case SC_MAXIMIZE:
						// Toggle full screen mode
						if (!_this->InFullScreenMode())
						{
							SendMessage(hwnd,WM_SYSCOMMAND,(WPARAM)ID_NORMALSCREEN,(LPARAM)0);
							Sleep(100);
						}
						_this->SetFullScreenMode(!_this->InFullScreenMode());
						return 0;
						break;

					case SC_RESTORE:
						_this->SetDormant(false);
						if (_this->m_hwndStatus)ShowWindow(_this->m_hwndStatus,SW_NORMAL);
						break;

					case ID_NEWCONN:
						_this->m_pApp->NewConnection(false);
						return 0;

					case ID_CONN_SAVE_AS:
						_this->SaveConnection();
						return 0;

					case IDC_OPTIONBUTTON:
						{
							if (_this->m_fOptionsOpen) return 0;
							_this->m_fOptionsOpen = true;

							// Modif sf@2002 - Server Scaling
							int nOldServerScale = _this->m_nServerScale;
							int prev_scale_num = _this->m_opts.m_scale_num;
							int prev_scale_den = _this->m_opts.m_scale_den;
							bool fOldToolbarState = _this->m_opts.m_ShowToolbar;
							bool nOldAutoMode = _this->m_opts.autoDetect;
							// adzm - 2010-07 - Extended clipboard
							bool bOldViewOnly = _this->m_opts.m_ViewOnly;
							bool bOldDisableClipboard = _this->m_opts.m_DisableClipboard;

							if (_this->m_opts.DoDialog(true,hwnd))
							{
								// Modif sf@2002 - Server Scaling
								_this->m_nServerScale = _this->m_opts.m_nServerScale;
								if (_this->m_nServerScale != nOldServerScale)
								{
									_this->SendServerScale(_this->m_nServerScale);
								}
								else
								{
									if (prev_scale_num != _this->m_opts.m_scale_num ||
										prev_scale_den != _this->m_opts.m_scale_den)
									{
										// Resize the window if scaling factors were changed
										_this->SizeWindow(/*false*/);
										InvalidateRect(hwnd, NULL, TRUE);
										// Make the window corresponds to the requested state
										_this->RealiseFullScreenMode();
									}
									if (fOldToolbarState != _this->m_opts.m_ShowToolbar)
										_this->SizeWindow();
									_this->m_pendingFormatChange = true;
								}

								// adzm 2010-10
								_this->m_PendingMouseMove.dwMinimumMouseMoveInterval = _this->m_opts.m_throttleMouse;
							}

							// adzm - 2010-07 - Extended clipboard
							if ( (bOldViewOnly != _this->m_opts.m_ViewOnly) || (bOldDisableClipboard != _this->m_opts.m_DisableClipboard) )
							{
								_this->UpdateRemoteClipboardCaps();

								if (!_this->m_opts.m_ViewOnly && !_this->m_opts.m_DisableClipboard) {
									_this->UpdateRemoteClipboard(); // update the clipboard if we are no longer view only and the clipboard is enabled
								}
							}

							 if (nOldAutoMode != _this->m_opts.autoDetect)
								 _this->m_nConfig = 0;
							_this->OldEncodingStatusWindow = -2; // force update in status window
							_this->m_fOptionsOpen = false;

							return 0;
						}

					case IDD_APP_ABOUT:
						ShowAboutBox();
						return 0;

					case ID_CONN_ABOUT:
						_this->ShowConnInfo();
						return 0;

					case ID_FULLSCREEN:
						// Toggle full screen mode
						if (!_this->InFullScreenMode()) //PGM

						{ //PGM
							SendMessage(hwnd,WM_SYSCOMMAND,(WPARAM)ID_NORMALSCREEN,(LPARAM)0); //PGM

							Sleep(100); //PGM
						} //PGM

						_this->SetFullScreenMode(!_this->InFullScreenMode());
						return 0;

					case ID_VIEWONLYTOGGLE:
						// Toggle view only mode
						_this->m_opts.m_ViewOnly = !_this->m_opts.m_ViewOnly;

						// adzm - 2010-07 - Extended clipboard
						if (_this->m_opts.m_ViewOnly) SetWindowText(_this->m_hwndMain, _this->m_desktopName_viewonly);
						else SetWindowText(_this->m_hwndMain, _this->m_desktopName);
						//_this->UpdateMenuItems(); // Handled in WM_INITMENUPOPUP

						_this->UpdateRemoteClipboardCaps();

						if (!_this->m_opts.m_ViewOnly && !_this->m_opts.m_DisableClipboard) {
							_this->UpdateRemoteClipboard(); // update the clipboard if we are no longer view only and the clipboard is enabled
						}
						return 0;

					case ID_REQUEST_REFRESH:
						// Request a full-screen update
						//adzm 2010-09
						_this->SendFullFramebufferUpdateRequest(false);
						return 0;

					case ID_VK_LWINDOWN:
						if (_this->m_opts.m_ViewOnly) return 0;
						_this->SendKeyEvent(XK_Super_L, true);
						//adzm 2010-09
						_this->FlushWriteQueue(true, 5);
						return 0;
					case ID_VK_LWINUP:
						if (_this->m_opts.m_ViewOnly) return 0;
						_this->SendKeyEvent(XK_Super_L, false);
						//adzm 2010-09
						_this->FlushWriteQueue(true, 5);
						return 0;
					case ID_VK_RWINDOWN:
						if (_this->m_opts.m_ViewOnly) return 0;
						_this->SendKeyEvent(XK_Super_R, true);
						//adzm 2010-09
						_this->FlushWriteQueue(true, 5);
						return 0;
					case ID_VK_RWINUP:
						if (_this->m_opts.m_ViewOnly) return 0;
						_this->SendKeyEvent(XK_Super_R, false);
						//adzm 2010-09
						_this->FlushWriteQueue(true, 5);
						return 0;
					case ID_VK_APPSDOWN:
						if (_this->m_opts.m_ViewOnly) return 0;
						_this->SendKeyEvent(XK_Menu, true);
						//adzm 2010-09
						_this->FlushWriteQueue(true, 5);
						return 0;
					case ID_VK_APPSUP:
						if (_this->m_opts.m_ViewOnly) return 0;
						_this->SendKeyEvent(XK_Menu, false);
						//adzm 2010-09
						_this->FlushWriteQueue(true, 5);
						return 0;

					// Send START Button
					case ID_CONN_CTLESC:
						if (_this->m_opts.m_ViewOnly) return 0;
						_this->SendKeyEvent(XK_Control_L,true);
						_this->SendKeyEvent(XK_Escape,true);
						_this->SendKeyEvent(XK_Control_L,false);
						_this->SendKeyEvent(XK_Escape,false);
						//adzm 2010-09
						_this->FlushWriteQueue(true, 5);
						return 0;

					// Send Ctrl-Alt-Del
					case ID_CONN_CTLALTDEL:
						if (_this->m_opts.m_ViewOnly) return 0;
						_this->SendKeyEvent(XK_Control_L, true);
						_this->SendKeyEvent(XK_Alt_L,     true);
						_this->SendKeyEvent(XK_Delete,    true);
						_this->SendKeyEvent(XK_Delete,    false);
						_this->SendKeyEvent(XK_Alt_L,     false);
						_this->SendKeyEvent(XK_Control_L, false);
						//adzm 2010-09
						_this->FlushWriteQueue(true, 5);
						return 0;

					case ID_CONN_CTLDOWN:
						if (_this->m_opts.m_ViewOnly) return 0;
						_this->SendKeyEvent(XK_Control_L, true);
						//adzm 2010-09
						_this->FlushWriteQueue(true, 5);
						return 0;

					case ID_CONN_CTLUP:
						if (_this->m_opts.m_ViewOnly) return 0;
						_this->SendKeyEvent(XK_Control_L, false);
						//adzm 2010-09
						_this->FlushWriteQueue(true, 5);
						return 0;

					case ID_CONN_ALTDOWN:
						if (_this->m_opts.m_ViewOnly) return 0;
						_this->SendKeyEvent(XK_Alt_L, true);
						//adzm 2010-09
						_this->FlushWriteQueue(true, 5);
						return 0;

					case ID_CONN_ALTUP:
						if (_this->m_opts.m_ViewOnly) return 0;
						_this->SendKeyEvent(XK_Alt_L, false);
						//adzm 2010-09
						_this->FlushWriteQueue(true, 5);
						return 0;

					case ID_CLOSEDAEMON:
						if (MessageBox(hwnd, sz_L75,
							sz_L76,
							MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
							PostQuitMessage(0);
						return 0;

						// Modif sf@2002 - FileTransfer
					case ID_FILETRANSFER:
						// Check if the Server knows FileTransfer
						if (!_this->m_fServerKnowsFileTransfer)
						{
							MessageBox(hwnd, sz_L77,
								sz_L78,
								MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST|MB_SYSTEMMODAL);
							return 0;
						}
						// Don't call FileTRansfer GUI is already open !
						if (_this->m_pFileTransfer->m_fFileTransferRunning)
						{
							_this->m_pFileTransfer->ShowFileTransferWindow(true);
							return 0;
							/*
							MessageBox(NULL, sz_L79,
								sz_L80,
								MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST);
							return 0;
							*/
						}
						if (_this->m_pTextChat->m_fTextChatRunning)
						{
							_this->m_pTextChat->ShowChatWindow(true);
							MessageBox(	hwnd,
										sz_L86,
										sz_L88,
										MB_OK | MB_ICONSTOP | MB_SETFOREGROUND | MB_TOPMOST|MB_SYSTEMMODAL);
							return 0;
						}

						// Call FileTransfer Dialog
						_this->m_pFileTransfer->m_fFileTransferRunning = true;
						_this->m_pFileTransfer->m_fFileCommandPending = false;
						_this->m_pFileTransfer->DoDialog();
						_this->m_pFileTransfer->m_fFileTransferRunning = false;
						// Refresh Screen
						// _this->SendFullFramebufferUpdateRequest();
						//adzm 2010-09
						if (_this->m_pFileTransfer->m_fVisible || _this->m_pFileTransfer->UsingOldProtocol())
							_this->SendAppropriateFramebufferUpdateRequest(false);
						return 0;

						// sf@2002 - Text Chat
					case ID_TEXTCHAT:
						// We use same flag as FT for now
						// Check if the Server knows FileTransfer
						if (!_this->m_fServerKnowsFileTransfer)
						{
							MessageBox(hwnd, sz_L81,
								sz_L82,
								MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST|MB_SYSTEMMODAL);
							return 0;
						}
						if (_this->m_pTextChat->m_fTextChatRunning)
						{
							_this->m_pTextChat->ShowChatWindow(true);
							return 0;
							/*
							MessageBox(NULL, sz_L83,
								sz_L84,
								MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST);
							return 0;
							*/
						}
						if (_this->m_pFileTransfer->m_fFileTransferRunning)
						{
							_this->m_pFileTransfer->ShowFileTransferWindow(true);
							MessageBox(hwnd,
										sz_L85,
										sz_L88,
										MB_OK | MB_ICONSTOP | MB_SETFOREGROUND | MB_TOPMOST|MB_SYSTEMMODAL);
							return 0;
						}
						_this->m_pTextChat->m_fTextChatRunning = true;
						_this->m_pTextChat->DoDialog();
						return 0;

						// sf@2002
					case ID_MAXCOLORS:
						if (_this->m_opts.m_Use8Bit)
						{
							_this->m_opts.m_Use8Bit = rfbPFFullColors; //false;
							_this->m_pendingFormatChange = true;
							InvalidateRect(hwnd, NULL, TRUE);
						}
						return 0;

						// sf@2002
					case ID_256COLORS:
						// if (!_this->m_opts.m_Use8Bit)
						{
							_this->m_opts.m_Use8Bit = rfbPF256Colors; //true;
							_this->m_pendingFormatChange = true;
							InvalidateRect(hwnd, NULL, TRUE);
						}
						return 0;

						// Modif sf@2002
					case ID_HALFSCREEN:
						{
							if (_this->InFullScreenMode()) //PGM

							{ //PGM
								SendMessage(hwnd,WM_SYSCOMMAND,(WPARAM)ID_AUTOSCALING,(LPARAM)0); //PGM

								return 0; //PGM
							} //PGM

							// Toggle halfSize screen mode (server side)
							int nOldServerScale = _this->m_nServerScale;

							// Modif sf@2002 - Server Scaling
							_this->m_opts.m_fAutoScaling = false;
							_this->m_nServerScale = 2;
							_this->m_opts.m_nServerScale = 2;
							_this->m_opts.m_scaling = true;
							_this->m_opts.m_scale_num = 100;
							_this->m_opts.m_scale_den = 100;

							if (_this->m_nServerScale != nOldServerScale)
							{
								_this->SendServerScale(_this->m_nServerScale);
								// _this->m_pendingFormatChange = true;
							}
							else
							{
								_this->SizeWindow();
								InvalidateRect(hwnd, NULL, TRUE);
								_this->RealiseFullScreenMode();
								_this->m_pendingFormatChange = true;
							}
							return 0;
						}

						// Modif sf@2002
					case ID_FUZZYSCREEN:
						{
							// Toggle fuzzy screen mode (server side)
							int nOldServerScale = _this->m_nServerScale;

							// We don't forbid AutoScaling if selected
							// so the viewer zoom factor is more accurate
							_this->m_nServerScale = 2;
							_this->m_opts.m_nServerScale = 2;
							_this->m_opts.m_scaling = true;
							_this->m_opts.m_scale_num = 200;
							_this->m_opts.m_scale_den = 100;

							if (_this->m_nServerScale != nOldServerScale)
							{
								_this->SendServerScale(_this->m_nServerScale);
								// _this->m_pendingFormatChange = true;
							}
							else
							{
								_this->SizeWindow();
								InvalidateRect(hwnd, NULL, TRUE);
								_this->RealiseFullScreenMode();
								_this->m_pendingFormatChange = true;
							}

							return 0;
						}

						// Modif sf@2002
					case ID_NORMALSCREEN:
						{
							// Toggle normal screen
							/*int nOldServerScale = _this->m_nServerScale;

							_this->m_opts.m_fAutoScaling = false;
							_this->m_nServerScale = 1;
							_this->m_opts.m_nServerScale = 1;
							_this->m_opts.m_scaling = false;
							_this->m_opts.m_scale_num = 100;
							_this->m_opts.m_scale_den = 100;

							if (_this->m_nServerScale != nOldServerScale)
							{
								_this->SendServerScale(_this->m_nServerScale);
								// _this->m_pendingFormatChange = true;
							}
							else
							{
								_this->SizeWindow();
								InvalidateRect(hwnd, NULL, TRUE);
								_this->SetFullScreenMode(false);
								_this->m_pendingFormatChange = true;
							}*/
							return 0;
						}

					case ID_NORMALSCREEN2:
						{
							// Toggle normal screen
							int nOldServerScale = _this->m_nServerScale;

							_this->m_opts.m_fAutoScaling = false;
							_this->m_nServerScale = 1;
							_this->m_opts.m_nServerScale = 1;
							_this->m_opts.m_scaling = false;
							_this->m_opts.m_scale_num = 100;
							_this->m_opts.m_scale_den = 100;

							if (_this->m_nServerScale != nOldServerScale)
							{
								_this->SendServerScale(_this->m_nServerScale);
								// _this->m_pendingFormatChange = true;
							}
							else
							{
								_this->SizeWindow();
								InvalidateRect(hwnd, NULL, TRUE);
								_this->SetFullScreenMode(false);
								_this->m_pendingFormatChange = true;
							}
							return 0;
						}

					// adzm - 2010-07 - Extended clipboard
					case ID_ENABLE_CLIPBOARD:
						_this->m_opts.m_DisableClipboard = !_this->m_opts.m_DisableClipboard;
						_this->UpdateRemoteClipboardCaps();

						if (!_this->m_opts.m_ViewOnly && !_this->m_opts.m_DisableClipboard) {
							_this->UpdateRemoteClipboard(); // update the clipboard if we are no longer view only and the clipboard is enabled
						}
						//_this->UpdateMenuItems(); // Handled in WM_INITMENUPOPUP
						break;
					case ID_CLIPBOARD_TEXT:
						// currently just toggle between default limit and auto; an interface
						// for limits will be useful once more complex (larger) formats are
						// implemented.
						_this->m_clipboard.settings.m_nLimitText =
							_this->m_clipboard.settings.m_nLimitText == 0 ?
								(_this->m_clipboard.settings.m_nRequestedLimitText == 0 ?
									ClipboardSettings::defaultLimit
								: _this->m_clipboard.settings.m_nRequestedLimitText)
							: 0;
						_this->UpdateRemoteClipboardCaps(true);
						//_this->UpdateMenuItems(); // Handled in WM_INITMENUPOPUP
						break;
					case ID_CLIPBOARD_RTF:
						_this->m_clipboard.settings.m_nLimitRTF =
							_this->m_clipboard.settings.m_nLimitRTF == 0 ?
								(_this->m_clipboard.settings.m_nRequestedLimitRTF == 0 ?
									ClipboardSettings::defaultLimit
								: _this->m_clipboard.settings.m_nRequestedLimitRTF)
							: 0;
						_this->UpdateRemoteClipboardCaps(true);
						//_this->UpdateMenuItems(); // Handled in WM_INITMENUPOPUP
						break;
					case ID_CLIPBOARD_HTML:
						_this->m_clipboard.settings.m_nLimitHTML =
							_this->m_clipboard.settings.m_nLimitHTML == 0 ?
								(_this->m_clipboard.settings.m_nRequestedLimitHTML == 0 ?
									ClipboardSettings::defaultLimit
								: _this->m_clipboard.settings.m_nRequestedLimitHTML)
							: 0;
						_this->UpdateRemoteClipboardCaps(true);
						//_this->UpdateMenuItems(); // Handled in WM_INITMENUPOPUP
						break;
					case ID_CLIPBOARD_SEND:
						_this->UpdateRemoteClipboard(clipProvide | clipText | clipRTF | clipHTML | clipDIB);
						break;
					case ID_CLIPBOARD_RECV:
						_this->RequestRemoteClipboard();
						break;
					} // end switch lowparam syscommand

					break;
				}//end case wm_syscommand

#ifndef UNDER_CE
				case WM_SIZING:
					if (_this->m_opts.m_Directx) return 0;
					{
						int a=GetSystemMetrics(SM_CYHSCROLL);
						int b=GetSystemMetrics(SM_CXVSCROLL);
						if(!_this->SB_HORZ_BOOL) a=0;
						if (!_this->SB_VERT_BOOL) b=0;
						// Don't allow sizing larger than framebuffer
						RECT *lprc = (LPRECT) lParam;
						switch (wParam) {
						case WMSZ_RIGHT:
						case WMSZ_TOPRIGHT:
						case WMSZ_BOTTOMRIGHT:
							lprc->right = min(lprc->right, lprc->left + (_this->m_fullwinwidth+b)+1 );
							break;
						case WMSZ_LEFT:
						case WMSZ_TOPLEFT:
						case WMSZ_BOTTOMLEFT:
							lprc->left = max(lprc->left, lprc->right - (_this->m_fullwinwidth+b));
							break;
						}

						switch (wParam) {
						case WMSZ_TOP:
						case WMSZ_TOPLEFT:
						case WMSZ_TOPRIGHT:
							if (_this->m_opts.m_ShowToolbar)
								lprc->top = max(lprc->top, lprc->bottom - (_this->m_fullwinheight+a) -_this->m_TBr.bottom);
							else
								lprc->top = max(lprc->top, lprc->bottom - (_this->m_fullwinheight+a));
							break;
						case WMSZ_BOTTOM:
						case WMSZ_BOTTOMLEFT:
						case WMSZ_BOTTOMRIGHT:
							if (_this->m_opts.m_ShowToolbar)
								lprc->bottom = min(lprc->bottom, lprc->top + (_this->m_fullwinheight+a) + _this->m_TBr.bottom);
							else
								lprc->bottom = min(lprc->bottom, lprc->top + (_this->m_fullwinheight+a));
							break;
						}

						return 0;
					}
#endif
				case WM_QUERYOPEN:
					_this->SetDormant(false);
					return true;

				case WM_SETFOCUS:
					if (_this->InFullScreenMode() && !_this->m_pFileTransfer->m_fFileTransferRunning && !_this->m_pTextChat->m_fTextChatRunning && !_this->m_fOptionsOpen)
					{
						SetWindowPos(hwnd, HWND_TOPMOST, 0,0,100,100, SWP_NOMOVE | SWP_NOSIZE);
					}

					TheAccelKeys.SetWindowHandle(_this->m_opts.m_NoHotKeys ? 0 : hwnd);
					_this->m_keymap->Reset();					
					return 0;

				case WM_KILLFOCUS:
					if (!_this->m_running) return 0;
					if (_this->InFullScreenMode()  && !_this->m_pFileTransfer->m_fFileTransferRunning && !_this->m_pTextChat->m_fTextChatRunning && !_this->m_fOptionsOpen) {
						SetWindowPos(hwnd, HWND_TOP, 0,0,100,100, SWP_NOMOVE | SWP_NOSIZE| SWP_NOACTIVATE);
					}
					if ( _this->m_opts.m_ViewOnly) return 0;
					_this->m_keymap->ReleaseAllKeys(_this);
					//adzm 2010-09
					_this->FlushWriteQueue(true, 5);
					/*
					_this->SendKeyEvent(XK_Alt_L,     false);
					_this->SendKeyEvent(XK_Control_L, false);
					_this->SendKeyEvent(XK_Shift_L,   false);
					_this->SendKeyEvent(XK_Alt_R,     false);
					_this->SendKeyEvent(XK_Control_R, false);
					_this->SendKeyEvent(XK_Shift_R,   false);
					*/
					return DefWindowProc(hwnd, iMsg, wParam, lParam);
					return 0;

				case WM_CLOSE:
					{
						_this->m_keepalive_timer=0;
                        // April 8 2008 jdp
						static bool boxopen=false;
						if (boxopen) return 0;
                        if (lParam == 0 && !_this->m_bKillThread)
						{
							if (_this->m_opts.m_fExitCheck) //PGM @ Advantig
							{ //PGM @ Advantig
								boxopen=true;
							    if (MessageBox(hwnd, sz_L75,sz_L76,MB_YESNO | MB_ICONQUESTION | MB_SETFOREGROUND | MB_TOPMOST|MB_SYSTEMMODAL) == IDNO)
									{
										boxopen=false;
										return 0;
									}
							} // PGM @ Advantig.com
							boxopen=false;
                            _this->m_bClosedByUser = true;
						}
						if (_this->m_pFileTransfer->m_fFileTransferRunning)
						{
                            if (_this->m_bKillThread)
                            {
                                ::SendMessage(_this->m_pFileTransfer->hWnd, WM_COMMAND, IDCANCEL, 0);
    							return 0;
                            }
							_this->m_pFileTransfer->ShowFileTransferWindow(true);
							MessageBox(hwnd, sz_L85,
								sz_L88,
								MB_OK | MB_ICONSTOP | MB_SETFOREGROUND | MB_TOPMOST|MB_SYSTEMMODAL);
							return 0;
						}

						// sf@2002 - Do not close vncviewer if the Text Chat GUI is open !
						if (_this->m_pTextChat->m_fTextChatRunning)
						{
                            if (_this->m_bKillThread)
                            {
                                _this->m_pTextChat->KillDialog();
    							return 0;
                            }

							_this->m_pTextChat->ShowChatWindow(true);
							MessageBox(hwnd, sz_L86,
								sz_L88,
								MB_OK | MB_ICONSTOP | MB_SETFOREGROUND | MB_TOPMOST|MB_SYSTEMMODAL);
							return 0;
						}

						if (_this->m_fOptionsOpen)
						{
                            MessageBox(hwnd, sz_L87,
								sz_L88,
								MB_OK | MB_ICONSTOP | MB_SETFOREGROUND | MB_TOPMOST|MB_SYSTEMMODAL);
							return 0;
						}

						// sf@2007 - Autoreconnect
						// If wParam == 0 then the close is "normal" and presumably wanted by the user
						if ( ( _this->m_autoReconnect == 0) || (wParam == 0))
						{
							_this->m_autoReconnect = 0; // Forbid autoreconnect when the CLOSE order comes from the user

                            // 8 April 2008 jdp hide window while shutting down
                            ::ShowWindow(hwnd, SW_HIDE);
							//make sure reconnecthread is closed before closing mother
							forcedexit=true;
							if(_this->rcth)
								{
									WaitForSingleObject(_this->rcth,10000);
									CloseHandle(_this->rcth);
								}
							_this->rcth=NULL;
							// Close the worker thread
							_this->KillThread();

							DestroyWindow(_this->m_hwndTB);
							DestroyWindow(_this->m_TrafficMonitor);
							DestroyWindow(_this->m_logo_wnd);
							DestroyWindow(_this->m_button_wnd);
	//						DestroyWindow(_this->m_hwndTBwin);
							DestroyWindow(_this->m_hwndcn);
							DestroyWindow(hwnd);
						}
						else // Autoreconnect allowed - We only suspend the working thread then reconnect a few seconds later
						{
							char temp[10];
							char wtext[150];
							_itoa(wParam,temp,10);
							strcpy(wtext,"UltraVNC Viewer - Connection dropped, trying to reconnect (");
							strcat(wtext,temp);
							strcat(wtext,")");
							SetWindowText(_this->m_hwndMain, wtext);
							_this->m_opts.m_NoStatus = true;
							_this->SuspendThread();
							//_this->Reconnect()
							DWORD dw;
							//Give time to close reconnect thread
							Sleep(1000);
							//should never happen, but jjust in case, make sure no 2 are runnning
							if(_this->rcth) {
								DWORD dwWaitResult=WaitForSingleObject(_this->rcth,1000);
								switch (dwWaitResult)
								{
										case WAIT_OBJECT_0:
											CloseHandle(_this->rcth);
											_this->rcth=NULL;
											_this->rcth=CreateThread(NULL,0,ReconnectThreadProc,_this,0,&dw);
											break;
										case WAIT_TIMEOUT:
											//reconnect still running, doubble call ignore
											break;
								}
							}
							else
								_this->rcth=CreateThread(NULL,0,ReconnectThreadProc,_this,0,&dw);
						}
						return 0;
					}

				// adzm - 2010-07 - Extended clipboard
				case WM_INITMENUPOPUP:
					{
						_this->UpdateMenuItems();
						break;
					}

				case WM_DESTROY:
					{
#ifndef UNDER_CE
						// Remove us from the clipboard viewer chain
						if (hwnd == _this->m_hwndcn && _this->m_hwndNextViewer != (HWND)INVALID_HANDLE_VALUE) {
							BOOL res = ChangeClipboardChain( _this->m_hwndcn, _this->m_hwndNextViewer);
							_this->m_hwndNextViewer = NULL;
							vnclog.Print(6, _T("WndProc ChangeClipboardChain m_hwndcn 0x%08x / hwnd 0x%08x, 0x%08x (%li)\n"), _this->m_hwndcn, hwnd, _this->m_hwndNextViewer, res);
						}
#endif
						if (_this->m_waitingOnEmulateTimer)
						{
							KillTimer(_this->m_hwndcn, _this->m_emulate3ButtonsTimer);
							_this->m_waitingOnEmulateTimer = false;
						}
						// adzm - 2010-07 - Extended clipboard

						if (_this->m_hPopupMenuClipboard) {
							DestroyMenu(_this->m_hPopupMenuClipboard);
							_this->m_hPopupMenuClipboard = NULL;
						}

						if (_this->m_hPopupMenuDisplay) {
							DestroyMenu(_this->m_hPopupMenuDisplay);
							_this->m_hPopupMenuDisplay = NULL;
						}

						if (_this->m_hPopupMenuKeyboard) {
							DestroyMenu(_this->m_hPopupMenuKeyboard);
							_this->m_hPopupMenuKeyboard = NULL;
						}

						if (_this->m_hPopupMenuClipboardFormats) {
							DestroyMenu(_this->m_hPopupMenuClipboardFormats);
							_this->m_hPopupMenuClipboardFormats = NULL;
						}

//						if (_this->m_FTtimer != 0)
//						{
//							KillTimer(hwnd, _this->m_FTtimer);
//							_this->m_FTtimer = 0;
//						}

						//_this->m_hwnd = 0;
						// We are currently in the main thread.
						// The worker thread should be about to finish if
						// it hasn't already. Wait for it.
						try {
							void *p;
							_this->join(&p);  // After joining, _this is no longer valid
						} catch (omni_thread_invalid& e) {
							// The thread probably hasn't been started yet,
						}

						//PostQuitMessage(0);
						return 0;
					}
// Verify
// Possible not needed as clientwindow receive inout

				case WM_KEYDOWN:
				case WM_KEYUP:
				case WM_SYSKEYDOWN:
				case WM_SYSKEYUP:
					{
						if (!_this->m_running) return 0;
						if ( _this->m_opts.m_ViewOnly) return 0;
						_this->ProcessKeyEvent((int) wParam, (DWORD) lParam);
						//adzm 2010-09
						_this->FlushWriteQueue(true, 5);
						return 0;
					}

				case WM_DEADCHAR:
				case WM_SYSDEADCHAR:
					return 0;

				case WM_WINDOWPOSCHANGED:
				case WM_SIZE:
					{
						// Calculate window dimensions
						RECT rect;
						RECT Rtb;
						GetWindowRect(hwnd, &rect);
						_this->m_winwidth = rect.right - rect.left;
						_this->m_winheight = rect.bottom - rect.top ;

						if (_this->m_opts.m_ShowToolbar)
						{
							GetWindowRect(_this->m_hwndTBwin, &Rtb);
							//MoveWindow(_this->m_hwndTB,
							//	0,0,_this->m_winwidth - 106, 32,TRUE);
							//SetWindowPos(_this->m_hwndTBwin, HWND_TOP, 0, 0, _this->m_winwidth, 32,SWP_FRAMECHANGED);
							if ((_this->m_winwidth) > 140+85+14*24)
							{
								if (_this->m_BigToolbar==false)
								{
								DestroyWindow(_this->m_hwndTB);
								_this->m_BigToolbar=true;
								_this->CreateButtons(false,_this->m_fServerKnowsFileTransfer);
								}
							}
							else
							{
								if (_this->m_BigToolbar)
								{
								_this->m_BigToolbar=false;
								DestroyWindow(_this->m_hwndTB);
								_this->CreateButtons(true,_this->m_fServerKnowsFileTransfer);
								}
							}
							SetWindowPos(_this->m_hwndTB,HWND_TOP,0,0,_this->m_winwidth - 200, 32,SWP_FRAMECHANGED);
							if (_this->m_TrafficMonitor)
							{
								MoveWindow(_this->m_TrafficMonitor,
									_this->m_winwidth - 55,2,35,30,TRUE);
								MoveWindow(_this->m_logo_wnd,
									_this->m_winwidth - 185,2,130,28,TRUE);
								MoveWindow(_this->m_button_wnd,
									_this->m_winwidth - 200,10,10,10,TRUE);
							}
							if (_this->m_logo_wnd)
							{
								MoveWindow(_this->m_logo_wnd,
									_this->m_winwidth - 185,2,130,28,TRUE);
								MoveWindow(_this->m_button_wnd,
									_this->m_winwidth - 200,10,10,10,TRUE);
							}
							UpdateWindow(_this->m_hwndTB);
							UpdateWindow(_this->m_logo_wnd);
							UpdateWindow(_this->m_button_wnd);
						}
						else
						{
							Rtb.top=0;Rtb.bottom=0;
						}

						// If the current window size would be large enough to hold the
						// whole screen without scrollbars, or if we're full-screen,
						// we turn them off.  Under CE, the scroll bars are unchangeable.

#ifndef UNDER_CE
						int h_scrollbar=GetSystemMetrics(SM_CYHSCROLL);
						int v_scrollbar=GetSystemMetrics(SM_CXVSCROLL);
						int h=0;
						int v=0;
						if (_this->SB_HORZ_BOOL) h=h_scrollbar;
						if (_this->SB_VERT_BOOL) v=v_scrollbar;

						if (_this->InFullScreenMode() || _this->m_opts.m_Directx)
						{
							_this->SB_HORZ_BOOL=false;
							_this->SB_VERT_BOOL=false;
							ShowScrollBar(hwnd, SB_HORZ, _this->SB_HORZ_BOOL);
							ShowScrollBar(hwnd, SB_VERT, _this->SB_VERT_BOOL);
						}
						else
						{
							if (_this->m_winheight >= (_this->m_fullwinheight + (Rtb.bottom - Rtb.top) + h) )
							{
								_this->SB_VERT_BOOL=false;
							}
							else _this->SB_VERT_BOOL=true;

							if(_this->m_winwidth  >= _this->m_fullwinwidth + v)
									{
										_this->SB_HORZ_BOOL=false;
									}
							else _this->SB_HORZ_BOOL=true;

							if (_this->SB_HORZ_BOOL) h=h_scrollbar;
							if (_this->SB_VERT_BOOL) v=v_scrollbar;

							if (_this->m_winheight >= (_this->m_fullwinheight + (Rtb.bottom - Rtb.top) + h) )
							{
								_this->SB_VERT_BOOL=false;
							}
							else _this->SB_VERT_BOOL=true;

							if(_this->m_winwidth  >= _this->m_fullwinwidth + v)
									{
										_this->SB_HORZ_BOOL=false;
									}
							else _this->SB_HORZ_BOOL=true;

							if (_this->SB_HORZ_BOOL) h=h_scrollbar;
							if (_this->SB_VERT_BOOL) v=v_scrollbar;

							if (_this->m_winheight >= (_this->m_fullwinheight + (Rtb.bottom - Rtb.top) + h) )
							{
								_this->SB_VERT_BOOL=false;
							}
							else _this->SB_VERT_BOOL=true;

							if(_this->m_winwidth  >= _this->m_fullwinwidth + v)
									{
										_this->SB_HORZ_BOOL=false;
									}
							else _this->SB_HORZ_BOOL=true;

							if ((_this->m_winwidth  == _this->m_fullwinwidth)&&
							   (_this->m_winheight == (_this->m_fullwinheight + (Rtb.bottom - Rtb.top))))
							{
								_this->SB_VERT_BOOL=false;
								_this->SB_HORZ_BOOL=false;
							}

						ShowScrollBar(hwnd, SB_HORZ, _this->SB_HORZ_BOOL);
						ShowScrollBar(hwnd, SB_VERT, _this->SB_VERT_BOOL);
						}

#endif

						// Update these for the record
						// And consider that in full-screen mode the window
						// is actually bigger than the remote screen.
						if (_this->m_opts.m_Directx)
						{
							GetClientRect(hwnd, &rect);

						_this->m_cliwidth = rect.right - rect.left;
						_this->m_cliheight = (int)(rect.bottom - rect.top);

						if (_this->m_opts.m_ShowToolbar)
							SetWindowPos(_this->m_hwndcn, _this->m_hwndTBwin, 0, _this->m_TBr.bottom, _this->m_cliwidth, _this->m_cliheight-_this->m_TBr.bottom, SWP_SHOWWINDOW);
						else SetWindowPos(_this->m_hwndcn, _this->m_hwndTBwin, 0, 0, _this->m_cliwidth, _this->m_cliheight, SWP_SHOWWINDOW);
						InvalidateRect(_this->m_hwndcn,&rect,false);
						}
						else
						{
						GetClientRect(hwnd, &rect);

						_this->m_cliwidth = min( (int)(rect.right - rect.left),
							                     (int)(_this->m_si.framebufferWidth * _this->m_opts.m_scale_num / _this->m_opts.m_scale_den));
						if (_this->m_opts.m_ShowToolbar)
							_this->m_cliheight = min( (int)rect.bottom - rect.top ,
							                          (int)_this->m_si.framebufferHeight * _this->m_opts.m_scale_num / _this->m_opts.m_scale_den + _this->m_TBr.bottom);
						else
							_this->m_cliheight = min( (int)(rect.bottom - rect.top) ,
							                          (int)(_this->m_si.framebufferHeight * _this->m_opts.m_scale_num / _this->m_opts.m_scale_den));

						_this->m_hScrollMax = (int)_this->m_si.framebufferWidth * _this->m_opts.m_scale_num / _this->m_opts.m_scale_den;
						if (_this->m_opts.m_ShowToolbar)
							_this->m_vScrollMax = (int)(_this->m_si.framebufferHeight *
							                            _this->m_opts.m_scale_num / _this->m_opts.m_scale_den)
														+ _this->m_TBr.bottom;
						else
							_this->m_vScrollMax = (int)(_this->m_si.framebufferHeight*
							                           _this->m_opts.m_scale_num / _this->m_opts.m_scale_den);

						int newhpos, newvpos;
						newhpos = max(0,
							          min(_this->m_hScrollPos,
					                      _this->m_hScrollMax - max(_this->m_cliwidth, 0)
										 )
									 );
						newvpos = max(0,
							          min(_this->m_vScrollPos,
							              _this->m_vScrollMax - max(_this->m_cliheight, 0)
										 )
								     );

						ScrollWindowEx(_this->m_hwndcn,
							           _this->m_hScrollPos - newhpos,
									   _this->m_vScrollPos - newvpos,
							           NULL, &rect, NULL, NULL,  SW_INVALIDATE);

						_this->m_hScrollPos = newhpos;
						_this->m_vScrollPos = newvpos;
						_this->UpdateScrollbars();
						}

					//Added by: Lars Werner (http://lars.werner.no)
					if(wParam==SIZE_MAXIMIZED&&_this->InFullScreenMode()==FALSE)
					{
						_this->SetFullScreenMode(!_this->InFullScreenMode());
						//MessageBox(NULL,"Fullscreeen from maximizehora...","KAKE",MB_OK);
						//return 0;
					}

					//Modified by: Lars Werner (http://lars.werner.no)
					if(_this->InFullScreenMode()==TRUE)
						return 0;
					else
						break;
					}

				case WM_HSCROLL:
					{
						int dx = 0;
						int pos = HIWORD(wParam);
						switch (LOWORD(wParam)) {
						case SB_LINEUP:
							dx = -2; break;
						case SB_LINEDOWN:
							dx = 2; break;
						case SB_PAGEUP:
							dx = _this->m_cliwidth * -1/4; break;
						case SB_PAGEDOWN:
							dx = _this->m_cliwidth * 1/4; break;
						case SB_THUMBPOSITION:
							dx = pos - _this->m_hScrollPos;
						case SB_THUMBTRACK:
							dx = pos - _this->m_hScrollPos;
						}
						_this->ScrollScreen(dx,0);

						return 0;
					}

				case WM_VSCROLL:
					{
						int dy = 0;
						int pos = HIWORD(wParam);
						switch (LOWORD(wParam)) {
						case SB_LINEUP:
							dy = -2; break;
						case SB_LINEDOWN:
							dy = 2; break;
						case SB_PAGEUP:
							dy = _this->m_cliheight * -1/4; break;
						case SB_PAGEDOWN:
							dy = _this->m_cliheight * 1/4; break;
						case SB_THUMBPOSITION:
							dy = pos - _this->m_vScrollPos;
						case SB_THUMBTRACK:
							dy = pos - _this->m_vScrollPos;
						}
						_this->ScrollScreen(0,dy);

						return 0;
					}

					// RealVNC 335 method
				case WM_MOUSEWHEEL:
					if (!_this->m_opts.m_ViewOnly) {
						_this->ProcessMouseWheel((SHORT)HIWORD(wParam));
						//adzm 2010-09
						_this->FlushWriteQueue(true, 5);
					}
					return 0;

				//Added by: Lars Werner (http://lars.werner.no) - These is the custom messages from the TitleBar
				case tbWM_CLOSE:
					SendMessage(_this->m_hwndMain, WM_CLOSE,NULL,0);
					return 0;

				case tbWM_MINIMIZE:
					_this->SetDormant(true);
					ShowWindow(_this->m_hwndMain, SW_MINIMIZE);
					return 0;

				case tbWM_MAXIMIZE:
					//_this->SetFullScreenMode(!_this->InFullScreenMode());
					_this->SetFullScreenMode(FALSE);
					return 0;
			} // end of iMsg switch

			//return DefWindowProc(hwnd, iMsg, wParam, lParam);

			// Process asynchronous FileTransfer in this thread
			if ((iMsg == FileTransferSendPacketMessage) && (_this->m_pFileTransfer != NULL))
			{
				if (LOWORD(wParam) == 0)
				{
//					if (_this->m_FTtimer != 0)_this->m_FTtimer=SetTimer(hwnd,11, 100, 0);//
					_this->m_pFileTransfer->SendFileChunk();
				}
				else
					_this->m_pFileTransfer->ProcessFileTransferMsg();
				return 0;
			}
            // 24 March 2008 jdp
            if (iMsg == RebuildToolbarMessage)
            {
		        if (_this->m_opts.m_ShowToolbar)
		        {
                    _this->RebuildToolbar(hwnd);
			        SendMessage(hwnd,WM_SIZE,(WPARAM)ID_DINPUT,(LPARAM)0);
		        }
            }
		} // End if Main Window
	}

	return DefWindowProc(hwnd, iMsg, wParam, lParam);

	// We know about an unused variable here.
#pragma warning(disable : 4101)
}

//
//
//
LRESULT CALLBACK ClientConnection::WndProcTBwin(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    ClientConnection *_this = helper::SafeGetWindowUserData<ClientConnection>(hwnd);

    if (_this == NULL) return DefWindowProc(hwnd, iMsg, wParam, lParam);

	HWND parent;
	if (_this->m_opts.m_ShowToolbar==true)
		{
			parent = _this->m_hwndMain;
			switch (iMsg)
			{
			case WM_PAINT:
				{
					if (_this->m_logo_wnd)
						{
						/*HDC hdcX,hdcBits;
						hdcX = GetDC(_this->m_logo_wnd);
						hdcBits = CreateCompatibleDC(hdcX);
						SelectObject(hdcBits,_this->m_logo_min);
						BitBlt(hdcX,0,0,70,28,hdcBits,0,0,SRCCOPY);
						DeleteDC(hdcBits);
						ReleaseDC(_this->m_logo_wnd,hdcX);*/
						UpdateWindow(_this->m_logo_wnd);
						}
					break;
				}

			case WM_COMMAND:
				if (LOWORD(wParam) == ID_BUTTON_INFO)
				{
					if (IsWindow(_this->m_hwndStatus)){
						if (_this->m_hwndStatus)SetForegroundWindow(_this->m_hwndStatus);
						if (_this->m_hwndStatus)ShowWindow(_this->m_hwndStatus, SW_NORMAL);
					}else{
						SECURITY_ATTRIBUTES   lpSec;
						DWORD				  threadID;
						_this->m_statusThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE )ClientConnection::GTGBS_ShowStatusWindow,(LPVOID)_this,0,&threadID);
					}
					return 0;
				}
				if (LOWORD(wParam) ==9998)
				{
					vnclog.Print(0,_T("CLICKK %d\n"),HIWORD(wParam));
					switch (HIWORD(wParam)) {
						case 0:
								{
								int port;
								TCHAR fulldisplay[256];
								TCHAR display[256];
								GetDlgItemText(hwnd, 9999, display, 256);
								_tcscpy(fulldisplay, display);
								vnclog.Print(0,_T("CLICKK %s\n"),fulldisplay);
								ParseDisplay(fulldisplay, display, 256, &port);
								if (strcmp(display, "ID") == 0) {
									return TRUE;
								}
								_this->m_pApp->NewConnection(false,display,port);
								}
						}
					break;
					return TRUE;
				}

				if (LOWORD(wParam) == ID_BUTTON_SEP)
				{
					UINT Key;
					//_this->SendKeyEvent(XK_Execute,     true);
					//_this->SendKeyEvent(XK_Execute,     false);
					Key = DialogBox(_this->m_pApp->m_instance,MAKEINTRESOURCE(IDD_CUSTUM_KEY),NULL,(DLGPROC)ClientConnection::GTGBS_SendCustomKey_proc);
					if (Key>0){
						vnclog.Print(0,_T("START Send Custom Key %d\n"),Key);
						if ( (Key & KEYMAP_LALT_FLAG) == KEYMAP_LALT_FLAG){
							_this->SendKeyEvent(XK_Alt_L,true);
							_this->SendKeyEvent(Key ^ KEYMAP_LALT_FLAG,true);
							_this->SendKeyEvent(Key ^ KEYMAP_LALT_FLAG,false);
							_this->SendKeyEvent(XK_Alt_L,false);
						}else if ( (Key & KEYMAP_RALT_FLAG) ==KEYMAP_RALT_FLAG){
							_this->SendKeyEvent(XK_Alt_R,true);
							_this->SendKeyEvent(XK_Control_R,true);
							_this->SendKeyEvent(Key ^ KEYMAP_RALT_FLAG,true);
							_this->SendKeyEvent(Key ^ KEYMAP_RALT_FLAG,false);
							_this->SendKeyEvent(XK_Alt_R,false);
							_this->SendKeyEvent(XK_Control_R,false);
						}else if ( (Key &  KEYMAP_RCONTROL_FLAG) == KEYMAP_RCONTROL_FLAG){
							_this->SendKeyEvent(XK_Control_R,true);
							_this->SendKeyEvent(Key ^ KEYMAP_RCONTROL_FLAG,true);
							_this->SendKeyEvent(Key ^ KEYMAP_RCONTROL_FLAG,false);
							_this->SendKeyEvent(XK_Control_R,false);
						}else{
							_this->SendKeyEvent(Key,true);
							_this->SendKeyEvent(Key,false);
						}

						//adzm 2010-09
						_this->FlushWriteQueue(true, 5);

						vnclog.Print(0,_T("END   Send Custom Key %d\n"),Key);
					}
					SetForegroundWindow(_this->m_hwndcn);

					return 0;
				}

				if (LOWORD(wParam) == ID_BUTTON_END )
				{
					SendMessage(parent,WM_CLOSE,(WPARAM)0,(LPARAM)0);
					return 0;
				}

				if (LOWORD(wParam) == ID_BUTTON_CAD )
				{
					SendMessage(parent,WM_SYSCOMMAND,(WPARAM)ID_CONN_CTLALTDEL,(LPARAM)0);
					return 0;
				}

				if (LOWORD(wParam) == ID_BUTTON_FULLSCREEN )
				{
					SendMessage(parent,WM_SYSCOMMAND,(WPARAM)ID_FULLSCREEN,(LPARAM)0);
					return 0;
				}

				if (LOWORD(wParam) == ID_BUTTON_FTRANS )
				{
					if (_this->m_pFileTransfer->m_fFileTransferRunning)
					{
						_this->m_pFileTransfer->ShowFileTransferWindow(true);
					}
					else
						SendMessage(parent,WM_SYSCOMMAND,(WPARAM)ID_FILETRANSFER,(LPARAM)0);
					return 0;
				}

				if (LOWORD(wParam) == ID_BUTTON_DBUTTON )
				{
					SendMessage(parent,WM_SYSCOMMAND,(WPARAM)ID_DBUTTON,(LPARAM)0);
					return 0;
				}

				if (LOWORD(wParam) == ID_BUTTON_SW )
				{
					SendMessage(parent,WM_SYSCOMMAND,(WPARAM)ID_SW,(LPARAM)0);
					return 0;
				}

				if (LOWORD(wParam) == ID_BUTTON_DESKTOP )
				{
					SendMessage(parent,WM_SYSCOMMAND,(WPARAM)ID_DESKTOP,(LPARAM)0);
					return 0;
				}

				if (LOWORD(wParam) == ID_BUTTON_TEXTCHAT )
				{
					if (_this->m_pTextChat->m_fTextChatRunning)
					{
						_this->m_pTextChat->ShowChatWindow(true);
					}
					else
						SendMessage(parent,WM_SYSCOMMAND,(WPARAM)ID_TEXTCHAT,(LPARAM)0);
					return 0;
				}

				if (LOWORD(wParam) == ID_BUTTON_DINPUT )
				{
					if (_this->m_remote_mouse_disable)
					{
						_this->m_remote_mouse_disable=false;
						SendMessage(parent,WM_SYSCOMMAND,(WPARAM)ID_INPUT,(LPARAM)0);
						SendMessage(parent,WM_SIZE,(WPARAM)ID_DINPUT,(LPARAM)0);
					}
					else
					{
						_this->m_remote_mouse_disable=true;
						SendMessage(parent,WM_SYSCOMMAND,(WPARAM)ID_DINPUT,(LPARAM)0);
						SendMessage(parent,WM_SIZE,(WPARAM)ID_DINPUT,(LPARAM)0);
					}
					return 0;
				}

				if (LOWORD(wParam) == ID_BUTTON_PROPERTIES )
				{
					SendMessage(parent,WM_SYSCOMMAND,(WPARAM)IDC_OPTIONBUTTON,(LPARAM)0);
					return 0;
				}

				if (LOWORD(wParam) == ID_BUTTON_REFRESH )
				{
					SendMessage(parent,WM_SYSCOMMAND,(WPARAM)ID_REQUEST_REFRESH,(LPARAM)0);
					return 0;
				}

				if (LOWORD(wParam) == ID_BUTTON_STRG_ESC )
				{
					SendMessage(parent,WM_SYSCOMMAND,(WPARAM)ID_CONN_CTLESC,(LPARAM)0);
					return 0;
				}
			}
		}
return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

//
//
//
LRESULT CALLBACK ClientConnection::WndProchwnd(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	//	HWND parent;
    ClientConnection *_this = helper::SafeGetWindowUserData<ClientConnection>(hwnd);

	if (_this == NULL && iMsg != WM_CREATE)
		return DefWindowProc(hwnd, iMsg, wParam, lParam);
	switch (iMsg)
			{
			case WM_CREATE:
				_this = (ClientConnection*)((CREATESTRUCT*)lParam)->lpCreateParams;
				if (_this == NULL) return 0;
				return 0;

			case WM_REQUESTUPDATE:
				//_this->DoBlit();
				//_this->SendAppropriateFramebufferUpdateRequest();
				_this->HandleFramebufferUpdateRequest(wParam, lParam);
				return 0;

			case WM_UPDATEREMOTECLIPBOARDCAPS:
				//adzm 2010-09
				_this->UpdateRemoteClipboardCaps();
				return 0;

			case WM_NOTIFYPLUGINSTREAMING:
				// adzm 2010-09 - Notify streaming DSM plugin support
				_this->NotifyPluginStreamingSupport();
				return 0;

			case WM_PAINT:
				paintbuzy=true;
				_this->DoBlit();
				paintbuzy=false;
				return 0;

			case WM_SENDKEEPALIVE:
				// adzm 2010-09
				_this->Internal_SendKeepAlive(wParam == 1);
				return 0;

			case WM_TIMER:
				if (wParam !=0) {
					if (wParam == _this->m_emulate3ButtonsTimer)
					{
						_this->SubProcessPointerEvent(
							_this->m_emulateButtonPressedX,
							_this->m_emulateButtonPressedY,
							_this->m_emulateKeyFlags);
						//adzm 2010-09
						_this->FlushWriteQueue(true, 5);
						KillTimer(_this->m_hwndcn, _this->m_emulate3ButtonsTimer);
						_this->m_waitingOnEmulateTimer = false;
					} else if (wParam == _this->m_keepalive_timer) {
						// adzm 2009-08-02
						_this->SendKeepAlive(false, true);
					} else if (wParam == _this->m_flushMouseMoveTimer) {
						// adzm 2010-09
						if (_this->FlushThrottledMouseMove()) {
							_this->FlushWriteQueue();
						}
					}
					else if (wParam == _this->m_idle_timer) {
						if (_this->m_idle_time<60000) 
							SetTimer(_this->m_hwndcn, 1013, 5000, NULL);
						else 
							PostMessage(_this->m_hwndMain, WM_CLOSE, 0, 0);
					}
					else if (wParam == 1013) {
						_this->SetDormant(2);
					}
#ifdef _Gii
					else if (wParam == TOUCH_REGISTER_TIMER) {
						bool ret = RegisterTouchWindow(hwnd, 0);
						DWORD err = GetLastError();
#ifdef _DEBUG
						char			szText[256];
						if (ret != 0)
						{
							sprintf(szText, "RegisterTouchWindow Success \n");
						}
						else
						{
							sprintf(szText, "RegisterTouchWindow Failed with error code = %i \n", err);
						}
						OutputDebugString(szText);
#endif
						Sleep(10);
						KillTimer(hwnd, TOUCH_REGISTER_TIMER);
					}
#endif
				}
				return 0;

			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
				if (_this->m_SWselect)
				{
					_this->m_SWpoint.x=LOWORD(lParam);
					_this->m_SWpoint.y=HIWORD(lParam);
					_this->SendSW(_this->m_SWpoint.x,_this->m_SWpoint.y);
					if (_this->m_opts.m_IdleInterval > 0) { KillTimer(_this->m_hwndcn, 1013);SetTimer(hwnd, _this->m_idle_timer, _this->m_idle_time, NULL); _this->SetDormant(false); }
					return 0;
				}
			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
			case WM_MOUSEMOVE:
				{
					if (_this->m_opts.m_IdleInterval > 0) { KillTimer(_this->m_hwndcn, 1013);SetTimer(hwnd, _this->m_idle_timer, _this->m_idle_time, NULL); _this->SetDormant(false); }
					if (_this->m_SWselect) {return 0;}
					if (!_this->m_running) return 0;
//					if (GetFocus() != hwnd) return 0;
//					if (GetFocus() != _this->m_hwnd) return 0;
					if (GetFocus() != _this->m_hwndMain) return 0;
					int x = LOWORD(lParam);
					int y = HIWORD(lParam);
					wParam = MAKEWPARAM(LOWORD(wParam), 0);
					if (_this->InFullScreenMode()) {
						if (_this->BumpScroll(x,y))
							return 0;
					}
					if ( _this->m_opts.m_ViewOnly) return 0;
#ifdef _Gii
					//Filter touch/pen events
					if(IsPenEvent(GetMessageExtraInfo()) || IsTouchEvent(GetMessageExtraInfo()))
					{
						//ignore mouse events.
						return 0;
					}
#endif
					//adzm 2010-09
					if (_this->ProcessPointerEvent(x,y, wParam, iMsg)) {
						_this->FlushWriteQueue(true, 5);
					}
					return 0;
				}
#ifdef _Gii
			case WM_TOUCH:
				//view_only is also for the touch
				if (_this->m_opts.m_ViewOnly) return 0;
				_this->mytouch->OnTouch(hwnd, wParam, lParam);
				return 0;
#endif
			case WM_KEYDOWN:
			case WM_KEYUP:
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
				{					
					if (_this->m_opts.m_IdleInterval > 0) {KillTimer(_this->m_hwndcn, 1013);SetTimer(hwnd, _this->m_idle_timer, _this->m_idle_time, NULL); _this->SetDormant(false);}
					if (!_this->m_running) return 0;
					if ( _this->m_opts.m_ViewOnly) return 0;
					_this->ProcessKeyEvent((int) wParam, (DWORD) lParam);
					//adzm 2010-09
					_this->FlushWriteQueue(true, 5);
					return 0;
				}

			case WM_CHAR:
			case WM_SYSCHAR:
			case WM_DEADCHAR:
			case WM_SYSDEADCHAR:
				return 0;

			case WM_SETFOCUS:
				if (_this->InFullScreenMode())
				{
					//HWND handleW1 = FindWindow("Shell_traywnd", "");
   					//SetWindowPos(handleW1, 0, 0, 0, 0, 0, 128);
					SetWindowPos(hwnd, HWND_TOPMOST, 0,0,100,100, SWP_NOMOVE | SWP_NOSIZE);
				}
				_this->m_keymap->Reset();				
				return 0;

				// Cacnel modifiers when we lose focus
			case WM_KILLFOCUS:
				{
					if (!_this->m_running) return 0;
					if (_this->InFullScreenMode()) {

						//HWND handleW1 = FindWindow("Shell_traywnd", "");
   						//SetWindowPos(handleW1, 0, 0, 0, 0, 0, 64);
						SetWindowPos(hwnd, HWND_TOP, 0,0,100,100, SWP_NOMOVE | SWP_NOSIZE| SWP_NOACTIVATE);
						// We must top being topmost, but we want to choose our
						// position carefully.
						/*HWND foreground = GetForegroundWindow();
						HWND hwndafter = NULL;
						if ((foreground == NULL) ||
							(GetWindowLong(foreground, GWL_EXSTYLE) & WS_EX_TOPMOST)) {
							hwndafter = HWND_NOTOPMOST;
						} else {
							hwndafter = GetNextWindow(foreground, GW_HWNDNEXT);
						}

						SetWindowPos(hwnd, hwndafter, 0,0,100,100, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);*/
					}
					if (_this->m_opts.m_ViewOnly) return 0;
					_this->m_keymap->ReleaseAllKeys(_this);
					//adzm 2010-09
					_this->FlushWriteQueue(true, 5);
					/*
					vnclog.Print(6, _T("Losing focus - cancelling modifiers\n"));
					_this->SendKeyEvent(XK_Alt_L,     false);
					_this->SendKeyEvent(XK_Control_L, false);
					_this->SendKeyEvent(XK_Shift_L,   false);
					_this->SendKeyEvent(XK_Alt_R,     false);
					_this->SendKeyEvent(XK_Control_R, false);
					_this->SendKeyEvent(XK_Shift_R,   false);
					*/
					return 0;
				}

			case WM_CLOSE:
				{
                    // April 8 2008 jdp
					static bool boxopen=false;
					if (boxopen) return 0;
                    if (lParam == 0)
					{
						if (_this->m_opts.m_fExitCheck) //PGM @ Advantig
						{ //PGM @ Advantig
						    boxopen=true;
						    if (MessageBox(hwnd, sz_L75,sz_L76,MB_YESNO | MB_ICONQUESTION | MB_SETFOREGROUND | MB_TOPMOST|MB_SYSTEMMODAL) == IDNO)
							{
								boxopen=false;
								return 0;
							}
						} // PGM @ Advantig.com
    					boxopen=false;
                        _this->m_bClosedByUser = true;
					}
					// sf@2002 - Do not close vncviewer if the File Transfer GUI is open !
					if (_this->m_pFileTransfer->m_fFileTransferRunning)
					{
						_this->m_pFileTransfer->ShowFileTransferWindow(true);
						MessageBox(hwnd, sz_L85,
							sz_L88,
							MB_OK | MB_ICONSTOP | MB_SETFOREGROUND | MB_TOPMOST|MB_SYSTEMMODAL);
						return 0;
					}

					// sf@2002 - Do not close vncviewer if the Text Chat GUI is open !
					if (_this->m_pTextChat->m_fTextChatRunning)
					{
						_this->m_pTextChat->ShowChatWindow(true);
						MessageBox(hwnd, sz_L86,
							sz_L88,
							MB_OK | MB_ICONSTOP | MB_SETFOREGROUND | MB_TOPMOST|MB_SYSTEMMODAL);
						return 0;
					}

					if (_this->m_fOptionsOpen)
					{
						MessageBox(hwnd, sz_L87,
							sz_L88,
							MB_OK | MB_ICONSTOP | MB_SETFOREGROUND | MB_TOPMOST|MB_SYSTEMMODAL);
						return 0;
					}

                    // 8 April 2008 jdp
                    ::ShowWindow(hwnd, SW_HIDE);
					// Close the worker thread as well
					_this->KillThread();

					DestroyWindow(hwnd);
					return 0;
				}

			case WM_DESTROY:
				{
				#ifndef UNDER_CE
				// Remove us from the clipboard viewer chain
				if (hwnd == _this->m_hwndcn) {
					BOOL res = ChangeClipboardChain( hwnd, _this->m_hwndNextViewer);
					_this->m_hwndNextViewer = NULL;
					vnclog.Print(6, _T("WndProchwnd ChangeClipboardChain hwnd 0x%08x / m_hwndcn 0x%08x, 0x%08x (%li)\n"), hwnd, _this->m_hwndcn, _this->m_hwndNextViewer, res);
				}
				#endif
#ifdef _Gii
				UnregisterTouchWindow(hwnd);
#endif
				KillTimer(_this->m_hwndcn, _this->m_idle_timer);
				KillTimer(_this->m_hwndcn, 1013);
				if (_this->m_waitingOnEmulateTimer)
				{
				KillTimer(_this->m_hwndcn, _this->m_emulate3ButtonsTimer);
				_this->m_waitingOnEmulateTimer = false;
				}
				/*
				  _this->m_hwnd = 0;
				  // We are currently in the main thread.
				  // The worker thread should be about to finish if
				  // it hasn't already. Wait for it.
				  try {
				  void *p;
				  _this->join(&p);  // After joining, _this is no longer valid
				  } catch (omni_thread_invalid& e) {
				  // The thread probably hasn't been started yet,
				  }*/

					return 0;
				}

			case WM_QUERYNEWPALETTE:
				{
					TempDC hDC(hwnd);

					// Select and realize hPalette
					PaletteSelector p(hDC, _this->m_hPalette);
					InvalidateRect(hwnd, NULL, FALSE);
					UpdateWindow(hwnd);
					return TRUE;
				}

			case WM_PALETTECHANGED:
				// If this application did not change the palette, select
				// and realize this application's palette
				if ((HWND) wParam != hwnd)
				{
					// Need the window's DC for SelectPalette/RealizePalette
					TempDC hDC(hwnd);
					PaletteSelector p(hDC, _this->m_hPalette);
					// When updating the colors for an inactive window,
					// UpdateColors can be called because it is faster than
					// redrawing the client area (even though the results are
					// not as good)
#ifndef UNDER_CE
					UpdateColors(hDC);
#else
					InvalidateRect(hwnd, NULL, FALSE);
					UpdateWindow(hwnd);
#endif
				}
				break;

#ifndef UNDER_CE
			case WM_SIZING:
				return 0;
				{
					// Don't allow sizing larger than framebuffer
					RECT *lprc = (LPRECT) lParam;
					switch (wParam) {
					case WMSZ_RIGHT:
					case WMSZ_TOPRIGHT:
					case WMSZ_BOTTOMRIGHT:
						lprc->right = min(lprc->right, lprc->left + _this->m_fullwinwidth+1);
						break;
					case WMSZ_LEFT:
					case WMSZ_TOPLEFT:
					case WMSZ_BOTTOMLEFT:
						lprc->left = max(lprc->left, lprc->right - _this->m_fullwinwidth);
						break;
					}

					switch (wParam) {
					case WMSZ_TOP:
					case WMSZ_TOPLEFT:
					case WMSZ_TOPRIGHT:
						lprc->top = max(lprc->top, lprc->bottom - _this->m_fullwinheight);
						break;
					case WMSZ_BOTTOM:
					case WMSZ_BOTTOMLEFT:
					case WMSZ_BOTTOMRIGHT:
						lprc->bottom = min(lprc->bottom, lprc->top + _this->m_fullwinheight);
						break;
					}

					return 0;
				}

			case WM_SETCURSOR:
				{
					// if we have the focus, let the cursor change as normal
					if (GetFocus() == hwnd)
						break;

					HCURSOR h;
					switch (_this->m_opts.m_localCursor) {
					case NOCURSOR:
						h= LoadCursor(_this->m_pApp->m_instance, MAKEINTRESOURCE(IDC_NOCURSOR));
						break;
					case NORMALCURSOR:
						h= LoadCursor(NULL, IDC_ARROW);
						break;
					case DOTCURSOR:
					default:
						h= LoadCursor(_this->m_pApp->m_instance, MAKEINTRESOURCE(IDC_DOTCURSOR));
					}
					if (_this->m_SWselect) h= LoadCursor(_this->m_pApp->m_instance, MAKEINTRESOURCE(IDC_CURSOR1));
					SetCursor(h);

					return 0;
				}

			case WM_DRAWCLIPBOARD:
				_this->ProcessLocalClipboardChange();
				return 0;

			case WM_CHANGECBCHAIN:
				{
					// The clipboard chain is changing
					HWND hWndRemove = (HWND) wParam;     // handle of window being removed
					HWND hWndNext = (HWND) lParam;       // handle of next window in chain
					vnclog.Print(6, _T("WM_CHANGECBCHAIN remove 0x%08x, next 0x%08x (current m_hwndcn 0x%08x / hwnd 0x%08x, next 0x%08x)\n"), hWndRemove, hWndNext, _this->m_hwndcn, hwnd, _this->m_hwndNextViewer);
					// If next window is closing, update our pointer.
					if (hWndRemove == _this->m_hwndNextViewer)
						_this->m_hwndNextViewer = hWndNext;
					// Otherwise, pass the message to the next link.
					else if (_this->m_hwndNextViewer != NULL && _this->m_hwndNextViewer != INVALID_HANDLE_VALUE) {
						// adzm - 2010-07 - Fix clipboard hangs
						// use SendNotifyMessage instead of SendMessage so misbehaving or hung applications
						// (like ourself before this) won't cause our thread to hang.
						::SendNotifyMessage(_this->m_hwndNextViewer, WM_CHANGECBCHAIN,
							(WPARAM) hWndRemove,  (LPARAM) hWndNext );
					}

					return 0;
				}
#endif

			// Modif VNCon MultiView support
			// Messages used by VNCon - Copyright (C) 2001-2003 - Alastair Burr
			case WM_GETSCALING:
				{
					WPARAM wPar;
					wPar = MAKEWPARAM(_this->m_hScrollMax, _this->m_vScrollMax);
					SendMessage((HWND)wParam, WM_GETSCALING, wPar, lParam);
					return TRUE;
				}

			case WM_SETSCALING:
				{
					_this->m_opts.m_scaling = true;
					_this->m_opts.m_scale_num = wParam;
					_this->m_opts.m_scale_den = lParam;
					if (_this->m_opts.m_scale_num == 1 && _this->m_opts.m_scale_den == 1)
						_this->m_opts.m_scaling = false;
					_this->SizeWindow();
					InvalidateRect(hwnd, NULL, TRUE);
					return TRUE;
				}

			case WM_SETVIEWONLY:
				{
					{
						// adzm - 2010-07 - Extended clipboard
						bool bNewViewOnly = (wParam == 1);
						bool bUpdateCaps = false;
						if (_this->m_opts.m_ViewOnly != bNewViewOnly) {
							bUpdateCaps = true;
						}
						_this->m_opts.m_ViewOnly = bNewViewOnly;

						if (bUpdateCaps) {
							_this->UpdateRemoteClipboardCaps();
						}
					}
					return TRUE;
				}
			// End Modif for VNCon MultiView support
			}//end switch (iMsg)

			return DefWindowProc(hwnd, iMsg, wParam, lParam);
}
void
ClientConnection:: ConvertAll(int width, int height, int xx, int yy,int bytes_per_pixel,BYTE* source,BYTE* dest,int framebufferWidth)
{
	int bytesPerInputRow = width * bytes_per_pixel;
	int bytesPerOutputRow = framebufferWidth * bytes_per_pixel;
	//8bit pitch need to be taken in account
	if (bytesPerOutputRow % 4)
		bytesPerOutputRow += 4 - bytesPerOutputRow % 4;

	BYTE *sourcepos,*destpos;
	destpos = (BYTE *)dest + (bytesPerOutputRow * yy)+(xx * bytes_per_pixel);
	sourcepos=(BYTE*)source;

    int y;
    width*=bytes_per_pixel;
    for (y=0; y<height; y++) {
        memcpy(destpos, sourcepos, width);
        sourcepos = (BYTE*)sourcepos + bytesPerInputRow;
        destpos = (BYTE*)destpos + bytesPerOutputRow;
    }
}

void
ClientConnection:: Copybuffer(int width, int height, int xx, int yy,int bytes_per_pixel,BYTE* source,BYTE* dest,int framebufferWidth)
{
	int bytesPerOutputRow = framebufferWidth * bytes_per_pixel;
	//8bit pitch need to be taken in account
	if (bytesPerOutputRow % 4)
		bytesPerOutputRow += 4 - bytesPerOutputRow % 4;
	BYTE *sourcepos,*destpos;
	destpos = (BYTE *)dest + (bytesPerOutputRow * yy)+(xx * bytes_per_pixel);
	sourcepos=(BYTE*)source + (bytesPerOutputRow * yy)+(xx * bytes_per_pixel);

    int y;
    width*=bytes_per_pixel;
    for (y=0; y<height; y++) {
        memcpy(destpos, sourcepos, width);
        sourcepos = (BYTE*)sourcepos + bytesPerOutputRow;
        destpos = (BYTE*)destpos + bytesPerOutputRow;
    }
}

void
ClientConnection:: Copyto0buffer(int width, int height, int xx, int yy,int bytes_per_pixel,BYTE* source,BYTE* dest,int framebufferWidth)
{
	int bytesPerOutputRow = framebufferWidth * bytes_per_pixel;
	//8bit pitch need to be taken in account
	if (bytesPerOutputRow % 4)
		bytesPerOutputRow += 4 - bytesPerOutputRow % 4;
	BYTE *sourcepos,*destpos;
	destpos = (BYTE *)dest;
	sourcepos=(BYTE*)source + (bytesPerOutputRow * yy)+(xx * bytes_per_pixel);
		int y;
		width*=bytes_per_pixel;
		for (y=0; y<height; y++) {
			memcpy(destpos, sourcepos, width);
			sourcepos = (BYTE*)sourcepos + bytesPerOutputRow;
			destpos = (BYTE*)destpos + width;
		}
}

void
ClientConnection:: Copyfrom0buffer(int width, int height, int xx, int yy,int bytes_per_pixel,BYTE* source,BYTE* dest,int framebufferWidth)
{
	int bytesPerOutputRow = framebufferWidth * bytes_per_pixel;
	//8bit pitch need to be taken in account
	if (bytesPerOutputRow % 4)
		bytesPerOutputRow += 4 - bytesPerOutputRow % 4;
	BYTE *sourcepos,*destpos;
	destpos = (BYTE *)dest + (bytesPerOutputRow * yy)+(xx * bytes_per_pixel);
	sourcepos=(BYTE*)source;

    int y;
    width*=bytes_per_pixel;
    for (y=0; y<height; y++) {
        memcpy(destpos, sourcepos, width);
        sourcepos = (BYTE*)sourcepos + width;
        destpos = (BYTE*)destpos + bytesPerOutputRow;
    }
}

void
ClientConnection:: Switchbuffer(int width, int height, int xx, int yy,int bytes_per_pixel,BYTE* source,BYTE* dest,int framebufferWidth)
{
	int bytesPerOutputRow = framebufferWidth * bytes_per_pixel;
	//8bit pitch need to be taken in account
	if (bytesPerOutputRow % 4)
		bytesPerOutputRow += 4 - bytesPerOutputRow % 4;
	BYTE *sourcepos,*destpos,*tempbuffer;
	destpos = (BYTE *)dest + (bytesPerOutputRow * yy)+(xx * bytes_per_pixel);
	sourcepos=(BYTE*)source + (bytesPerOutputRow * yy)+(xx * bytes_per_pixel);
    int y;
    width*=bytes_per_pixel;
	tempbuffer=new BYTE[width];
    for (y=0; y<height; y++) {
		memcpy(tempbuffer, destpos, width);
        memcpy(destpos, sourcepos, width);
		memcpy(sourcepos,tempbuffer, width);
        sourcepos = (BYTE*)sourcepos + bytesPerOutputRow;
        destpos = (BYTE*)destpos + bytesPerOutputRow;
    }
	delete [] tempbuffer;
}

void
ClientConnection:: ConvertPixel(int xx, int yy,int bytes_per_pixel,BYTE* source,BYTE* dest,int framebufferWidth)
{
	int bytesPerInputRow = bytes_per_pixel;
	int bytesPerOutputRow = framebufferWidth * bytes_per_pixel;
	//8bit pitch need to be taken in account
	if (bytesPerOutputRow % 4)
		bytesPerOutputRow += 4 - bytesPerOutputRow % 4;
	BYTE *sourcepos,*destpos;
	destpos = (BYTE *)dest + (bytesPerOutputRow * yy)+(xx * bytes_per_pixel);
	sourcepos=(BYTE*)source;
    memcpy(destpos, sourcepos, bytes_per_pixel);
}

void
ClientConnection:: ConvertPixel_to_bpp_from_32(int xx, int yy,int bytes_per_pixel,BYTE* source,BYTE* dest,int framebufferWidth)
{
	int bytesPerInputRow = bytes_per_pixel;
	int bytesPerOutputRow = framebufferWidth * bytes_per_pixel;
	//8bit pitch need to be taken in account
	if (bytesPerOutputRow % 4)
		bytesPerOutputRow += 4 - bytesPerOutputRow % 4;
	BYTE *destpos;
	destpos = (BYTE *)dest + (bytesPerOutputRow * yy)+(xx * bytes_per_pixel);
	BYTE r=source[0];
	BYTE g=source[1];
	BYTE b=source[2];
	SETUP_COLOR_SHORTCUTS;
	switch(bytes_per_pixel)
	{
	case 1:
		{
		unsigned char color=0;
		if (gs==1) // 8 color
		{
			color=((r >> 7)<<2) | ((g>>7) << 1) | (b >> 7);
		}
		if (gs==2) // 64 color
		{
			color=((r >> 6)<<4) | ((g>>6) << 2) | (b >> 6);
		}
		if (gs==3) // 254 color
		{
			color=(r >> 5) | ((g>>5) << 3) | (b >> 6)<<6;
		}
		memcpy(destpos, &color, bytes_per_pixel);
		}
		break;
	case 2:
		{
		unsigned short color;
		if (gs==5) color=(r>>3 << 11)|(g>>2 << 5)|(b>>3);
		else color=(r>>3 << 11)|(g>>3 << 5)|(b>>3)<<1;
		memcpy(destpos, &color, bytes_per_pixel);
		}
		break;
	case 4:
		{
		BYTE color[4];
		color[0]=source[2];
		color[1]=source[1];
		color[2]=source[0];
		color[3]=0;
		memcpy(destpos, &color, bytes_per_pixel);
		}
		break;
	}
}

void
ClientConnection::SolidColor(int width, int height, int xx, int yy,int bytes_per_pixel,BYTE* source,BYTE* dest,int framebufferWidth)
{
	int bytesPerOutputRow = framebufferWidth * bytes_per_pixel;
	//8bit pitch need to be taken in account
	if (bytesPerOutputRow % 4)
		bytesPerOutputRow += 4 - bytesPerOutputRow % 4;
	BYTE *sourcepos,*destpos;
	destpos = (BYTE *)dest + (bytesPerOutputRow * yy)+(xx * bytes_per_pixel);
	sourcepos=(BYTE*)source;

    int y,x;
    for (y=0; y<height; y++) {
		for (x=0; x< width;x++)
			{
				memcpy(destpos+x*bytes_per_pixel, sourcepos, bytes_per_pixel);
			}
        destpos = (BYTE*)destpos + bytesPerOutputRow;
    }
}

bool
ClientConnection:: Check_Rectangle_borders(int x,int y,int w,int h)
{
	if (x<0) return false;
	if (y<0) return false;
	if (x+w>m_si.framebufferWidth) return false;
	if (y+h>m_si.framebufferHeight) return false;
	if (x+w<x) return false;
	if (y+h<y) return false;
	return true;
}

// adzm 2010-09
void ClientConnection::SendKeepAlive(bool bForce, bool bAsync)
{
	if (bAsync) {
		PostMessage(m_hwndcn, WM_SENDKEEPALIVE, (WPARAM)(bForce ? 1 : 0), (LPARAM)0);
	} else {
		SendMessage(m_hwndcn, WM_SENDKEEPALIVE, (WPARAM)(bForce ? 1 : 0), (LPARAM)0);
	}
}

// adzm 2010-09
void ClientConnection::Internal_SendKeepAlive(bool bForce)
{
    if (m_server_wants_keepalives)
    {
		//adzm 2010-08-01
		if (m_keepalive_timer != 0) {
			KillTimer(m_hwndcn, m_keepalive_timer);
		}

		//adzm 2010-08-01
		DWORD nInterval = (DWORD)m_opts.m_keepAliveInterval * 1000;
		DWORD nTicksSinceLastSent = GetTickCount() - m_LastSentTick;

		if (!bForce && nTicksSinceLastSent < nInterval) {
			//adzm 2010-08-01
			DWORD nDelay = nInterval - nTicksSinceLastSent;
			if (nDelay >= 100) {
				if (m_keepalive_timer != 0) {
					SetTimer(m_hwndcn, m_keepalive_timer, nDelay, NULL);
				}
				return;
			}
		}

#if defined(_DEBUG)
        char msg[255];
        sprintf(msg, "keepalive requested %u ms since last one\n", nTicksSinceLastSent);
        OutputDebugString(msg);

#endif
        rfbKeepAliveMsg kp;
        memset(&kp, 0, sizeof kp);
        kp.type = rfbKeepAlive;
        WriteExact_timeout((char*)&kp, sz_rfbKeepAliveMsg, rfbKeepAlive,5);

		if (m_keepalive_timer != 0 && m_opts.m_keepAliveInterval > 0) {
			SetTimer(m_hwndcn, m_keepalive_timer, m_opts.m_keepAliveInterval * 1000, NULL);
		}
    }
}

//adzm 2010-09 -
ClientConnection::PendingMouseMove::PendingMouseMove()
	: dwLastSentMouseMove(0),
	x(0),
	y(0),
	keyflags(0),
	bValid(false),
	dwMinimumMouseMoveInterval(0) // changed to 0 from 150; need to have an interface for this
{
	
}

// adzm 2010-09 - Notify streaming DSM plugin support
void ClientConnection::NotifyPluginStreamingSupport()
{
	rfbNotifyPluginStreamingMsg msg;
    memset(&msg, 0, sizeof(rfbNotifyPluginStreamingMsg));
	msg.type = rfbNotifyPluginStreaming;

	//adzm 2010-09 - minimize packets. SendExact flushes the queue.
	WriteExact((char *)&msg, sz_rfbNotifyPluginStreamingMsg, rfbNotifyPluginStreaming);
	m_fPluginStreamingOut = true;
}

static HWND hList=NULL;  // List View identifier
LVCOLUMN LvCol; // Make Coluom struct for ListView
LVITEM LvItem;  // ListView Item struct
int iSelect=0;
int flag=0;
HWND hEdit=NULL;

BOOL CALLBACK DialogProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
//	if ((Message!=WM_TIMER || Message!=WM_CLOSE) && scanner) return false;
  switch(Message)
  {
		case WM_CLOSE:
			{
			     EndDialog(hWnd,0); // kill dialog
			}
			break;

		case WM_NOTIFY:
		{
			switch(LOWORD(wParam))
			{
			    case IDC_LIST1:
                if(((LPNMHDR)lParam)->code == NM_DBLCLK)
				{
				  char Text[255]={0};
				  char Temp[255]={0};
				  char Temp1[255]={0};
				  int iSlected=0;
				  int j=0;

				  iSlected=SendMessage(hList,LVM_GETNEXTITEM,-1,LVNI_FOCUSED);

				  if(iSlected==-1)
				  {
                    MessageBox(hWnd,"No Items in ListView","Error",MB_OK|MB_ICONINFORMATION);
					break;
				  }

				  memset(&LvItem,0,sizeof(LvItem));
                  LvItem.mask=LVIF_TEXT;
				  LvItem.iSubItem=0;
				  LvItem.pszText=Text;
				  LvItem.cchTextMax=256;
				  LvItem.iItem=iSlected;
				  SendMessage(hList,LVM_GETITEMTEXT, iSlected, (LPARAM)&LvItem);
				  EndDialog(hWnd,iSlected+1);
				}
				if(((LPNMHDR)lParam)->code == NM_CLICK)
				{
					iSelect=SendMessage(hList,LVM_GETNEXTITEM,-1,LVNI_FOCUSED);

					if(iSelect==-1)
					{
                      MessageBox(hWnd,"No Vnc server selected","Error",MB_OK|MB_ICONINFORMATION);
					  break;
					}
					flag=1;
				}

				if(((LPNMHDR)lParam)->code == LVN_BEGINLABELEDIT)
				{
                  hEdit=ListView_GetEditControl(hList);
				}

				if(((LPNMHDR)lParam)->code == LVN_ENDLABELEDIT)
				{
					int iIndex;
					char text[255]="";
					iIndex=SendMessage(hList,LVM_GETNEXTITEM,-1,LVNI_FOCUSED);
				    LvItem.iSubItem=0;
                    LvItem.pszText=text;
                    GetWindowText(hEdit, text, sizeof(text));
					SendMessage(hList,LVM_SETITEMTEXT,(WPARAM)iIndex,(LPARAM)&LvItem);
				}
				break;
			}
			return 0;
		}

		case WM_PAINT:
			{
				InvalidateRect(hWnd,NULL,true);
				return 0;
			}
			break;

		// This Window Message is the heart of the dialog  //
		//================================================//
		case WM_INITDIALOG:
			{
				//CentreWindow(hWnd);
				ClientConnection *cc=(ClientConnection*)lParam;

                SetFocus(hWnd);
				hList=GetDlgItem(hWnd,IDC_LIST1); // get the ID of the ListView
				SendMessage(hList,LVM_SETEXTENDEDLISTVIEWSTYLE,0,LVS_EX_FULLROWSELECT); // Set style

				// Here we put the info on the Coulom headers
				// this is not data, only name of each header we like
                memset(&LvCol,0,sizeof(LvCol)); // Reset Coluom
				LvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM; // Type of mask
				LvCol.cx=0x100;                                // width between each coloum
				LvCol.pszText="Select Helpdesk to connect to .......";                     // First Header
// 				LvCol.cx=0x60;

				// Inserting Couloms as much as we want
				SendMessage(hList,LVM_INSERTCOLUMN,0,(LPARAM)&LvCol); // Insert/Show the coloum
                memset(&LvItem,0,sizeof(LvItem)); // Reset Item Struct

				//  Setting properties Of Items:

				LvItem.mask=LVIF_TEXT;   // Text Style
				LvItem.cchTextMax = 256; // Max size of test

				LvItem.iItem=0;          // choose item
				LvItem.iSubItem=0;       // Put in first coluom
				LvItem.pszText="Item 0"; // Text to display (can be from a char variable) (Items)

				int iItem=0;
				CARD8 nr_lines;
				cc->ReadExact((char *)&nr_lines, sizeof(nr_lines));
				char line[128];
				for ( int a=0;a<nr_lines;a++)
				{
					memset(line,0,128);
					cc->ReadExact((char *)line, 128);
					iItem=SendMessage(hList,LVM_GETITEMCOUNT,0,0);
					LvItem.iItem=iItem;            // choose item
					LvItem.iSubItem=0;         // Put in first coluom
					LvItem.pszText=line;
					SendMessage(hList,LVM_INSERTITEM,0,(LPARAM)&LvItem);
					iItem++;
				}
				return TRUE; // Always True
			}
			break;

     // This Window Message will control the dialog  //
	//==============================================//
        case WM_COMMAND:
			switch (LOWORD(wParam))
				{
					case IDOK:
						{
						 int iSlected=0;
						  iSlected=SendMessage(hList,LVM_GETNEXTITEM,-1,LVNI_FOCUSED);
						  if(iSlected==-1)
						  {
							MessageBox(hWnd,"No Items in ListView","Error",MB_OK|MB_ICONINFORMATION);
							break;
						  }
						EndDialog(hWnd,iSlected+1); // kill dialog
						}
						break;
					case IDCANCEL:
						EndDialog(hWnd,0);
						break;
					}
			break;

	    default:return FALSE;
    }

	return TRUE;
}