//  Copyright (C) 2002 Ultr@VNC Team Members. All Rights Reserved.
//
//  Copyright (C) 2000-2002 Const Kaplinsky. All Rights Reserved.
//
//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
//
//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file is part of the VNC system.
//
//  The VNC system is free software; you can redistribute it and/or modify
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
// If the source code for the VNC system is not available from the place
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.


// Many thanks to Randy Brown <rgb@inven.com> for providing the 3-button
// emulation code.

// This is the main source for a ClientConnection object.
// It handles almost everything to do with a connection to a server.
// The decoding of specific rectangle encodings is done in separate files.


#define _WIN32_WINDOWS 0x0410
#define WINVER 0x0400

#include "stdhdrs.h"

#include "vncviewer.h"

#ifdef UNDER_CE
#include "omnithreadce.h"
#define SD_BOTH 0x02
#else
#include "omnithread.h"
#endif

#include "ClientConnection.h"
#include "SessionDialog.h"
#include "AuthDialog.h"
#include "AboutBox.h"
#include "LowLevelHook.h"
#include "common/win32_helpers.h"

#include "Exception.h"
extern "C" {
	#include "vncauth.h"
}

#include <rdr/FdInStream.h>
#include <rdr/ZlibInStream.h>
#include <rdr/Exception.h>

#include <rfb/dh.h>

#include <DSMPlugin/DSMPlugin.h> // sf@2002

#define INITIALNETBUFSIZE 4096
#define MAX_ENCODINGS (LASTENCODING+10)
#define VWR_WND_CLASS_NAME _T("VNCviewer")
#define VWR_WND_CLASS_NAME_VIEWER _T("VNCviewerwindow")
#define SESSION_MRU_KEY_NAME _T("Software\\ORL\\VNCviewer\\MRU")
#define ID_MDI_FIRSTCHILD 60000

const UINT FileTransferSendPacketMessage = RegisterWindowMessage("UltraVNC.Viewer.FileTransferSendPacketMessage");
extern bool g_passwordfailed;
bool havetobekilled;
bool forcedexit=false;
BOOL ResizeAllWindows( void );
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
extern char sz_L51[64];
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
extern char sz_L77[64];
extern char sz_L78[64];
extern char sz_L79[64];
extern char sz_L80[64];
extern char sz_L81[64];
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
extern char sz_L94[64];

extern char sz_F1[64];
extern char sz_F5[128];
extern char sz_F6[64];
extern bool command_line;

extern HWND m_hwndMain;
//extern HWND m_hwndTBwin;
extern HWND m_TrafficMonitor;
extern HWND m_hMDIClient;
extern HWND m_hwndTab;

extern int g_iMaxChild;
extern int g_iNumChild;
extern bool fullscreen;
extern CHILD* g_child;
extern HANDLE m_bitmapBACK;
extern HANDLE m_bitmapNONE;



// *************************************************************************
//  A Client connection involves two threads - the main one which sets up
//  connections and processes window messages and inputs, and a 
//  client-specific one which receives, decodes and draws output data 
//  from the remote server.
//  This first section contains bits which are generally called by the main
//  program thread.
// *************************************************************************
ClientConnection::ClientConnection()
{
	m_keymap = NULL;
	m_keymapJap = NULL;
}

ClientConnection::ClientConnection(VNCviewerApp *pApp) 
  : fis(0), zis(0)
{
	Init(pApp);
}

ClientConnection::ClientConnection(VNCviewerApp *pApp,LPTSTR file) 
  : fis(0), zis(0)
{
	Init(pApp);
	strcpy(m_opts.m_configFilename,file);
	m_opts.m_configSpecified=true;
//	m_opts.m_NoStatus=true;
}

ClientConnection::ClientConnection(VNCviewerApp *pApp, SOCKET sock) 
  : fis(0), zis(0)
{
	Init(pApp);
    if (m_opts.autoDetect)
	{
      m_opts.m_Use8Bit = rfbPF256Colors; //true;
	  m_opts.m_fEnableCache = true; // sf@2002
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

ClientConnection::ClientConnection(VNCviewerApp *pApp, LPTSTR host, int port)
  : fis(0), zis(0)
{
	Init(pApp);
    if (m_opts.autoDetect)
	{
		m_opts.m_Use8Bit = rfbPF256Colors; //true;
		m_opts.m_fEnableCache = true; // sf@2002
	}
	_tcsncpy(m_host, host, MAX_HOST_NAME_LEN);
	m_port = port;
	_tcsncpy(m_proxyhost,m_opts.m_proxyhost, MAX_HOST_NAME_LEN);
	m_proxyport=m_opts.m_proxyport;
	m_fUseProxy = m_opts.m_fUseProxy;
}

void ClientConnection::Init(VNCviewerApp *pApp)
{
	Pressed_Cancel=false;
	saved_set=false;
	m_hwnd = 0;
	m_desktopName = NULL;
	m_desktopName_viewonly = NULL;
	m_port = -1;
	m_proxyport = -1;
//	m_proxy = 0;
	m_serverInitiated = false;
	m_netbuf = NULL;
	m_netbufsize = 0;
	m_queuebuff=NULL;
	m_queuesize=0;
	m_queuebuffsize=0;
	m_direct=true;
	m_zlibbuf = NULL;
	m_zlibbufsize = 0;
	m_decompStreamInited = false;
	m_hwndNextViewer = NULL;	
	m_pApp = pApp;
	m_dormant = false;
	m_hBitmapDC = NULL;
	m_hBitmap = NULL;
	m_bitsCache=NULL;
	m_bitsCacheSwapBuffer=0;
	m_hPalette = NULL;
	m_encPasswd[0] = '\0';
	m_encPasswdMs[0] = '\0'; // act: add mspasswd storage
	m_ms_user[0] = '\0';    // act: add msuser storage
	m_cmdlnUser[0] = '\0'; // act: add user option on command line
	m_clearPasswd[0] = '\0'; // Modif sf@2002
	// static window
	m_BytesSend=0;
	m_BytesRead=0;

	// We take the initial conn options from the application defaults
	m_opts = m_pApp->m_options;

	m_sock = INVALID_SOCKET;
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
	m_lLastChangeTime = 0; // 0 because we want the first encoding switching to occur quickly
	                     // (in Auto mode, ZRLE is used: pointless over a LAN)

	m_fScalingDone = false;

    zis = new rdr::ZlibInStream;

	// tight cusorhandling
	prevCursorSet = false;
	SaveAreaSet=false;
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
	m_reconnectcounter = 0;

	//ms logon
	m_ms_logon=false;

	// sf@2002 - FileTransfer on server
	m_fServerKnowsFileTransfer = false;

	// Auto Mode
	m_nConfig = 0;

	// sf@2002 - Options window flag
	m_fOptionsOpen = false;

	// Tight encoding
	for (int i = 0; i < 4; i++)
		m_tightZlibStreamActive[i] = false;

	m_hwnd=NULL;
//	m_hwndTB=NULL;
//	m_hwndTBwin=NULL;
//	m_hwndMain=NULL;
	m_hwndStatus=NULL;
//	m_TrafficMonitor=NULL;
//	m_logo_wnd=NULL;
//	m_button_wnd=NULL;
	// m_ToolbarEnable=true;
	m_remote_mouse_disable=false;
	m_SWselect=false;
	previous_scaling=true;
	EncodingStatusWindow = -1;
	OldEncodingStatusWindow = -2;

	m_nStatusTimer = 0;
//	m_FTtimer = 0;
	dormantTimer = 0;
	skipprompt2=true;
//	flash=NULL;
	// UltraFast
	m_hmemdc=NULL;
	m_DIBbits=NULL;
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
	rcSourcecolor=NULL;
	rcMask=NULL;
	zywrle_level = 1;

	m_autoReconnect = m_opts.m_autoReconnect;
	ThreadSocketTimeout=NULL;
	m_statusThread=NULL;
	m_bClosedByUser = false;
	m_server_wants_keepalives = false;

	m_SavedAreaCursor=NULL;
	m_TBr.left=0;
	m_TBr.top=0;
	m_TBr.bottom=28;
	m_Inputlock=true;
	sleep=0;
	old_autoscale=2;

	m_keymap = new KeyMap;
	m_keymapJap = new KeyMapJap;
	m_keymap->SetKeyMapOption1(false);
    m_keymap->SetKeyMapOption2(true);
}

// 
// Run() creates the connection if necessary, does the initial negotiations
// and then starts the thread running which does the output (update) processing.
// If Run throws an Exception, the caller must delete the ClientConnection object.
//

void ClientConnection::Global_options()
{
		GetConnectDetails_global();
		if (saved_set)
			{
				saved_set=FALSE;
				Save_Latest_Connection();
				return;
			}
}

void ClientConnection::Run()
{
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
	if ( (strlen(	m_pApp->m_options.m_cmdlnUser) > 0) & !m_pApp->m_options.m_NoMoreCommandLineUserPassword) // Fix by Act
		strcpy(m_cmdlnUser, m_pApp->m_options.m_cmdlnUser);

	// Modif sf@2002 - bit of a hack...and unsafe
	if ( (strlen(	m_pApp->m_options.m_clearPassword) > 0) & !m_pApp->m_options.m_NoMoreCommandLineUserPassword)
		strcpy(m_clearPasswd, m_pApp->m_options.m_clearPassword);

	if (saved_set)
	{
		saved_set=FALSE;
		Save_Latest_Connection();
		return;
	}
	DoConnection();

	SendMessage(m_hMDIClient, WM_MDIGETACTIVE, 0, (LPARAM)&bMaximized);

	if(bMaximized == TRUE)
    {
        SendMessage(m_hMDIClient, WM_SETREDRAW, FALSE, 0);
    }
	if (g_iNumChild==1) bMaximized=TRUE;

	CreateDisplay();

	LowLevelHook::Initialize(m_hwnd);
	start_undetached();
	
	EndDialog(m_hwndStatus,0);
}
void ClientConnection::DoConnection()
{
	// Connect if we're not already connected
	if (m_sock == INVALID_SOCKET) 
		if (strcmp(m_proxyhost,"")!=NULL && m_fUseProxy)ConnectProxy();
		else Connect();

	SetSocketOptions();

	SetDSMPluginStuff(); // The Plugin is now activated BEFORE the protocol negociation 
						 // so ALL the communication data travel through the DSMPlugin
	if (strcmp(m_proxyhost,"")!=NULL && m_fUseProxy)
		NegotiateProxy();
	NegotiateProtocolVersion();
	Authenticate();

	CheckQueueBufferSize(32000);
}

void ClientConnection::Reconnect()
{
	Sleep( m_autoReconnect * 1000 );
	try
	{
		DoConnection();
		SendClientInit();
		ReadServerInit();
		CreateLocalFramebuffer();
		SetupPixelFormat();
		Createdib();
		SetFormatAndEncodings();
		reconnectcounter=m_reconnectcounter;

		m_bKillThread = false;
		m_running = true;

		SendFullFramebufferUpdateRequest();
	}
	catch (Exception &e)
	{
		if( !m_autoReconnect )
			e.Report();
		reconnectcounter--;
		if (reconnectcounter<0) reconnectcounter=0;
		PostMessage(m_hwnd, WM_CLOSE, reconnectcounter, 1); 
	}
}

HWND ClientConnection::GTGBS_ShowConnectWindow()
{
	DWORD				  threadID;
	m_statusThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE )ClientConnection::GTGBS_ShowStatusWindow,(LPVOID)this,0,&threadID);
	ResumeThread(m_statusThread);
	Sleep(400);
	//this->m_hwndStatus=FindWindow(NULL,"VNC Viewer Status");
	return (HWND)0;
}

////////////////////////////////////////////////////////
#include <commctrl.h>
#include <shellapi.h>
#include <lmaccess.h>
#include <lmat.h>
#include <lmalert.h>

void ClientConnection::CreateDisplay() 
{
	WNDCLASSEX wc;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = ClientConnection::WndProchwnd;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = m_pApp->m_instance;
    wc.hIcon = LoadIcon(m_pApp->m_instance, MAKEINTRESOURCE(IDI_MAINICON));
	wc.hCursor		= NULL;
    wc.hbrBackground = (HBRUSH)(COLOR_3DSHADOW);	// Background color (Only seen if OGL fails)
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "MDI_CHILD";
    wc.hIconSm = LoadIcon(m_pApp->m_instance, MAKEINTRESOURCE(IDI_MAINICON));

    RegisterClassEx(&wc);
    

	if(g_iNumChild < g_iMaxChild)      // If the number of children hasn't reached the maximum
				g_iNumChild++;                 // Increment the number of children
    else
       return; // Otherwise break out of the routine
                    
     CREATESTRUCT cs;
     ZeroMemory(&cs, sizeof(CREATESTRUCT));
     m_hwnd = CreateWindowEx(
               WS_EX_MDICHILD,
               "MDI_CHILD",
               "MDI_CHILD",
               WS_CHILD | WS_VISIBLE | WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN  ,
               CW_USEDEFAULT,
               CW_USEDEFAULT,
               CW_USEDEFAULT,
               CW_USEDEFAULT,
               m_hMDIClient,
               NULL,
               m_pApp->m_instance,
               &cs
               );
    if(!m_hwnd)
        {
            MessageBox(NULL, "Child window failed", "Error",MB_ICONEXCLAMATION | MB_OK);
        }
    g_child[g_iNumChild - 1].iType = 1;             // Set child windows type
    g_child[g_iNumChild - 1].hWnd = m_hwnd;         // Set child windows handle
	g_child[0].hWnd = NULL;         // Set child windows handle
	TCITEM tie;
	tie.mask = TCIF_TEXT; 
    tie.iImage = -1; 
    tie.pszText = m_desktopName; 
	if (TabCtrl_InsertItem(m_hwndTab, g_iNumChild - 1, &tie) == -1) { 
            DestroyWindow(m_hwndTab);  
        } 
	/*if (g_child[g_iNumChild - 2].hWnd==NULL) ShowWindow(m_hwnd, SW_MAXIMIZE);
	else if (IsZoomed(g_child[g_iNumChild - 2].hWnd)) ShowWindow(m_hwnd, SW_MAXIMIZE);
	else ShowWindow(m_hwnd, SW_HIDE);
	*/
	//ShowWindow(m_hwnd, SW_HIDE);

	// record which client created this window
	helper::SafeSetWindowUserData(m_hwnd, (LONG)this);
//	SetClassLong(m_hwnd, GCL_HBRBACKGROUND, (LONG)GetStockObject(NULL_BRUSH));
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

	// Set up clipboard watching
#ifndef _WIN32_WCE
	// We want to know when the clipboard changes, so
	// insert ourselves in the viewer chain. But doing
	// this will cause us to be notified immediately of
	// the current state.
	// We don't want to send that.
	m_initialClipboardSeen = false;
	m_hwndNextViewer = SetClipboardViewer(m_hwnd); 	
#endif
}


