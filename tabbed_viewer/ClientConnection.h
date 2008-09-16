//  Copyright (C) 2002 Ultr@VNC Team Members. All Rights Reserved.
//  Copyright (C) 2000-2002 Const Kaplinsky. All Rights Reserved.
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

#ifndef CLIENTCONNECTION_H__
#define CLIENTCONNECTION_H__

#pragma once

#include "stdhdrs.h"
#ifdef UNDER_CE
#include "omnithreadce.h"
#else
#include "omnithread.h"
#endif
#include "VNCOptions.h"
#include "VNCviewerApp.h"
#include "KeyMap.h"
#include "KeyMapJap.h"
#include <rdr/types.h>
#include "zlib/zlib.h"
extern "C"
{
	#include "libjpeg/jpeglib.h" // For Tight encoding
}
#include "FileTransfer.h" // sf@2002
#include "TextChat.h" // sf@2002
//#include "bmpflasher.h"
#include "MRU.h"
// #include <DSMPlugin/DSMPlugin.h> // sf@2002

//#include "FullScreenTitleBar.h" //Added by: Lars Werner (http://lars.werner.no)

extern const UINT FileTransferSendPacketMessage;

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#define SETTINGS_KEY_NAME "Software\\ORL\\VNCviewer\\Settings"
#define MAX_HOST_NAME_LEN 250

#define ZLIBHEX_DECOMP_UNINITED (-1)
#define BZIP2HEX_DECOMP_UNINITED (-1)

// Messages used by VNCon - Copyright (C) 2001-2003 - Alastair Burr
#define WM_SETSCALING WM_USER+101
#define WM_SETVIEWONLY WM_USER+102
#define WM_GETSCALING WM_USER+103


#define TIGHT_ZLIB_BUFFER_SIZE 512 // Tight encoding
class ClientConnection;
class CDSMPlugin;
typedef void (ClientConnection:: *tightFilterFunc)(int);

struct mybool {
 bool b0 : 1;
 bool b1 : 1;
 bool b2 : 1;
 bool b3 : 1;
 bool b4 : 1;
 bool b5 : 1;
 bool b6 : 1;
 bool b7 : 1;
};

struct BitmapInfo {
  BITMAPINFOHEADER bmiHeader;
  union {
    struct {
      DWORD red;
      DWORD green;
      DWORD blue;
    } mask;
    RGBQUAD color[256];
  };
};

typedef struct CHILD                           
{
    int iType;                                                                 
    HWND hWnd;
	int tab_nr;
	TCHAR temptitle[25];
} CHILD;

namespace rdr { class InStream; class FdInStream; class ZlibInStream; }

class ClientConnection  : public omni_thread
{
public:

	HWND m_hSessionDialog;
	int m_port;
	int m_proxyport;
//	int m_proxy;
	bool saved_set;
    TCHAR m_host[MAX_HOST_NAME_LEN];
	TCHAR m_proxyhost[MAX_HOST_NAME_LEN];
	bool m_fUseProxy;
//	TCHAR m_remotehost[MAX_HOST_NAME_LEN];
	int  LoadConnection(char *fname, bool fFromDialog,bool default_file);
	int  LoadConnection_global(char *fname, bool fFromDialog,bool default_file);
	void HandleQuickOption();

	void GTGBS_CreateDisplay(void);
	void GTGBS_ScrollToolbar(int dx, int dy);
	void CreateButtons(BOOL mini,BOOL ultra);
	ClientConnection();
	ClientConnection(VNCviewerApp *pApp);
	ClientConnection(VNCviewerApp *pApp, SOCKET sock);
	ClientConnection(VNCviewerApp *pApp, LPTSTR host, int port);
	ClientConnection(VNCviewerApp *pApp, LPTSTR configFile);
	virtual ~ClientConnection();
	void CloseWindows();
	void Run();
	void Global_options();
	void KillThread();
	void SuspendThread();

	// Exceptions 
	class UserCancelExc {};
	class AuthenticationExc {};
	class SocketExc {};
	class ProtocolExc {};
	class Fatal {};
	HANDLE KillEvent;
	bool SetSendTimeout(int msecs = -1);
    bool SetRecvTimeout(int msecs = -1);


	bool bMaximized;
	int old_autoscale;

