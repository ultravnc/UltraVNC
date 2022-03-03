//  Copyright (C) 2002 UltraVNC Team Members. All Rights Reserved.
//  Copyright (C) 2000-2002 Const Kaplinsky. All Rights Reserved.
//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
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


// vncServer.cpp

// vncServer class implementation

// Includes
#include "stdhdrs.h"
#include <omnithread.h>
#include <string.h>
#include <lmcons.h>

// Custom
#include "winvnc.h"
#include "vncserver.h"
#include "vncsockconnect.h"
#include "vncclient.h"
#include "vncservice.h"
#include "vnctimedmsgbox.h"
#include "mmsystem.h" // sf@2002

#include "vncmenu.h"
#include "VirtualDisplay.h"

#include "Localization.h" // ACT : Add localization on messages
bool g_Server_running;
extern bool g_Desktop_running;
extern bool g_DesktopThread_running;
void*	vncServer::pThis;
BOOL G_ipv6_allowed = false;

// adzm 2009-07-05
extern BOOL SPECIAL_SC_PROMPT;
//extern BOOL G_HTTP;
// vncServer::UpdateTracker routines

void
vncServer::ServerUpdateTracker::add_changed(const rfb::Region2D &rgn) {
	vncClientList::iterator i;
	
	omni_mutex_lock l(m_server->m_clientsLock,5);

	// Post this update to all the connected clients
	for (i = m_server->m_authClients.begin(); i != m_server->m_authClients.end(); i++)
	{
		// Post the update
		// REalVNC 336 change
		// m_server->GetClient(*i)->GetUpdateTracker().add_changed(rgn);
		vncClient* client = m_server->GetClient(*i);
		omni_mutex_lock ll(client->GetUpdateLock(),6);
		client->GetUpdateTracker().add_changed(rgn);

	}
}

void
vncServer::ServerUpdateTracker::add_cached(const rfb::Region2D &rgn) {
	vncClientList::iterator i;
	
	omni_mutex_lock l(m_server->m_clientsLock,7);

	// Post this update to all the connected clients
	for (i = m_server->m_authClients.begin(); i != m_server->m_authClients.end(); i++)
	{
		// Post the update
		// RealVNC 336 change
		// m_server->GetClient(*i)->GetUpdateTracker().add_cached(rgn);
		vncClient* client = m_server->GetClient(*i);
		omni_mutex_lock ll(client->GetUpdateLock(),8);
		client->GetUpdateTracker().add_cached(rgn);
	}
}





void
vncServer::ServerUpdateTracker::add_copied(const rfb::Region2D &dest, const rfb::Point &delta) {
	vncClientList::iterator i;
	
	omni_mutex_lock l(m_server->m_clientsLock,9);

	// Post this update to all the connected clients
	for (i = m_server->m_authClients.begin(); i != m_server->m_authClients.end(); i++)
	{
		// Post the update
		// RealVNC 336 change
		//m_server->GetClient(*i)->GetUpdateTracker().add_copied(dest, delta);
		vncClient* client = m_server->GetClient(*i);
		omni_mutex_lock ll(client->GetUpdateLock(),10);
		client->GetUpdateTracker().add_copied(dest, delta);

	}
}



// Constructor/destructor
vncServer::vncServer()
{
	// used for our retry timer proc;
	pThis = this;
	AutoReconnect_counter=0;

	// Initialise some important stuffs...
	g_Server_running=true;
	m_socketConn = NULL;
	m_httpConn = NULL;
	m_enableHttpConn = false;

	m_desktop = NULL;
	m_name = NULL;
	m_port = DISPLAY_TO_PORT(0);
	m_port_http = DISPLAY_TO_HPORT(0); // TightVNC 1.2.7
	m_autoportselect = TRUE;
	m_passwd_required = TRUE;
	m_auth_hosts = 0;
	m_blacklist = 0;
	{
	    vncPasswd::FromClear clearPWD(m_Secure);
	    memcpy(m_password, clearPWD, MAXPWLEN);
	}
	m_querysetting = 2;
	m_queryaccept = 0;
	m_querytimeout = 10;
	m_querydisabletime = 10;

	m_maxViewerSetting = 0;
	m_maxViewers = 128;
	m_Collabo = false;
	// Autolock settings
	m_lock_on_exit = 0;

	// Set the polling mode options
	m_poll_fullscreen = FALSE;
	m_poll_foreground = FALSE;
	m_poll_undercursor = TRUE;

	m_poll_oneventonly = FALSE;
	m_MaxCpu=100;
	m_MaxFPS = 25;
	m_poll_consoleonly = TRUE;

	m_driver = FALSE;
	m_hook = FALSE;
	m_virtual = FALSE;
	sethook=false;
	
	// General options
	m_loopbackOnly = FALSE;
	m_loopback_allowed = TRUE;
	G_ipv6_allowed = FALSE;
	m_lock_on_exit = 0;
	m_connect_pri = 0;
	m_disableTrayIcon = FALSE;
	m_Rdpmode = FALSE;
	m_NoScreensaver = FALSE;
	m_AllowEditClients = FALSE;

	// Set the input options
	m_enable_remote_inputs = TRUE;
	m_disable_local_inputs = FALSE;
	m_enable_jap_input = FALSE;
	m_enable_unicode_input = FALSE;
	m_enable_win8helper = FALSE;

	// Clear the client mapping table
	for (int x=0; x<MAX_CLIENTS; x++)
		m_clientmap[x] = NULL;
	m_nextid = 0;

	// Initialise the update tracker
	m_update_tracker.init(this);

	// Signal set when a client quits
	m_clientquitsig = new omni_condition(&m_clientsLock);

	// Modif sf@2002
	m_TurboMode = false;
	// m_fCursorMoved = false;

	// sf@2002 - v1.1.2
	// m_fQueuingEnabled = false;
	m_fFileTransferEnabled = true;
	m_nDefaultScale = 1;

	// sf@2002 - Data Stream Modification Plugin handling
	m_pDSMPlugin = new CDSMPlugin();

	m_fDSMPluginEnabled = false;
	strcpy_s(m_szDSMPlugin, "");

	m_fMSLogonRequired = false;
	m_Secure = false;

	m_fXRichCursor = false;

	// sf@2003 - Autoreconnect
	m_fAutoReconnect = false;
	m_fIdReconnect = false;
	m_AutoReconnectPort = 0;
	strcpy_s(m_szAutoReconnectAdr, "");
	strcpy_s(m_szAutoReconnectId, "");

	// sf@2005 - No FT User Impersonnation
	m_fFTUserImpersonation = true;

	m_impersonationtoken=NULL; //byteboon

	m_impersonationtoken=NULL; // Modif Jeremy C. 

	m_fRunningFromExternalService = false;
	m_fAutoRestart = false;
    m_ftTimeout = FT_RECV_TIMEOUT;
    m_keepAliveInterval = KEEPALIVE_INTERVAL;
	m_IdleInputTimeout = 0;
	
	//adzm 2010-05-12 - dsmplugin config
	m_szDSMPluginConfig[0] = '\0';
	OS_Shutdown=false;

	m_autocapt = 1;

    startTime = GetTickCount();
	m_fSendExtraMouse = TRUE;
	retryThreadHandle = NULL;
	retrysock = NULL;
	virtualDisplay = NULL;
	m_virtualDisplaySupported = VirtualDisplay::InstallDriver(false);
	if (m_virtualDisplaySupported)
		virtualDisplay = new VirtualDisplay();
	DriverWanted = FALSE;
	HookWanted = FALSE;
	DriverWantedSet = FALSE;
	m_Frame = FALSE;
	m_Notification = FALSE;
	m_OSD = FALSE;
	m_NotificationSelection = 0;
}

vncServer::~vncServer()
{	
	vnclog.Print(LL_STATE, VNCLOG("shutting down server object1\n"));

	// We don't want to retry when we are shutting down...
	m_fAutoReconnect = FALSE;
	m_fIdReconnect = FALSE;

	// If there is a socket_conn object then delete it
	if (m_socketConn != NULL)
	{
		delete m_socketConn;
		m_socketConn = NULL;
	}

	if (m_httpConn != NULL)
	{
		delete m_httpConn;
		m_httpConn = NULL;
	}

	if (retrysock != NULL)
		retrysock->Shutdown();
	//if some reconnect is running wait until timeout
	if (retryThreadHandle != NULL) {
		 WaitForSingleObject(retryThreadHandle,INFINITE);
		 CloseHandle(retryThreadHandle);
		 retryThreadHandle = NULL;
	}

	// Modif Jeremy C. 
	if(m_impersonationtoken) 
		CloseHandle(m_impersonationtoken);

	// Remove any active clients!
	KillAuthClients();
	KillUnauthClients();

	// Wait for all the clients to die
	WaitUntilAuthEmpty();
	WaitUntilUnauthEmpty();

	// Don't free the desktop until no KillClient is likely to free it
	{	omni_mutex_lock l(m_desktopLock,11);

		if (m_desktop != NULL)
		{
			delete m_desktop;
			m_desktop = NULL;
		}
	}
	while (g_Desktop_running)
	{
		Sleep(100);
		vnclog.Print(LL_STATE, VNCLOG("Waiting for desktop to shutdown\n"));
	}

	// Don't free the authhosts string until no more connections are possible
	if (m_auth_hosts != 0)
	{
		free(m_auth_hosts);
		m_auth_hosts = 0;
	}

	if (m_name != NULL)
	{
		free(m_name);
		m_name = NULL;
	}

	if (m_clientquitsig != NULL)
	{
		delete m_clientquitsig;
		m_clientquitsig = NULL;
	}

	// Modif sf@2002 - DSMPlugin handling
	if (m_pDSMPlugin != NULL)
	{
		delete(m_pDSMPlugin);
		m_pDSMPlugin=NULL;
		vnclog.Print(LL_SOCKINFO, VNCLOG("~server m_pDSMPlugin = NULL \n"));
	}

	// Free the host blacklist
	omni_mutex_lock l(m_clientsLockBlackList, 611);
	while (m_blacklist) {
		vncServer::BlacklistEntry *current = m_blacklist;
		m_blacklist=m_blacklist->_next;

		free (current->_machineName);
		delete current;
	}
	//We need to give the client thread to give some time to close
	// bad hack
	//Sleep(500);
	//sometimes crash, vnclog seems already removed
	//	vnclog.Print(LL_STATE, VNCLOG("shutting down server object(4)\n"));
	g_Server_running=false;

	if (virtualDisplay)
		delete virtualDisplay;
}