//
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
			m_pDSMPlugin->SetEnabled(false);
		}

		if (!m_pDSMPlugin->IsLoaded())
		{
			m_pDSMPlugin->LoadPlugin(m_opts.m_szDSMPluginFilename, m_opts.m_listening);
			if (m_pDSMPlugin->IsLoaded())
			{
				if (m_pDSMPlugin->InitPlugin())
				{
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
	if (m_pDSMPlugin->IsEnabled())
	{
		char szParams[256+16];

		// Does the plugin need the VNC password to do its job ?
		if (!_stricmp(m_pDSMPlugin->GetPluginParams(), "VNCPasswordNeeded"))
		{
			// Yes. The user must enter the VNC password
			// He won't be prompted again for password if ms_logon is not used.
			if (strlen(m_clearPasswd) == 0) // Possibly set using -password command line
			{
				AuthDialog ad;
				if (ad.DoDialog(false))
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
		m_opts.m_PreferredEncoding = rfbEncodingZRLE;
		m_opts.m_Use8Bit = rfbPFFullColors; //false; 
		m_opts.m_fEnableCache = true;
		m_opts.autoDetect = true;
		break;

	case 2:
		m_opts.m_PreferredEncoding = rfbEncodingHextile;
		m_opts.m_Use8Bit = rfbPFFullColors; // false; // Max colors
		m_opts.autoDetect = false;
		m_opts.m_fEnableCache = false;
//		m_opts.m_localCursor = NOCURSOR;
		// m_opts.m_requestShapeUpdates = true;
		// m_opts.m_ignoreShapeUpdates = false;
		break;

	case 3:
		m_opts.m_PreferredEncoding = rfbEncodingZRLE; // rfbEncodingZlibHex;
		m_opts.m_Use8Bit = rfbPF256Colors; //false; 
		m_opts.autoDetect = false;
		m_opts.m_fEnableCache = false;
//		m_opts.m_localCursor = NOCURSOR;
		break;

	case 4:
		m_opts.m_PreferredEncoding = rfbEncodingZRLE;
		m_opts.m_Use8Bit = rfbPF64Colors; //true; 
		m_opts.autoDetect = false;
		m_opts.m_fEnableCache = true;
		break;

	case 5:
		m_opts.m_PreferredEncoding = rfbEncodingZRLE;
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
		m_opts.m_PreferredEncoding = rfbEncodingUltra;
		m_opts.m_Use8Bit = rfbPFFullColors; //false; // Max colors
		m_opts.autoDetect = false;
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
		LoadConnection(m_opts.m_configFilename, false,false);
	}
	else
	{
		char optionfile[MAX_PATH];
		char *tempvar=NULL;
		tempvar = getenv( "TEMP" );
		if (tempvar) strcpy(optionfile,tempvar);
		else strcpy(optionfile,"");
		strcat(optionfile,"\\options.vnc");
		if (!command_line)
		{
			if (LoadConnection(optionfile, false,true)==-1)
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
						m_opts.m_Use8Bit = rfbPF256Colors;
						m_opts.m_fEnableCache = true; // sf@2002
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
				m_opts.m_Use8Bit = rfbPF256Colors;
				m_opts.m_fEnableCache = true; // sf@2002
			}
		}
		
	}
	// This is a bit of a hack: 
	// The config file may set various things in the app-level defaults which 
	// we don't want to be used except for the first connection. So we clear them
	// in the app defaults here.
	m_pApp->m_options.m_host_options[0] = '\0';
	m_pApp->m_options.m_port = -1;
	m_pApp->m_options.m_proxyhost[0] = '\0';
	m_pApp->m_options.m_proxyport = -1;
	m_pApp->m_options.m_connectionSpecified = false;
	m_pApp->m_options.m_configSpecified = false;

}

void ClientConnection::GetConnectDetails_global()
{
		char optionfile[MAX_PATH];
		char *tempvar=NULL;
		tempvar = getenv( "TEMP" );
		if (tempvar) strcpy(optionfile,tempvar);
		else strcpy(optionfile,"");
		strcat(optionfile,"\\options.vnc");
		if (LoadConnection_global(optionfile, false,true)==-1)
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
						m_opts.m_Use8Bit = rfbPF256Colors;
						m_opts.m_fEnableCache = true; // sf@2002
					}
				}
		
	// This is a bit of a hack: 
	// The config file may set various things in the app-level defaults which 
	// we don't want to be used except for the first connection. So we clear them
	// in the app defaults here.
	m_pApp->m_options.m_host_options[0] = '\0';
	m_pApp->m_options.m_port = -1;
	m_pApp->m_options.m_proxyhost[0] = '\0';
	m_pApp->m_options.m_proxyport = -1;
	m_pApp->m_options.m_connectionSpecified = false;
	m_pApp->m_options.m_configSpecified = false;

}

DWORD WINAPI SocketTimeout(LPVOID lpParam) 
    { 
            SOCKET *sock; 
            sock=(SOCKET*) lpParam; 
            int counter=0; 
            while (havetobekilled && !forcedexit) 
            { 
                    Sleep(100); 
                    counter++; 
                    if (counter>100) break; 
            } 
            if (havetobekilled) 
            { 
                    closesocket(*sock); 
            } 
            return 0; 
    } 


void ClientConnection::Connect()
{
	struct sockaddr_in thataddr;
	int res;
	if (!m_opts.m_NoStatus) GTGBS_ShowConnectWindow();
	
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
		
		if (lphost == NULL) { 
			//if(myDialog!=0)DestroyWindow(myDialog);
			if (m_hwndStatus) SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L46);
			throw WarningException(sz_L46); 
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
	DWORD                             threadID;
	ThreadSocketTimeout = CreateThread(NULL,0,SocketTimeout,(LPVOID)&m_sock,0,&threadID);
	havetobekilled=true;
	res = connect(m_sock, (LPSOCKADDR) &thataddr, sizeof(thataddr));
	//Force break
	 havetobekilled=false;
	if (res == SOCKET_ERROR) 
		{
			if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L48);
			SetEvent(KillEvent);
			if (!Pressed_Cancel) throw WarningException(sz_L48);
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
	
	res = connect(m_sock, (LPSOCKADDR) &thataddr, sizeof(thataddr));
	if (res == SOCKET_ERROR) {if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L48);throw WarningException(sz_L48);}
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

        fis = new rdr::FdInStream(m_sock);
		fis->SetDSMMode(m_pDSMPlugin->IsEnabled()); // sf@2003 - Special DSM mode for ZRLE encoding
}


void ClientConnection::NegotiateProtocolVersion()
{
	rfbProtocolVersionMsg pv;

   /* if the connection is immediately closed, don't report anything, so
       that pmw's monitor can make test connections */

	if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L89);
    try
	{
		ReadExact(pv, sz_rfbProtocolVersionMsg);
	}
	catch (Exception &c)
	{
		vnclog.Print(0, _T("Error reading protocol version: %s\n"),
                          c.m_info);
		if (m_fUsePlugin)
			throw WarningException("Connection failed - Error reading Protocol Version\r\n\n\r"
									"Possible causes:\r\r"
									"- You've forgotten to select a DSMPlugin and the Server uses a DSMPlugin\r\n"
									"- The selected DSMPlugin is not compatible with the one running on the Server\r\n"
									"- The selected DSMPlugin is not correctly configured (also possibly on the Server)\r\n"
									"- The password you've possibly entered is incorrect\r\n"
									);
		else
			throw WarningException("Connection failed - Error reading Protocol Version\r\n\n\r"
									"Possible causes:\r\r"
									"- You've forgotten to select a DSMPlugin and the Server uses a DSMPlugin\r\n"
									"- Viewer and Server are not compatible (they use different RFB protocoles)\r\n"
									"- Bad connection\r\n"
									);

		throw QuietException(c.m_info);
	}

    pv[sz_rfbProtocolVersionMsg] = 0;

	// XXX This is a hack.  Under CE we just return to the server the
	// version number it gives us without parsing it.  
	// Too much hassle replacing sscanf for now. Fix this!
#ifdef UNDER_CE
	m_majorVersion = rfbProtocolMajorVersion;
	m_minorVersion = rfbProtocolMinorVersion;
#else
    if (sscanf(pv,rfbProtocolVersionFormat,&m_majorVersion,&m_minorVersion) != 2)
	{
		if (m_fUsePlugin)
			throw WarningException("Connection failed - Invalid protocol !\r\n\r\n"
									"Possible causes:\r\r"
									"- You've forgotten to select a DSMPlugin and the Server uses a DSMPlugin\r\n"
									"- The selected DSMPlugin is not compatible with the one running on the Server\r\n"
									"- The selected DSMPlugin is not correctly configured (also possibly on the Server)\r\n"
									"- The password you've possibly entered is incorrect\r\n"
									);
		else
			throw WarningException("Connection failed - Invalid protocol !\r\n\r\n"
									"Possible causes:\r\r"
									"- You've forgotten to select a DSMPlugin and the Server uses a DSMPlugin\r\n"
									"- Viewer and Server are not compatible (they use different RFB protocoles)\r\n"
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
		m_ms_logon = true;
		m_fServerKnowsFileTransfer = true;
	}
	if (m_minorVersion == 6) // 6 because 5 already used in TightVNC viewer for some reason
	{
		m_ms_logon = false;
		m_fServerKnowsFileTransfer = true;
	}
	// Added for SC so we can do something before actual data transfer start
	if (m_minorVersion == 14 || m_minorVersion == 16)
	{
		m_fServerKnowsFileTransfer = true;
	}

    else if ((m_majorVersion == 3) && (m_minorVersion < 3)) {
		
        /* if server is 3.2 we can't use the new authentication */
		vnclog.Print(0, _T("Can't use IDEA authentication\n"));
        /* This will be reported later if authentication is requested*/

    } else {
		
        /* any other server version, just tell the server what we want */
		m_majorVersion = rfbProtocolMajorVersion;
		m_minorVersion = rfbProtocolMinorVersion; // always 4 for Ultra Viewer

    }

    sprintf(pv,rfbProtocolVersionFormat, m_majorVersion, m_minorVersion);
#endif

    WriteExact(pv, sz_rfbProtocolVersionMsg);
	if (m_minorVersion == 14 || m_minorVersion == 16)
	{
		int size;
		ReadExact((char *)&size,sizeof(int));
		char mytext[1025]; //10k
		//block
		if (size<0 || size >1024)
		{
			throw WarningException("Buffer to big, ");
			if (size<0) size=0;
			if (size>1024) size=1024;
		}
		ReadExact(mytext,size);
		mytext[size]=0;

		int returnvalue=MessageBox(NULL,   mytext,"Accept Incoming SC connection", MB_YESNO |  MB_TOPMOST);
		if (returnvalue==IDNO) 
		{
			int nummer=0;
			WriteExact((char *)&nummer,sizeof(int));
			throw WarningException("You refused connection.....");
		}
		else
		{
			int nummer=1;
			WriteExact((char *)&nummer,sizeof(int));

		}
		
	}


	vnclog.Print(0, _T("Connected to RFB server, using protocol version %d.%d\n"),
		rfbProtocolMajorVersion, rfbProtocolMinorVersion);


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

    if (sscanf(pv,rfbProtocolVersionFormat,&m_majorVersion,&m_minorVersion) != 2)
	{
		if (m_fUsePlugin)
			throw WarningException("Proxy Connection failed - Invalid protocol !\r\n\r\n"
									"Possible causes:\r\r"
									"- You've forgotten to select a DSMPlugin and the Server uses a DSMPlugin\r\n"
									"- The selected DSMPlugin is not compatible with the one running on the Server\r\n"
									"- The selected DSMPlugin is not correctly configured (also possibly on the Server)\r\n"
									"- The password you've possibly entered is incorrect\r\n"
									);
		else
			throw WarningException("Proxy Connection failed - Invalid protocol !\r\n\r\n"
									"Possible causes:\r\r"
									"- You've forgotten to select a DSMPlugin and the Server uses a DSMPlugin\r\n"
									"- Viewer and Server are not compatible (they use different RFB protocoles)\r\n"
									);
    }

    vnclog.Print(0, _T("Connected to proxy \n"),
	    m_majorVersion,m_minorVersion);

	if (m_majorVersion==0 && m_minorVersion==0)
	{
	TCHAR tmphost[MAX_HOST_NAME_LEN];
	TCHAR tmphost2[256];
	_tcscpy(tmphost,m_host);
	if (strcmp(tmphost,"")!=NULL)
	{
	_tcscat(tmphost,":");
	_tcscat(tmphost,_itoa(m_port,tmphost2,10));
	}
    WriteExactProxy(tmphost,MAX_HOST_NAME_LEN);

	vnclog.Print(0, _T("Connected to RFB server, using protocol version %d.%d\n"),
		rfbProtocolMajorVersion, rfbProtocolMinorVersion);
	}


}

