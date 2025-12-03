// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2000-2002 Const Kaplinsky. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//


// vncServer.h

// vncServer class handles the following functions:
// - Allowing clients to be dynamically added and removed
// - Propagating updates from the local vncDesktop object
//   to all the connected clients
// - Propagating mouse movements and keyboard events from
//   clients to the local vncDesktop
// It also creates the vncSockConnect
// servers, which respectively allow connections via sockets
// and via the ORB interface
extern bool			fShutdownOrdered;
class vncServer;
class VirtualDisplay;

#if (!defined(_WINVNC_VNCSERVER))
#define _WINVNC_VNCSERVER

// Custom
#include "vncsockconnect.h"
#include "vnchttpconnect.h"
#include "vncclient.h"
#include "rfbRegion.h"
#include "vncpasswd.h"

// Includes
#include "stdhdrs.h"
#include <omnithread.h>
#include <list>
#include <thread>
#include <string>
#include <memory>

// adzm - 2010-07 - Extended clipboard
#include "common/Clipboard.h"

// Define a datatype to handle lists of windows we wish to notify
typedef std::list<HWND> vncNotifyList;

// Some important constants;
const int MAX_CLIENTS = 128;

// The vncServer class itself

class CloudThread;

class vncServer
{
public:

	friend class vncClientThread;
	HANDLE m_impersonationtoken;

	// Constructor/destructor
	vncServer();
	~vncServer();

	char code[18]{};
	char* generateCode();

	// Client handling functions
	virtual vncClientId AddClient(VSocket *socket, BOOL auth, BOOL shared, BOOL outgoing);
	virtual vncClientId AddClient(VSocket *socket, BOOL auth, BOOL shared, rfbProtocolVersionMsg *protocolMsg, BOOL outgoing);
	virtual vncClientId AddClient(VSocket *socket,
		BOOL auth, BOOL shared, int capability,rfbProtocolVersionMsg *protocolMsg, BOOL outgoing);
	virtual vncClientId AddClient(VSocket *socket,
		BOOL auth, BOOL shared, int capability,rfbProtocolVersionMsg *protocolMsg, VString szRepeaterID, VString szHost, VCard port, BOOL outgoing);

	virtual BOOL Authenticated(vncClientId client);
	virtual void KillClient(vncClientId client);
	virtual void KillClient(LPSTR szClientName); // sf@2002
	virtual void TextChatClient(LPSTR szClientName); // sf@2002
	bool IsUltraVNCViewer();
	bool AreThereMultipleViewers();
	bool singleExtendRequested();

	virtual UINT AuthClientCount();
	virtual UINT UnauthClientCount();

	virtual void KillAuthClients();
	virtual void ListAuthClients(HWND hListBox);
	virtual void WaitUntilAuthEmpty();

	virtual void KillUnauthClients();
	virtual void ListUnauthClients(HWND hListBox); // adzm 2009-07-05
	virtual void WaitUntilUnauthEmpty();

	// Are any clients ready to send updates?
	virtual BOOL UpdateWanted();

	// Has at least one client had a remote event?
	virtual BOOL RemoteEventReceived();

	// Client info retrieval/setup
	virtual vncClient* GetClient(vncClientId clientid);
	virtual vncClientList ClientList();

	virtual void SetCapability(vncClientId client, int capability);

	virtual int GetCapability(vncClientId client);
	virtual const char* GetClientNameName(vncClientId client);

	// Let a client remove itself
	virtual void RemoveClient(vncClientId client);

	// Connect/disconnect notification
	virtual BOOL AddNotify(HWND hwnd);
	virtual BOOL RemNotify(HWND hwnd);

	virtual vncDesktop* GetDesktopPointer() {return m_desktop;}
	virtual void SetNewSWSize(long w,long h,BOOL desktop);
	virtual void SetBufferOffset(int x,int y);
	virtual void SetScreenOffset(int x,int y, bool single_display); //never locked
	virtual void InitialUpdate(bool value);
	short getOldestViewer();
	UINT getNumberViewers();

	virtual BOOL All_clients_initialalized();
	void initialCapture_done();

	// Lock to protect the client list from concurrency - lock when reading/updating client list
	omni_mutex			m_clientsLock;
	omni_mutex			m_clientsLockBlackList;
	omni_mutex			m_clientsLock_notifyList;

