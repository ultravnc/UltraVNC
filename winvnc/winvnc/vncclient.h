/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
//  Copyright (C) 2000-2002 Const Kaplinsky. All Rights Reserved.
//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
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
//  If the source code for the program is not available from the place from
//  which you received this file, check
//  https://uvnc.com/
//
////////////////////////////////////////////////////////////////////////////


// vncClient.h

// vncClient class handles the following functions:
// - Recieves requests from the connected client and
//   handles them
// - Handles incoming updates properly, using a vncBuffer
//   object to keep track of screen changes
// It uses a vncBuffer and is passed the vncDesktop and
// vncServer to communicate with.

class DesktopUsersToken;
class vncClient;
typedef SHORT vncClientId;

#if (!defined(_WINVNC_VNCCLIENT))
#define _WINVNC_VNCCLIENT
#ifndef __GNUC__
#pragma warning(disable : 4786)
#endif

#include <list>
#include <string>
#include <vector>
#include <algorithm>
#include "../common/win32_helpers.h"

typedef std::list<vncClientId> vncClientList;

// Includes
#include "stdhdrs.h"
#include "vsocket.h"
#include <omnithread.h>

// Custom
#include "vncdesktop.h"
#include "rfbRegion.h"
#include "rfbUpdateTracker.h"
#include "vncbuffer.h"
#include "vncencodemgr.h"
#include "TextChat.h" // sf@2002 - Text Chat
#include "ZipUnZip32/ZipUnZip32.h"
//#include "timer.h"
// adzm - 2010-07 - Extended clipboard
#include "../common/Clipboard.h"

#include "MouseSimulator.h"

// The vncClient class itself
typedef UINT (WINAPI *pSendinput)(UINT,LPINPUT,INT);
#define SPI_GETMOUSESPEED         0x0070
#define SPI_SETMOUSESPEED         0x0071
#define MOUSEEVENTF_VIRTUALDESK	  0x4000

class vncClientUpdateThread;

#define FT_PROTO_VERSION_OLD 1  // <= RC18 UltraVNC Server "fOldFTPRotocole" version
#define FT_PROTO_VERSION_2   2  // base File Transfer Protocol
#define FT_PROTO_VERSION_3   3  // new File Transfer Protocol session messages

#ifdef _Gii
struct MyTouchINfo
{
	DWORD TouchId;
	DWORD pointerflag;
	DWORD touchmask;
	int X;
	int Y;
	int ContactWidth;
	int ContactHeight;
	DWORD time;
};
#ifdef _USE_DLL
typedef BOOL(__cdecl*PInitializeTouchInjection)(int);
typedef BOOL(__cdecl*PInjectTouch)(int points, MyTouchINfo *ti_array);
#endif
typedef BOOL(WINAPI* PtrInjectTouchInput)(UINT32, POINTER_TOUCH_INFO*);
typedef BOOL(WINAPI* PtrInitializeTouchInjection)(UINT32, DWORD);
#endif

extern int CheckUserGroupPasswordUni(char * userin,char *password,const char *machine);

using namespace rfb;

class vncClientUpdateThread : public omni_thread
{
public:

	// Init
	BOOL Init(vncClient* client);

	// Kick the thread to send an update
	void Trigger();

	// Kill the thread
	void Kill();

	// Disable/enable updates
	void EnableUpdates(BOOL enable);

	void get_time_now(unsigned long* abs_sec, unsigned long* abs_nsec);

	// The main thread function
	virtual void* run_undetached(void* arg);

protected:
	virtual ~vncClientUpdateThread();

	// Fields
protected:
	vncClient* m_client;
	omni_condition* m_signal;
	omni_condition* m_sync_sig;
	BOOL m_active;
	BOOL m_enable;
	bool first_run;
};

class vncClient
{
public:
	// Constructor/destructor
	vncClient();
	virtual ~vncClient();

	// Allow the client thread to see inside the client object
	friend class vncClientThread;
	friend class vncClientUpdateThread;

	// Init
	virtual BOOL Init(vncServer *server, VSocket *socket, BOOL auth, BOOL shared, vncClientId newid);

	// Kill
	// The server uses this to close the client socket, causing the
	// client thread to fail, which in turn deletes the client object
	virtual void Kill(bool deleted = false);