void ClientConnection::Authenticate()
{
	CARD32 authScheme, reasonLen, authResult;
    CARD8 challenge[CHALLENGESIZE];
	CARD8 challengems[CHALLENGESIZEMS];
	
	ReadExact((char *)&authScheme, 4);
    authScheme = Swap32IfLE(authScheme);
	if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L90);
    switch (authScheme) {
		
    case rfbConnFailed:
		ReadExact((char *)&reasonLen, 4);
		reasonLen = Swap32IfLE(reasonLen);
		
		CheckBufferSize(reasonLen+1);
		ReadString(m_netbuf, reasonLen);
		
		vnclog.Print(0, _T("RFB connection failed, reason: %s\n"), m_netbuf);
		if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L91);
		throw WarningException(m_netbuf);
        break;
		
    case rfbNoAuth:
		if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L92);
		vnclog.Print(0, _T("No authentication needed\n"));
		break;
		
    case rfbVncAuth:
		{
            if ((m_majorVersion == 3) && (m_minorVersion < 3)) 
			{
                /* if server is 3.2 we can't use the new authentication */
                vnclog.Print(0, _T("Can't use IDEA authentication\n"));

                MessageBox(NULL, 
                    sz_L51, 
                    sz_L52, 
                    MB_OK | MB_ICONSTOP | MB_SETFOREGROUND | MB_TOPMOST);

                throw WarningException("Can't use IDEA authentication any more!");
            }
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
			if (m_ms_logon) 
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
				if (m_ms_logon) strcpy(user, m_cmdlnUser);
			} 
			else if (strlen((const char *) m_encPasswd)>0)
			{
				char *pw = vncDecryptPasswd(m_encPasswd);
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
				///////////////ppppppppppppppppppppppppppppppppppppppppp
				if (ad.DoDialog(m_ms_logon))
				{
//					flash = new BmpFlasher;
					#ifndef UNDER_CE
					strncpy(passwd, ad.m_passwd,254);
					strncpy(user, ad.m_user,254);
					strncpy(domain, ad.m_domain,254);
					#else
					int origlen = _tcslen(ad.m_passwd);
					int newlen = WideCharToMultiByte(
						CP_ACP,    // code page
						0,         // performance and mapping flags
						ad.m_passwd, // address of wide-character string
						origlen,   // number of characters in string
						passwd,    // address of buffer for new string
						255,       // size of buffer
						NULL, NULL );
					
					passwd[newlen]= '\0';
					//user
					origlen = _tcslen(ad.m_user);
					newlen = WideCharToMultiByte(
						CP_ACP,    // code page
						0,         // performance and mapping flags
						ad.m_user, // address of wide-character string
						origlen,   // number of characters in string
						user,    // address of buffer for new string
						255,       // size of buffer
						NULL, NULL );
					
					user[newlen]= '\0';
					//domain
					origlen = _tcslen(ad.m_domain);
					newlen = WideCharToMultiByte(
						CP_ACP,    // code page
						0,         // performance and mapping flags
						ad.m_domain, // address of wide-character string
						origlen,   // number of characters in string
						domain,    // address of buffer for new string
						255,       // size of buffer
						NULL, NULL );
					
					domain[newlen]= '\0';
#endif
					if (strlen(user)==0 ||!m_ms_logon)//need longer passwd for ms
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
					if (m_ms_logon) 
					{
						vncEncryptPasswdMs(m_encPasswdMs, passwd);
						strcpy(m_ms_user, user);
					}
					else
						vncEncryptPasswd(m_encPasswd, passwd);
				} 
				else 
				{
//					if (flash) {flash->Killflash();}
					throw QuietException(sz_L54);
				}
			}
			/*
			// sf@2002 - DSM Plugin
			if (m_pDSMPlugin->IsEnabled())
			{
				// Initialize the DSL Plugin with the entered password
				if (!m_pDSMPlugin->SetPluginParams(NULL, passwd))
				{
					m_pDSMPlugin->SetEnabled(false);
					m_fUsePlugin = false;
					vnclog.Print(0, _T("DSMPlugin cannot be configured\n"));
					throw WarningException("DSMPlugin cannot be configured");
				}

				m_fUsePlugin = true;

				// TODO: Make a special challenge with time stamp 
				// to prevent recording the logon session for later replay

			}
			*/

			// sf@2002 
			// m_ms_logon = false;
			if (m_ms_logon) ReadExact((char *)challengems, CHALLENGESIZEMS);
			ReadExact((char *)challenge, CHALLENGESIZE);

			// MS logon
			if (m_ms_logon) 
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
			else // Regular VNC logon
			{
				vncEncryptBytes(challenge, passwd);

				/* Lose the plain-text password from memory */
				int nLen = (int)strlen(passwd);
				for (int i=0; i< nLen; i++) {
					passwd[i] = '\0';
				}
			
				WriteExact((char *) challenge, CHALLENGESIZE);
			}
			ReadExact((char *) &authResult, 4);
			
			authResult = Swap32IfLE(authResult);
			
			switch (authResult) 
			{
			case rfbVncAuthOK:
				if (m_hwndStatus)vnclog.Print(0, _T("VNC authentication succeeded\n"));
				SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L55);
				g_passwordfailed=false;
				break;
			case rfbVncAuthFailed:
				vnclog.Print(0, _T("VNC authentication failed!"));
				m_pApp->m_options.m_NoMoreCommandLineUserPassword = TRUE;
				g_passwordfailed=true;
				if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L56);
//				if (flash) {flash->Killflash();}
				throw WarningException(sz_L57);
			case rfbVncAuthTooMany:
				throw WarningException(
					sz_L58);
			default:
				vnclog.Print(0, _T("Unknown VNC authentication result: %d\n"),
					(int)authResult);
//				if (flash) {flash->Killflash();}
				throw ErrorException(sz_L59);
			}
			break;
		}
    case rfbMsLogon:
		AuthMsLogon();
		break;		
	default:
		vnclog.Print(0, _T("Unknown authentication scheme from RFB server: %d\n"),
			(int)authScheme);
//		if (flash) {flash->Killflash();}
		throw ErrorException(sz_L60);
    }
}

// marscha@2006: Try to better hide the windows password.
// I know that this is no breakthrough in modern cryptography.
// It's just a patch/kludge/workaround.
void ClientConnection::AuthMsLogon() {
	char gen[8], mod[8], pub[8], resp[8];
	char user[256], passwd[64];
	unsigned char key[8];

	memset(m_clearPasswd, 0, sizeof(m_clearPasswd)); // ??
	
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
	if ((strlen(m_cmdlnUser)>0)||(strlen(m_clearPasswd)>0))
    {
		vnclog.Print(0, _T("Command line MS-Logon.\n"));
#ifndef UNDER_CE
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
		if (ad.DoDialog(m_ms_logon, true)) {
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
	
	WriteExact(user, sizeof(user));
	WriteExact(passwd, sizeof(passwd));


	CARD32 authResult;
	ReadExact((char *) &authResult, 4);
	
	authResult = Swap32IfLE(authResult);
	
	switch (authResult) {
	case rfbVncAuthOK:
		vnclog.Print(0, _T("MS-Logon (DH) authentication succeeded\n"));
		if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L55);
		g_passwordfailed=false;
		break;
	case rfbVncAuthFailed:
		vnclog.Print(0, _T("MS-Logon (DH) authentication failed!"));
		if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L56);
		m_pApp->m_options.m_NoMoreCommandLineUserPassword = TRUE; // Fix by Act
		g_passwordfailed=true;
		throw WarningException(sz_L57);
	case rfbVncAuthTooMany:
		throw WarningException(sz_L58);
	default:
		vnclog.Print(0, _T("Unknown MS-Logon (DH) authentication result: %d\n"),
			(int)authResult);
		throw ErrorException(sz_L59);
	}
}

void ClientConnection::SendClientInit()
{
    rfbClientInitMsg ci;
	ci.shared = m_opts.m_Shared;

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
	if (m_opts.m_ViewOnly) SetWindowText(m_hwnd, m_desktopName_viewonly);
	else SetWindowText(m_hwnd, m_desktopName);

	TCHAR temptitle[25];
	if (m_opts.m_ViewOnly) strncpy(temptitle,m_desktopName,24);
	else strncpy(temptitle,m_desktopName_viewonly,24);
	temptitle[24]=0;
	TCITEM tie;
	tie.mask = TCIF_TEXT; 
    tie.iImage = -1; 
    tie.pszText = temptitle; 
	TabCtrl_SetItem(m_hwndTab,g_iNumChild - 1,&tie);
	strcpy(g_child[g_iNumChild - 1].temptitle,temptitle);
	vnclog.Print(0, _T("Desktop name \"%s\"\n"),m_desktopName);
	vnclog.Print(1, _T("Geometry %d x %d depth %d\n"),
		m_si.framebufferWidth, m_si.framebufferHeight, m_si.format.depth );

	if (m_pDSMPlugin->IsEnabled())
	{
			char szMess[255];
			memset(szMess, 0, 255);
			sprintf(szMess, "--- Ultr@VNC Viewer + %s-v%s",
					m_pDSMPlugin->GetPluginName(),
					m_pDSMPlugin->GetPluginVersion()
					);
			strcat(m_desktopName, szMess);
	}
	strcpy(m_desktopName_viewonly,m_desktopName);
	strcat(m_desktopName_viewonly,"viewonly");

	if (m_opts.m_ViewOnly) SetWindowText(m_hwnd, m_desktopName);
	else SetWindowText(m_hwnd, m_desktopName);
	tie.mask = TCIF_TEXT; 
    tie.iImage = -1; 
    tie.pszText = temptitle; 
	TabCtrl_SetItem(m_hwndTab,g_iNumChild - 1,&tie);
	strcpy(g_child[g_iNumChild - 1].temptitle,temptitle);
	SizeWindow();
}