void
vncServer::ShutdownServer()
{
	vnclog.Print(LL_STATE, VNCLOG("shutting down server object2\n"));

	// We don't want to retry when we are shutting down...
	m_fAutoReconnect = FALSE;
	m_fIdReconnect = FALSE;

	// If there is a socket_conn object then delete it
	if (m_socketConn != NULL)
	{
		delete m_socketConn;
		m_socketConn = NULL;
	}

	if (m_httpConn != NULL)
	{
		delete m_httpConn;
		m_httpConn = NULL;
	}

	if (retrysock != NULL)
		retrysock->Shutdown();

	//if some reconnect is running wait until timeout
	if (retryThreadHandle != NULL) {
		 WaitForSingleObject(retryThreadHandle,INFINITE);
		 CloseHandle(retryThreadHandle);
		 retryThreadHandle = NULL;
	}

	// Modif Jeremy C. 
	if(m_impersonationtoken) 
		CloseHandle(m_impersonationtoken);

	// Remove any active clients!
	KillAuthClients();
	KillUnauthClients();

	// Wait for all the clients to die
	WaitUntilAuthEmpty();
	WaitUntilUnauthEmpty();

	// Don't free the desktop until no KillClient is likely to free it
	{	omni_mutex_lock l(m_desktopLock,12);

		if (m_desktop != NULL)
		{
			delete m_desktop;
			m_desktop = NULL;
		}
	}
	while (g_Desktop_running)
	{
		Sleep(100);
		vnclog.Print(LL_STATE, VNCLOG("Waiting for desktop to shutdown\n"));
	}

	// Don't free the authhosts string until no more connections are possible
	if (m_auth_hosts != 0)
	{
		free(m_auth_hosts);
		m_auth_hosts = 0;
	}

	if (m_name != NULL)
	{
		free(m_name);
		m_name = NULL;
	}

	if (m_clientquitsig != NULL)
	{
		delete m_clientquitsig;
		m_clientquitsig = NULL;
	}

	// Modif sf@2002 - DSMPlugin handling
	if (m_pDSMPlugin != NULL)
	{
		delete(m_pDSMPlugin);
		m_pDSMPlugin=NULL;
		vnclog.Print(LL_SOCKINFO, VNCLOG("ShutdownServer m_pDSMPlugin = NULL \n"));
	}

	// Free the host blacklist
	omni_mutex_lock l(m_clientsLockBlackList, 611);
	while (m_blacklist) {		
		vncServer::BlacklistEntry *current = m_blacklist;
		m_blacklist=m_blacklist->_next;

		free (current->_machineName);
		delete current;
	}
	//We need to give the client thread to give some time to close
	// bad hack
	//Sleep(500);
	//sometimes crash, vnclog seems already removed
//	vnclog.Print(LL_STATE, VNCLOG("shutting down server object(4)\n"));
	g_Server_running=false;
}

// Client handling functions
vncClientId vncServer::AddClient(VSocket *socket, BOOL auth, BOOL shared, BOOL outgoing)
{
	return AddClient(socket, auth, shared, 0, NULL, outgoing);
}

vncClientId vncServer::AddClient(VSocket *socket, BOOL auth, BOOL shared, rfbProtocolVersionMsg *protocolMsg, BOOL outgoing)
{
	return AddClient(socket, auth, shared,  0, protocolMsg, outgoing);
}

// adzm 2009-07-05 - repeater IDs
vncClientId vncServer::AddClient(VSocket *socket,
					 BOOL auth,
					 BOOL shared,
					 int capability,
					 rfbProtocolVersionMsg *protocolMsg, BOOL outgoing)
{
	return AddClient(socket, auth, shared,  0, protocolMsg, NULL, NULL, 0, outgoing);
}

// adzm 2009-07-05 - repeater IDs
vncClientId vncServer::AddClient(VSocket *socket,
					 BOOL auth,
					 BOOL shared,
					 int capability,
					 rfbProtocolVersionMsg *protocolMsg,
					 VString szRepeaterID,
					 VString szHost,
					 VCard port, BOOL outgoing)
{
	vnclog.Print(LL_STATE, VNCLOG("AddClient() started\n"));
	
	vncClient *client;

	omni_mutex_lock l(m_clientsLock,13);

	// Try to allocate a client id...
	vncClientId clientid = m_nextid;
	do
	{
		clientid = (clientid+1) % MAX_CLIENTS;
		if (clientid == m_nextid)
		{
			delete socket;
			return -1;
		}
	}
	while (m_clientmap[clientid] != NULL);

	// Create a new client and add it to the relevant client list
	client = new vncClient();
	if (client == NULL)
	{
		delete socket;
		return -1;
	}
	client->setTiming(GetTickCount());
	// Set the client's settings
	client->SetOutgoing(outgoing);
	client->SetProtocolVersion(protocolMsg);
	client->SetCapability(capability);
	client->EnableKeyboard(/*keysenabled &&*/ m_enable_remote_inputs);
	client->EnablePointer(/*ptrenabled &&*/ m_enable_remote_inputs);
    client->EnableJap(/*ptrenabled &&*/ m_enable_jap_input ? true : false);
	client->EnableUnicode(/*ptrenabled &&*/ m_enable_unicode_input ? true : false);

	// adzm 2009-07-05 - repeater IDs
	if (szRepeaterID) {
		client->SetRepeaterID(szRepeaterID);
	}
	// adzm 2009-08-02
	if (szHost) {
		client->SetHost(szHost);
	}
	client->SetHostPort(port);

	// Start the client
	if (!client->Init(this, socket, auth, shared, clientid))
	{
		// The client will delete the socket for us...
		vnclog.Print(LL_CONNERR, VNCLOG("failed to initialise client object\n"));
		delete client;
		return -1;
	}

	m_clientmap[clientid] = client;

	// Add the client to unauth the client list
	m_unauthClients.push_back(clientid);

	// Notify anyone interested about this event
	DoNotify(WM_SRV_CLIENT_CONNECT, 0, 0);

	vnclog.Print(LL_INTINFO, VNCLOG("AddClient() done\n"));

	// adzm 2009-07-05 - Balloon
	if (SPECIAL_SC_PROMPT) {
		vncClientList::iterator i;
		char szInfo[256];
		strcpy_s(szInfo, "Waiting for connection... ");

		for (i = m_unauthClients.begin(); i != m_unauthClients.end(); i++)
		{


			vncClient* pclient = GetClient(*i);
			if (pclient->GetRepeaterID() && (strlen(pclient->GetRepeaterID()) > 0) ) {
				strncat_s(szInfo, 255, pclient->GetRepeaterID(), _TRUNCATE);
			} else {
				strncat_s(szInfo, 255, pclient->GetClientName(), _TRUNCATE);
			}			
			
			// adzm 2009-07-05			
			strncat_s(szInfo, 255, ", ", _TRUNCATE);
		}

		if (m_unauthClients.size() > 0) {
			szInfo[strlen(szInfo) - 2] = '\0';
			vncMenu::NotifyBalloon(szInfo);
		}		
	}

	return clientid;
}