	bool IsDormant(){ return m_dormant;};
	void SendKeyEvent(CARD32 key, bool down);
	void SendKeepAlive(bool bForce = false); // 16 july 2008 jdp

private:
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK WndProcTBwin(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK WndProchwnd(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
	void DoBlit();
	VNCviewerApp *m_pApp;

	SOCKET m_sock;
	bool m_serverInitiated;

	HWND m_hwnd,m_hwndStatus;
	HANDLE m_statusThread;
	RECT m_TBr;
	// bool m_ToolbarEnable;
	bool m_remote_mouse_disable;
	bool m_SWselect;
	bool previous_scaling;
	POINT m_SWpoint;
	HCURSOR hNewCursor;
	HCURSOR hOldCursor;
	BOOL skipprompt2;

	void Init(VNCviewerApp *pApp);
	void CreateDisplay();
	void GTGBS_CreateToolbar();
	HWND GTGBS_ShowConnectWindow();
	//DWORD WINAPI GTGBS_ShowStatusWindow(LPVOID lpParameter);
	static LRESULT CALLBACK GTGBS_ShowStatusWindow(LPVOID lpParameter);
	static LRESULT CALLBACK GTGBS_StatusProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK GTGBS_SendCustomKey_proc(HWND Dlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
	void LoadDSMPlugin(bool  fForceReload); // sf@2002 - DSM Plugin
	void SetDSMPluginStuff();
	void GetConnectDetails();
	void GetConnectDetails_global();
	void Connect();
	void ConnectProxy();
	void SetSocketOptions();
	void Authenticate();
	void AuthMsLogon();
	void NegotiateProtocolVersion();
	void NegotiateProxy();
	void ReadServerInit();
	void SendClientInit();
	void CreateLocalFramebuffer();
	void SaveConnection();
	void Save_Latest_Connection();
	void Save_auto_Connection();
	void Save_auto_Connection_all();
	
	void SetupPixelFormat();
	void SetFormatAndEncodings();
	void SendSetPixelFormat(rfbPixelFormat newFormat);

	void SendIncrementalFramebufferUpdateRequest();
	void SendFullFramebufferUpdateRequest();
	void SendAppropriateFramebufferUpdateRequest();
	void SendFramebufferUpdateRequest(int x, int y, int w, int h, bool incremental);
	
	void ProcessPointerEvent(int x, int y, DWORD keyflags, UINT msg);
 	void SubProcessPointerEvent(int x, int y, DWORD keyflags);
	void ProcessMouseWheel(int delta); // RealVNC 335 method
	void SendPointerEvent(int x, int y, int buttonMask);
    void ProcessKeyEvent(int virtkey, DWORD keyData);
	
	void ReadScreenUpdate();
	void Update(RECT *pRect);
	void SizeWindow();
	void SizeWindowNoSet();
	bool ScrollScreen(int dx, int dy);
	void UpdateScrollbars();
    
	void ReadRawRect(rfbFramebufferUpdateRectHeader *pfburh);
	void ReadUltraRect(rfbFramebufferUpdateRectHeader *pfburh);
	void ReadUltraZip(rfbFramebufferUpdateRectHeader *pfburh,HRGN *prgn);
	void ReadCopyRect(rfbFramebufferUpdateRectHeader *pfburh);
    void ReadRRERect(rfbFramebufferUpdateRectHeader *pfburh);
	void ReadCoRRERect(rfbFramebufferUpdateRectHeader *pfburh);
	void ReadHextileRect(rfbFramebufferUpdateRectHeader *pfburh);
	void ReadZlibRect(rfbFramebufferUpdateRectHeader *pfburh,int XOR);
	void ReadSolidRect(rfbFramebufferUpdateRectHeader *pfburh);
	void HandleHextileEncoding8(int x, int y, int w, int h);
	void HandleHextileEncoding16(int x, int y, int w, int h);
	void HandleHextileEncoding32(int x, int y, int w, int h);
	
	void ReadRBSRect(rfbFramebufferUpdateRectHeader *pfburh);
	BOOL DrawRBSRect8(int x, int y, int w, int h, CARD8 **pptr);
	BOOL DrawRBSRect16(int x, int y, int w, int h, CARD8 **pptr);
	BOOL DrawRBSRect32(int x, int y, int w, int h, CARD8 **pptr);

	// ClientConnectionFullScreen.cpp
//	void SetFullScreenMode(bool enable);
	bool InFullScreenMode();
//	void RealiseFullScreenMode();
//	bool BumpScroll(int x, int y);
//	CTitleBar TitleBar; //Added by: Lars Werner (http://lars.werner.no)

	//SINGLE WINDOW
	void ReadNewFBSize(rfbFramebufferUpdateRectHeader *pfburh);

	// Caching
	void SaveArea(RECT &r);
	void CleanCache(RECT &r);
	void RestoreArea(RECT &r);
	void ReadCacheRect(rfbFramebufferUpdateRectHeader *pfburh);
	void Check_m_bitsCacheSwapBuffer(int bufsize);
//	void ClearCache();
	void ReadCacheZip(rfbFramebufferUpdateRectHeader *pfburh,HRGN *prgn);
	void ReadSolMonoZip(rfbFramebufferUpdateRectHeader *pfburh,HRGN *prgn);

	// ClientConnectionTight.cpp
	void ReadTightRect(rfbFramebufferUpdateRectHeader *pfburh);
	int ReadCompactLen();
	int InitFilterCopy (int rw, int rh);
	int InitFilterGradient (int rw, int rh);
	int InitFilterPalette (int rw, int rh);
	void FilterCopy8 (int numRows);
	void FilterCopy16 (int numRows);
	void FilterCopy24 (int numRows);
	void FilterCopy32 (int numRows);
	void FilterGradient8 (int numRows);
	void FilterGradient16 (int numRows);
	void FilterGradient24 (int numRows);
	void FilterGradient32 (int numRows);
	void FilterPalette (int numRows);
	void DecompressJpegRect(int x, int y, int w, int h);

	// Tight ClientConnectionCursor.cpp
	bool prevCursorSet;
	bool SaveAreaSet;
	BYTE *m_SavedAreaCursor;
	COLORREF *rcSource;
	BYTE *rcSourcecolor;
	bool *rcMask;
	int rcHotX, rcHotY, rcWidth, rcHeight;
	int rcCursorX, rcCursorY;
	int rcLockX, rcLockY, rcLockWidth, rcLockHeight;
	bool rcCursorHidden, rcLockSet;
	void ReadCursorShape(rfbFramebufferUpdateRectHeader *pfburh);
	void SoftCursorLockArea(int x, int y, int w, int h);
	void SoftCursorUnlockScreen();
	void SoftCursorMove(int x, int y);
	void SoftCursorFree();
	bool SoftCursorInLockedArea();
	void SoftCursorSaveArea();
	void SoftCursorRestoreArea();
	void SoftCursorDraw();
	void SoftCursorToScreen(RECT *screenArea, POINT *cursorOffset);
	void InvalidateScreenRect(const RECT *pRect);
	void InvalidateRegion(const RECT *pRect,HRGN *prgn);

	// ClientConnectionZlibHex.cpp
	void HandleZlibHexEncoding8(int x, int y, int w, int h);
	void HandleZlibHexEncoding16(int x, int y, int w, int h);
	void HandleZlibHexEncoding32(int x, int y, int w, int h);
	void HandleZlibHexSubencodingStream8(int x, int y, int w, int h, int subencoding,CARD8 *bgcolor,CARD8 *fgcolor);
	void HandleZlibHexSubencodingStream16(int x, int y, int w, int h, int subencoding,CARD16 *bgcolor,CARD16 *fgcolor);
	void HandleZlibHexSubencodingStream32(int x, int y, int w, int h, int subencoding,CARD32 *bgcolor,CARD32 *fgcolor);
	void HandleZlibHexSubencodingBuf8(int x, int y, int w, int h, int subencoding, unsigned char * buffer,CARD8 *bgcolor,CARD8 *fgcolor);
	void HandleZlibHexSubencodingBuf16(int x, int y, int w, int h, int subencoding, unsigned char * buffer,CARD16 *bgcolor,CARD16 *fgcolor);
	void HandleZlibHexSubencodingBuf32(int x, int y, int w, int h, int subencoding, unsigned char * buffer,CARD32 *bgcolor,CARD32 *fgcolor);
	void ReadZlibHexRect(rfbFramebufferUpdateRectHeader *pfburh);

	bool zlibDecompress(unsigned char *from_buf, unsigned char *to_buf, unsigned int count, unsigned int size, z_stream *decompressor);

	// ClientConnectionClipboard.cpp
	void ProcessLocalClipboardChange();
	void UpdateLocalClipboard(char *buf, int len);
	void SendClientCutText(char *str, int len);
	void ReadServerCutText();

	void ReadBell();
	
	void SendRFBMsg(CARD8 msgType, void* data, int length);
	void ReadExact(char *buf, int bytes);
	void ReadExactProxy(char *buf, int bytes);
	void ReadString(char *buf, int length);
	void WriteExact(char *buf, int bytes);
	void WriteExactProxy(char *buf, int bytes);
	void WriteExact(char *buf, int bytes, CARD8 msgType); //sf@2002 - DSM Plugin
	void Sendqueue(char *buf, int bytes);
	void SendPrequeue(char *buf, int bytes);

	void GetFriendlySizeString(__int64 Size, char* szText);
	void UpdateStatusFields();	// sf@2002

	// This is what controls the thread
	void * run_undetached(void* arg);
	bool m_bKillThread;

	// Modif sf@2002 - FileTransfer
	friend class FileTransfer;  
	friend class TextChat;  

	// Modif sf@2002 - Server Scaling
	bool SendServerScale(int nScale);
	// Modif rdv@2002 - Server dis/enable input
	bool SendServerInput(BOOL enabled);
	bool SendSW(int x, int y);
	// sf@2002 - DSM Plugin
	void CheckNetRectBufferSize(int nBufSize);
	void CheckZRLENetRectBufferSize(int nBufSize);
	//
	int EncodingStatusWindow,OldEncodingStatusWindow;

	// Utilities

	// These draw a solid rectangle of colour on the bitmap
	// They assume the bitmap is already selected into the DC, and the
	// DC is locked if necessary.
#ifndef UNDER_CE
	// Normally this is an inline call to a GDI method.
	inline void FillSolidRect(RECT *pRect, COLORREF color) {
		COLORREF oldbgcol = SetBkColor(m_hBitmapDC, color);
		// This is the call MFC uses for FillSolidRect. Who am I to argue?
		::ExtTextOut(m_hBitmapDC, 0, 0, ETO_OPAQUE, pRect, NULL, 0, NULL);			
	};
#else
	// Under WinCE this is a manual insert into a pixmap, 
	// and is a little too complicated for an inline.
	void FillSolidRect(RECT *pRect, COLORREF color);
#endif // UNDER_CE

	inline void FillSolidRect(int x, int y, int w, int h, COLORREF color) {
		RECT r;
		r.left = x;		r.right = x + w;
		r.top = y;		r.bottom = y + h;
		ConvertSolid(w, h, x, y,m_myFormat.bitsPerPixel/8,(BYTE*) &color,(BYTE *)m_DIBbits,m_si.framebufferWidth);

//		FillSolidRect(&r, color);
	};
	inline void FillSolidRect8(int x, int y, int w, int h, CARD8 *color) {
		RECT r;
		r.left = x;		r.right = x + w;
		r.top = y;		r.bottom = y + h;
		ConvertSolid(w, h, x, y,m_myFormat.bitsPerPixel/8,(BYTE*) color,(BYTE *)m_DIBbits,m_si.framebufferWidth);

//		FillSolidRect(&r, color);
	};
	inline void FillSolidRect16(int x, int y, int w, int h, CARD16 *color) {
		RECT r;
		r.left = x;		r.right = x + w;
		r.top = y;		r.bottom = y + h;
		ConvertSolid(w, h, x, y,m_myFormat.bitsPerPixel/8,(BYTE*) color,(BYTE *)m_DIBbits,m_si.framebufferWidth);

//		FillSolidRect(&r, color);
	};
	inline void FillSolidRect32(int x, int y, int w, int h, CARD32 *color) {
		RECT r;
		r.left = x;		r.right = x + w;
		r.top = y;		r.bottom = y + h;
		ConvertSolid(w, h, x, y,m_myFormat.bitsPerPixel/8,(BYTE*) color,(BYTE *)m_DIBbits,m_si.framebufferWidth);

//		FillSolidRect(&r, color);
	};

    // how many other windows are owned by this process?
    unsigned int CountProcessOtherWindows();

    // Buffer for network operations
	void CheckBufferSize(int bufsize);
	void CheckQueueBufferSize(int bufsize);
	char *m_netbuf;
	int m_netbufsize;
	char *m_queuebuff;
	int m_queuesize;
	int m_queuebuffsize;
	bool m_direct;
	omni_mutex	m_bufferMutex, m_zlibBufferMutex,
				m_bitmapdcMutex,  m_clipMutex,
				m_writeMutex, m_sockMutex,m_cursorMutex, m_readMutex  ;
	
	// Buffer for zlib decompression.
	void CheckZlibBufferSize(int bufsize);
	unsigned char *m_zlibbuf;
	int m_zlibbufsize;

	// zlib decompression state
	bool m_decompStreamInited;
	z_stream m_decompStream;
	z_stream m_decompStreamRaw;
	z_stream m_decompStreamEncoded;

	void CheckZipBufferSize(int bufsize);
	unsigned char *m_zipbuf;
	int m_zipbufsize;

	// sf@2002 - v1.1.0 - Buffer for zip decompression (FileTransfer)
	void CheckFileZipBufferSize(int bufsize);
	unsigned char *m_filezipbuf;
	int m_filezipbufsize;

	void CheckFileChunkBufferSize(int bufsize);
	unsigned char *m_filechunkbuf;
	int m_filechunkbufsize;


	// Variables used by tight encoding:
	// Separate buffer for tight-compressed data.
	char m_tightbuf[TIGHT_ZLIB_BUFFER_SIZE];

	// Four independent compression streams for zlib library.
	z_stream m_tightZlibStream[4];
	bool m_tightZlibStreamActive[4];

	// Tight filter stuff. Should be initialized by filter initialization code.
	tightFilterFunc m_tightCurrentFilter;
	bool m_tightCutZeros;
	int m_tightRectWidth, m_tightRectColors;
	COLORREF m_tightPalette[256];
	CARD8 m_tightPrevRow[2048*3*sizeof(CARD16)];
	//

	// Bitmap for local copy of screen, and DC for writing to it.
	HBITMAP m_hBitmap;
	HDC		m_hBitmapDC;
	HPALETTE m_hPalette;
	// Bitmap for cache copy of screen, and DC for writing to it.
	BYTE *m_bitsCache;
	BYTE *m_bitsCacheSwapBuffer;
	int m_bitsCacheSwapBufferSize;

#ifdef UNDER_CE
	// Under WinCE this points to the DIB pixels.
	BYTE* m_bits;
#endif
 
	// Keyboard mapper
    KeyMap *m_keymap;
	KeyMapJap *m_keymapJap;

	// RFB settings
	VNCOptions m_opts;

	FileTransfer *m_pFileTransfer; // Modif sf@2002 - FileTransfer
	TextChat *m_pTextChat;			// Modif sf@2002 - Text Chat
	int m_nServerScale; 	       // Modif sf@2002 - Server Scaling

	int m_reconnectcounter;
	bool m_bClosedByUser;
	// Modif sf@2002 - Data Stream Modification Plugin handling
	int m_nTO;
	CDSMPlugin *m_pDSMPlugin;
	bool m_fUsePlugin;
	BYTE* m_pNetRectBuf;
	bool m_fReadFromNetRectBuf;  // 
	int m_nNetRectBufOffset;
	int m_nReadSize;
	int m_nNetRectBufSize;
	BYTE* m_pZRLENetRectBuf;
	bool m_fReadFromZRLENetRectBuf;  // 
	int m_nZRLENetRectBufOffset;
	int m_nZRLEReadSize;
	int m_nZRLENetRectBufSize;
	omni_mutex	m_NetRectBufferMutex;
	omni_mutex	m_ZRLENetRectBufferMutex;
	omni_mutex	m_ZipBufferMutex;
	omni_mutex	m_FileZipBufferMutex;
	omni_mutex	m_FileChunkBufferMutex;
	omni_mutex	m_ZlibBufferMutex;
	
	TCHAR *m_desktopName;
	unsigned char m_encPasswd[8];
	unsigned char m_encPasswdMs[32];
	char m_ms_user[256];  // act: add user storage for mslogon autoreconnect
	char m_cmdlnUser[256]; // act: add user option on command line
	char m_clearPasswd[256]; // Modif sf@2002

	rfbServerInitMsg m_si;
	rfbPixelFormat m_myFormat, m_pendingFormat;
	// protocol version in use.
	int m_majorVersion, m_minorVersion;
	bool m_threadStarted, m_running;
	// mid-connection format change requested

	// sf@2002 - v1.1.0
	bool m_pendingFormatChange;
	bool m_pendingScaleChange;
	bool m_pendingCacheInit;

	// Display connection info;
	void ShowConnInfo();

	// Window may be scrollable - these control the scroll position
	int m_hScrollPos, m_hScrollMax, m_vScrollPos, m_vScrollMax;
	// The current window size
	int m_winwidth, m_winheight;
	__int64 m_BytesSend;
	__int64 m_BytesRead;
//	HANDLE m_bitmapFRONT,m_bitmapBACK,m_bitmapNONE,m_logo_min;
	// The size of the current client area
	int m_cliwidth, m_cliheight;
	// The size of a window needed to hold entire screen without scrollbars
	int m_fullwinwidth, m_fullwinheight;
	// The size of the CE CommandBar
	int m_barheight;

	// Dormant basically means minimized; updates will not be requested 
	// while dormant.
	void SetDormant(bool newstate);
	bool m_dormant;

	// The number of bytes required to hold at least one pixel.
	unsigned int m_minPixelBytes;
	// Next window in clipboard chain
	HWND m_hwndNextViewer; 
	bool m_initialClipboardSeen;		

	// Are we waiting on a timer for emulating three buttons?
	bool m_waitingOnEmulateTimer;
	// Or are we emulating the middle button now?
	bool m_emulatingMiddleButton;
	// Emulate 3 buttons mouse timer:
	UINT m_emulate3ButtonsTimer;
	// Buttons pressed, waiting for timer in emulating 3 buttons:
	DWORD m_emulateKeyFlags;
	int m_emulateButtonPressedX;
	int m_emulateButtonPressedY;

//	BmpFlasher *flash;

	// ms logon
	BOOL m_ms_logon;
	char m_ad_passwd[256];
	char m_ad_domain[256];
	char m_ad_user[256];

	// sf@2002 - FileTRansfer on server
	BOOL m_fServerKnowsFileTransfer;

	// sf@2002 - Auto mode
	int  m_nConfig;

	// sf@2002 - Options Window flag
	BOOL m_fOptionsOpen;

	BOOL m_fStatusOpen;
	int  m_nStatusTimer;
//	int m_FTtimer;
	int dormantTimer;
	BitmapInfo bi;

	int oldPointerX, oldPointerY, oldButtonMask;
	
	int kbitsPerSecond;
	int m_lLastChangeTime; // sf@2003 - Last time the Auto mode has changed the encoding
	bool m_fScalingDone; // sf@2003 - Auto Scaling flag

	rdr::FdInStream* fis;
	rdr::ZlibInStream* zis;
	void zrleDecode(int x, int y, int w, int h);
	void zrleDecode8NE(int x, int y, int w, int h, rdr::InStream* is,
		rdr::ZlibInStream* zis, rdr::U8* buf);
	void zrleDecode15LE(int x, int y, int w, int h, rdr::InStream* is,
		rdr::ZlibInStream* zis, rdr::U16* buf);
	void zrleDecode16LE(int x, int y, int w, int h, rdr::InStream* is,
		rdr::ZlibInStream* zis, rdr::U16* buf);
	void zrleDecode24ALE(int x, int y, int w, int h, rdr::InStream* is,
		rdr::ZlibInStream* zis, rdr::U32* buf);
	void zrleDecode24BLE(int x, int y, int w, int h, rdr::InStream* is,
		rdr::ZlibInStream* zis, rdr::U32* buf);
	void zrleDecode32LE(int x, int y, int w, int h, rdr::InStream* is,
		rdr::ZlibInStream* zis, rdr::U32* buf);
	long zywrle;
	long zywrle_level;
	int zywrleBuf[rfbZRLETileWidth*rfbZRLETileHeight];
	//UltraFast
	void ConvertAll(int width, int height, int xx, int yy,int bytes_per_pixel,BYTE* source,BYTE* dest,int framebufferWidth);
	void ConvertSolid(int width, int height, int xx, int yy,int bytes_per_pixel,BYTE* source,BYTE* dest,int framebufferWidth);
	void ConvertAll_dest0_source(int width, int height, int xx, int yy,int bytes_per_pixel,BYTE* source,BYTE* dest,int framebufferWidth);
	void ConvertAll_source_dest(int width, int height, int xx, int yy,int bytes_per_pixel,BYTE* source,BYTE* dest,int framebufferWidth);
	HDC				m_hmemdc;
 	HBITMAP			m_membitmap;
 	VOID			*m_DIBbits;
	bool			UltraFast;
	void ClientConnection::Createdib();
	bool Check_Rectangle_borders(int x,int y,int w,int h);
	BOOL m_BigToolbar;
	DWORD newtick;
	DWORD oldtick;
	bool Pressed_Cancel;

	// sf@2007 - AutoReconnect
	int m_autoReconnect;
	void Reconnect();
	void DoConnection();
	int reconnectcounter;
	HANDLE ThreadSocketTimeout;

    bool m_server_wants_keepalives;

int sleep;
bool m_Inputlock;
//PixelFormat m_directxformat;

};

// Some handy classes for temporary GDI object selection
// These select objects when constructed and automatically release them when destructed.
class ObjectSelector {
public:
	ObjectSelector(HDC hdc, HGDIOBJ hobj) { m_hdc = hdc; m_hOldObj = SelectObject(hdc, hobj); }
	~ObjectSelector() { m_hOldObj = SelectObject(m_hdc, m_hOldObj); }
	HGDIOBJ m_hOldObj;
	HDC m_hdc;
};

class PaletteSelector {
public:
	PaletteSelector(HDC hdc, HPALETTE hpal) { 
		m_hdc = hdc; 
		if (hpal)
		{
		m_hOldPal = SelectPalette(hdc, hpal, FALSE); 
		RealizePalette(hdc);
		}
	}
	~PaletteSelector() { 
		if(m_hOldPal)
		{
		m_hOldPal = SelectPalette(m_hdc, m_hOldPal, FALSE); 
		RealizePalette(m_hdc);
		}
	}
	HPALETTE m_hOldPal;
	HDC m_hdc;
};

class TempDC {
public:
	TempDC(HWND hwnd) { m_hdc = GetDC(hwnd); m_hwnd = hwnd; }
	~TempDC() { ReleaseDC(m_hwnd, m_hdc); }
	operator HDC() {return m_hdc;};
	HDC m_hdc;
	HWND m_hwnd;
};

// Colour decoding utility functions
// Define rs,rm, bs,bm, gs & gm before using, eg with the following:

// read a pixel from the given address, and return a color value
#define SETUP_COLOR_SHORTCUTS \
	 CARD8 rs = m_myFormat.redShift;   CARD16 rm = m_myFormat.redMax;   \
     CARD8 gs = m_myFormat.greenShift; CARD16 gm = m_myFormat.greenMax; \
     CARD8 bs = m_myFormat.blueShift;  CARD16 bm = m_myFormat.blueMax;  \

#define COLOR_FROM_PIXEL8_ADDRESS(p) (PALETTERGB( \
                (int) (((*(CARD8 *)(p) >> rs) & rm) * 255 / rm), \
                (int) (((*(CARD8 *)(p) >> gs) & gm) * 255 / gm), \
                (int) (((*(CARD8 *)(p) >> bs) & bm) * 255 / bm) ))

#define COLOR_FROM_PIXEL16_ADDRESS(p) (PALETTERGB( \
                (int) ((( *(CARD16 *)(p) >> rs) & rm) * 255 / rm), \
                (int) ((( *(CARD16 *)(p) >> gs) & gm) * 255 / gm), \
                (int) ((( *(CARD16 *)(p) >> bs) & bm) * 255 / bm) ))