void ClientConnection::SizeWindow()
{
	// Find how large the desktop work area is
	RECT workrect;

	SystemParametersInfo(SPI_GETWORKAREA, 0, &workrect, 0);
	int workwidth = workrect.right -  workrect.left;
	int workheight = workrect.bottom - workrect.top;
	vnclog.Print(2, _T("Screen work area is %d x %d\n"), workwidth, workheight);
//	Beep(1000,10);

	// sf@2003 - AutoScaling 
	if (m_opts.m_fAutoScaling)
	{
		
		if (!m_fScalingDone)
		{// We save the scales values coming from options
		m_opts.m_saved_scale_num = m_opts.m_scale_num;
		m_opts.m_saved_scale_den = m_opts.m_scale_den;
		m_opts.m_saved_scaling = m_opts.m_scaling;
		}

		RECT clrect;
		GetClientRect(m_hwnd,&clrect);

	
		m_opts.m_scale_num = (int)(((clrect.bottom-clrect.top) * 1000) / m_si.framebufferHeight);
		m_opts.m_scale_num_v=(int)(((clrect.right-clrect.left) * 1000) / m_si.framebufferWidth);
		m_opts.m_scale_den = 1000;
		m_opts.m_scaling = true;
		m_fScalingDone = true;
		if (m_opts.m_scale_num==0) m_opts.m_scale_num=1;
		if (m_opts.m_scale_num_v==0) m_opts.m_scale_num_v=1;
	}
	
	if (!m_opts.m_fAutoScaling && m_fScalingDone)
	{
		// Restore scale values to the original options values
		m_opts.m_scale_num = m_opts.m_saved_scale_num;
		m_opts.m_scale_num_v = m_opts.m_saved_scale_num;
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
				m_si.framebufferWidth * m_opts.m_scale_num_v / m_opts.m_scale_den,
				m_si.framebufferHeight * m_opts.m_scale_num / m_opts.m_scale_den);
	else 
		SetRect(&fullwinrect, 0, 0, m_si.framebufferWidth, m_si.framebufferHeight);

	AdjustWindowRectEx(&fullwinrect, 
			   GetWindowLong(m_hwnd, GWL_STYLE) & ~WS_VSCROLL & ~WS_HSCROLL, 
			   FALSE, GetWindowLong(m_hwnd, GWL_EXSTYLE));

	m_fullwinwidth = fullwinrect.right - fullwinrect.left;
	m_fullwinheight = (fullwinrect.bottom - fullwinrect.top);

	m_winwidth  = min(m_fullwinwidth,  workwidth);
	m_winheight = min(m_fullwinheight, workheight);

	if (m_opts.m_fAutoScaling)
	{
		RECT clrect;
		GetClientRect(m_hwnd,&clrect);
		SetWindowPos(m_hwnd, HWND_TOP, 0, 0, m_winwidth, m_winheight, SWP_SHOWWINDOW|SWP_NOMOVE);
	}
	if (!m_opts.m_fAutoScaling)
	{
		if (m_opts.m_scaling)
		SetRect(&fullwinrect, 0, 0,
				m_si.framebufferWidth * m_opts.m_scale_num_v / m_opts.m_scale_den,
				m_si.framebufferHeight * m_opts.m_scale_num / m_opts.m_scale_den);
		else 
		SetRect(&fullwinrect, 0, 0, m_si.framebufferWidth, m_si.framebufferHeight);
		m_fullwinwidth = fullwinrect.right - fullwinrect.left;
		m_fullwinheight = (fullwinrect.bottom - fullwinrect.top);
		
		RECT clrect;
		GetClientRect(m_hwnd,&clrect);
		m_winwidth  = min(m_winwidth,  (clrect.right-clrect.left));
		m_winheight = min(m_winheight, (clrect.bottom-clrect.top));
		vnclog.Print(0, _T("%i %i %i %i \n"),fullwinrect.right - fullwinrect.left,fullwinrect.bottom - fullwinrect.top,m_winwidth,m_winheight);

		if (m_winwidth>=m_fullwinwidth-35)
		{
			ShowScrollBar(m_hwnd,SB_HORZ,FALSE);
		}
		else
		{
			ShowScrollBar(m_hwnd,SB_HORZ,TRUE);
		}

		if (m_winheight>=m_fullwinheight-35)
		{
			ShowScrollBar(m_hwnd,SB_VERT,FALSE);
		}
		else
		{
			ShowScrollBar(m_hwnd,SB_VERT,TRUE);
		}

	}
	

	AdjustWindowRectEx(&fullwinrect, 
			   GetWindowLong(m_hwnd, GWL_STYLE) & ~WS_VSCROLL & ~WS_HSCROLL, 
			   FALSE, GetWindowLong(m_hwnd, GWL_EXSTYLE));

	m_fullwinwidth = fullwinrect.right - fullwinrect.left;
	m_fullwinheight = (fullwinrect.bottom - fullwinrect.top);

	m_winwidth  = min(m_fullwinwidth,  workwidth);
	m_winheight = min(m_fullwinheight, workheight);

	if (m_opts.m_fAutoScaling && !m_fScalingDone )
		SetWindowPos(m_hwndMain, HWND_TOP,
				workrect.left + (workwidth-m_winwidth) / 2,
				workrect.top + (workheight-m_winheight) / 2,
				m_winwidth, m_winheight, SWP_SHOWWINDOW|SWP_NOMOVE);

	if (m_opts.m_scaling)
		SetRect(&fullwinrect, 0, 0,
				m_si.framebufferWidth * m_opts.m_scale_num_v / m_opts.m_scale_den,
				m_si.framebufferHeight * m_opts.m_scale_num / m_opts.m_scale_den);
	else 
		SetRect(&fullwinrect, 0, 0, m_si.framebufferWidth, m_si.framebufferHeight);

	AdjustWindowRectEx(&fullwinrect, 
			   GetWindowLong(m_hwnd, GWL_STYLE) & ~WS_VSCROLL & ~WS_HSCROLL, 
			   FALSE, GetWindowLong(m_hwnd, GWL_EXSTYLE));

	m_fullwinwidth = fullwinrect.right - fullwinrect.left;
	m_fullwinheight = (fullwinrect.bottom - fullwinrect.top);
	vnclog.Print(0, _T("%i %i  \n"),m_fullwinwidth,m_fullwinheight);






}

void ClientConnection::SizeWindowNoSet()
{
	// Find how large the desktop work area is
	RECT workrect;

	SystemParametersInfo(SPI_GETWORKAREA, 0, &workrect, 0);
	int workwidth = workrect.right -  workrect.left;
	int workheight = workrect.bottom - workrect.top;
	vnclog.Print(2, _T("Screen work area is %d x %d\n"), workwidth, workheight);
//	Beep(1000,10);

	// sf@2003 - AutoScaling 
	if (m_opts.m_fAutoScaling)
	{
		
		if (!m_fScalingDone)
		{// We save the scales values coming from options
		m_opts.m_saved_scale_num = m_opts.m_scale_num;
		m_opts.m_saved_scale_den = m_opts.m_scale_den;
		m_opts.m_saved_scaling = m_opts.m_scaling;
		}

		RECT clrect;
		GetClientRect(m_hwnd,&clrect);

	
		m_opts.m_scale_num = (int)(((clrect.bottom-clrect.top) * 1000) / m_si.framebufferHeight);
		m_opts.m_scale_num_v=(int)(((clrect.right-clrect.left) * 1000) / m_si.framebufferWidth);
		m_opts.m_scale_den = 1000;
		m_opts.m_scaling = true;
		m_fScalingDone = true;
		if (m_opts.m_scale_num==0) m_opts.m_scale_num=1;
		if (m_opts.m_scale_num_v==0) m_opts.m_scale_num_v=1;
	}
	
	if (!m_opts.m_fAutoScaling && m_fScalingDone)
	{
		// Restore scale values to the original options values
		m_opts.m_scale_num = m_opts.m_saved_scale_num;
		m_opts.m_scale_num_v = m_opts.m_saved_scale_num;
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
				m_si.framebufferWidth * m_opts.m_scale_num_v / m_opts.m_scale_den,
				m_si.framebufferHeight * m_opts.m_scale_num / m_opts.m_scale_den);
	else 
		SetRect(&fullwinrect, 0, 0, m_si.framebufferWidth, m_si.framebufferHeight);

	AdjustWindowRectEx(&fullwinrect, 
			   GetWindowLong(m_hwnd, GWL_STYLE) & ~WS_VSCROLL & ~WS_HSCROLL, 
			   FALSE, GetWindowLong(m_hwnd, GWL_EXSTYLE));

	m_fullwinwidth = fullwinrect.right - fullwinrect.left;
	m_fullwinheight = (fullwinrect.bottom - fullwinrect.top);

	m_winwidth  = min(m_fullwinwidth,  workwidth);
	m_winheight = min(m_fullwinheight, workheight);
	

	AdjustWindowRectEx(&fullwinrect, 
			   GetWindowLong(m_hwnd, GWL_STYLE) & ~WS_VSCROLL & ~WS_HSCROLL, 
			   FALSE, GetWindowLong(m_hwnd, GWL_EXSTYLE));

	m_fullwinwidth = fullwinrect.right - fullwinrect.left;
	m_fullwinheight = (fullwinrect.bottom - fullwinrect.top);

	m_winwidth  = min(m_fullwinwidth,  workwidth);

//	if (m_opts.m_ShowToolbar)
//		m_winheight = min(m_fullwinheight + m_TBr.bottom + m_TBr.top+16 , workheight);
//	else
		m_winheight = min(m_fullwinheight, workheight);

	if (m_opts.m_scaling)
		SetRect(&fullwinrect, 0, 0,
				m_si.framebufferWidth * m_opts.m_scale_num_v / m_opts.m_scale_den,
				m_si.framebufferHeight * m_opts.m_scale_num / m_opts.m_scale_den);
	else 
		SetRect(&fullwinrect, 0, 0, m_si.framebufferWidth, m_si.framebufferHeight);

	AdjustWindowRectEx(&fullwinrect, 
			   GetWindowLong(m_hwnd, GWL_STYLE) & ~WS_VSCROLL & ~WS_HSCROLL, 
			   FALSE, GetWindowLong(m_hwnd, GWL_EXSTYLE));

	m_fullwinwidth = fullwinrect.right - fullwinrect.left;
	m_fullwinheight = (fullwinrect.bottom - fullwinrect.top);

//	InvalidateRect(m_hwndMain,NULL,TRUE);


}

// We keep a local copy of the whole screen.  This is not strictly necessary
// for VNC, but makes scrolling & deiconifying much smoother.