	// Client manipulation functions for use by the server
	virtual void SetBuffer(vncBuffer *buffer);
	bool	NotifyUpdate(rfbFramebufferUpdateRequestMsg fur);

	// Update handling functions
	// These all lock the UpdateLock themselves
	virtual void UpdateMouse();
	//virtual void UpdateClipText(const char* text);
	// adzm - 2010-07 - Extended clipboard
	virtual void UpdateClipTextEx(ClipboardData& clipboardData, CARD32 overrideFlags = 0);
	virtual void UpdatePalette(bool lock);
	virtual void UpdateLocalFormat(bool lock);
	int nr_incr_rgn_empty;
	char displayname[256] = {};
	// Is the client waiting on an update?
	// YES IFF there is an incremental update region,
	//     AND no changed or copied updates intersect it
	virtual BOOL UpdateWanted() {
		omni_mutex_lock l(GetUpdateLock(),324);
#ifdef _DEBUG
										OutputDevMessage("%i %i %i %i",!m_incr_rgn.is_empty(),
											m_incr_rgn.intersect(m_update_tracker.get_changed_region()).is_empty() ,
											m_incr_rgn.intersect(m_update_tracker.get_cached_region()).is_empty() ,
											m_incr_rgn.intersect(m_update_tracker.get_copied_region()).is_empty());
#endif
		if (sendingUpdate == true)		
			return true;
		BOOL value =!m_incr_rgn.is_empty() && m_incr_rgn.intersect(m_update_tracker.get_changed_region()).is_empty() &&
			m_incr_rgn.intersect(m_update_tracker.get_cached_region()).is_empty() &&
			m_incr_rgn.intersect(m_update_tracker.get_copied_region()).is_empty();
		if (m_incr_rgn.is_empty())
		{
			nr_incr_rgn_empty++;
			if (nr_incr_rgn_empty > 300)
			{
				nr_incr_rgn_empty=0;				
				rfbFramebufferUpdateRequestMsg fur{};
				fur.x = 0;
				fur.y = 0;
				fur.w = 10;
				fur.h = 10;
				NotifyUpdate(fur);
			}
		}
		else nr_incr_rgn_empty = 0;
		return value;

	};

	// Has the client sent an input event?
	virtual BOOL RemoteEventReceived() {BOOL result = m_remoteevent;m_remoteevent = FALSE;return result;};

	// The UpdateLock
	// This must be held for a number of routines to be successfully invoked...
	virtual omni_mutex& GetUpdateLock() {return m_encodemgr.GetUpdateLock();};

	// Functions for setting & getting the client settings
	virtual void EnableKeyboard(BOOL enable) 
	{
		if (m_keyboardenabled == true)
			m_keyboardenabled = enable;
	};
	virtual void EnablePointer(BOOL enable) 
	{
		if (m_pointerenabled == true)
			m_pointerenabled = enable;
	};

	virtual void EnableGii(BOOL enable)
	{
		if (m_GiiEnabled == true)
			m_GiiEnabled = enable;
	};

	virtual void EnableJap(bool enable) {m_jap = enable;};
	virtual void EnableUnicode(bool enable) {m_unicode = enable;};
	virtual void SetCapability(int capability) {m_capability = capability;};

	virtual int GetCapability() {return m_capability;};
	virtual const char *GetClientDomainUsername();
	virtual const char *GetClientNameName();
	const char* GetClientNameAddress();
	virtual vncClientId GetClientId() {return m_id;};

	// Disable/enable protocol messages to the client
	virtual void DisableProtocol();
	virtual void EnableProtocol();
	virtual void DisableProtocol_no_mutex();
	virtual void EnableProtocol_no_mutex();
	// resize desktop
	virtual BOOL SetNewSWSize(long w,long h,BOOL desktop);
	virtual void SetBufferOffset(int x,int y);
	virtual void SetScreenOffset(int x,int y, bool single_display);
	virtual void InitialUpdate(bool value);