char *vncDeskThreadError(DWORD code)
{
    // create default message
    static char msg[255];
    _snprintf_s(msg, sizeof msg, "Unknown error %u", static_cast<unsigned int>(code));
    msg[sizeof msg - 1] = 0;

    switch (code)
    {
        case ERROR_DESKTOP_NO_PALETTE:
            return "Unable to determine color palette";
        case ERROR_DESKTOP_INIT_FAILED:
            return "Unable to select input desktop";
        case ERROR_DESKTOP_UNSUPPORTED_PIXEL_ORGANIZATION:
            return "Incompatible graphics device driver (planar pixel format)";
        case ERROR_DESKTOP_UNSUPPORTED_PIXEL_FORMAT:
            return "Unsupported true color pixel format";
        case ERROR_DESKTOP_NO_HOOKWINDOW:
            return "Unable to create hook window";
        case ERROR_DESKTOP_NO_ROOTDC:
        	return "Unable to create compatible device context";
        case ERROR_DESKTOP_NO_BITBLT:
        	return "Unsupported graphics device driver (no BitBlt)";
        case ERROR_DESKTOP_NO_GETDIBITS:
        	return "Unsupported graphics device driver (no GetDIBits)";
		case ERROR_DESKTOP_NO_COMPATBITMAP:
			return "Unable to create compatible bitmap";
		case ERROR_DESKTOP_NO_DISPLAYFORMAT:
			return 	"Unable to get Display format";
        case ERROR_DESKTOP_OUT_OF_MEMORY:
            return "Out of memory";
        case ERROR_DESKTOP_NO_DESKTOPTHREAD:
            return "Unable to create desktop hook thread";
		case ERROR_DESKTOP_NO_DIBSECTION_OR_COMPATBITMAP:
			return "Unable to create device independent bitmap or fallback bitmap";
        default:
            break;
    }

    return msg;
}
BOOL
vncServer::Authenticated(vncClientId clientid)
{
	vncClientList::iterator i;
	BOOL authok = TRUE;
//	vnclog.Print(LL_INTINFO, VNCLOG("Lock2\n"));
	omni_mutex_lock l1(m_desktopLock,14);
//	vnclog.Print(LL_INTINFO, VNCLOG("Lock3\n"));
	omni_mutex_lock l2(m_clientsLock,15);

	vncClient *client = NULL;

	// Search the unauthenticated client list
	for (i = m_unauthClients.begin(); i != m_unauthClients.end(); i++)
	{
		// Is this the right client?
		if ((*i) == clientid)
		{
			client = GetClient(clientid);
			

			// Yes, so remove the client and add it to the auth list
			m_unauthClients.erase(i);

			// Add the client to the auth list
			m_authClients.push_back(clientid);

			//If we are the only client, we give it mouse access.
			//Other client can take access when the click a mouse button
			if (getCollabo()) {
				if (m_authClients.size() == 1)
					client->SetHasMouse(true);
				else
					client->SetHasMouse(false);
			}


			// Create the screen handler if necessary
			if (m_desktop == NULL)
			{
				m_desktop = new vncDesktop();

				if (m_desktop == NULL)
				{
					client->Kill();
					authok = FALSE;
					//m_authClients.erase(i);
					break;
				}
				// Preset toggle prim/sec/both
				// change, to get it final stable, we only gonna handle single and multi monitors
				// 1=single monitor, 2 is multi monitor
				m_desktop->m_buffer.SetAllMonitors(false);
				if (Secondary()) 
					m_desktop->m_buffer.SetAllMonitors(true);


                DWORD startup_status = 0;
				if ((startup_status = m_desktop->Init(this)) != 0)
				{
					vnclog.Print(LL_INTINFO, VNCLOG("Desktop init failed, unlock in application mode ? \n"));
					client->Kill();
					authok = FALSE;
					delete m_desktop;
					m_desktop = NULL;
					//m_authClients.erase(i);
					break;
				}
			}

			// Tell the client about this new buffer
			client->SetBuffer(&(m_desktop->m_buffer));

			break;
		}
	}

	// Notify anyone interested of this event
	DoNotify(WM_SRV_CLIENT_AUTHENTICATED, 0, 0);

	if (client != NULL) {
		// adzm 2009-07-05 - Balloon
		if (SPECIAL_SC_PROMPT) {
			char szInfo[256] = { 0 };
				if (client->GetRepeaterID() && (strlen(client->GetRepeaterID()) > 0)) {
					_snprintf_s(szInfo, 255, "UltraVNC is controling your device. \r Remote access from ID: %s", client->GetRepeaterID());
				}
				else {
					_snprintf_s(szInfo, 255, "UltraVNC is controling your device. \r Remote access from ip address %s", client->GetClientName());
				}
			vncMenu::NotifyBalloon(szInfo);
		}

		if (m_Notification) {

			if (strlen(client->infoMsg) > 0) {
				char szInfo[256] = { 0 };
				char szTitle[63] = { 0 };
				if (client->GetRepeaterID() && (strlen(client->GetRepeaterID()) > 0)) {
					_snprintf_s(szTitle, 255, "Connection from: %s", client->GetRepeaterID());
				}
				else {
					_snprintf_s(szTitle, 255, "Connection from: %s", client->GetClientName());
				}
				strcpy_s(szInfo, 255, client->infoMsg);
				vncMenu::NotifyBalloon(szInfo, szTitle);
			}
			else if (m_NotificationSelection == 0) {
				char szInfo[256] = { 0 };
				if (client->GetRepeaterID() && (strlen(client->GetRepeaterID()) > 0)) {
					_snprintf_s(szInfo, 255, "UltraVNC is controling your device. \r Remote access from ID: %s", client->GetRepeaterID());
				}
				else {
					_snprintf_s(szInfo, 255, "UltraVNC is controling your device. \r Remote access from ip address %s", client->GetClientName());
				}
				vncMenu::NotifyBalloon(szInfo);
			}

		}
	}

	vnclog.Print(LL_INTINFO, VNCLOG("Authenticated() done\n"));

	return authok;
}

void
vncServer::KillClient(vncClientId clientid)
{
	vncClientList::iterator i;
	BOOL done = FALSE;

	omni_mutex_lock l(m_clientsLock,16);

	// Find the client in one of the two lists
	for (i = m_unauthClients.begin(); i != m_unauthClients.end(); i++)
	{
		// Is this the right client?
		if ((*i) == clientid)
		{
			vnclog.Print(LL_INTINFO, VNCLOG("killing unauth client\n"));

			// Ask the client to die
			vncClient *client = GetClient(clientid);
			client->Kill(true);
			delete client; 
			done = TRUE;
			break;
		}
	}
	if (!done)
	{
		for (i = m_authClients.begin(); i != m_authClients.end(); i++)
		{
			// Is this the right client?
			if ((*i) == clientid)
			{
				vnclog.Print(LL_INTINFO, VNCLOG("killing auth client\n"));

				// Yes, so kill it
				vncClient *client = GetClient(clientid);
				client->Kill();

				done = TRUE;
				break;
			}
		}
	}

	vnclog.Print(LL_INTINFO, VNCLOG("KillClient() done\n"));
}

//
// sf@2002 - Kill the client which has the passed name
//
void vncServer::KillClient(LPSTR szClientName)
{
	vncClientList::iterator i;
	omni_mutex_lock l(m_clientsLock,17);
	vncClient *pClient = NULL;

	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		pClient = GetClient(*i);
		if (!_stricmp(pClient->GetClientName(), szClientName))
		{
			vnclog.Print(LL_INTINFO, VNCLOG("Killing client named: %s\n"), szClientName);
			pClient->Kill();
		}
	}
	vnclog.Print(LL_INTINFO, VNCLOG("KillClient() from name done\n"));
}


//
// sf@2002 - Open a textchat window with the named client
//
void vncServer::TextChatClient(LPSTR szClientName)
{
	vncClientList::iterator i;
	omni_mutex_lock l(m_clientsLock,18);
	vncClient *pClient = NULL;

	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		pClient = GetClient(*i);
		if (!_stricmp(pClient->GetClientName(), szClientName))
		{
			if (!pClient->IsUltraViewer())
			{
				vnclog.Print(LL_INTINFO, VNCLOG("Client %s is not Ultra. Doesn't know TextChat\n"), szClientName);
				vncTimedMsgBox::Do(
									sz_ID_ULTRAVNC_TEXTCHAT,
									sz_ID_ULTRAVNC_WARNING,
									MB_ICONINFORMATION | MB_OK
									);
				break;
			}
			vnclog.Print(LL_INTINFO, VNCLOG("TextChat with client named: %s\n"), szClientName);
			pClient->GetTextChatPointer()->OrderTextChat();
			break;
		}
	}
	vnclog.Print(LL_INTINFO, VNCLOG("KillClient() from name done\n"));
}

bool vncServer::IsUltraVNCViewer()
{
	vncClientList::iterator i;
	omni_mutex_lock l(m_clientsLock,19);
	vncClient *pClient = NULL;

	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{	
		if (GetClient(*i)->IsUltraViewer())return true;
	}
	return false;
}

bool vncServer::AreThereMultipleViewers()
{
	if (m_authClients.size()<=1) return false;
	else return true;
}

bool vncServer::singleExtendRequested()
{
	vncClientList::iterator i;
	vncClient* pClient = NULL;

	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		if (GetClient(*i)->singleExtendRequested())return true;
	}
	return false;
}


void
vncServer::KillAuthClients()
{
	vncClientList::iterator i;
	omni_mutex_lock l(m_clientsLock,21);

	// Tell all the authorised clients to die!
	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		vnclog.Print(LL_INTINFO, VNCLOG("killing auth client\n"));

		// Kill the client
		GetClient(*i)->Kill();
	}
	//autoreconnect and service
	//if (m_fRunningFromExternalService) fShutdownOrdered=true;
	vnclog.Print(LL_INTINFO, VNCLOG("KillAuthClients() done\n"));
}

//
// sf@2002 - Fill the passed ListBox with clients names
//
void vncServer::ListAuthClients(HWND hListBox)
{
	vncClientList::iterator i;
	omni_mutex_lock l(m_clientsLock,22);

	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		// adzm 2009-07-05
		vncClient* client = GetClient(*i);
		if (client->GetRepeaterID() && (strlen(client->GetRepeaterID()) > 0) ) {
			char szDescription[256];
			_snprintf_s(szDescription, 255, "%s - %s", client->GetRepeaterID(), client->GetClientName());
			szDescription[255] = '\0';

			SendMessage(hListBox, 
						LB_ADDSTRING,
						0,
						(LPARAM) szDescription
						);
		} else {
			SendMessage(hListBox, 
						LB_ADDSTRING,
						0,
						(LPARAM) client->GetClientName()
						);
		}
	}
}

// adzm 2009-07-05
void vncServer::ListUnauthClients(HWND hListBox)
{
	vncClientList::iterator i;
	omni_mutex_lock l(m_clientsLock,23);

	for (i = m_unauthClients.begin(); i != m_unauthClients.end(); i++)
	{
		// adzm 2009-07-05
		vncClient* client = GetClient(*i);
		if (client->GetRepeaterID() && (strlen(client->GetRepeaterID()) > 0) ) {
			char szDescription[256];
			_snprintf_s(szDescription, 255, "%s - %s", client->GetRepeaterID(), client->GetClientName());
			szDescription[255] = '\0';

			SendMessage(hListBox, 
						LB_ADDSTRING,
						0,
						(LPARAM) szDescription
						);
		} else {
			SendMessage(hListBox, 
						LB_ADDSTRING,
						0,
						(LPARAM) client->GetClientName()
						);
		}
	}
}