	UINT				m_port;
	UINT				m_port_http; // TightVNC 1.2.7
	int					m_autocapt;

	virtual void ShutdownServer();
	HANDLE retryThreadHandle;
	
	// VNC Bridge methods
	virtual BOOL StartBridge();
	virtual void StopBridge();
	virtual const char* GetDiscoveryCode();
	virtual BOOL IsBridgeRunning();
	virtual void UpdateBridgeSettings(); // Handle settings changes

protected:
	// Send a notification message
	

public:
	int AutoReconnect_counter;
	virtual void DoNotify(UINT message, WPARAM wparam, LPARAM lparam);
	// Update handling, used by the screen server
	virtual rfb::UpdateTracker &GetUpdateTracker() {return m_update_tracker;};
	virtual void UpdateMouse();
	// adzm - 2010-07 - Extended clipboard
	//virtual void UpdateClipText(const char* text);
	virtual void UpdateClipTextEx(HWND hwndOwner, vncClient* excludeClient = NULL);
	virtual void UpdatePalette(bool lock);
	virtual void UpdateLocalFormat(bool lock);

	virtual void Driver(BOOL enable);

	BOOL DriverWanted;
	BOOL HookWanted;
	BOOL DriverWantedSet;

	virtual void Hook(BOOL enable);
	virtual void SetHookings();

	// Client manipulation of the clipboard
	virtual void UpdateLocalClipText(LPSTR text);
	// adzm - 2010-07 - Extended clipboard
	virtual void UpdateLocalClipTextEx(ExtendedClipboardDataMessage& extendedClipboardDataMessage, vncClient* sourceClient);

	virtual void SetName(const char * name);
	virtual void SetPorts(const UINT port_rfb, const UINT port_http);	
	virtual void KillSockConnect();
	virtual void SetAutoPortSelect(const BOOL autoport);
	virtual void EnableRemoteInputs(BOOL enable);
	virtual void EnableJapInput(BOOL enable);
	virtual void EnableUnicodeInput(BOOL enable);

	// General connection handling

	// Socket connection handling
	virtual BOOL EnableConnections(BOOL on);
	virtual BOOL SockConnected();
	virtual BOOL SetLoopbackOnly(BOOL loopbackOnly);
	//virtual BOOL LoopbackOnly();

	void SetSendExtraMouse(BOOL i_fSendExtraMouse);
	BOOL SendExtraMouse();

	// Tray icon disposition
	virtual void SetNoScreensaver(BOOL NoScreensaver);

	// HTTP daemon handling
	virtual BOOL EnableHTTPConnect(BOOL enable);

	virtual void GetScreenInfo(int &width, int &height, int &depth);

	// Handling of per-client connection authorisation
	virtual void SetAuthHosts(const char *hostlist);
	virtual char *AuthHosts();
	enum AcceptQueryReject {aqrAccept, aqrQuery, aqrReject};
	virtual AcceptQueryReject VerifyHost(const char *hostname);

	// Blacklisting of machines which fail connection attempts too often
	// Such machines will fail VerifyHost for a short period
	virtual void AddAuthHostsBlacklist(const char *machine);
	virtual void RemAuthHostsBlacklist(const char *machine);

	virtual UINT QueryAccept();
	virtual UINT QueryAcceptForSave();
	virtual UINT QueryAcceptLocked();

	// sf@2002 - DSM Plugin
	virtual BOOL SetDSMPlugin(BOOL fForceReload);
	virtual CDSMPlugin* GetDSMPluginPointer() { return m_pDSMPlugin;};

	// sf@2002 - Cursor handling
	virtual void EnableXRichCursor(BOOL fEnable);
	virtual BOOL IsXRichCursorEnabled() {return m_fXRichCursor;}; 

	// sf@2002
	virtual void DisableCacheForAllClients();
	virtual bool IsThereASlowClient();
	virtual bool IsThereAUltraEncodingClient();
	virtual bool IsThereAUltra2EncodingClient();
	virtual bool IsEncoderSet();
	virtual bool IsThereFileTransBusy();