	virtual TextChat* GetTextChatPointer() { return m_pTextChat; }; // sf@2002
	virtual void SetUltraViewer(bool fTrue) { m_fUltraViewer = fTrue;};
	virtual bool IsUltraViewer() { return m_fUltraViewer;};
	bool singleExtendRequested() { return m_singleExtendMode; }
	virtual void EnableCache(BOOL enabled);
	// sf@2002
	virtual void SetConnectTime(long lTime) {m_lConnectTime = lTime;};
	virtual long GetConnectTime() {return m_lConnectTime;};
	virtual bool IsSlowEncoding() {return m_encodemgr.IsSlowEncoding();};
	virtual bool IsUltraEncoding() {return m_encodemgr.IsUltraEncoding();};
	virtual bool IsUltra2Encoding() {return m_encodemgr.IsUltra2Encoding();};
	virtual bool IsEncoderSet() { return m_encodemgr.IsEncoderSet(); };
	virtual bool IsFileTransBusy(){return (m_fFileUploadRunning||m_fFileDownloadRunning || m_fFileSessionOpen);};
	void SetProtocolVersion(rfbProtocolVersionMsg *protocolMsg);
	void SetOutgoing(bool outgoing) {m_outgoing = outgoing;};
	void Clear_Update_Tracker();
	void TriggerUpdate();
	void UpdateCursorShape();
	void setTiming(DWORD value) {m_timing = value;}
	DWORD getTiming() {return m_timing;}
	bool forceBlacklist;

	// adzm 2009-07-05 - repeater IDs
	void SetRepeaterID(char* szid)
	{
		if (m_szRepeaterID) {
			free(m_szRepeaterID);
		}

		m_szRepeaterID = NULL;

		if (szid != NULL && strlen(szid) > 0) {
			m_szRepeaterID = _strdup(szid);
		}
	};
	char* GetRepeaterID() {return m_szRepeaterID;};

	// adzm 2009-08-02
	void SetHost(char* szHost)
	{
		if (m_szHost) {
			free(m_szHost);
		}

		m_szHost = NULL;

		if (szHost != NULL && strlen(szHost) > 0) {
			m_szHost = _strdup(szHost);
		}
	};
	char* GetHost() {return m_szHost;};

	void SetHostPort(int port) {
		m_hostPort = port;
	};
	int GetHostPort() {
		return m_hostPort;
	};

	// sf@2004 - Asynchronous File Transfer - Delta Transfer
	int  GenerateFileChecksums(HANDLE hFile, char* lpCSBuffer, int nCSBufferSize);
	bool ReceiveDestinationFileChecksums(int nSize, int nLen);
	bool ReceiveFileChunk(int nLen, int nSize);
	void FinishFileReception();
	bool SendFileChunk();
	void FinishFileSending();
	bool GetSpecialFolderPath(int nId, char* szPath);
	int  ZipPossibleDirectory(LPSTR szSrcFileName);
	int  CheckAndZipDirectoryForChecksuming(LPSTR szSrcFileName);
	bool  UnzipPossibleDirectory(LPSTR szFileName);
	bool MyGetFileSize(char* szFilePath, ULARGE_INTEGER* n2FileSize);
    HANDLE m_hPToken; //used to set File Transfer thread to correct user token
#ifndef SC_20
	bool DoFTUserImpersonation();
	void UndoFTUserImpersonation();
#endif // SC_20

    // jdp@2008 - File Transfer event hooks
    void FTUploadStartHook();
    void FTUploadCancelledHook();
    void FTUploadFailureHook();
    void FTUploadCompleteHook();

    void FTDownloadStartHook();
    void FTDownloadCancelledHook();
    void FTDownloadFailureHook();
    void FTDownloadCompleteHook();