//
// sf@2002 - test if there's a slow client connected
// The criteria is a client that has been using a slow
// encoding for more than 10 seconds after its connection
//
bool vncServer::IsThereASlowClient()
{
	vncClientList::iterator i;
	bool fFound = false;
	omni_mutex_lock l(m_clientsLock,24);

	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		if (GetClient(*i)->IsSlowEncoding())
		{
			if (GetTimeFunction() - GetClient(*i)->GetConnectTime() > 10000) 
			{
				fFound = true;
				break;
			}	
			else
				continue;
		}
		else
			continue;
	}
	return fFound;
}

bool vncServer::IsThereAUltraEncodingClient()
{
	vncClientList::iterator i;
	bool fFound = false;
	omni_mutex_lock l(m_clientsLock,25);

	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		if (GetClient(*i)->IsUltraEncoding())
		{
			return true;
		}
	}
	return false;
}

bool vncServer::IsThereAUltra2EncodingClient()
{
	vncClientList::iterator i;
	bool fFound = false;
	omni_mutex_lock l(m_clientsLock,25);

	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		if (GetClient(*i)->IsUltra2Encoding())
		{
			return true;
		}
	}
	return false;
}

bool vncServer::IsEncoderSet()
{
	vncClientList::iterator i;
	bool fFound = false;
	omni_mutex_lock l(m_clientsLock, 25);

	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		if (GetClient(*i)->IsEncoderSet())
		{
			return true;
		}
	}
	return false;
}


bool vncServer::IsThereFileTransBusy()
{
	vncClientList::iterator i;
	bool fFound = false;
	omni_mutex_lock l(m_clientsLock,26);

	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		if (GetClient(*i)->IsFileTransBusy())
		{
			return true;
		}
	}
	return false;
}


void
vncServer::KillUnauthClients()
{
	vncClientList::iterator i;
	omni_mutex_lock l(m_clientsLock,27);

	// Tell all the authorised clients to die!
	for (i = m_unauthClients.begin(); i != m_unauthClients.end(); i++)
	{
		vnclog.Print(LL_INTINFO, VNCLOG("killing unauth client\n"));

		// Kill the client
		GetClient(*i)->Kill(true);
		delete GetClient(*i);
	}

	vnclog.Print(LL_INTINFO, VNCLOG("KillUnauthClients() done\n"));
}


UINT
vncServer::AuthClientCount()
{
	//omni_mutex_lock l(m_clientsLock,28);

	return (UINT)m_authClients.size();
}

UINT
vncServer::UnauthClientCount()
{
	//omni_mutex_lock l(m_clientsLock,30);

	return (UINT)m_unauthClients.size();
}

BOOL
vncServer::UpdateWanted()
{
	vncClientList::iterator i;
	omni_mutex_lock l(m_clientsLock,29);

	// Iterate over the authorised clients
	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		if (GetClient(*i)->UpdateWanted())
			return TRUE;
	}
	return FALSE;
}

BOOL
vncServer::RemoteEventReceived()
{
	vncClientList::iterator i;
	BOOL result = FALSE;
	omni_mutex_lock l(m_clientsLock,31);

	// Iterate over the authorised clients
	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		result = result || GetClient(*i)->RemoteEventReceived();
	}
	return result;
}

void
vncServer::WaitUntilAuthEmpty()
{
	omni_mutex_lock l(m_clientsLock,32);

	// Wait for all the clients to exit
	while (!m_authClients.empty())
	{
		// Wait for a client to quit
		m_clientquitsig->wait();
	}
}

void
vncServer::WaitUntilUnauthEmpty()
{
	omni_mutex_lock l(m_clientsLock,33);

	// Wait for all the clients to exit
	while (!m_unauthClients.empty())
	{
		// Wait for a client to quit
		m_clientquitsig->wait();
	}
}

// Client info retrieval/setup
vncClient*
vncServer::GetClient(vncClientId clientid)
{
	if ((clientid >= 0) && (clientid < MAX_CLIENTS))
		return m_clientmap[clientid];
	return NULL;
}

vncClientList
vncServer::ClientList()
{
	vncClientList clients;

	omni_mutex_lock l(m_clientsLock,34);

	clients = m_authClients;

	return clients;
}

void
vncServer::SetCapability(vncClientId clientid, int capability)
{
	omni_mutex_lock l(m_clientsLock,35);

	vncClient *client = GetClient(clientid);
	if (client != NULL)
		client->SetCapability(capability);
}

int
vncServer::GetCapability(vncClientId clientid)
{
	omni_mutex_lock l(m_clientsLock,38);

	vncClient *client = GetClient(clientid);
	if (client != NULL)
		return client->GetCapability();
	return 0;
}

const char*
vncServer::GetClientName(vncClientId clientid)
{
	omni_mutex_lock l(m_clientsLock,39);

	vncClient *client = GetClient(clientid);
	if (client != NULL)
		return client->GetClientName();
	return NULL;
}

// RemoveClient should ONLY EVER be used by the client to remove itself.
void
vncServer::RemoveClient(vncClientId clientid)
{
	vncClientList::iterator i;
	BOOL done = FALSE;
//	vnclog.Print(LL_INTINFO, VNCLOG("Lock1\n"));
	omni_mutex_lock l1(m_desktopLock,40);
//	vnclog.Print(LL_INTINFO, VNCLOG("Lock3\n"));
	{	omni_mutex_lock l2(m_clientsLock,41);

		// Find the client in one of the two lists
		for (i = m_unauthClients.begin(); i != m_unauthClients.end(); i++)
		{
			// Is this the right client?
			if ((*i) == clientid)
			{
				vnclog.Print(LL_INTINFO, VNCLOG("removing unauthorised client\n"));

				// Yes, so remove the client and kill it
				m_unauthClients.erase(i);
				if ( clientid>=0 && clientid< 512) m_clientmap[clientid] = NULL;
				done = TRUE;
				break;
			}
		}
		if (!done)
		{
			for (i = m_authClients.begin(); i != m_authClients.end(); i++)
			{
				// Is this the right client?
				if ((*i) == clientid)
				{
					vnclog.Print(LL_INTINFO, VNCLOG("removing authorised client\n"));

					// Yes, so remove the client and kill it
					m_authClients.erase(i);
					if ( clientid>=0 && clientid< 512) m_clientmap[clientid] = NULL;

					done = TRUE;
					break;
				}
			}
		}

		// Signal that a client has quit
		m_clientquitsig->signal();

	} // Unlock the clientLock

	// Are there any authorised clients connected?
	if (m_authClients.empty() && (m_desktop != NULL))
	{
		vnclog.Print(LL_STATE, VNCLOG("deleting desktop server\n"));

		// sf@2007 - Do not lock/logoff even if required when WinVNC autorestarts (on desktop change (XP FUS / Vista))
		if (!AutoRestartFlag() && !OS_Shutdown)
		{
			// Are there locksettings set?
			if (LockSettings() == 1 || clearconsole)
			{
				// Yes - lock the machine on disconnect!
				vncService::LockWorkstation();
			} 
			else if (LockSettings() > 1)
			{
				char username[UNLEN+1];

				vncService::CurrentUser((char *)&username, sizeof(username));
				if (strcmp(username, "") != 0)
				{
					// Yes - force a user logoff on disconnect!
					if (!ExitWindowsEx(EWX_LOGOFF, 0))
						vnclog.Print(LL_CONNERR, VNCLOG("client disconnect - failed to logoff user!\n"));
				}
			}
			if (vncService::RunningAsService() && m_Rdpmode) {
				fShutdownOrdered = true;
				vnclog.Print(LL_CONNERR, VNCLOG("last client disconnect - restart server for rdpmode\n"));

			}
		}

		// Delete the screen server
		delete m_desktop;
		m_desktop = NULL;
		vnclog.Print(LL_STATE, VNCLOG("desktop deleted\n"));
	}

	// Notify anyone interested of the change
	DoNotify(WM_SRV_CLIENT_DISCONNECT, 0, 0);

	vnclog.Print(LL_INTINFO, VNCLOG("RemoveClient() done\n"));
}

// NOTIFICATION HANDLING!

// Connect/disconnect notification
BOOL
vncServer::AddNotify(HWND hwnd)
{
	omni_mutex_lock l(m_clientsLock_notifyList, 44);

	// Add the window handle to the list
	m_notifyList.push_front(hwnd);

	return TRUE;
}

BOOL
vncServer::RemNotify(HWND hwnd)
{
	omni_mutex_lock l(m_clientsLock_notifyList, 45);

	// Remove the window handle from the list
	vncNotifyList::iterator i;
	for (i=m_notifyList.begin(); i!=m_notifyList.end(); i++)
	{
		if ((*i) == hwnd)
		{
			// Found the handle, so remove it
			m_notifyList.erase(i);
			return TRUE;
		}
	}

	return FALSE;
}

// Send a notification message
void
vncServer::DoNotify(UINT message, WPARAM wparam, LPARAM lparam)
{
	omni_mutex_lock l(m_clientsLock_notifyList, 46);

	// Send the given message to all the notification windows
	vncNotifyList::iterator i;
	for (i=m_notifyList.begin(); i!=m_notifyList.end(); i++)
	{
		PostMessage((*i), message, wparam, lparam);
	}
}

void
vncServer::UpdateMouse()
{
	vncClientList::iterator i;
	
	omni_mutex_lock l(m_clientsLock,47);

	// Post this mouse update to all the connected clients
	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		// Post the update
		GetClient(*i)->UpdateMouse();
	}
}


// adzm - 2010-07 - Extended clipboard
void
vncServer::UpdateClipTextEx(HWND hwndOwner, vncClient* excludeClient)
{
	ClipboardData clipboardData;
	if (clipboardData.Load(hwndOwner)) {
		vncClientList::iterator i;
		
		omni_mutex_lock l(m_clientsLock,48);

		// Post this update to all the connected clients
		for (i = m_authClients.begin(); i != m_authClients.end(); i++)
		{
			// Post the update
			vncClient* client = GetClient(*i);
			if (client != excludeClient) {
				client->UpdateClipTextEx(clipboardData);
			}
		}
	}
}