	// sf@2003 - AutoReconnect
	virtual BOOL AutoReconnect()
	{
		if (fShutdownOrdered) return false;
		return m_fAutoReconnect;
	};
	virtual BOOL IdReconnect(){return m_fIdReconnect;};
	virtual UINT AutoReconnectPort(){return m_AutoReconnectPort;};
	virtual char* AutoReconnectAdr(){return m_szAutoReconnectAdr;}
	virtual char* AutoReconnectId(){return m_szAutoReconnectId;}
	virtual void AutoReconnect(BOOL fEnabled){m_fAutoReconnect = fEnabled;};
	virtual void IdReconnect(BOOL fEnabled){m_fIdReconnect = fEnabled;};
	virtual void AutoReconnectPort(UINT nPort){m_AutoReconnectPort = nPort;};
	virtual void AutoReconnectAdr(char* szAdr){strcpy_s(m_szAutoReconnectAdr,255, szAdr);}
	virtual void AutoReconnectId(char* szId){strcpy_s(m_szAutoReconnectId,MAX_PATH, szId);}
	virtual void AutoConnectRetry( );
	void actualRetryThread();
	VSocket *retrysock;

	virtual void Clear_Update_Tracker();
	virtual void UpdateCursorShape();

	bool IsClient(vncClient* pClient);

    void NotifyClients_StateChange(CARD32 state, CARD32 value);

	void TriggerUpdate();
	void SetHasMouse();

	bool OS_Shutdown;
	void StopReconnectAll();
	char* getInfoMsg();
	int m_virtualDisplaySupported;
	VirtualDisplay *virtualDisplay;

protected:
	// The vncServer UpdateTracker class
	// Behaves like a standard UpdateTracker, but propagates update
	// information to active clients' trackers

	class ServerUpdateTracker : public rfb::UpdateTracker {
	public:
		ServerUpdateTracker() : m_server(0) {};

		virtual void init(vncServer *server) {m_server=server;};

		virtual void add_changed(const rfb::Region2D &region);
		virtual void add_cached(const rfb::Region2D &region);
		virtual void add_copied(const rfb::Region2D &dest, const rfb::Point &delta);
	protected:
		vncServer *m_server;
	};

	friend class ServerUpdateTracker;

	ServerUpdateTracker	m_update_tracker;

	// Internal stuffs
protected:
	static void*	pThis;

	// Connection servers
	vncSockConnect		*m_socketConn;
	vncHTTPConnect		*m_httpConn;
	BOOL				m_enableHttpConn;


	// The desktop handler
	vncDesktop			*m_desktop;
	// Name of this desktop
	char				*m_name;

	// Blacklist structures
	struct BlacklistEntry {
		BlacklistEntry *_next;
		char *_machineName;
		LARGE_INTEGER _lastRefTime;
		UINT _failureCount;
		BOOL _blocked;
	};
	BlacklistEntry		*m_blacklist;
	
	// The client lists - list of clients being authorised and ones
	// already authorised
	vncClientList		m_unauthClients;
	vncClientList		m_authClients;
	vncClient			*m_clientmap[MAX_CLIENTS];
	vncClientId			m_nextid;

	omni_mutex			m_desktopLock;
	// Signal set when a client removes itself
	omni_condition		*m_clientquitsig;
	// Set of windows to send notifications to
	vncNotifyList		m_notifyList;

	// Modif sf@2002

	// Modif sf@2002 - v1.1.x

	// sf@2002 - DSMPlugin
	CDSMPlugin *m_pDSMPlugin;

	// sf@2002 - Cursor handling
	BOOL m_fXRichCursor; 

	// sf@2003 - AutoReconnect
	BOOL m_fAutoReconnect;
	BOOL m_fIdReconnect;
	UINT m_AutoReconnectPort;
	char m_szAutoReconnectAdr[255];
	char m_szAutoReconnectId[MAX_PATH];

	HINSTANCE   hWtsLib;

	DWORD startTime;
    BOOL m_fSendExtraMouse;
	bool KillAuthClientsBuzy;	
	BOOL sethook;
	CloudThread* cloudThread;
	
	// VNC Bridge support
	std::unique_ptr<class VncBridge> m_bridge;
	std::unique_ptr<std::thread> m_bridge_thread;
	bool m_bridge_running;
	std::string m_discovery_code;
};

#endif