void ClientConnection::CreateLocalFramebuffer()
{
	omni_mutex_lock l(m_bitmapdcMutex);
	
	// We create a bitmap which has the same pixel characteristics as
	// the local display, in the hope that blitting will be faster.

	TempDC hdc(m_hwnd);

	if (m_hBitmap != NULL)
		DeleteObject(m_hBitmap);

	m_hBitmap = CreateCompatibleBitmap(hdc, m_si.framebufferWidth, m_si.framebufferHeight);
	if (m_hBitmap == NULL)
		throw WarningException(sz_L61);
	// Select this bitmap into the DC with an appropriate palette
	ObjectSelector b(m_hBitmapDC, m_hBitmap);
	PaletteSelector p(m_hBitmapDC, m_hPalette);
	// Modif RDV@2002 - Cache Encoding
	// Modif sf@2002
	if (m_opts.m_fEnableCache)
	{

		if (m_bitsCache != NULL) delete [] m_bitsCache;
		m_bitsCache=NULL;
		if (m_bitsCacheSwapBuffer != NULL) delete [] m_bitsCacheSwapBuffer;
		m_bitsCacheSwapBuffer=NULL;
		if (m_si.format.bitsPerPixel==8)
		m_bitsCache = new BYTE[m_si.framebufferWidth*m_si.framebufferHeight*16/8];
		else
		m_bitsCache = new BYTE[m_si.framebufferWidth*m_si.framebufferHeight* m_si.format.bitsPerPixel/8];
		m_bitsCacheSwapBufferSize=0;
		vnclog.Print(4, _T("m_bitsCache %i prt %d \n"),m_si.framebufferWidth*m_si.framebufferHeight* m_myFormat.bitsPerPixel / 8,m_bitsCache);
		vnclog.Print(0, _T("Cache: Cache buffer bitmap creation\n"));
	}
	
	RECT rect;

	SetRect(&rect, 0,0, m_si.framebufferWidth, m_si.framebufferHeight);
	COLORREF bgcol = RGB(0, 0, 50);
	FillSolidRect(&rect, bgcol);
	
	COLORREF oldbgcol  = SetBkColor(  m_hBitmapDC, bgcol);
	COLORREF oldtxtcol = SetTextColor(m_hBitmapDC, RGB(255,255,255));
	rect.right = m_si.framebufferWidth / 2;
	rect.bottom = m_si.framebufferHeight / 2;
	
	DrawText (m_hBitmapDC, sz_L62, -1, &rect,
		DT_SINGLELINE | DT_CENTER | DT_VCENTER);

	SetBkColor(  m_hBitmapDC, oldbgcol);
	SetTextColor(m_hBitmapDC, oldtxtcol);
	InvalidateRect(m_hwnd, NULL, FALSE);

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

    WriteExact((char *)&spf, sz_rfbSetPixelFormatMsg, rfbSetPixelFormat);

    // The number of bytes required to hold at least one pixel.
	m_minPixelBytes = (m_myFormat.bitsPerPixel + 7) >> 3;

	// Set encodings
    char buf[sz_rfbSetEncodingsMsg + MAX_ENCODINGS * 4];
    rfbSetEncodingsMsg *se = (rfbSetEncodingsMsg *)buf;
    CARD32 *encs = (CARD32 *)(&buf[sz_rfbSetEncodingsMsg]);
    int len = 0;
	
    se->type = rfbSetEncodings;
    se->nEncodings = 0;

	bool useCompressLevel = false;
	int i = 0;
	// Put the preferred encoding first, and change it if the
	// preferred encoding is not actually usable.
	for (i = LASTENCODING; i >= rfbEncodingRaw; i--)
	{
		if (m_opts.m_PreferredEncoding == i) {
			if (m_opts.m_UseEnc[i])
			{
				encs[se->nEncodings++] = Swap32IfLE(i);
	  			if ( i == rfbEncodingZlib ||
					 i == rfbEncodingTight ||
					 i == rfbEncodingZlibHex
			   )
				{
					useCompressLevel = true;
				}
				if ( i == rfbEncodingZYWRLE
			   )
				{
					zywrle = 1;
				}else{
					zywrle = 0;
				}
			}
			else 
			{
				m_opts.m_PreferredEncoding--;
			}
		}
	}

	// Now we go through and put in all the other encodings in order.
	// We do rather assume that the most recent encoding is the most
	// desirable!
	for (i = LASTENCODING; i >= rfbEncodingRaw; i--)
	{
		if ( (m_opts.m_PreferredEncoding != i) &&
			 (m_opts.m_UseEnc[i]))
		{
			encs[se->nEncodings++] = Swap32IfLE(i);
			if ( i == rfbEncodingZlib ||
				 i == rfbEncodingTight ||
				 i == rfbEncodingZlibHex
				)
			{
				useCompressLevel = true;
			}
		}
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
		encs[se->nEncodings++] = Swap32IfLE(rfbEncodingXCursor);
		encs[se->nEncodings++] = Swap32IfLE(rfbEncodingRichCursor);
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
	
    // sf@2002 - DSM Plugin
	int nEncodings = se->nEncodings;
	se->nEncodings = Swap16IfLE(se->nEncodings);
	// WriteExact((char *)buf, len);
	WriteExact((char *)buf, sz_rfbSetEncodingsMsg, rfbSetEncodings);
	for (int x = 0; x < nEncodings; x++)
	{ 
		WriteExact((char *)&encs[x], sizeof(CARD32));
	}
}
void ClientConnection::Createdib()
{
	omni_mutex_lock l(m_bitmapdcMutex);

	TempDC hdc(m_hwnd);
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
	bi.bmiHeader.biXPelsPerMeter=0;
	bi.bmiHeader.biYPelsPerMeter=0;
	bi.bmiHeader.biClrUsed=0;
	bi.bmiHeader.biClrImportant=0;

	if (m_hmemdc != NULL) {DeleteDC(m_hmemdc);m_hmemdc = NULL;m_DIBbits=NULL;}
	if (m_membitmap != NULL) {DeleteObject(m_membitmap);m_membitmap= NULL;}
//	m_hmemdc = CreateCompatibleDC(hdc);
	m_hmemdc = CreateCompatibleDC(m_hBitmapDC);
	m_membitmap = CreateDIBSection(m_hmemdc, (BITMAPINFO*)&bi.bmiHeader, iUsage, &m_DIBbits, NULL, 0);

	ObjectSelector bb(m_hmemdc, m_membitmap);

	if (m_myFormat.bitsPerPixel==8 && m_myFormat.trueColour)
	{
		struct Colour {
		int r, g, b;
		};
		Colour rgbQ[256];
        /*UINT num_entries;
		num_entries =GetPaletteEntries(m_hPalette, 0, 0, NULL);              
        size_t pal_size = sizeof(LOGPALETTE) +(num_entries - 1) * sizeof(PALETTEENTRY);
        LOGPALETTE* pLogPal =(LOGPALETTE*) new unsigned char[pal_size];
        UINT num_got = GetPaletteEntries( m_hPalette, 0, num_entries, pLogPal->palPalEntry);
          for (UINT i=0; i<num_got; ++i)
          {
            rgbQ[i].rgbRed = pLogPal->palPalEntry[i].peRed;
            rgbQ[i].rgbGreen = pLogPal->palPalEntry[i].peGreen;
            rgbQ[i].rgbBlue = pLogPal-> palPalEntry[i].peBlue;
          }

         delete [] pLogPal;*/

		 for (int i=0; i < (1<<(m_myFormat.depth)); i++) {
			rgbQ[i].b = ((((i >> m_myFormat.blueShift) & m_myFormat.blueMax) * 65535) + m_myFormat.blueMax/2) / m_myFormat.blueMax;
			rgbQ[i].g = ((((i >> m_myFormat.greenShift) & m_myFormat.greenMax) * 65535) + m_myFormat.greenMax/2) / m_myFormat.greenMax;
			rgbQ[i].r = ((((i >> m_myFormat.redShift) & m_myFormat.redMax) * 65535) + m_myFormat.redMax/2) / m_myFormat.redMax;
		 }
#ifdef VC2005
		 for (int i=0; i<256; i++)
	{
		bi.color[i].rgbRed      = rgbQ[i].r >> 8;
		bi.color[i].rgbGreen    = rgbQ[i].g >> 8;
		bi.color[i].rgbBlue     = rgbQ[i].b >> 8;
		bi.color[i].rgbReserved = 0;
	}
#else
	for (i=0; i<256; i++)
	{
		bi.color[i].rgbRed      = rgbQ[i].r >> 8;
		bi.color[i].rgbGreen    = rgbQ[i].g >> 8;
		bi.color[i].rgbBlue     = rgbQ[i].b >> 8;
		bi.color[i].rgbReserved = 0;
	}
#endif
	SetDIBColorTable(m_hmemdc, 0, 256, bi.color);
	}
}
// Closing down the connection.
// Close the socket, kill the thread.
void ClientConnection::KillThread()
{
	vnclog.Print(0, _T("KILLTHREAD\n"));
	m_bKillThread = true;
	m_running = false;

	if (m_sock != INVALID_SOCKET) {
		shutdown(m_sock, SD_BOTH);
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}
	WaitForSingleObject(KillEvent, 100000);
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

	for (int i = 0; i < 4; i++)
		m_tightZlibStreamActive[i] = false;

	// Reinit DSM stuff
	m_nTO = 1;
	LoadDSMPlugin(true);
	m_fUseProxy = false;
	m_pNetRectBuf = NULL;
	m_fReadFromNetRectBuf = false; 
	m_nNetRectBufOffset = 0;
	m_nReadSize = 0;
	m_nNetRectBufSize = 0;
	m_pZRLENetRectBuf = NULL;
	m_fReadFromZRLENetRectBuf = false;
	m_nZRLENetRectBufOffset = 0;
	m_nZRLEReadSize = 0;
	m_nZRLENetRectBufSize = 0;

	if (m_sock != INVALID_SOCKET)
	{
		shutdown(m_sock, SD_BOTH);
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}
}

void
ClientConnection::CloseWindows()
{
	SetEvent(KillEvent);
	if (m_hwnd)SendMessage(m_hwnd, WM_CLOSE, 0, 1);
}

ClientConnection::~ClientConnection()
{
	if (m_hwndStatus)
		EndDialog(m_hwndStatus,0);
	if (m_hwnd!=NULL) 
		DestroyWindow(m_hwnd);

	if (m_pNetRectBuf != NULL)
		delete [] m_pNetRectBuf;
	LowLevelHook::Release();

	// Modif sf@2002 - FileTransfer
	if (m_pFileTransfer) 
		delete(m_pFileTransfer);

	// Modif sf@2002 - Text Chat
	if (m_pTextChat)
		delete(m_pTextChat);

	// Modif sf@2002 - DSMPlugin handling
	if (m_pDSMPlugin != NULL)
		delete(m_pDSMPlugin);

    if (zis)
      delete zis;

    if (fis)
      delete fis;

	if (m_pZRLENetRectBuf != NULL)
		delete [] m_pZRLENetRectBuf;

	if (m_sock != INVALID_SOCKET) {
		shutdown(m_sock, SD_BOTH);
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}

	if (m_desktopName != NULL) delete [] m_desktopName;
	if (m_desktopName_viewonly != NULL) delete [] m_desktopName_viewonly;
	if (m_netbuf != NULL) delete [] m_netbuf;
	if (m_queuebuff != NULL) delete [] m_queuebuff;

	if (m_bitsCache != NULL)
			delete [] m_bitsCache;
	m_bitsCache=NULL;
	if (m_bitsCacheSwapBuffer != NULL)
			delete [] m_bitsCacheSwapBuffer;
	m_bitsCacheSwapBuffer=NULL;
	m_bitsCacheSwapBufferSize=0;
	
	if (m_hBitmapDC != NULL)
		DeleteDC(m_hBitmapDC);
	if (m_hBitmapDC != NULL)
		DeleteObject(m_hBitmapDC);
	if (m_hBitmap != NULL)
		DeleteObject(m_hBitmap);

	if (m_hPalette != NULL)
		DeleteObject(m_hPalette);
	//UltraFast
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
	if (rcSource!=NULL)
		delete[] rcSource;
	if (rcSourcecolor!=NULL)
		delete[] rcSourcecolor;
	if (rcMask!=NULL)
		delete[] rcMask;
	if (m_keymap) {
        delete m_keymap;
        m_keymap = NULL;
    }
	if (m_keymapJap) {
        delete m_keymapJap;
        m_keymapJap = NULL;
    }
	if (KillEvent) CloseHandle(KillEvent);
	if (ThreadSocketTimeout) CloseHandle(ThreadSocketTimeout);
	if (m_statusThread) CloseHandle(m_statusThread);
}



// ProcessPointerEvent handles the delicate case of emulating 3 buttons
// on a two button mouse, then passes events off to SubProcessPointerEvent.
void ClientConnection::ProcessPointerEvent(int x, int y, DWORD keyflags, UINT msg) 
{
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
		  return;
		}
	      
	      // if we reached here, we don't need the timer anymore.
	      KillTimer(m_hwnd, m_emulate3ButtonsTimer);
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
			     m_hwnd, 
			     IDT_EMULATE3BUTTONSTIMER, 
			     m_opts.m_Emul3Timeout, 
			     NULL);
		  
		  if (!m_emulate3ButtonsTimer)
		    {
		      vnclog.Print(0, _T("Failed to create timer for emulating 3 buttons"));
		      PostMessage(m_hwnd, WM_CLOSE, 0, 0);
		      return;
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
			(x + m_hScrollPos) * m_opts.m_scale_den / m_opts.m_scale_num_v;
		int y_scaled =
			(y + m_vScrollPos) * m_opts.m_scale_den / m_opts.m_scale_num;

		SendPointerEvent(x_scaled, y_scaled, mask);

		if ((short)HIWORD(keyflags) != 0) {
			// Immediately send a "button-up" after mouse wheel event.
			mask &= !(rfbButton4Mask | rfbButton5Mask);
			SendPointerEvent(x_scaled, y_scaled, mask);
		}
	} catch (Exception &e) {
		if( !m_autoReconnect )
			e.Report();
		PostMessage(m_hwnd, WM_CLOSE, reconnectcounter, 1);
	}
}


//
// RealVNC 335 method
// 
void ClientConnection::ProcessMouseWheel(int delta)
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
	m_direct=false;
	if (!m_Inputlock) WriteExact((char *)&pe, sz_rfbPointerEventMsg, rfbPointerEvent); // sf@2002 - For DSM Plugin
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

void ClientConnection::ProcessKeyEvent(int virtkey, DWORD keyData)
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
	if (m_opts.m_JapKeyboard==0 && virtkey!=69)
	{
		try {
			m_keymap->PCtoX(virtkey, keyData, this);
		} catch (Exception &e) {
			if( !m_autoReconnect )
				e.Report();
			PostMessage(m_hwnd, WM_CLOSE, reconnectcounter, 1);
		}
	}
	else
	{

	try {
		KeyActionSpec kas = m_keymapJap->PCtoX(virtkey, keyData);    
		
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
			if( !m_autoReconnect )
				e.Report();
			PostMessage(m_hwnd, WM_CLOSE, 4, 1);
		}
	}

}

//
// SendKeyEvent
//

void
ClientConnection::SendKeyEvent(CARD32 key, bool down)
{
	if (m_pFileTransfer->m_fFileTransferRunning && ( m_pFileTransfer->m_fVisible || m_pFileTransfer->UsingOldProtocol())) return;
	if (m_pTextChat->m_fTextChatRunning && m_pTextChat->m_fVisible) return;

    rfbKeyEventMsg ke;

    ke.type = rfbKeyEvent;
    ke.down = down ? 1 : 0;
    ke.key = Swap32IfLE(key);
    if (!m_Inputlock) WriteExact((char *)&ke, sz_rfbKeyEventMsg, rfbKeyEvent);
    //vnclog.Print(0, _T("SendKeyEvent: key = x%04x status = %s ke.key=%d\n"), key, 
      //  down ? _T("down") : _T("up"),ke.key);
}

#ifndef UNDER_CE
//
// SendClientCutText
//

void ClientConnection::SendClientCutText(char *str, int len)
{
	if (m_pFileTransfer->m_fFileTransferRunning && ( m_pFileTransfer->m_fVisible || m_pFileTransfer->UsingOldProtocol())) return;
	if (m_pTextChat->m_fTextChatRunning && m_pTextChat->m_fVisible) return;

	rfbClientCutTextMsg cct;

    cct.type = rfbClientCutText;
    cct.length = Swap32IfLE(len);
    if (!m_Inputlock) WriteExact((char *)&cct, sz_rfbClientCutTextMsg, rfbClientCutText);
	if (!m_Inputlock) WriteExact(str, len);
	vnclog.Print(6, _T("Sent %d bytes of clipboard\n"), len);
}
#endif

// Copy any updated areas from the bitmap onto the screen.