void
vncServer::UpdateCursorShape()
{
	vncClientList::iterator i;
	
	omni_mutex_lock l(m_clientsLock,49);

	// Post this update to all the connected clients
	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		// Post the update
		GetClient(*i)->UpdateCursorShape();
	}
}

void
vncServer::UpdatePalette(bool lock)
{
	vncClientList::iterator i;
	
	omni_mutex_lock l(m_clientsLock,50);

	// Post this update to all the connected clients
	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		// Post the update
		GetClient(*i)->UpdatePalette(lock);
	}
}

void
vncServer::UpdateLocalFormat(bool lock)
{
	vncClientList::iterator i;
	
	omni_mutex_lock l(m_clientsLock,51);

	// Post this update to all the connected clients
	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		// Post the update
		GetClient(*i)->UpdateLocalFormat(lock);
	}
}

void
vncServer::UpdateLocalClipText(LPSTR text)
{
//	vnclog.Print(LL_INTINFO, VNCLOG("Lock5\n"));
	omni_mutex_lock l(m_desktopLock,52);

	if (m_desktop != NULL)
		m_desktop->SetClipText(text);
}


// adzm - 2010-07 - Extended clipboard
void
vncServer::UpdateLocalClipTextEx(ExtendedClipboardDataMessage& extendedClipboardDataMessage, vncClient* sourceClient)
{
//	vnclog.Print(LL_INTINFO, VNCLOG("Lock5\n"));
	{
		omni_mutex_lock l(m_desktopLock,53);

		if (m_desktop != NULL)
			m_desktop->SetClipTextEx(extendedClipboardDataMessage);
	}
}

// Name and port number handling
void
vncServer::SetName(const char * name)
{
	// Set the name of the desktop
	if (m_name != NULL)
	{
		free(m_name);
		m_name = NULL;
	}
	
	m_name = _strdup(name);
}

// TightVNC 1.2.7
void
vncServer::SetPorts(const UINT port_rfb, const UINT port_http)
{
	if (m_port != port_rfb || m_port_http != port_http) {
		// Set port numbers to use
		m_port = port_rfb;
		m_port_http = port_http;

		// If there is already a listening socket then close and re-open it...
		BOOL socketon = SockConnected();
		SockConnect(FALSE);
		if (socketon)
			SockConnect(TRUE);
    }
}

// RealVNC method
/*
void
vncServer::SetPort(const UINT port)
{
    if (m_port != port)
    {
	/////////////////////////////////
	// Adjust the listen socket

	// Set the port number to use
	m_port = port;

	// If there is already a listening socket then close and re-open it...
	BOOL socketon = SockConnected();

	SockConnect(FALSE);
	if (socketon)
	    SockConnect(TRUE);

    }
}


UINT
vncServer::GetPort()
{
	return m_port;
}
*/

void
vncServer::SetPassword(const char *passwd)
{
	memcpy(m_password, passwd, MAXPWLEN);
}

void
vncServer::GetPassword(char *passwd)
{
	memcpy(passwd, m_password, MAXPWLEN);
}

void //PGM
vncServer::SetPassword2(const char *passwd2) //PGM
{ //PGM
	memcpy(m_password2, passwd2, MAXPWLEN); //PGM
} //PGM

void //PGM
vncServer::GetPassword2(char *passwd2) //PGM
{ //PGM
	memcpy(passwd2, m_password2, MAXPWLEN); //PGM
} //PGM

// Remote input handling
void
vncServer::EnableRemoteInputs(BOOL enable)
{
	m_enable_remote_inputs = enable;
	vncClientList::iterator i;
		
	omni_mutex_lock l(m_clientsLock,54);

	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		GetClient(*i)->EnableKeyboard(m_enable_remote_inputs);
		GetClient(*i)->EnablePointer(m_enable_remote_inputs);
	}
}

BOOL vncServer::RemoteInputsEnabled()
{
	return m_enable_remote_inputs;
}

// Local input handling
void
vncServer::DisableLocalInputs(BOOL disable)
{
	m_disable_local_inputs = disable;
}

bool vncServer::LocalInputsDisabled()
{
    return m_disable_local_inputs ? true : false;
}

BOOL vncServer::JapInputEnabled()
{
	return m_enable_jap_input;
}

BOOL vncServer::UnicodeInputEnabled()
{
	return m_enable_unicode_input;
}

BOOL vncServer::Win8HelperEnabled()
{
	return m_enable_win8helper;
}
void vncServer:: Win8HelperEnabled(BOOL enable)
{
	m_enable_win8helper = enable;
}


void
vncServer::EnableJapInput(BOOL enable)
{
	m_enable_jap_input = enable;
	vncClientList::iterator i;
		
	omni_mutex_lock l(m_clientsLock,55);

	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
        GetClient(*i)->EnableJap(m_enable_jap_input ? true : false);
	}
}

void
vncServer::EnableUnicodeInput(BOOL enable)
{
	m_enable_unicode_input = enable;
	vncClientList::iterator i;
		
	omni_mutex_lock l(m_clientsLock,55);

	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
        GetClient(*i)->EnableUnicode(m_enable_unicode_input ? true : false);
	}
}

void
vncServer::Clearconsole(BOOL enable)
{
	clearconsole=(enable != FALSE);
}



void 
vncServer::KillSockConnect()
{
	if (m_socketConn != NULL)
		delete m_socketConn;
	m_socketConn = NULL;

}

// Socket connection handling
BOOL
vncServer::SockConnect(BOOL On)
{
	// Are we being asked to switch socket connects on or off?
	vnclog.Print(LL_SOCKINFO, VNCLOG("SockConnect %d\n"), On);
	if (On)
	{
		// Is there a listening socket?
		if (m_socketConn == NULL)
		{
			m_socketConn = new vncSockConnect();
			if (m_socketConn == NULL)
				return FALSE;

			// Are we to use automatic port selection?
			if (m_autoportselect)
			{
				BOOL ok = FALSE;

				// Yes, so cycle through the ports, looking for a free one!
				for (int i=0; i < 99; i++)
				{
					m_port = DISPLAY_TO_PORT(i);
					m_port_http = DISPLAY_TO_HPORT(i);

					vnclog.Print(LL_CLIENTS, VNCLOG("trying port number %d\n"), m_port);

					// Attempt to connect to the port
					VSocket tempsock;
#ifdef IPV6V4
					if (!tempsock.CreateConnect("localhost", m_port))
#else
					if (tempsock.Create())
					{
						if (!tempsock.Connect("localhost", m_port))
#endif
						{
							// Couldn't connect, so this port is probably usable!
							if (m_socketConn->Init(this, m_port))
							{
								ok = TRUE;
								break;
							}
						}
#ifdef IPV6V4
#else
					}
#endif

				}

				if (!ok)
				{
					delete m_socketConn;
					m_socketConn = NULL;
					return FALSE;
				}
			} else
			{
				// No autoportselect
				if (!m_socketConn->Init(this, m_port))
				{
					delete m_socketConn;
					m_socketConn = NULL;
					return FALSE;
				}
			}

			// Now let's start the HTTP connection stuff
			EnableHTTPConnect(m_enableHttpConn);
			vnclog.Print(LL_SOCKINFO, VNCLOG("SockConnect  Done %d\n"), On);
		}
	}
	else
	{

		// Is there a listening socket?
		if (m_socketConn != NULL)
		{

			// *** JNW - Trying to fix up a lock-up when the listening socket closes
			vnclog.Print(LL_INTINFO, VNCLOG("KillAuthClients() fix up a lock-up \n"));
			KillAuthClients();
			KillUnauthClients();
			WaitUntilAuthEmpty();
			WaitUntilUnauthEmpty();

			// Close the socket
			delete m_socketConn;
			m_socketConn = NULL;
		}

		// Is there an HTTP socket active?
		EnableHTTPConnect(m_enableHttpConn);
	}

	return TRUE;
}

BOOL
vncServer::SockConnected()
{
	return m_socketConn != NULL;
}

BOOL
vncServer::EnableHTTPConnect(BOOL enable)
{
	m_enableHttpConn = enable;
	if (enable && m_socketConn)
	{
		if (m_httpConn == NULL)
		{
			m_httpConn = new vncHTTPConnect;
			if (m_httpConn != NULL)
			{
				// Start up the HTTP server
				// TODO: allow applet params handling like in TightVNC 1.2.7
				if (!m_httpConn->Init(this,
									// PORT_TO_DISPLAY(m_port) + HTTP_PORT_OFFSET)
									m_port_http)
					)
				{
					delete m_httpConn;
					m_httpConn = NULL;
					return FALSE;
				}
			}
		}
	}
	else
	{
		if (m_httpConn != NULL)
		{
			// Close the socket
			delete m_httpConn;
			m_httpConn = NULL;
		}
	}

	return TRUE;
}

BOOL
vncServer::SetLoopbackOnly(BOOL loopbackOnly)
{
	if (loopbackOnly != m_loopbackOnly)
	{
		m_loopbackOnly = loopbackOnly;
		BOOL socketConn = SockConnected();
		SockConnect(FALSE);
		SockConnect(socketConn);
	}
	return TRUE;
}

BOOL
vncServer::LoopbackOnly()
{
	return m_loopbackOnly;
}

void vncServer::SetSendExtraMouse(BOOL i_fSendExtraMouse)
{
	m_fSendExtraMouse = i_fSendExtraMouse;
}

BOOL vncServer::SendExtraMouse()
{
	return m_fSendExtraMouse;
}