#define COLOR_FROM_PIXEL24_ADDRESS(p) (PALETTERGB( \
                (int) (((CARD8 *)(p))[0]), \
                (int) (((CARD8 *)(p))[1]), \
                (int) (((CARD8 *)(p))[2]) ))

#define COLOR_FROM_PIXEL32_ADDRESS(p) (PALETTERGB( \
                (int) ((( *(CARD32 *)(p) >> rs) & rm) * 255 / rm), \
                (int) ((( *(CARD32 *)(p) >> gs) & gm) * 255 / gm), \
                (int) ((( *(CARD32 *)(p) >> bs) & bm) * 255 / bm) ))

// The following may be faster if you already have a pixel value of the appropriate size
#define COLOR_FROM_PIXEL8(p) (PALETTERGB( \
                (int) (((p >> rs) & rm) * 255 / rm), \
                (int) (((p >> gs) & gm) * 255 / gm), \
                (int) (((p >> bs) & bm) * 255 / bm) ))

#define COLOR_FROM_PIXEL16(p) (PALETTERGB( \
                (int) ((( p >> rs) & rm) * 255 / rm), \
                (int) ((( p >> gs) & gm) * 255 / gm), \
                (int) ((( p >> bs) & bm) * 255 / bm) ))

#define COLOR_FROM_PIXEL32(p) (PALETTERGB( \
                (int) (((p >> rs) & rm) * 255 / rm), \
                (int) (((p >> gs) & gm) * 255 / gm), \
                (int) (((p >> bs) & bm) * 255 / bm) ))