void ClientConnection::DoBlit() 
{

	if (m_hBitmap == NULL) return;
	if (!m_running) return;
				
	// No other threads can use bitmap DC
	omni_mutex_lock l(m_bitmapdcMutex);

	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(m_hwnd, &ps);

	// Select and realize hPalette
//	PaletteSelector p(hdc, m_hPalette);
//	ObjectSelector b(m_hBitmapDC, m_hBitmap);
	
	if (m_opts.m_delay) {
		// Display the area to be updated for debugging purposes
		COLORREF oldbgcol = SetBkColor(hdc, RGB(0,0,0));
		::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &ps.rcPaint, NULL, 0, NULL);
		SetBkColor(hdc,oldbgcol);
		::Sleep(m_pApp->m_options.m_delay);
	}

	


	
	if (m_opts.m_scaling)
	{
		int n = m_opts.m_scale_num;
		int v = m_opts.m_scale_num_v;
		int d = m_opts.m_scale_den;

		SetStretchBltMode(hdc, HALFTONE);
		SetBrushOrgEx(hdc, 0,0, NULL);

		/*int a=0;
		HDRAWDIB hdd = DrawDibOpen(); 
		HDC testdc=GetDC(m_hwnd);
        a=DrawDibDraw(hdd,testdc,0,0,m_si.framebufferWidth,m_si.framebufferHeight,(BITMAPINFOHEADER*)&bi.bmiHeader,
			m_DIBbits,0, 
					0,
					m_si.framebufferWidth, 
					m_si.framebufferHeight,
					DDF_HALFTONE);
		DrawDibClose(hdd);
		int b=GetLastError();
		a=2;*/

		/*StretchDIBits(hdc,
			ps.rcPaint.left, 
				ps.rcPaint.top, 
				ps.rcPaint.right-ps.rcPaint.left, 
				ps.rcPaint.bottom-ps.rcPaint.top, 
  
  				(ps.rcPaint.left+m_hScrollPos)     * d / v, 
				(ps.rcPaint.top+m_vScrollPos)      * d / n,
				(ps.rcPaint.right-ps.rcPaint.left) * d / v, 
				(ps.rcPaint.bottom-ps.rcPaint.top) * d / n, 
				m_DIBbits,
				(BITMAPINFO*)&bi,
				DIB_RGB_COLORS ,       
				SRCCOPY);*/


		
		/*SetStretchBltMode(hdc, HALFTONE);*/
		SetBrushOrgEx(hdc, 0,0, NULL);
		{
			if(UltraFast && m_hmemdc)
			{
				ObjectSelector bb(m_hmemdc, m_membitmap);
				StretchBlt(
				hdc, 
				ps.rcPaint.left, 
				ps.rcPaint.top, 
				ps.rcPaint.right-ps.rcPaint.left, 
				ps.rcPaint.bottom-ps.rcPaint.top, 
				m_hmemdc, 
				(ps.rcPaint.left+m_hScrollPos)     * d / v, 
				(ps.rcPaint.top+m_vScrollPos)      * d / n,
				(ps.rcPaint.right-ps.rcPaint.left) * d / v, 
				(ps.rcPaint.bottom-ps.rcPaint.top) * d / n, 
				SRCCOPY);
			}
		}
	}
	else
	{
		if (UltraFast && m_hmemdc)
		{
			ObjectSelector bb(m_hmemdc, m_membitmap);
			BitBlt(hdc, ps.rcPaint.left, ps.rcPaint.top, 
			ps.rcPaint.right-ps.rcPaint.left, ps.rcPaint.bottom-ps.rcPaint.top, 
			m_hmemdc, ps.rcPaint.left+m_hScrollPos, ps.rcPaint.top+m_vScrollPos, SRCCOPY);
		}
	}
	EndPaint(m_hwnd, &ps);

}

// You can specify a dx & dy outside the limits; the return value will
// tell you whether it actually scrolled.
extern HWND hWndRebar;
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
//		if (m_opts.m_ShowToolbar)
//			GetClientRect(hWndRebar, &Rtb);
//		else 
			{
				Rtb.top=0;
				Rtb.bottom=0;
			}
		
		clirect.top += Rtb.top;
		clirect.bottom += Rtb.bottom;
		ScrollWindowEx(m_hwnd, -dx, -dy, NULL, &clirect, NULL, NULL,  SW_INVALIDATE);
		UpdateScrollbars();
		UpdateWindow(m_hwnd);
		
		return true;
	}
	return false;
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
	
	if (setInfo && (GetWindowLong(m_hwnd, GWL_STYLE) & WS_HSCROLL))
		SetScrollInfo(m_hwnd, SB_HORZ, &scri, TRUE);
	
	scri.cbSize = sizeof(scri);
	scri.fMask = SIF_ALL;
	scri.nMin = 0;

	scri.nMax = m_vScrollMax ;
	scri.nPage= m_cliheight;
	scri.nPos = m_vScrollPos; 
	
	if (setInfo && (GetWindowLong(m_hwnd, GWL_STYLE) & WS_VSCROLL)) 
		SetScrollInfo(m_hwnd, SB_VERT, &scri, TRUE);
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
	MessageBox(NULL, buf, _T("VNC connection info"), MB_ICONINFORMATION | MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
}

// ********************************************************************
//  Methods after this point are generally called by the worker thread.
//  They finish the initialisation, then chiefly read data from the server.
// ********************************************************************


void* ClientConnection::run_undetached(void* arg) {

	SendClientInit();
	
	ReadServerInit();
	
	CreateLocalFramebuffer();

	SetupPixelFormat();

	Createdib();

    SetFormatAndEncodings();

	reconnectcounter=m_reconnectcounter;

	if(bMaximized == TRUE)
    {
        ShowWindow(m_hwnd, SW_SHOWMAXIMIZED);

        SendMessage(m_hMDIClient, WM_SETREDRAW, TRUE, 0);
        RedrawWindow(m_hMDIClient, NULL, NULL,RDW_INVALIDATE | RDW_ALLCHILDREN);
    }
	SizeWindow();
       
	// This starts the worker thread.
	// The rest of the processing continues in run_undetached.
	// LowLevelHook::Initialize(m_hwnd);

	vnclog.Print(9,_T("Update-processing thread started\n"));

	m_threadStarted = true;

	// Modif sf@2002 - Server Scaling
	m_nServerScale = m_opts.m_nServerScale;
	if (m_nServerScale > 1) SendServerScale(m_nServerScale);

	m_reconnectcounter = m_opts.m_reconnectcounter;
	reconnectcounter = m_reconnectcounter;

	SendFullFramebufferUpdateRequest();

	SizeWindow();
//		RealiseFullScreenMode();
//		if (!InFullScreenMode()) SizeWindow();

	m_running = true;
	UpdateWindow(m_hwnd);
	

		// sf@2002 - Attempt to speed up the thing
		// omni_thread::set_priority(omni_thread::PRIORITY_LOW);

    rdr::U8 msgType;
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
				m_nTO = 0; // we'll need to read the whole transformed rfb message that follows
			}
				
            switch (msgType)
			{
			case rfbKeepAlive:
                    m_server_wants_keepalives = true;
                    if (sz_rfbKeepAliveMsg > 1)
                    {
                  	rfbKeepAliveMsg kp;
                	ReadExact(((char *) &kp)+m_nTO, sz_rfbKeepAliveMsg-m_nTO);
                    }
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
				SendMessage(m_hwnd, FileTransferSendPacketMessage, 1, 0); 
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
				rfbResizeFrameBufferMsg rsmsg;
				ReadExact(((char*)&rsmsg) + m_nTO, sz_rfbResizeFrameBufferMsg - m_nTO);

				m_si.framebufferWidth = Swap16IfLE(rsmsg.framebufferWidth);
				m_si.framebufferHeight = Swap16IfLE(rsmsg.framebufferHeigth);

//				ClearCache();
				CreateLocalFramebuffer();
				// SendFullFramebufferUpdateRequest();
				Createdib();
				m_pendingScaleChange = true;
				m_pendingFormatChange = true;
				SendAppropriateFramebufferUpdateRequest();
				
				SizeWindow();
				InvalidateRect(m_hwnd, NULL, TRUE);
//				RealiseFullScreenMode();	
				break;
			}

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
	catch (WarningException)
	{
		/*// sf@2002
		 m_pFileTransfer->m_fFileTransferRunning = false;
		 m_pTextChat->m_fTextChatRunning = false;
		 vnclog.Print(4, _T("_this->KillEvent\n"));
		 SetEvent(KillEvent);
		//if (!m_bKillThread )PostMessage(m_hwnd, WM_CLOSE, 0, 0);*/

		 m_bKillThread = true;
		PostMessage(m_hwnd, WM_CLOSE, reconnectcounter, 1);
	}
	catch (QuietException &e)
	{
		/*// sf@2002
		 m_pFileTransfer->m_fFileTransferRunning = false;
		 m_pTextChat->m_fTextChatRunning = false;
		 vnclog.Print(4, _T("_this->KillEvent\n"));
		 SetEvent(KillEvent);
		//if (!m_bKillThread )PostMessage(m_hwnd, WM_CLOSE, 0, 0);*/

		 m_bKillThread = true;
			PostMessage(m_hwnd, WM_CLOSE, reconnectcounter, 1);
	}
	catch (rdr::Exception& e)
	{
		/*vnclog.Print(0,"rdr::Exception (1): %s\n",e.str());
		m_pFileTransfer->m_fFileTransferRunning = false;
		m_pTextChat->m_fTextChatRunning = false;
		//throw QuietException(e.str());
		//throw WarningException("Stopping\n");
		//if (!m_bKillThread)PostMessage(m_hwnd, WM_CLOSE, 0, 0);*/

		if ((strcmp(e.str(),"rdr::EndOfStream: read")==NULL) && !m_bClosedByUser)
			{
				WarningException w(sz_L94,200);
               // w.Report();
			}
            else if (!(/*m_pFileTransfer->m_fFileTransferRunning || m_pTextChat->m_fTextChatRunning ||*/ m_bClosedByUser))
            {
                WarningException w(sz_L69);
                w.Report();
            }
			m_bKillThread = true;
			PostMessage(m_hwnd, WM_CLOSE, reconnectcounter, 1);
	}
		Sleep(0);
		Sleep(2000);
	}
		// sf@2002
	m_pFileTransfer->m_fFileTransferRunning = false;
	m_pTextChat->m_fTextChatRunning = false;
	vnclog.Print(4, _T("run_undetach ended\n") );
	vnclog.Print(4, _T("_this->KillEvent\n"));
	SetEvent(KillEvent);
	return this;
}


//
// Requesting screen updates from the server
//

inline void
ClientConnection::SendFramebufferUpdateRequest(int x, int y, int w, int h, bool incremental)
{
	if (m_pFileTransfer->m_fFileTransferRunning && ( m_pFileTransfer->m_fVisible || m_pFileTransfer->UsingOldProtocol())) return;
	if (m_pTextChat->m_fTextChatRunning && m_pTextChat->m_fVisible) return;

//	omni_mutex_lock l(m_UpdateMutex);

    rfbFramebufferUpdateRequestMsg fur;

	// vnclog.Print(0, _T("Request %s update x=%d,y=%d,w=%d,h=%d\n"), incremental ? _T("incremental") : _T("full"),x,y,w,h);

    fur.type = rfbFramebufferUpdateRequest;
    fur.incremental = incremental ? 1 : 0;
    fur.x = Swap16IfLE(x);
    fur.y = Swap16IfLE(y);
    fur.w = Swap16IfLE(w);
    fur.h = Swap16IfLE(h);
	//vnclog.Print(10, _T("Request %s update\n"), incremental ? _T("incremental") : _T("full"));	
    WriteExact((char *)&fur, sz_rfbFramebufferUpdateRequestMsg, rfbFramebufferUpdateRequest);
}

inline void ClientConnection::SendIncrementalFramebufferUpdateRequest()
{
//	vnclog.Print(0, _T("SendIncrementalFramebufferUpdateRequest\n"));
    SendFramebufferUpdateRequest(0, 0, m_si.framebufferWidth,
					m_si.framebufferHeight, true);
}

void ClientConnection::SendFullFramebufferUpdateRequest()
{
	vnclog.Print(0, _T("SSendFullFramebufferUpdateRequest\n"));
    SendFramebufferUpdateRequest(0, 0, m_si.framebufferWidth,
					m_si.framebufferHeight, false);
}


void ClientConnection::SendAppropriateFramebufferUpdateRequest()
{
	if (m_pendingFormatChange) 
	{
		m_Inputlock=true;
		vnclog.Print(3, _T("Requesting new pixel format\n") );

		// Cache init/reinit - A SetFormatAndEncoding() implies a cache reinit on server side
		// Cache enabled, so it's going to be reallocated/reinited on server side
		if (m_opts.m_fEnableCache)
		{
			// create viewer cache buffer if necessary
			if (m_bitsCache == NULL)
			{
				m_bitsCache= new BYTE [m_si.framebufferWidth*m_si.framebufferHeight*m_myFormat.bitsPerPixel / 8];
//				vnclog.Print(4, _T("m_bitsCache %i prt %d \n"),m_si.framebufferWidth*m_si.framebufferHeight* m_myFormat.bitsPerPixel / 8,m_bitsCache);
//				Sleep(2000);
			}
//			ClearCache(); // Clear the cache
			m_pendingCacheInit = true; // Order full update to synchronize both sides caches
		}
		else // No cache requested - The cache on the other side is to be cleared/deleted
			 // Todo: fix the cache switching pb when viewer has been started without cache
		{
			/* causes balck rects after cache off/on
			// Delete local cache
			DeleteDC(m_hCacheBitmapDC);
			if (m_hCacheBitmap != NULL) DeleteObject(m_hCacheBitmap);
			if (m_hCacheBitmapDC != NULL) DeleteObject(m_hCacheBitmapDC);			
			m_hCacheBitmap = NULL;
			m_pendingCacheInit = false;
			*/
		}
		SoftCursorFree();
		rfbPixelFormat oldFormat = m_myFormat;
		SetupPixelFormat();
		// tight cursor handling
		Createdib();
		SetFormatAndEncodings();
		m_pendingFormatChange = false;
		
		SendFullFramebufferUpdateRequest();

		m_pendingScaleChange = false;
		m_pendingCacheInit = false;
	}
	else 
	{
		m_Inputlock=false;
		if (sleep==0)
			SendIncrementalFramebufferUpdateRequest();
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
			(x + m_hScrollPos) * m_opts.m_scale_den / m_opts.m_scale_num_v;
		int y_scaled =
			(y + m_vScrollPos) * m_opts.m_scale_den / m_opts.m_scale_num;
		
		sw.type = rfbSetSW;
		sw.x = Swap16IfLE(x_scaled);
		sw.y = Swap16IfLE(y_scaled);
	}
    WriteExact((char*)&sw, sz_rfbSetSWMsg, rfbSetSW);
	m_SWselect=false;
    return true;
}