void
vncServer::GetScreenInfo(int &width, int &height, int &depth)
{
	rfbServerInitMsg scrinfo;
//	vnclog.Print(LL_INTINFO, VNCLOG("Lock6\n"));
	omni_mutex_lock l(m_desktopLock,56);

	//vnclog.Print(LL_INTINFO, VNCLOG("GetScreenInfo called\n"));

	// Is a desktop object currently active?
	// No, so create a dummy desktop and interrogate it
	if (m_desktop == NULL)
	{
		HDC				hrootdc;
		hrootdc = GetDC(NULL);
		if (hrootdc == NULL) {
				vnclog.Print(LL_INTERR, VNCLOG("Failed rootdc \n"));
				scrinfo.framebufferWidth = 0;
				scrinfo.framebufferHeight = 0;
				scrinfo.format.bitsPerPixel = 0;
		}
		else
		{
			scrinfo.framebufferWidth = GetDeviceCaps(hrootdc, HORZRES);
			scrinfo.framebufferHeight = GetDeviceCaps(hrootdc, VERTRES);
			HBITMAP membitmap = CreateCompatibleBitmap(hrootdc, scrinfo.framebufferWidth, scrinfo.framebufferHeight);
			if (membitmap == NULL) {
				scrinfo.framebufferWidth = 0;
				scrinfo.framebufferHeight = 0;
				scrinfo.format.bitsPerPixel = 0;
				}
			else
			{
				struct _BMInfo {
				BOOL			truecolour;
				BITMAPINFO		bmi;
				// Colormap info - comes straight after BITMAPINFO - **HACK**
				RGBQUAD			cmap[256];
				} m_bminfo;
				int result;
				HDC				hmemdc;
				memset(&m_bminfo, 0, sizeof(m_bminfo));
				m_bminfo.bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
				m_bminfo.bmi.bmiHeader.biBitCount = 0;
				hmemdc = CreateCompatibleDC(hrootdc);
				if (hmemdc == NULL) 
					{
						scrinfo.framebufferWidth = 0;
						scrinfo.framebufferHeight = 0;
						scrinfo.format.bitsPerPixel = 0;
					}
				else
					{
						result = ::GetDIBits(hmemdc, membitmap, 0, 1, NULL, &m_bminfo.bmi, DIB_RGB_COLORS);
						if (result == 0) 
							{
											scrinfo.framebufferWidth = 0;
											scrinfo.framebufferHeight = 0;
											scrinfo.format.bitsPerPixel = 0;
							}
						else
							{
								result = ::GetDIBits(hmemdc, membitmap,  0, 1, NULL, &m_bminfo.bmi, DIB_RGB_COLORS);
								if (result == 0) 
									{
										scrinfo.framebufferWidth = 0;
										scrinfo.framebufferHeight = 0;
										scrinfo.format.bitsPerPixel = 0;
									}
								else
									{
										scrinfo.format.bitsPerPixel = (CARD8)(m_bminfo.bmi.bmiHeader.biBitCount);
										if (scrinfo.format.bitsPerPixel==24) scrinfo.format.bitsPerPixel=32;
									}
							}//result
					if (hmemdc != NULL) DeleteDC(hmemdc);
					}//memdc
				if (membitmap != NULL) DeleteObject(membitmap);
			}//membitmap
			if (hrootdc != NULL) ReleaseDC(NULL, hrootdc);
		}//rootdc
	}
	else
	{
		m_desktop->FillDisplayInfo(&scrinfo);
	}

	// Get the info from the scrinfo structure
	width = scrinfo.framebufferWidth;
	height = scrinfo.framebufferHeight;
	depth = scrinfo.format.bitsPerPixel;
}

void
vncServer::SetAuthHosts(const char*hostlist) {
	omni_mutex_lock l(m_clientsLock,57);

	if (hostlist == 0) {
		vnclog.Print(LL_INTINFO, VNCLOG("authhosts cleared\n"));
		m_auth_hosts = 0;
		return;
	}

	//vnclog.Print(LL_INTINFO, VNCLOG("authhosts set to \"%s\"\n"), hostlist);
	if (m_auth_hosts != 0)
		free(m_auth_hosts);

	m_auth_hosts = _strdup(hostlist);
}

char*
vncServer::AuthHosts() {
	omni_mutex_lock l(m_clientsLock,58);

	if (m_auth_hosts == 0)
		return _strdup("");
	else
		return _strdup(m_auth_hosts);
}

inline BOOL
MatchStringToTemplate(const char *addr, UINT addrlen,
				      const char *filtstr, UINT filtlen) {
	if (filtlen == 0)
		return 1;
	if (addrlen < filtlen)
		return 0;
	for (UINT x = 0; x < filtlen; x++) {
		if (addr[x] != filtstr[x])
			return 0;
	}
	if ((addrlen > filtlen) && (addr[filtlen] != '.'))
		return 0;
	return 1;
}

vncServer::AcceptQueryReject
vncServer::VerifyHost(const char *hostname) {
	omni_mutex_lock l(m_clientsLockBlackList, 59);

	// -=- Is the specified host blacklisted?
	vncServer::BlacklistEntry	*current = m_blacklist;
	vncServer::BlacklistEntry	*previous = 0;
	SYSTEMTIME					systime;
	FILETIME					ftime;
	LARGE_INTEGER				now;

	// Get the current time as a 64-bit value
	GetSystemTime(&systime);
	SystemTimeToFileTime(&systime, &ftime);
	now.LowPart=ftime.dwLowDateTime;now.HighPart=ftime.dwHighDateTime;
	now.QuadPart /= 10000000; // Convert it into seconds

	while (current) {

		// Has the blacklist entry timed out?
		if ((now.QuadPart - current->_lastRefTime.QuadPart) > 0) {

			// Yes.  Is it a "blocked" entry?
			if (current->_blocked) {
				// Yes, so unblock it & re-set the reference time
				current->_blocked = FALSE;
				current->_lastRefTime.QuadPart = now.QuadPart + 10;
			} else {
				// No, so remove it
				if (previous)
					previous->_next = current->_next;
				else
					m_blacklist = current->_next;
				vncServer::BlacklistEntry *next = current->_next;
				free(current->_machineName);
				delete current;
				current = next;
				continue;
			}

		}

		// Is this the entry we're interested in?
		if ((_stricmp(current->_machineName, hostname) == 0) &&
			(current->_blocked)) {
			// Machine is blocked, so just reject it
    		vnclog.Print(LL_CONNERR, VNCLOG("client %s rejected due to blacklist entry\n"), hostname);

			return vncServer::aqrReject;
		}

		previous = current;
		current = current->_next;
	}

	// Has a hostname been specified?
	if (hostname == 0) {
		vnclog.Print(LL_INTWARN, VNCLOG("verify failed - null hostname\n"));
		return vncServer::aqrReject;
	}

	// Set the state machine into the correct mode & process the filter
	enum vh_Mode {vh_ExpectDelimiter, vh_ExpectIncludeExclude, vh_ExpectPattern};
	vh_Mode machineMode = vh_ExpectIncludeExclude;
	
	vncServer::AcceptQueryReject verifiedHost = vncServer::aqrAccept;

	vncServer::AcceptQueryReject patternType = vncServer::aqrReject;
	UINT authHostsPos = 0;
	UINT patternStart = 0;
	UINT hostNameLen = (UINT)strlen(hostname);

	// Run through the auth hosts string until we hit the end
	if (m_auth_hosts) {
		while (1) {

			// Which mode are we in?
			switch (machineMode) {

				// ExpectIncludeExclude - we should see a + or -.
			case vh_ExpectIncludeExclude:
				if (m_auth_hosts[authHostsPos] == '+') {
					patternType = vncServer::aqrAccept;
					patternStart = authHostsPos+1;
					machineMode = vh_ExpectPattern;
				} else if (m_auth_hosts[authHostsPos] == '-') {	
					patternType = vncServer::aqrReject;
					patternStart = authHostsPos+1;
					machineMode = vh_ExpectPattern;
				} else if (m_auth_hosts[authHostsPos] == '?') {	
					patternType = vncServer::aqrQuery;
					patternStart = authHostsPos+1;
					machineMode = vh_ExpectPattern;
				} else if (m_auth_hosts[authHostsPos] != '\0') {
					vnclog.Print(LL_INTWARN, VNCLOG("verify host - malformed AuthHosts string\n"));
					machineMode = vh_ExpectDelimiter;
				}
				break;

				// ExpectPattern - we expect to see a valid pattern
			case vh_ExpectPattern:
				// ExpectDelimiter - we're scanning for the next ':', skipping a pattern
			case vh_ExpectDelimiter:
				if ((m_auth_hosts[authHostsPos] == ':') ||
					(m_auth_hosts[authHostsPos] == '\0')) {
					if (machineMode == vh_ExpectPattern) {
						if (patternStart == 0) {
							vnclog.Print(LL_INTWARN, VNCLOG("verify host - pattern processing failed!\n"));
						} else {
							// Process the match
							if (MatchStringToTemplate(hostname, hostNameLen,
								&(m_auth_hosts[patternStart]), authHostsPos-patternStart)) {
								// The hostname matched - apply the include/exclude rule
								verifiedHost = patternType;
							}
						}
					}

					// We now expect another + or -
					machineMode = vh_ExpectIncludeExclude;
				}
				break;
			}

			// Have we hit the end of the pattern string?
			if (m_auth_hosts[authHostsPos] == '\0')
				break;
			authHostsPos++;
		}
	}

    vnclog.Print(LL_INTINFO, VNCLOG("client %s verifiedHost %u prior to adjustment\n"), hostname, verifiedHost);
	//
	bool autoAccept = false;
	if ((GetTickCount() - startTime) < QueryDisableTime()*1000)
		autoAccept = true; 

	// Based on the server's QuerySetting, adjust the verification result
	switch (verifiedHost) {
	case vncServer::aqrAccept:
		if (QuerySetting() >= 3)
			verifiedHost = autoAccept 
					? vncServer::aqrAccept
					: vncServer::aqrQuery;
		break;
	case vncServer::aqrQuery:
		if (QuerySetting() <= 1)
			verifiedHost = vncServer::aqrAccept;
		else if (QuerySetting() == 4)
			verifiedHost = vncServer::aqrReject;
		break;
	case vncServer::aqrReject:
		if (QuerySetting() == 0)
			verifiedHost = autoAccept 
					? vncServer::aqrAccept
					: vncServer::aqrQuery;
		break;
	};

    vnclog.Print(LL_INTINFO, VNCLOG("client %s verifiedHost %u after adjustment\n"), hostname, verifiedHost);
	return verifiedHost;
}