    void FTNewFolderHook(std::string name);
    void FTDeleteHook(std::string name, bool isDir);
    void FTRenameHook(std::string oldName, std::string newname);
    void SendServerStateUpdate(CARD32 state, CARD32 value);
	void Record_SendServerStateUpdate(CARD32 state, CARD32 value);
    void SendKeepAlive(bool bForce = false);
    void SendFTProtocolMsg();
	// adzm - 2010-07 - Extended clipboard
	void NotifyExtendedClipboardSupport();
	// adzm 2010-09 - Notify streaming DSM plugin support
	void NotifyPluginStreamingSupport();
	bool cl_connected;
	int filetransferrequestPart2(int nDirZipRet);
	char m_szSrcFileName[MAX_PATH + 64]; // Path + timestring
	HANDLE ThreadHandleCompressFolder;
	// sf@2002 
	// Update routines
	char infoMsg[255] = { 0 };
protected:
	DesktopUsersToken* desktopUsersToken;
	BOOL SendUpdate(rfb::SimpleUpdateTracker &update);
	BOOL SendRFBMsg(CARD8 type, BYTE *buffer, int buflen);
	//adzm 2010-09 - minimize packets. SendExact flushes the queue.
	BOOL SendRFBMsgQueue(CARD8 type, BYTE *buffer, int buflen);
	BOOL SendRectangles(const rfb::RectVector &rects);
	BOOL SendRectangle(const rfb::Rect &rect);
	BOOL SendCopyRect(const rfb::Rect &dest, const rfb::Point &source);
	BOOL SendPalette();
	// CACHE
	BOOL SendCacheRectangles(const rfb::RectVector &rects);
	BOOL SendCacheRect(const rfb::Rect &dest);
	BOOL SendCacheZip(const rfb::RectVector &rects); // sf@2002
	// Tight - CURSOR HANDLING
	BOOL SendCursorShapeUpdate();
	// nyama/marscha - PointerPos
	BOOL SendCursorPosUpdate();
	BOOL SendLastRect(); // Tight
	void TriggerUpdateThread();
	CARD32 m_state;
	CARD32 m_value;
	bool m_want_update_state; 
	int unlockcounter;
	int filetransferrequestPart1(rfbClientToServerMsg msg, bool fUserOk);	
	// Specialised client-side UpdateTracker
	// This update tracker stores updates it receives and
	// kicks the client update thread every time one is received
	class ClientUpdateTracker : public rfb::SimpleUpdateTracker {
	public:
		ClientUpdateTracker() : m_client(0) {};
		virtual ~ClientUpdateTracker() {};

		void init(vncClient *client) {m_client=client;}

		virtual void add_changed(const rfb::Region2D &region) {
			{
				// RealVNC 336 change - omni_mutex_lock l(m_client->GetUpdateLock());
				SimpleUpdateTracker::add_changed(region);
				m_client->TriggerUpdateThread();
			}
		}
		virtual void add_cached(const rfb::Region2D &region) {
			{
				// RealVNC 336 change - omni_mutex_lock l(m_client->GetUpdateLock());
				SimpleUpdateTracker::add_cached(region);
				m_client->TriggerUpdateThread();
			}
		}
		
		virtual void add_copied(const rfb::Region2D &dest, const rfb::Point &delta) {
			{
				// RealVNC 336 change - omni_mutex_lock l(m_client->GetUpdateLock());
				SimpleUpdateTracker::add_copied(dest, delta);
				m_client->TriggerUpdateThread();
			}
		}

		virtual void clear() {
			// RealVNC 336 change - omni_mutex_lock l(m_client->GetUpdateLock());
			SimpleUpdateTracker::clear();
		}

		virtual void flush_update(rfb::UpdateInfo &info, const rfb::Region2D &cliprgn) {;
			// RealVNC 336 change - omni_mutex_lock l(m_client->GetUpdateLock());
			SimpleUpdateTracker::flush_update(info, cliprgn);
		}
		virtual void flush_update(rfb::UpdateTracker &to, const rfb::Region2D &cliprgn) {;
			// RealVNC 336 change - omni_mutex_lock l(m_client->GetUpdateLock());
			SimpleUpdateTracker::flush_update(to, cliprgn);
		}

		virtual void get_update(rfb::UpdateInfo &info) const {;
			// RealVNC 336 change - omni_mutex_lock l(m_client->GetUpdateLock());
			SimpleUpdateTracker::get_update(info);
		}
		virtual void get_update(rfb::UpdateTracker &to) const {
			// RealVNC 336 change - omni_mutex_lock l(m_client->GetUpdateLock());
			SimpleUpdateTracker::get_update(to);
		}

		virtual bool is_empty() const{
			// RealVNC 336 change -  omni_mutex_lock l(m_client->GetUpdateLock());
			return SimpleUpdateTracker::is_empty();
		}
	protected:
		vncClient *m_client;
	};

	friend class ClientUpdateTracker;

	// Make the update tracker available externally
public:

	rfb::UpdateTracker &GetUpdateTracker() {return m_update_tracker;};
	int				monitor_Offsetx;
	int				monitor_Offsety;
	int				m_ScreenOffsetx;
	int				m_ScreenOffsety;
	int				m_single_display;
	BOOL			m_NewSWUpdateWaiting;
	rfbProtocolVersionMsg ProtocolVersionMsg;
	//Timer Sendtimer;
	//int roundrobin_counter;
	//int timearray[rfbEncodingZRLE+1][31];
	//int sizearray[rfbEncodingZRLE+1][31];
	//int Totalsend;
	BOOL client_settings_passed;
	bool initialCapture_done;
	void SetHasMouse(bool has_mouse);
	bool ask_mouse;
	bool sendingUpdate;
	bool		m_Autoreconnect;
	// The socket
	VSocket			*m_socket;

	// Internal stuffs
protected:
	bool has_mouse;
	SimulateCursor* simulateCursor;
	// Per-client settings
	bool			m_IsLoopback;
	bool			m_keyboardenabled = true;
	bool			m_pointerenabled = true;
	bool			m_GiiEnabled = true;
	bool			m_jap;
	bool			m_unicode;
	int				m_capability;
	vncClientId		m_id;
	long			m_lConnectTime;

	// Pixel translation & encoding handler
	vncEncodeMgr	m_encodemgr;
	bool			m_singleExtendMode;
	bool			m_firstExtDesktop;
	bool			m_firstExtDesktopIncremental;
	DWORD			m_timing;

	// The server
	vncServer		*m_server;
	
	char			*m_client_domain_username;
	char			*m_client_name_name;
	char			* m_client_name_address;

	// The client thread
	omni_thread		*m_thread_ClientThread;

	// adzm 2009-07-05
	char*			m_szRepeaterID;
	// adzm 2009-08-02
	char*			m_szHost;
	int				m_hostPort;


	// Count to indicate whether updates, clipboards, etc can be sent
	// to the client. If 0 then OK, otherwise not.
	ULONG			m_disable_protocol;

	// User input information
	rfb::Rect		m_oldmousepos;
	BOOL			m_mousemoved;
	rfbPointerEventMsg	m_ptrevent;
	// vncKeymap		m_keymap;

	// Update tracking structures
	ClientUpdateTracker	m_update_tracker;

	// Client update transmission thread
	vncClientUpdateThread *m_updatethread;

	// Requested update region & requested flag
	rfb::Region2D	m_incr_rgn;

	// Full screen rectangle
//	rfb::Rect		m_fullscreen;

	// When the local display is palettized, it sometimes changes...
	BOOL			m_palettechanged;

	// Information used in polling mode!
	BOOL			m_remoteevent;

	// Clipboard data
	//char*			m_clipboard_text;
	Clipboard		m_clipboard;

	//SINGLE WINDOW
	BOOL			m_use_NewSWSize;
	BOOL			m_use_ExtDesktopSize;
	int m_requestedDesktopSizeChange;
	int m_lastDesktopSizeChangeError;
	BOOL			m_NewSWDesktop;
	int				NewsizeW;
	int				NewsizeH;

	// CURSOR HANDLING
	BOOL			m_cursor_update_pending;
	BOOL			m_cursor_update_sent;
	// nyama/marscha - PointerPos
	BOOL			m_cursor_pos_changed;
	BOOL			m_use_PointerPos;
	POINT			m_cursor_pos;

	// Modif sf@2002 - File Transfer 
	BOOL m_fFileTransferRunning;
	CZipUnZip32		*m_pZipUnZip;

	char  m_szFullDestName[MAX_PATH + 64];
	char  m_szFileTime[18];
	char* m_pBuff;
	char* m_pCompBuff;
	HANDLE m_hDestFile;
	DWORD m_dwFileSize; 
	DWORD m_dwNbPackets;
	DWORD m_dwNbReceivedPackets;
	DWORD m_dwNbBytesWritten;
	DWORD m_dwTotalNbBytesWritten;
	bool m_fFileDownloadError;
	bool m_fFileDownloadRunning;
    bool m_fFileSessionOpen;

    // 8 April 2008 jdp
    bool m_fUserAbortedFileTransfer;
	