// A ScreenUpdate message has been received
inline void ClientConnection::ReadScreenUpdate()
{
	HDC hdcX,hdcBits;

	bool fTimingAlreadyStopped = false;
	fis->startTiming();
	
	rfbFramebufferUpdateMsg sut;
	ReadExact(((char *) &sut)+m_nTO, sz_rfbFramebufferUpdateMsg-m_nTO);
    sut.nRects = Swap16IfLE(sut.nRects);
	HRGN UpdateRegion=CreateRectRgn(0,0,0,0);
	bool Recover_from_sync=false;
	
    //if (sut.nRects == 0) return;  XXX tjr removed this - is this OK?
	
	for (UINT i=0; i < sut.nRects; i++)
	{
		rfbFramebufferUpdateRectHeader surh;
		ReadExact((char *) &surh, sz_rfbFramebufferUpdateRectHeader);
		surh.r.x = Swap16IfLE(surh.r.x);
		surh.r.y = Swap16IfLE(surh.r.y);
		surh.r.w = Swap16IfLE(surh.r.w);
		surh.r.h = Swap16IfLE(surh.r.h);
		surh.encoding = Swap32IfLE(surh.encoding);
//		 vnclog.Print(0, _T("%d %d\n"), i,sut.nRects);
//		vnclog.Print(0, _T("encoding %d\n"), surh.encoding);
		
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
		
		if (surh.encoding !=rfbEncodingNewFBSize && surh.encoding != rfbEncodingCacheZip && surh.encoding != rfbEncodingSolMonoZip && surh.encoding !=rfbEncodingUltraZip)
			SoftCursorLockArea(surh.r.x, surh.r.y, surh.r.w, surh.r.h);
		
		// Modif sf@2002 - DSM Plugin
		// With DSM, all rects contents (excepted caches) are buffered into memory in one shot
		// then they will be read in this buffer by the "regular" Read*Type*Rect() functions
		if (m_fUsePlugin && m_pDSMPlugin->IsEnabled())
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
			
			// ZRLE special case
			if (!fis->GetReadFromMemoryBuffer())
			{
				if ((surh.encoding == rfbEncodingZYWRLE)||(surh.encoding == rfbEncodingZRLE))
				{
					// Get the size of the rectangle data buffer
					ReadExact((char*)&(m_nZRLEReadSize), sizeof(CARD32));
					m_nZRLEReadSize = Swap32IfLE(m_nZRLEReadSize);
					// Read the whole  rect buffer and put the result in m_netbuf
					CheckZRLENetRectBufferSize((int)(m_nZRLEReadSize));
					CheckBufferSize((int)(m_nZRLEReadSize)); // sf@2003
					ReadExact((char*)(m_pZRLENetRectBuf), (int)(m_nZRLEReadSize));
					// Tell the following Read() function calls to Read Data from memory
					fis->SetReadFromMemoryBuffer(m_nZRLEReadSize, (char*)(m_pZRLENetRectBuf));
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
			hdcX = GetDC(m_TrafficMonitor);
			hdcBits = CreateCompatibleDC(hdcX);
			SelectObject(hdcBits,m_bitmapBACK);
			RECT rect;
			GetWindowRect(m_TrafficMonitor,&rect);
			BitBlt(hdcX,0,0,rect.right-rect.left,rect.bottom-rect.top,hdcBits,0,0,SRCCOPY);
			//BitBlt(hdcX,4,2,22,20,hdcBits,0,0,SRCCOPY);
			DeleteDC(hdcBits);
			ReleaseDC(m_TrafficMonitor,hdcX);
		}

		UltraFast=true;

		// sf@2004
		if (m_fUsePlugin && m_pDSMPlugin->IsEnabled() && (m_fReadFromNetRectBuf || fis->GetReadFromMemoryBuffer()))
		{
			fis->stopTiming();
			kbitsPerSecond = fis->kbitsPerSecond();
			fTimingAlreadyStopped = true;
		}

//	     vnclog.Print(0, _T("known encoding %d - not supported!\n"), surh.encoding);

										
		switch (surh.encoding)
		{
		case rfbEncodingHextile:
			SaveArea(cacherect);
			ReadHextileRect(&surh);
			EncodingStatusWindow=rfbEncodingHextile;
			break;
		case rfbEncodingUltra:
			ReadUltraRect(&surh);
			EncodingStatusWindow=rfbEncodingUltra;
			break;
		case rfbEncodingUltraZip:
			ReadUltraZip(&surh,&UpdateRegion);
			break;
		case rfbEncodingRaw:
			SaveArea(cacherect);
			ReadRawRect(&surh);
			EncodingStatusWindow=rfbEncodingRaw;
			break;
		case rfbEncodingCopyRect:
			ReadCopyRect(&surh);
			break;
		case rfbEncodingCache:
			ReadCacheRect(&surh);
			break;
		case rfbEncodingCacheZip:
			ReadCacheZip(&surh,&UpdateRegion);
			break;
		case rfbEncodingSolMonoZip:
			ReadSolMonoZip(&surh,&UpdateRegion);
			break;
		case rfbEncodingRRE:
			SaveArea(cacherect);
			ReadRRERect(&surh);
			EncodingStatusWindow=rfbEncodingRRE;
			break;
		case rfbEncodingCoRRE:
			SaveArea(cacherect);
			ReadCoRRERect(&surh);
			EncodingStatusWindow=rfbEncodingCoRRE;
			break;
		case rfbEncodingZlib:
			SaveArea(cacherect);
			ReadZlibRect(&surh,0);
			EncodingStatusWindow=rfbEncodingZlib;
			break;
		case rfbEncodingZlibHex:
			SaveArea(cacherect);
			ReadZlibHexRect(&surh);
			EncodingStatusWindow=rfbEncodingZlibHex;
			break;
		case rfbEncodingXOR_Zlib:
			SaveArea(cacherect);
			ReadZlibRect(&surh,1);
			break;
		case rfbEncodingXORMultiColor_Zlib:
			SaveArea(cacherect);
			ReadZlibRect(&surh,2);
			break;
		case rfbEncodingXORMonoColor_Zlib:
			SaveArea(cacherect);
			ReadZlibRect(&surh,3);
			break;
		case rfbEncodingSolidColor:
			SaveArea(cacherect);
			ReadSolidRect(&surh);
			break;
		case rfbEncodingZRLE:
			zywrle = 0;
		case rfbEncodingZYWRLE:
			SaveArea(cacherect);
			zrleDecode(surh.r.x, surh.r.y, surh.r.w, surh.r.h);
			EncodingStatusWindow=zywrle ? rfbEncodingZYWRLE : rfbEncodingZRLE;
			break;
		case rfbEncodingTight:
			SaveArea(cacherect);
			ReadTightRect(&surh);
			EncodingStatusWindow=rfbEncodingTight;
			break;
		default:
			vnclog.Print(0, _T("Unknown encoding %d - not supported!\n"), surh.encoding);
			// Try to empty buffer...
			// so next update should be back in sync
			BYTE * buffer;
			int i=0;
			while (TRUE)
			{
				int aantal=fis->Check_if_buffer_has_data();
				if (aantal>0) buffer = new BYTE [aantal];
				if (aantal>0)
				{
					i=0;
					ReadExact(((char *) buffer), aantal);
					delete [] buffer;
					Sleep(5);
				}
				else if (aantal==0)
				{
					if (i==5) break;
					Sleep(200);
					i++;
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
		
		if (surh.encoding !=rfbEncodingNewFBSize && surh.encoding != rfbEncodingCacheZip && surh.encoding != rfbEncodingSolMonoZip && surh.encoding != rfbEncodingUltraZip)
		{
			RECT rect;
			rect.left   = surh.r.x;
			rect.top    = surh.r.y;
			rect.right  = surh.r.x + surh.r.w ;
			rect.bottom = surh.r.y + surh.r.h; 
			InvalidateScreenRect(&rect); 			
		}
		else if (surh.encoding !=rfbEncodingNewFBSize)
		{
			InvalidateRgn(m_hwnd, UpdateRegion, FALSE);
			HRGN tempregion=CreateRectRgn(0,0,0,0);
			CombineRgn(UpdateRegion,UpdateRegion,tempregion,RGN_AND);
			DeleteObject(tempregion);
		}

		if (m_TrafficMonitor)
		{
			hdcX = GetDC(m_TrafficMonitor);
			hdcBits = CreateCompatibleDC(hdcX);
			SelectObject(hdcBits,m_bitmapNONE);
			RECT rect;
			GetWindowRect(m_TrafficMonitor,&rect);
			BitBlt(hdcX,0,0,rect.right-rect.left,rect.bottom-rect.top,hdcBits,0,0,SRCCOPY);
			DeleteDC(hdcBits);
			ReleaseDC(m_TrafficMonitor,hdcX);
		}

		SoftCursorUnlockScreen();
	}

	if (!fTimingAlreadyStopped)
	{
		fis->stopTiming();
		kbitsPerSecond = fis->kbitsPerSecond();
	}

	// sf@2002
	// We only change the preferred encoding if FileTransfer is not running and if
	// the last encoding change occured more than 30s ago
	if (m_opts.autoDetect 
		&&
		!m_pFileTransfer->m_fFileTransferRunning
		&& 
		(timeGetTime() - m_lLastChangeTime) > 30000)
	{
		int nOldServerScale = m_nServerScale;

		// If connection speed > 1Mbits/s - All to the max
		/*if (kbitsPerSecond > 2000 && (m_nConfig != 7))
		{
			m_nConfig = 1;
			m_opts.m_PreferredEncoding = rfbEncodingUltra;
			m_opts.m_Use8Bit = false; // Max colors
			m_opts.m_fEnableCache = false;
			m_pendingFormatChange = true;
			m_lLastChangeTime = timeGetTime();
		}*/

		if (kbitsPerSecond > 1000 && (m_nConfig != 1))
		{
			m_nConfig = 1;
			m_opts.m_PreferredEncoding = rfbEncodingHextile;
			m_opts.m_Use8Bit = rfbPFFullColors; // Max colors
			m_opts.m_fEnableCache = false;
			m_pendingFormatChange = true;
			m_lLastChangeTime = timeGetTime();
		}
		// Medium connection speed 
		else if (kbitsPerSecond < 256 && kbitsPerSecond > 0 && (m_nConfig != 2))
		{
			m_nConfig = 2;
			m_opts.m_PreferredEncoding = rfbEncodingZRLE; //rfbEncodingZlibHex;
			m_opts.m_Use8Bit = rfbPF256Colors; 
			// m_opts.m_compressLevel = 9;
			m_opts.m_fEnableCache = false;
			m_pendingFormatChange = true;
			m_lLastChangeTime = timeGetTime();
		}
		// Modem (including cable modem) connection speed 
		/*else if (kbitsPerSecond < 128 && kbitsPerSecond > 19 && (m_nConfig != 3))
		{
			m_nConfig = 3;
			m_opts.m_PreferredEncoding = rfbEncodingZRLE; // rfbEncodingZRLE;
			m_opts.m_Use8Bit = rfbPF64Colors; 
			// m_opts.m_compressLevel = 9;
			m_opts.m_fEnableCache = false;
			m_pendingFormatChange = true;
			m_lLastChangeTime = timeGetTime();
			Beep(1000,100);
		}
		
		// Slow Modem connection speed 
		// Not sure it's a good thing in Auto mode...because in some cases
		// (CTRL-ALT-DEL, initial screen loading, connection short hangups...)
		// the speed can be momentary VERY slow. The fast fuzzy/normal modes switching
		// can be quite disturbing and useless in these situations.
		else if (kbitsPerSecond < 19 && kbitsPerSecond > 5 && (m_nConfig != 4))
		{
			m_nConfig = 4;
			m_opts.m_PreferredEncoding = rfbEncodingZRLE; //rfbEncodingZRLE;
			m_opts.m_Use8Bit = rfbPF8Colors; 
			// m_opts.m_compressLevel = 9; 
			// m_opts.m_scaling = true;
			// m_opts.m_scale_num = 2;
			// m_opts.m_scale_den = 1;
			// m_nServerScale = 2;
			// m_opts.m_nServerScale = 2;
			m_opts.m_fEnableCache = false;
			m_pendingFormatChange = true;
		}*/
		/*
		if (m_nServerScale != nOldServerScale)
		{
			SendServerScale(m_nServerScale);
		}
		*/
    }
		
	// Inform the other thread that an update is needed.
	
	PostMessage(m_hwnd, WM_REGIONUPDATED, NULL, NULL);
	DeleteObject(UpdateRegion);
}	



void ClientConnection::SetDormant(bool newstate)
{
	vnclog.Print(5, _T("%s dormant mode\n"), newstate ? _T("Entering") : _T("Leaving"));
	m_dormant = newstate;
	if (!m_dormant)
		SendIncrementalFramebufferUpdateRequest();
}

// The server has copied some text to the clipboard - put it 
// in the local clipboard too.

void ClientConnection::ReadServerCutText() 
{
	rfbServerCutTextMsg sctm;
	vnclog.Print(6, _T("Read remote clipboard change\n"));
	ReadExact(((char *) &sctm)+m_nTO, sz_rfbServerCutTextMsg-m_nTO);
	int len = Swap32IfLE(sctm.length);
	
	CheckBufferSize(len);
	if (len == 0) {
		m_netbuf[0] = '\0';
	} else {
		ReadString(m_netbuf, len);
	}
	UpdateLocalClipboard(m_netbuf, len);
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
		if (IsIconic(m_hwnd)) {
			SetDormant(false);
			ShowWindow(m_hwnd, SW_SHOWNORMAL);
		}
	}
	vnclog.Print(6, _T("Bell!\n"));
}


// General utilities -------------------------------------------------

// Reads the number of bytes specified into the buffer given

void ClientConnection::ReadExact(char *inbuf, int wanted)
{
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
				omni_mutex_lock l(m_pDSMPlugin->m_RestMutex); 

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
					BYTE* pTransBuffer = m_pDSMPlugin->RestoreBufferStep1(NULL, wanted, &nTransDataLen);
					if (pTransBuffer == NULL)
					{
						// m_pDSMPlugin->RestoreBufferUnlock();
						throw WarningException(sz_L65);
					}
					
					// Read bytes directly into Plugin Dest rest. buffer
					fis->readBytes(pTransBuffer, nTransDataLen);
					
					// Ask plugin to restore data from its local rest. buffer into inbuf
					int nRestDataLen = 0;
					m_pDSMPlugin->RestoreBufferStep2((BYTE*)inbuf, nTransDataLen, &nRestDataLen);
					
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
		throw QuietException(e.str());
	}


}

void ClientConnection::ReadExactProxy(char *inbuf, int wanted)
{
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

//
// sf@2002 - DSM Plugin
//
void ClientConnection::WriteExact(char *buf, int bytes, CARD8 msgType)
{
	if (!m_fUsePlugin)
	{
		WriteExact(buf, bytes);
	}
	else if (m_pDSMPlugin->IsEnabled())
	{
		// Send the transformed message type first 
		WriteExact((char*)&msgType, sizeof(msgType));
		// Then send the transformed rfb message content
		WriteExact(buf, bytes);
	}
}

// Sends the number of bytes specified from the buffer
void ClientConnection::WriteExact(char *buf, int bytes)
{
	omni_mutex_lock l(m_writeMutex);
	if (bytes == 0) return;

	//vnclog.Print(10, _T("  writing %d bytes\n"), bytes);

	m_BytesSend += bytes;

	int i = 0;

	// sf@2002 - DSM Plugin
	char *pBuffer = buf;
	if (m_fUsePlugin)
	{
		if (m_pDSMPlugin->IsEnabled())
		{
			int nTransDataLen = 0;
			pBuffer = (char*)(m_pDSMPlugin->TransformBuffer((BYTE*)buf, bytes, &nTransDataLen));
			if (pBuffer == NULL || (bytes > 0 && nTransDataLen == 0))
				throw WarningException(sz_L68);
			bytes = nTransDataLen;
		}
	}

	if (m_direct)
	{
		SendPrequeue(pBuffer,bytes);
	}
	else
	{
		m_direct=true;
		newtick=GetTickCount();
		if ((newtick-oldtick)<50) 
		{
			Sendqueue(pBuffer,bytes);
		}
		else 
		{
			SendPrequeue(pBuffer,bytes);
			oldtick=newtick;
		}
		
	}

}

void ClientConnection::Sendqueue(char *buf, int bytes)
{
	int i = 0;
    int j;
	int size=m_queuesize+bytes;

	if (size>8100)
	{
		memcpy(m_queuebuff+m_queuesize,buf,bytes);
		while (i < size)
			{
				j = send(m_sock, m_queuebuff+i, size-i, 0);
				vnclog.Print(1, _T("Bytes send %d: \n"), size);
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
						PostMessage(m_hwnd,WM_CLOSE, 0, 0);
						throw WarningException(sz_L69);
					}
				i += j;
			}
		m_queuesize=0;
		
	}
	else
	{
		memcpy(m_queuebuff+m_queuesize,buf,bytes);
		m_queuesize+=bytes;
	}
}

void ClientConnection::SendPrequeue(char *buf, int bytes)
{
	int i = 0;
    int j;
	int size=m_queuesize+bytes;
	if (m_queuesize==0)
	{
		while (i < bytes)
			{
				j = send(m_sock, buf+i, bytes-i, 0);
				vnclog.Print(1, _T("Bytes send %d: \n"), bytes);
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
						PostMessage(m_hwnd,WM_CLOSE, 0, 0);
						throw WarningException(sz_L69);
					}
				i += j;
			}
		return;

	}
		memcpy(m_queuebuff+m_queuesize,buf,bytes);
		while (i < size)
			{
				j = send(m_sock, m_queuebuff+i, size-i, 0);
				vnclog.Print(1, _T("Bytes send %d: \n"), size);
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
						PostMessage(m_hwnd,WM_CLOSE, 0, 0);
						throw WarningException(sz_L69);
					}
				i += j;
			}
		m_queuesize=0;
}