void
vncServer::AddAuthHostsBlacklist(const char *machine) {
	omni_mutex_lock l(m_clientsLockBlackList, 60);

	// -=- Is the specified host blacklisted?
	vncServer::BlacklistEntry	*current = m_blacklist;

	// Get the current time as a 64-bit value
	SYSTEMTIME					systime;
	FILETIME					ftime;
	LARGE_INTEGER				now;
	GetSystemTime(&systime);
	SystemTimeToFileTime(&systime, &ftime);
	now.LowPart=ftime.dwLowDateTime;now.HighPart=ftime.dwHighDateTime;
	now.QuadPart /= 10000000; // Convert it into seconds

	while (current) {

		// Is this the entry we're interested in?
		if (_stricmp(current->_machineName, machine) == 0) {

			// If the host is already blocked then ignore
			if (current->_blocked)
				return;

			// Set the RefTime & failureCount
			current->_lastRefTime.QuadPart = now.QuadPart + 10*current->_failureCount;
			current->_failureCount++;

			if (current->_failureCount > 5)
				current->_blocked = TRUE;
			return;
		}

		current = current->_next;
	}

	// Didn't find the entry
	current = new vncServer::BlacklistEntry;
	current->_blocked = FALSE;
	current->_failureCount = 0;
	current->_lastRefTime.QuadPart = now.QuadPart + 10;
	current->_machineName = _strdup(machine);
	current->_next = m_blacklist;
	m_blacklist = current;
}

void
vncServer::RemAuthHostsBlacklist(const char *machine) {
	omni_mutex_lock l(m_clientsLockBlackList,61);

	// -=- Is the specified host blacklisted?
	vncServer::BlacklistEntry	*current = m_blacklist;
	vncServer::BlacklistEntry	*previous = 0;

	while (current) {

		// Is this the entry we're interested in?
		if (_stricmp(current->_machineName, machine) == 0) {
			if (previous)
				previous->_next = current->_next;
			else
				m_blacklist = current->_next;
			vncServer::BlacklistEntry *next = current->_next;
			free (current->_machineName);
			delete current;
			current = next;
			continue;
		}

		previous = current;
		current = current->_next;
	}
}

// Modef rdv@202
void
vncServer::SetNewSWSize(long w,long h,BOOL desktop)
{
	vncClientList::iterator i;
		
	omni_mutex_lock l(m_clientsLock,62);

	// Post this screen size update to all the connected clients
	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		// Post the update
		if (!GetClient(*i)->SetNewSWSize(w,h,desktop)) {
			vnclog.Print(LL_INTINFO, VNCLOG("Unable to set new desktop size\n"));
			KillClient(*i);
		}
	}
}

void
vncServer::SetBufferOffset(int x,int y)
{
	vncClientList::iterator i;
		
	omni_mutex_lock l(m_clientsLock,64);

	// Post this screen size update to all the connected clients
	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		// Post the update
		GetClient(*i)->SetBufferOffset(x,y);
	}

}

void
vncServer::InitialUpdate(bool value)
{
	vncClientList::iterator i;
		
	omni_mutex_lock l(m_clientsLock,65);

	// Post this screen size update to all the connected clients
	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		// Post the update
		GetClient(*i)->InitialUpdate(value);
	}

}

void
vncServer::SetScreenOffset(int x,int y, bool single_display)
{
	vncClientList::iterator i;
		
	omni_mutex_lock l(m_clientsLock,66);

	// Post this screen size update to all the connected clients
	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		// Post the update
		GetClient(*i)->SetScreenOffset(x, y, single_display);
	}

}

// Modif sf@2002 - v1.1.0 - Default Scaling
UINT vncServer::GetDefaultScale()
{
	return m_nDefaultScale;
}


BOOL vncServer::SetDefaultScale(int nScale)
{
	m_nDefaultScale = nScale;

	return TRUE;
}


BOOL vncServer::FileTransferEnabled()
{
	return m_fFileTransferEnabled;
}

BOOL vncServer::EnableFileTransfer(BOOL fEnable)
{
	m_fFileTransferEnabled = fEnable;

	return TRUE;
}
BOOL vncServer::Secure()
{
	return m_Secure;
}

BOOL vncServer::MSLogonRequired()
{
	return m_fMSLogonRequired;
}

BOOL vncServer::Secure(BOOL fEnable)
{
	m_Secure = fEnable;
	return TRUE;
}

BOOL vncServer::RequireMSLogon(BOOL fEnable)
{
	m_fMSLogonRequired = fEnable;

	return TRUE;
}

BOOL vncServer::GetNewMSLogon()
{
	return m_fNewMSLogon;
}

BOOL vncServer::SetNewMSLogon(BOOL fEnable)
{
	m_fNewMSLogon = fEnable;

	return TRUE;
}

BOOL vncServer::GetReverseAuthRequired()
{
	return m_fReverseAuthRequired;
}

BOOL vncServer::SetReverseAuthRequired(BOOL fEnable)
{
	m_fReverseAuthRequired = fEnable;

	return TRUE;
}


//
// sf@2002 - v1.1.x - DSM Plugin
//
BOOL vncServer::IsDSMPluginEnabled()
{
	return m_fDSMPluginEnabled;
}

void vncServer::EnableDSMPlugin(BOOL fEnable)
{
	m_fDSMPluginEnabled = fEnable;
}

char* vncServer::GetDSMPluginName()
{
	return m_szDSMPlugin;
}


void vncServer::SetDSMPluginName(char* szDSMPlugin)
{
	strcpy_s(m_szDSMPlugin, 128,szDSMPlugin);
	return;
}

//
// Load if necessary and initialize the current DSMPlugin
// This can only be done if NO client is connected (for the moment)
//
BOOL vncServer::SetDSMPlugin(BOOL bForceReload)
{
	//vnclog.Print(LL_INTINFO, VNCLOG("$$$$$$$$$$ SetDSMPlugin - Entry \n"));
	if (AuthClientCount() > 0) return FALSE;

	if (!IsDSMPluginEnabled()) return FALSE;

	//vnclog.Print(LL_INTINFO, VNCLOG("$$$$$$$$$$ SetDSMPlugin - Enabled \n"));

	// If the plugin is loaded, unload it first
	// sf@2003 - it has been possibly pre-configured by
	// clicking on the plugin config button
	if (m_pDSMPlugin->IsLoaded())
	{
		//vnclog.Print(LL_INTINFO, VNCLOG("$$$$$$$$$$ SetDSMPlugin - Is Loaded \n"));

		// sf@2003 - We check if the loaded plugin is the same than
		// the currently selected one or not
		m_pDSMPlugin->DescribePlugin();
		if (_stricmp(m_pDSMPlugin->GetPluginFileName(), GetDSMPluginName()) || bForceReload)
		{
			//if (bForceReload)
			//	vnclog.Print(LL_INTINFO, VNCLOG("$$$$$$$$$$ SetDSMPlugin - FORCE RELOADING OF THE PLUGIN \n"));
			//vnclog.Print(LL_INTINFO, VNCLOG("$$$$$$$$$$ SetDSMPlugin - New one - Unload the current \n"));
			m_pDSMPlugin->SetEnabled(false);
			m_pDSMPlugin->UnloadPlugin();
		}
	}

	if (!m_pDSMPlugin->IsLoaded())
	{
		//vnclog.Print(LL_INTINFO, VNCLOG("$$$$$$$$$$ SetDSMPlugin - Plugin NOT loaded - Try to load it \n"));
		if (!m_pDSMPlugin->LoadPlugin(GetDSMPluginName(), false))
		{
			vnclog.Print(LL_INTINFO, VNCLOG("$$$$$$$$$$ DSMPlugin cannot be loaded\n"));
			return FALSE;
		}
	}

	vnclog.Print(LL_INTINFO, VNCLOG("$$$$$$$$$$ SetDSMPlugin - Plugin successfully loaded \n"));

	// Now that it is loaded, init it
	if (m_pDSMPlugin->InitPlugin())
	{
		//vnclog.Print(LL_INTINFO, VNCLOG("$$$$$$$$$$ SetDSMPlugin - Init plugin call \n"));
		char szParams[MAXPWLEN + 64];
		char password[MAXPWLEN];
		GetPassword(password);
		// Does the plugin need the VNC password to do its job ?
		if (!_stricmp(m_pDSMPlugin->GetPluginParams(), "VNCPasswordNeeded"))
			strcpy_s(szParams, vncDecryptPasswd((char *)password, m_Secure));
		else
			strcpy_s(szParams, "NoPassword");

		// The second parameter tells the plugin the kind of program is using it
		// (in WinVNC : "server-app" or "server-svc"
		strcat_s(szParams, ",");
		strcat_s(szParams, vncService::RunningAsService() ? "server-svc" : "server-app");

		//::MessageBoxSecure(NULL, szParams, "SetDSMPlugin info", MB_OK);

		//vnclog.Print(LL_INTINFO, VNCLOG("$$$$$$$$$$ SetDSMPlugin - SetPluginParams call \n"));


		//adzm 2010-05-12 - dsmplugin config
		if (m_pDSMPlugin->SetPluginParams(NULL, szParams, GetDSMPluginConfig(), NULL))
		{
			m_pDSMPlugin->SetEnabled(true); // The plugin is ready to be used
			vnclog.Print(LL_INTINFO, VNCLOG("DSMPlugin Params OK\n"));
			return TRUE;
		}
		else
		{
			m_pDSMPlugin->SetEnabled(false);
			vnclog.Print(LL_INTINFO, VNCLOG("Unable to set DSMPlugin Params\n"));
		}
	}
	else
	{
		m_pDSMPlugin->SetEnabled(false);
		vnclog.Print(LL_INTINFO, VNCLOG("Unable to init DSMPlugin\n"));
	}

	/*
	MessageBoxSecure(NULL, 
	_T(_this->m_pDSMPlugin->DescribePlugin()),
	_T("Plugin Description"), MB_OK | MB_ICONEXCLAMATION );
	*/

	return TRUE;
}