	HANDLE m_hSrcFile;
	bool m_fEof;
	DWORD m_dwNbBytesRead;
	DWORD m_dwTotalNbBytesRead;
	int m_nPacketCount;
	bool m_fCompressionEnabled;
	bool m_fFileUploadError;
	bool m_fFileUploadRunning;

	// sf@2004 - Delta Transfer
	char*	m_lpCSBuffer;
	int		m_nCSOffset;
	int		m_nCSBufferSize;

	// Modif sf@2002 - Scaling
	rfb::Rect		m_ScaledScreen;
	UINT			m_nScale;
	UINT			m_nScale_viewer;
	bool			fNewScale;
	bool			m_fPalmVNCScaling;
	bool			fFTRequest;

	// sf@2002 
	BYTE* m_pCacheZipBuf;
	unsigned int m_nCacheZipBufSize;
	BYTE* m_pRawCacheZipBuf;
	unsigned int m_nRawCacheZipBufSize;

	friend class TextChat; 
	TextChat *m_pTextChat;	// Modif sf@2002 - Text Chat

	bool m_fUltraViewer; // sf@2002 

	// sf@2005 - FTUserImpersonation
	bool m_fFTUserImpersonatedOk;
	char m_szTempDir[MAX_PATH];
	DWORD m_lLastFTUserImpersonationTime;

	//stats
	int totalraw;

    helper::DynamicFn<pSendinput> Sendinput;

    std::string m_OrigSourceDirectoryName;
    bool        m_wants_ServerStateUpdates;
    bool        m_bClientHasBlockedInput;
	bool		m_Support_rfbSetServerInput;
    bool        m_wants_KeepAlive;
	bool		m_session_supported;
	bool		m_initial_update;
	bool		m_outgoing;
};


// vncClient thread class

class vncClientThread : public omni_thread
{
public:

	// Init
	virtual BOOL Init(vncClient *client,
		vncServer *server,
		VSocket *socket,
		BOOL auth,
		BOOL shared);

	// Sub-Init routines
	virtual BOOL InitVersion();
#ifdef _Gii
	BOOL InitGiiVersion();
#endif
	virtual BOOL InitAuthenticate();
	virtual BOOL AuthenticateClient(std::vector<CARD8>& current_auth, bool isconnected);
	virtual BOOL AuthenticateLegacyClient(bool isconnected);

	BOOL AuthSecureVNCPlugin(std::string& auth_message); // must SetHandshakeComplete after sending auth result!
	BOOL AuthSecureVNCPlugin_old(std::string& auth_message);
	BOOL AuthMsLogon(std::string& auth_message);
	BOOL AuthVnc(std::string& auth_message);
	BOOL AuthSCPrompt(std::string& auth_message); // adzm 2010-10
	BOOL AuthSessionSelect(std::string& auth_message); // adzm 2010-10

	BOOL FilterClients_Blacklist();
#ifndef SC_20
	BOOL FilterClients_Ask_Permission();
#endif // SC_20
	BOOL CheckEmptyPasswd();
	BOOL CheckLoopBack();
	void LogAuthResult(bool success, bool isconnected);
	void SendConnFailed(const char* szMessage);

	// adzm 2010-08
	virtual bool InitSocket();
	virtual bool TryReconnect();

	// The main thread function
	virtual void run(void *arg);
	bool m_autoreconnectcounter_quit;

	UINT m_AutoReconnectPort;
	char m_szAutoReconnectAdr[255];
	char m_szAutoReconnectId[MAX_PATH];
	bool m_deleted;

protected:
	virtual ~vncClientThread();

	// Fields
protected:
	VSocket *m_socket;
	vncServer *m_server;
	vncClient *m_client;
	BOOL m_auth;
	BOOL m_shared;
	BOOL m_ms_logon;
	int m_major;
	int m_minor;
#ifdef _Gii
#ifdef _USE_DLL
	PInitializeTouchInjection DLL_InitializeTouchInjection;
	PInjectTouch DLL_PInjectTouch;
	HMODULE win8dllHandle;
#endif
	PtrInjectTouchInput InjectTouchInputUVNC;
	PtrInitializeTouchInjection InitializeTouchInjectionUVNC;
	bool *point_status;
	DWORD nr_points;
#endif
};
#endif