// Sends the number of bytes specified from the buffer
void ClientConnection::WriteExactProxy(char *buf, int bytes)
{

	if (bytes == 0) return;
	omni_mutex_lock l(m_writeMutex);
	//vnclog.Print(10, _T("  writing %d bytes\n"), bytes);

	m_BytesSend += bytes;
	int i = 0;
    int j;

	// sf@2002 - DSM Plugin
	char *pBuffer = buf;

    while (i < bytes)
	{
		j = send(m_sock, pBuffer+i, bytes-i, 0);
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
			PostMessage(m_hwnd,WM_CLOSE, 0, 0);
			throw WarningException(sz_L69);
		}
		i += j;
    }

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




// Makes sure netbuf is at least as big as the specified size.
// Note that netbuf itself may change as a result of this call.
// Throws an exception on failure.
void ClientConnection::CheckQueueBufferSize(int bufsize)
{
	if (bufsize < 0 || bufsize > 104857600) // 100 MBytes max
	{
		throw ErrorException(sz_L71);
	}
	if (m_queuebuffsize > bufsize) return;

	omni_mutex_lock l(m_bufferMutex);

	char *newbuf = new char[bufsize+256];
	if (newbuf == NULL) {
		throw ErrorException(sz_L70);
	}

	// Only if we're successful...

	if (m_queuebuff != NULL)
		delete [] m_queuebuff;
	m_queuebuff = newbuf;
	m_queuebuffsize=bufsize + 256;
	vnclog.Print(4, _T("bufsize expanded to %d\n"), m_netbufsize);
}

// Processing NewFBSize pseudo-rectangle. Create new framebuffer of
// the size specified in pfburh->r.w and pfburh->r.h, and change the
// window size correspondingly.
//
void ClientConnection::ReadNewFBSize(rfbFramebufferUpdateRectHeader *pfburh)
{
	m_si.framebufferWidth = pfburh->r.w;
	m_si.framebufferHeight = pfburh->r.h;
//	ClearCache();
	CreateLocalFramebuffer();
    SendFullFramebufferUpdateRequest();
	Createdib();\
	m_pendingScaleChange = true;
	m_pendingFormatChange = true;
	SendAppropriateFramebufferUpdateRequest();
	SizeWindow();
	InvalidateRect(m_hwnd, NULL, TRUE);
//	RealiseFullScreenMode();
}

//
// sf@2002 - DSMPlugin 
//

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
	if (m_hwndStatus)SetDlgItemInt(m_hwndStatus, IDC_SPEED, kbitsPerSecond, false);

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
			case rfbEncodingZlib:		
				if (m_hwndStatus)SetDlgItemText(m_hwndStatus, IDC_ENCODER, m_opts.m_fEnableCache ? "XORZlib, Cache" : "XORZlib");
				break;
  			case rfbEncodingZRLE:		
				if (m_hwndStatus)SetDlgItemText(m_hwndStatus, IDC_ENCODER, m_opts.m_fEnableCache ? "ZRLE, Cache" :"ZRLE");
  				break;
			case rfbEncodingZYWRLE:		
				if (m_hwndStatus)SetDlgItemText(m_hwndStatus, IDC_ENCODER, m_opts.m_fEnableCache ? "ZYWRLE, Cache" :"ZYWRLE");
  				break;
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

LRESULT CALLBACK ClientConnection::GTGBS_ShowStatusWindow(LPVOID lpParameter)
{
	ClientConnection *_this = (ClientConnection*)lpParameter;

	 _this->m_fStatusOpen = true;
	DialogBoxParam(_this->m_pApp->m_instance,MAKEINTRESOURCE(IDD_STATUS),NULL,(DLGPROC)ClientConnection::GTGBS_StatusProc,(LPARAM)_this);
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
			
			ClientConnection *_this = (ClientConnection *)lParam;
			helper::SafeSetWindowUserData(hwnd, lParam);
			SetDlgItemInt(hwnd,IDC_RECEIVED,_this->m_BytesRead,false);
			SetDlgItemInt(hwnd,IDC_SEND,_this->m_BytesSend,false);
			
			/*if (_this->m_host != NULL) {
				SetDlgItemText(hwnd,IDC_VNCSERVER,_this->m_host);
				sprintf(wt,"%s %s",sz_L72,_this->m_host);
				SetWindowText(hwnd,wt);
			} else
			*/
			{
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
				closesocket(_this->m_sock);
				EndDialog(hwnd, TRUE);
			}
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
					Key |=KEYMAP_LALT;
				if (ALTGR != 0)
					Key |= KEYMAP_RALT;
				if (STRG != 0)
					Key |= KEYMAP_RCONTROL;
				
				if (Okay)
					EndDialog(Dlg, Key);
				else
					EndDialog(Dlg, 0);
			}
		}
	}
	return 0;
}

void
ClientConnection:: ConvertAll(int width, int height, int xx, int yy,int bytes_per_pixel,BYTE* source,BYTE* dest,int framebufferWidth)
{
	if (dest==NULL || source==NULL) return;
	int bytesPerInputRow = width * bytes_per_pixel;
	int bytesPerOutputRow = framebufferWidth * bytes_per_pixel;
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
ClientConnection:: ConvertAll_source_dest(int width, int height, int xx, int yy,int bytes_per_pixel,BYTE* source,BYTE* dest,int framebufferWidth)
{
	if (dest==NULL || source==NULL) return;
//	int bytesPerInputRow = framebufferWidth * bytes_per_pixel;
	int bytesPerOutputRow = framebufferWidth * bytes_per_pixel;
	BYTE *sourcepos,*destpos;
	destpos = (BYTE *)dest + (bytesPerOutputRow * yy)+(xx * bytes_per_pixel);
	sourcepos=(BYTE*)source + (bytesPerOutputRow * yy)+(xx * bytes_per_pixel);
//	vnclog.Print(4, _T("m_bitsCache %d %d %d\n"),dest,destpos,destpos-dest);


    int y;
    width=width*bytes_per_pixel;
    for (y=0; y<height; y++) {
        memcpy(destpos, sourcepos, width);
        sourcepos = (BYTE*)sourcepos + bytesPerOutputRow;
        destpos = (BYTE*)destpos + bytesPerOutputRow;
//		vnclog.Print(4, _T("m_bitsCache %d %d %d\n"),dest,destpos,destpos-dest);
    }
}

void
ClientConnection:: ConvertAll_dest0_source(int width, int height, int xx, int yy,int bytes_per_pixel,BYTE* dest,BYTE* source,int framebufferWidth)
{
	if (dest==NULL || source==NULL) return;
	int bytesPerOutputRow = width * bytes_per_pixel;
	int bytesPerInputRow = framebufferWidth * bytes_per_pixel;
	BYTE *sourcepos,*destpos;
	destpos = (BYTE *)dest ;
	sourcepos=(BYTE*)source+ (bytesPerInputRow * yy)+(xx * bytes_per_pixel);

    int y;
    width*=bytes_per_pixel;
    for (y=0; y<height; y++) {
        memcpy(destpos, sourcepos, width);
        sourcepos = (BYTE*)sourcepos + bytesPerInputRow;
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

void ClientConnection::SendKeepAlive(bool bForce)
{
    if (m_server_wants_keepalives) 
    {
        static time_t lastSent = 0;
        time_t now = time(&now);
        int delta = now - lastSent;

        if (!bForce && delta < KEEPALIVE_INTERVAL)
            return;

        lastSent = now;
#if defined(_DEBUG)
        char msg[255];
        sprintf(msg, "keepalive requested %u seconds since last one\n", delta);
        OutputDebugString(msg);

#endif
        rfbKeepAliveMsg kp;
        memset(&kp, 0, sizeof kp);
        kp.type = rfbKeepAlive;
        WriteExact((char*)&kp, sz_rfbKeepAliveMsg, rfbKeepAlive);
    }
}

void
ClientConnection:: ConvertSolid(int width, int height, int xx, int yy,int bytes_per_pixel,BYTE* color,BYTE* dest,int framebufferWidth)
{
	if (dest==NULL || color==NULL) return;
	int bytesPerOutputRow = framebufferWidth * bytes_per_pixel;
	BYTE *sourcepos,*destpos,*solidline;
	int pixeline=width*bytes_per_pixel;
	destpos = (BYTE *)dest + (bytesPerOutputRow * yy)+(xx * bytes_per_pixel);
	sourcepos=(BYTE*)color;

    int y;
	int x;
	for (x=0;x<width;x++)
		{
			memcpy(destpos+x*bytes_per_pixel, sourcepos, bytes_per_pixel);
		}
		solidline=destpos;
        destpos = (BYTE*)destpos + bytesPerOutputRow;

    for (y=1; y<height; y++) {	
        memcpy(destpos, solidline, pixeline);
        destpos = (BYTE*)destpos + bytesPerOutputRow;
    }
}

#pragma warning(default :4101)