//adzm 2010-05-12 - dsmplugin config
void vncServer::SetDSMPluginConfig(char* szDSMPluginConfig)
{
	strncpy_s(m_szDSMPluginConfig, sizeof(m_szDSMPluginConfig) - 1, szDSMPluginConfig, _TRUNCATE);
}

//
// sgf@2002 - for now, we disable cache rects when more than one client
// 
void vncServer::DisableCacheForAllClients()
{
	vncClientList::iterator i;
		
	omni_mutex_lock l(m_clientsLock,67);

	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		GetClient(*i)->EnableCache(FALSE);
	}

}

void
vncServer::Clear_Update_Tracker() {
	vncClientList::iterator i;
	omni_mutex_lock l(m_clientsLock,68);
	// Post this update to all the connected clients
	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		GetClient(*i)->Clear_Update_Tracker();

	}
}


void vncServer::Driver(BOOL enable)
{
	sethook=true;
	m_driver = enable;
}
	
void vncServer::Hook(BOOL enable)
{
	sethook=true;
	m_hook=enable;
}

void vncServer::SetHookings()
{
	if (sethook && m_desktop)
	{
		 m_desktop->SethookMechanism(Hook(),Driver());
	}
	sethook=false;
}

void vncServer::EnableXRichCursor(BOOL fEnable)
{
	m_fXRichCursor = fEnable;
}

BOOL
vncServer::SetDisableTrayIcon(BOOL disableTrayIcon)
{
	m_disableTrayIcon = disableTrayIcon;
	return TRUE;
}

BOOL
vncServer::GetDisableTrayIcon()
{
	return m_disableTrayIcon;
}

BOOL
vncServer::SetRdpmode(BOOL Rdpmode)
{
	m_Rdpmode = Rdpmode;
	return TRUE;
}

BOOL
vncServer::GetRdpmode()
{
	return m_Rdpmode;
}

BOOL
vncServer::SetNoScreensaver(BOOL NoScreensaver)
{
	m_NoScreensaver = NoScreensaver;
	if (m_desktop)
		m_desktop->PreventScreensaver(true);
	return TRUE;
}

BOOL
vncServer::GetNoScreensaver()
{
	return m_NoScreensaver;
}

BOOL
vncServer::SetAllowEditClients(BOOL AllowEditClients)
{
	m_AllowEditClients = AllowEditClients;
	return TRUE;
}

BOOL
vncServer::GetAllowEditClients()
{
	return m_AllowEditClients;
}

BOOL
vncServer::All_clients_initialalized() {
	vncClientList::iterator i;
	// Post this update to all the connected clients
	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		if (!GetClient(*i)->client_settings_passed) 
			return false;

	}
	return true;
}

void
vncServer::initialCapture_done() {
	vncClientList::iterator i;
	// Post this update to all the connected clients
	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		GetClient(*i)->initialCapture_done=true;

	}
}

void
vncServer::TriggerUpdate() {
	omni_mutex_lock l1(m_desktopLock, 150);
	omni_mutex_lock l(m_clientsLock, 150);
	vncClientList::iterator i;
	// Post this update to all the connected clients
	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		GetClient(*i)->TriggerUpdate();

	}
}

void
vncServer::SetHasMouse()
{
vncClientList::iterator i;
omni_mutex_lock l(m_clientsLock, 99);
	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		if (GetClient(*i)->ask_mouse == true) {
			GetClient(*i)->SetHasMouse(true);
			GetClient(*i)->ask_mouse = false;
		}
		else
			GetClient(*i)->SetHasMouse(false);


	}
}


bool vncServer::IsClient(vncClient* pClient)
{
  vncClientList::iterator i;
  for (i = m_authClients.begin(); i != m_authClients.end(); i++)
    if (GetClient(*i) == pClient) return true;

  return false;
}

DWORD WINAPI retryThread(LPVOID lpParam)
{
	vncServer* pIface = (vncServer*)lpParam;
	pIface->actualRetryThread();
	return true;
}

void vncServer::AutoConnectRetry( )
{
	//Start thread if not running
	if ( m_fAutoReconnect && !fShutdownOrdered) {
		vnclog.Print(LL_INTINFO, VNCLOG("AutoConnectRetry(): started\n"));
		if (retryThreadHandle != NULL) {
			DWORD dwWait = WaitForSingleObject(retryThreadHandle, 0);
			if (dwWait == WAIT_OBJECT_0) {
				CloseHandle(retryThreadHandle);
				DWORD dwTId;
				retryThreadHandle = CreateThread(NULL, 0, retryThread, pThis, 0, &dwTId);
			} 
			else if (dwWait == WAIT_TIMEOUT) {
				// thread still running
			}
		}
		else {
			DWORD dwTId;
			retryThreadHandle = CreateThread(NULL, 0, retryThread, pThis, 0, &dwTId);
		}

	}
}

void vncServer::actualRetryThread()
{	
	while ( m_fAutoReconnect && strlen(m_szAutoReconnectAdr) > 0 && !fShutdownOrdered)
	{
		vnclog.Print(LL_INTINFO, VNCLOG("Attempting AutoReconnect....\n"));	
		retrysock = new VSocket;
		if (retrysock) {
			// Connect out to the specified host on the VNCviewer listen port
#ifdef IPV6V4
			if (retrysock->CreateConnect(m_szAutoReconnectAdr, m_AutoReconnectPort)) {
#else
			retrysock->Create();
			if (retrysock->Connect(m_szAutoReconnectAdr, m_AutoReconnectPort)) {
#endif
				if ( strlen( m_szAutoReconnectId ) > 0 ) {
					retrysock->Send(m_szAutoReconnectId,250);
					retrysock->SetTimeout(0);
					// adzm 2009-07-05 - repeater IDs
					// Add the new client to this server
					AddClient(retrysock, !GetReverseAuthRequired(), TRUE, 0, NULL, m_szAutoReconnectId, m_szAutoReconnectAdr, m_AutoReconnectPort,true);
					vnclog.Print(LL_INTINFO, VNCLOG("leaving reconnectThread....\n"));
					return;
				} else {
					// Add the new client to this server
					// adzm 2009-08-02
					AddClient(retrysock, !GetReverseAuthRequired(), TRUE, 0, NULL, NULL, m_szAutoReconnectAdr, m_AutoReconnectPort,true);
					vnclog.Print(LL_INTINFO, VNCLOG("leaving reconnectThread....\n"));
					return;
				}
			} else {
				vnclog.Print(LL_INTINFO, VNCLOG("connect failed, waitin 5 sec to retry....\n"));	
				delete retrysock;	//retry
				retrysock = NULL;
				if ( m_fAutoReconnect && strlen(m_szAutoReconnectAdr) > 0 && !fShutdownOrdered) 
					Sleep(5000);
			}
		} 
	}
	vnclog.Print(LL_INTINFO, VNCLOG("leaving reconnectThread....\n"));
}

void vncServer::NotifyClients_StateChange(CARD32 state, CARD32 value)
{
	omni_mutex_lock l(m_clientsLock,42);
    vncClient *client = NULL;

    vncClientList::iterator i;

	for (i = m_unauthClients.begin(); i != m_unauthClients.end(); i++)
		{

			// Is this the right client?
        client = GetClient(*i);
        if (!client)
            continue;

        client->Record_SendServerStateUpdate(state, value);
		}

	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		// Is this the right client?
        client = GetClient(*i);
        if (!client)
            continue;

        client->Record_SendServerStateUpdate(state, value);
	}
}

void vncServer::StopReconnectAll()
{
	omni_mutex_lock l(m_clientsLock,43);
    vncClient *client = NULL;

    vncClientList::iterator i;

	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		// Is this the right client?
        client = GetClient(*i);
        if (!client)
            continue;

        client->m_Autoreconnect=false;
	}
}

char *vncServer::getInfoMsg()
{
	vncClient* client = NULL;

	vncClientList::iterator i;

	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		// Is this the right client?
		if (strlen(GetClient(*i)->infoMsg) > 0)
			return GetClient(*i)->infoMsg;
	}
	return "";
}

void vncServer::SetFTTimeout(int msecs)
{
    m_ftTimeout = msecs;
}

void vncServer::AutoCapt(int autocapt)
{
	m_autocapt = autocapt; 
}

short vncServer::getOldestViewer()
{
	omni_mutex_lock l(m_clientsLock, 43);
	short clientId = 0;
	DWORD timing = 0;

	vncClientList::iterator i;

	for (i = m_authClients.begin(); i != m_authClients.end(); i++)
	{
		if (!GetClient(*i))
			continue;
		if (timing == 0 || GetClient(*i)->getTiming() < timing) {
			timing = GetClient(*i)->getTiming();
			clientId = GetClient(*i)->GetClientId();
		}
	}
	return clientId;
}

UINT vncServer::getNumberViewers()
{
	return  m_authClients.size();
}

BOOL vncServer::getOSD() 
{ 
	return m_OSD; 
};

void vncServer::setOSD(const BOOL setting) 
{ 
	m_OSD = setting; 
};