#define COLRORREF_TO_DIB(R,G,B,DIB) {\
									 DIB = R*rm/255;\
									 DIB = DIB << (rs-gs);\
									 DIB |= G*gm/255;\
					                 DIB = DIB << (gs-bs);\
					                 DIB |= B*bm/255;\
									}


#ifdef UNDER_CE
#define SETPIXEL(b,x,y,c) SetPixel((b),(x),(y),(c))
#else
#define SETPIXEL(b,x,y,c) SetPixelV((b),(x),(y),(c))
#endif

#define SETPIXELS(buffer, bpp, x, y, w, h)										\
	{																			\
		CARD##bpp *p = (CARD##bpp *) buffer;									\
        register CARD##bpp pix;													\
		for (int k = y; k < y+h; k++) {											\
			for (int j = x; j < x+w; j++) {										\
                    pix = *p;													\
                    SETPIXEL(m_hBitmapDC, j,k, COLOR_FROM_PIXEL##bpp##(pix));	\
					p++;														\
			}																	\
		}																		\
	}


////////////TIGHT OS NOT WORKING
#define SETPIXELS_NOCONV(buffer, x, y, w, h)									\
	{																			\
		INT bpp=32;																\
		int bytes_per_pixel=bpp/8;												\
		int bytesPerOutputRow = m_si.framebufferWidth * bytes_per_pixel;		\
		int bytesPerInputRow = w * bytes_per_pixel;								\
		BYTE *sourcepos,*destpos;												\
		destpos = (BYTE *)m_DIBbits + (bytesPerOutputRow * y)+(x * bytes_per_pixel);\
		sourcepos=(BYTE*)buffer;												\
		int y2;																	\
		w*=bytes_per_pixel;													\
		for (y2=0; y2<h; y2++) {											\
	        memcpy(destpos, sourcepos, w);									\
			sourcepos = (BYTE*)sourcepos + bytesPerInputRow;					\
			destpos = (BYTE*)destpos + bytesPerOutputRow;						\
		}																		\
	}

#define SETXORPIXELS(mask,buffer, bpp, x, y, w, h,aantal)						\
	{																			\
		CARD##bpp *p = (CARD##bpp *) buffer;									\
        register CARD##bpp pix;													\
		BYTE *prt;																\
		int i=0;																\
		bool result;															\
		int bytes_per_pixel=bpp/8;												\
		int bytesPerOutputRow = m_si.framebufferWidth * bytes_per_pixel;		\
		for (int k = y; k < y+h; k++) {											\
			for (int j = x; j < x+w; j++) {										\
					MYMASK(mask,i,result);										\
					if (result)													\
						{														\
						prt=(BYTE *)m_DIBbits+ (bytesPerOutputRow * k)+(j * bytes_per_pixel);\
						memcpy(prt,(BYTE*)p,bytes_per_pixel);\
						p++;													\
						aantal++;												\
						}														\
					i++;														\
			}																	\
		}																		\
	}

#define SETXORSOLPIXELS(mask,buffer, color, bpp, x, y, w, h)				\
	{																			\
		CARD##bpp *p = (CARD##bpp *) buffer;									\
		CARD##bpp *pc = (CARD##bpp *) color;									\
		BYTE *prt;																\
		int i=0;																\
		bool result;															\
		int bytes_per_pixel=bpp/8;												\
		int bytesPerOutputRow = m_si.framebufferWidth * bytes_per_pixel;		\
		for (int k = y; k < y+h; k++) {											\
			for (int j = x; j < x+w; j++) {										\
					MYMASK(mask,i,result);										\
					if (result)													\
						{														\
						prt=(BYTE *)m_DIBbits+ (bytesPerOutputRow * k)+(j * bytes_per_pixel);\
						memcpy(prt,(BYTE*)p,bytes_per_pixel);\
						p++;													\
						}														\
					else														\
						{														\
						prt=(BYTE *)m_DIBbits+ (bytesPerOutputRow * k)+(j * bytes_per_pixel);\
						memcpy(prt,(BYTE*)pc,bytes_per_pixel);\
						}														\
					i++;														\
			}																	\
		}																		\
	}

#define SETXORMONOPIXELS(mask,color2, color, bpp, x, y, w, h)					\
	{																			\
		CARD##bpp *pc2 = (CARD##bpp *) color2;									\
		CARD##bpp *pc = (CARD##bpp *) color;									\
		BYTE *prt;																\
        register CARD##bpp pix;													\
		int i=0;																\
		bool result;															\
		int bytes_per_pixel=bpp/8;												\
		int bytesPerOutputRow = m_si.framebufferWidth * bytes_per_pixel;		\
		for (int k = y; k < y+h; k++) {											\
			for (int j = x; j < x+w; j++) {										\
				MYMASK(mask,i,result);											\
					if (result)												\
						{														\
						prt=(BYTE *)m_DIBbits+ (bytesPerOutputRow * k)+(j * bytes_per_pixel);\
						memcpy(prt,(BYTE*)pc2,bytes_per_pixel);\
						}														\
					else														\
						{														\
						prt=(BYTE *)m_DIBbits+ (bytesPerOutputRow * k)+(j * bytes_per_pixel);\
						memcpy(prt,(BYTE*)pc,bytes_per_pixel);\
						}														\
					i++;														\
			}																	\
		}																		\
	}

#define MYMASK(mask,i,result)						\
	{												\
		int byte_nr,bit_nr;							\
		byte_nr=i/8;								\
		bit_nr=i%8;								\
		if (bit_nr==0) result=mask[byte_nr].b0;		\
		if (bit_nr==1) result=mask[byte_nr].b1;		\
		if (bit_nr==2) result=mask[byte_nr].b2;		\
		if (bit_nr==3) result=mask[byte_nr].b3;		\
		if (bit_nr==4) result=mask[byte_nr].b4;		\
		if (bit_nr==5) result=mask[byte_nr].b5;		\
		if (bit_nr==6) result=mask[byte_nr].b6;		\
		if (bit_nr==7) result=mask[byte_nr].b7;		\
	}

#endif