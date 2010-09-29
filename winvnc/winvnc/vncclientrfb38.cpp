//  Copyright (C) 2010 Ultr@VNC Team Members. All Rights Reserved.
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
// whence you received this file, check http://www.uvnc.com


// Includes
#include "stdhdrs.h"
#include <omnithread.h>
#include "resource.h"

// Custom
#include "vncServer.h"
#include "vncClient.h"
#include "VSocket.h"
#include "vncDesktop.h"
#include "rfbRegion.h"
#include "vncBuffer.h"
#include "vncService.h"
#include "vncPasswd.h"
#include "vncAcceptDialog.h"
#include "vncKeymap.h"
#include "vncMenu.h"

#include "rfb/dh.h"
#include "vncAuth.h"

#ifdef IPP
#include "ipp_zlib/zlib.h"
#else
#include "zlib/zlib.h"
#endif
#include "mmSystem.h" // sf@2002
#include "sys/types.h"
#include "sys/stat.h"

#include <string>
#include <shlobj.h>
#include "vncOSVersion.h"
#include "common/win32_helpers.h"

bool isDirectoryTransfer(const char *szFileName);
extern BOOL SPECIAL_SC_PROMPT;
extern BOOL SPECIAL_SC_EXIT;
int getinfo(char mytext[1024]);
extern CDPI g_dpi;

BOOL
vncClientThread::InitVersion()
{
	rfbProtocolVersionMsg protocol_ver;
	protocol_ver[12] = 0;
	if (strcmp(m_client->ProtocolVersionMsg,"0.0.0.0")==NULL)
	{
		// Generate the server's protocol version
		rfbProtocolVersionMsg protocolMsg;
		sprintf((char *)protocolMsg,
					rfbProtocolVersionFormat,
					rfbProtocolMajorVersion,
					rfbProtocolMinorVersion);
		// adzm 2010-08
		bool bRetry = true;
		bool bReady = false;
		int nRetry = 0;
		while (!bReady && bRetry) {
			// Send our protocol version, and get the client's protocol version
			if (!m_socket->SendExact((char *)&protocolMsg, sz_rfbProtocolVersionMsg) || 
				!m_socket->ReadExact((char *)&protocol_ver, sz_rfbProtocolVersionMsg)) {
				bReady = false;
				// we need to reconnect!
				
				Sleep(min(nRetry * 1000, 30000));

				if (TryReconnect()) {
					// reconnect if in SC mode and not already using AutoReconnect
					bRetry = true;
					nRetry++;
				} else {
					bRetry = false;
				}
			} else {
				bReady = true;
			}
		}

		if (!bReady) {
			return FALSE;
		}
	}
	else 
		memcpy(protocol_ver,m_client->ProtocolVersionMsg, sz_rfbProtocolVersionMsg);

	// sf@2006 - Trying to fix neverending authentication bug - Check if this is RFB protocole
	if (strncmp(protocol_ver,"RFB", 3)!=NULL)
		return FALSE;

	// Check viewer's the protocol version
	sscanf((char *)&protocol_ver, rfbProtocolVersionFormat, &major, &minor);
	if (major != rfbProtocolMajorVersion)
		return FALSE;

	m_ms_logon = m_server->MSLogonRequired();
	if (minor==3)  // old server or other flavor
		{
			if (m_ms_logon) return FALSE; // minor3, ms_logon not supported
			m_client->SetUltraViewer(false);
		}

	return TRUE;
}

BOOL
vncClientThread::FilterClients()
{

	// Verify the peer host name against the AuthHosts string
	vncServer::AcceptQueryReject verified;
	if (m_auth) {
		verified = vncServer::aqrAccept;
	} else {
		verified = m_server->VerifyHost(m_socket->GetPeerName());
	}
	
	// If necessary, query the connection with a timed dialog
	char username[UNLEN+1];
	if (!vncService::CurrentUser(username, sizeof(username))) return false;
	if ((strcmp(username, "") != 0) || m_server->QueryIfNoLogon()) // marscha@2006 - Is AcceptDialog required even if no user is logged on
    {
		if (verified == vncServer::aqrQuery) {
            // 10 Dec 2008 jdp reject/accept all incoming connections if the workstation is locked
            if (vncService::IsWSLocked() && !m_server->QueryIfNoLogon()) {
                verified = m_server->QueryAccept() == 1 ? vncServer::aqrAccept : vncServer::aqrReject;
            } else {

			vncAcceptDialog *acceptDlg = new vncAcceptDialog(m_server->QueryTimeout(),m_server->QueryAccept(), m_socket->GetPeerName());
	
			if (acceptDlg == NULL) 
				{
					if (m_server->QueryAccept()==1) 
					{
						verified = vncServer::aqrAccept;
					}
					else 
					{
						verified = vncServer::aqrReject;
					}
				}
			else 
				{
						HDESK desktop;
						desktop = OpenInputDesktop(0, FALSE,
													DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
													DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
													DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
													DESKTOP_SWITCHDESKTOP | GENERIC_WRITE
													);

						HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());
						
					
						SetThreadDesktop(desktop);

						if ( !(acceptDlg->DoDialog()) ) verified = vncServer::aqrReject;

						SetThreadDesktop(old_desktop);
						CloseDesktop(desktop);
				}
            }
		}
    }

	if (verified == vncServer::aqrReject) {

		if (minor>=7)
			{
				// 0 = Failure
				CARD8 value=0;
				if (!m_socket->SendExact((char *)&value, sizeof(value)))
					return FALSE;
			}
		else
			{
				CARD32 auth_val = Swap32IfLE(rfbConnFailed);
				if (!m_socket->SendExact((char *)&auth_val, sizeof(auth_val)))
					return FALSE;
			}
		char *errmsg = "Your connection has been rejected.";
		CARD32 errlen = Swap32IfLE(strlen(errmsg));
		if (!m_socket->SendExact((char *)&errlen, sizeof(errlen)))
			return FALSE;
		m_socket->SendExact(errmsg, strlen(errmsg));
		return FALSE;
	}
	return TRUE;
}

BOOL vncClientThread::CheckEmptyPasswd()
{
	char password[MAXPWLEN];
	m_server->GetPassword(password);
	vncPasswd::ToText plain(password);
	// By default we disallow passwordless workstations!
	if ((strlen(plain) == 0) && m_server->AuthRequired())
	{
		vnclog.Print(LL_CONNERR, VNCLOG("no password specified for server - client rejected\n"));

		if (minor>=7)
			{
				// 0 = Failure
				CARD8 value=0;
				if (!m_socket->SendExact((char *)&value, sizeof(value)))
					return FALSE;
			}
		else
			{
				CARD32 auth_val = Swap32IfLE(rfbConnFailed);
				if (!m_socket->SendExact((char *)&auth_val, sizeof(auth_val)))
					return FALSE;
		}
		char *errmsg =
			"This server does not have a valid password enabled.  "
			"Until a password is set, incoming connections cannot be accepted.";
		CARD32 errlen = Swap32IfLE(strlen(errmsg));
		if (!m_socket->SendExact((char *)&errlen, sizeof(errlen)))
			return FALSE;
		m_socket->SendExact(errmsg, strlen(errmsg));
		return FALSE;
	}
	return TRUE;
}

BOOL
vncClientThread::CheckLoopBack()
{
	// By default we filter out local loop connections, because they're pointless
	if (!m_server->LoopbackOk())
	{
		char *localname = _strdup(m_socket->GetSockName());
		char *remotename = _strdup(m_socket->GetPeerName());

		// Check that the local & remote names are different!
		if ((localname != NULL) && (remotename != NULL))
		{
			BOOL ok = _stricmp(localname, remotename) != 0;

			if (localname != NULL)
				free(localname);

			if (remotename != NULL)
				free(remotename);

			if (!ok)
			{
				vnclog.Print(LL_CONNERR, VNCLOG("loopback connection attempted - client rejected\n"));
				
				if (minor>=7)
					{
						// 0 = Failure
						CARD8 value=0;
						if (!m_socket->SendExact((char *)&value, sizeof(value)))
							return FALSE;
					}
				else
					{
						CARD32 auth_val = Swap32IfLE(rfbConnFailed);
						if (!m_socket->SendExact((char *)&auth_val, sizeof(auth_val)))
							return FALSE;
					}
				char *errmsg = "Local loop-back connections are disabled.";
				CARD32 errlen = Swap32IfLE(strlen(errmsg));
				if (!m_socket->SendExact((char *)&errlen, sizeof(errlen)))
					return FALSE;
				m_socket->SendExact(errmsg, strlen(errmsg));
				return FALSE;
			}
		}
	}
	else
	{
		char *localname = _strdup(m_socket->GetSockName());
		char *remotename = _strdup(m_socket->GetPeerName());

		// Check that the local & remote names are different!
		if ((localname != NULL) && (remotename != NULL))
		{
			BOOL ok = _stricmp(localname, remotename) != 0;

			if (localname != NULL)
				free(localname);

			if (remotename != NULL)
				free(remotename);

			if (!ok)
			{
				vnclog.Print(LL_CONNERR, VNCLOG("loopback connection attempted - client accepted\n"));
				m_client->m_IsLoopback=true;
			}
		}

	}
	return TRUE;
}

BOOL vncClientThread::VNCAUTH()
{
	char password[MAXPWLEN];
	m_server->GetPassword(password);
	vncPasswd::ToText plain(password);

	BOOL auth_ok = TRUE;
		{
			// Now create a 16-byte challenge
			char challenge[16];
			char challenge2[16]; //PGM
			char response[16];
			vncRandomBytes((BYTE *)&challenge);
			memcpy(challenge2, challenge, 16); //PGM
			

			{
				vnclog.Print(LL_INTINFO, "password authentication");
				if (!m_socket->SendExact(challenge, sizeof(challenge)))
                {
					vnclog.Print(LL_SOCKERR, VNCLOG("Failed to send challenge to client\n"));
					return FALSE;
                }


				// Read the response
				if (!m_socket->ReadExact(response, sizeof(response)))
                {
					vnclog.Print(LL_SOCKERR, VNCLOG("Failed to receive challenge response from client\n"));
					return FALSE;
                }
				// Encrypt the challenge bytes
				vncEncryptBytes((BYTE *)&challenge, plain);

				// Compare them to the response
				for (int i=0; i<sizeof(challenge); i++)
				{
					if (challenge[i] != response[i])
					{
						auth_ok = FALSE;
						break;
					}
				}
				if (!auth_ok) //PGM
				{ //PGM
					memset(password, '\0', MAXPWLEN); //PGM
					m_server->GetPassword2(password); //PGM
					vncPasswd::ToText plain2(password); //PGM
					if ((strlen(plain2) > 0)) //PGM
					{ //PGM
						vnclog.Print(LL_INTINFO, "View-only password authentication"); //PGM
						m_client->EnableKeyboard(false); //PGM
						m_client->EnablePointer(false); //PGM
						auth_ok = TRUE; //PGM

						// Encrypt the view-only challenge bytes //PGM
						vncEncryptBytes((BYTE *)&challenge2, plain2); //PGM

						// Compare them to the response //PGM
						for (int i=0; i<sizeof(challenge2); i++) //PGM
						{ //PGM
							if (challenge2[i] != response[i]) //PGM
							{ //PGM
								auth_ok = FALSE; //PGM
								break; //PGM
							} //PGM
						} //PGM
					} //PGM
				} //PGM
			}
		}

		return auth_ok;
}


int  
vncClientThread::AuthMSLOGON() {
	//adzm 2010-05-10
	if (m_socket->IsUsePluginEnabled() && m_server->GetDSMPluginPointer()->IsEnabled() && m_socket->GetIntegratedPlugin() != NULL) {
		m_socket->GetIntegratedPlugin()->SetHandshakeComplete();
	}

	DH dh;
	char gen[8], mod[8], pub[8], resp[8];
	char user[256], passwd[64];
	unsigned char key[8];
	
	dh.createKeys();
	int64ToBytes(dh.getValue(DH_GEN), gen);
	int64ToBytes(dh.getValue(DH_MOD), mod);
	int64ToBytes(dh.createInterKey(), pub);
		
	if (!m_socket->SendExact(gen, sizeof(gen))) return FALSE;
	if (!m_socket->SendExact(mod, sizeof(mod))) return FALSE;
	if (!m_socket->SendExact(pub, sizeof(pub))) return FALSE;
	if (!m_socket->ReadExact(resp, sizeof(resp))) return FALSE;
	if (!m_socket->ReadExact(user, sizeof(user))) return FALSE;
	if (!m_socket->ReadExact(passwd, sizeof(passwd))) return FALSE;

	int64ToBytes(dh.createEncryptionKey(bytesToInt64(resp)), (char*) key);
	vnclog.Print(0, "After DH: g=%I64u, m=%I64u, i=%I64u, key=%I64u\n", bytesToInt64(gen), bytesToInt64(mod), bytesToInt64(pub), bytesToInt64((char*) key));
	vncDecryptBytes((unsigned char*) user, sizeof(user), key); user[255] = '\0';
	vncDecryptBytes((unsigned char*) passwd, sizeof(passwd), key); passwd[63] = '\0';

	int result = CheckUserGroupPasswordUni(user, passwd, m_client->GetClientName());
	vnclog.Print(LL_INTINFO, "CheckUserGroupPasswordUni result=%i\n", result);
	if (result == 2) {
		m_client->EnableKeyboard(false);
		m_client->EnablePointer(false);
	}
	return result;
}

void vncClientThread::Logging(bool value)
{
	if (!value)
	{
		vnclog.Print(LL_CONNERR, VNCLOG("authentication failed\n"));
		typedef BOOL (*LogeventFn)(char *machine);
		LogeventFn Logevent = 0;
		char szCurrentDir[MAX_PATH];
		if (GetModuleFileName(NULL, szCurrentDir, MAX_PATH))
			{
				char* p = strrchr(szCurrentDir, '\\');
				*p = '\0';
				strcat (szCurrentDir,"\\logging.dll");
			}
		HMODULE hModule = LoadLibrary(szCurrentDir);
		if (hModule)
			{
				BOOL result=false;
				Logevent = (LogeventFn) GetProcAddress( hModule, "LOGFAILED" );
				Logevent((char *)m_client->GetClientName());
				FreeLibrary(hModule);
			}		
	}
	else
	{
		typedef BOOL (*LogeventFn)(char *machine);
		LogeventFn Logevent = 0;
		char szCurrentDir[MAX_PATH];
		if (GetModuleFileName(NULL, szCurrentDir, MAX_PATH))
			{
				char* p = strrchr(szCurrentDir, '\\');
				*p = '\0';
				strcat (szCurrentDir,"\\logging.dll");
			}
		HMODULE hModule = LoadLibrary(szCurrentDir);
		if (hModule)
			{
				BOOL result=false;
				Logevent = (LogeventFn) GetProcAddress( hModule, "LOGLOGON" );
				Logevent((char *)m_client->GetClientName());
				FreeLibrary(hModule);
			}
	}
}


BOOL vncClientThread::SecureVNCPlugin()
{
	char password[MAXPWLEN];
	m_server->GetPassword(password);
	vncPasswd::ToText plain(password);
	m_socket->SetDSMPluginConfig(m_server->GetDSMPluginConfig());
	BOOL auth_ok = FALSE;

	// Send auth-required message
	CARD32 auth_val = Swap32IfLE(rfbUltraVNC_SecureVNCPlugin);
	if (!m_socket->SendExact((char *)&auth_val, sizeof(auth_val)))
		return FALSE;

	const char* plainPassword = plain;

	if (!m_ms_logon && plainPassword && strlen(plainPassword) > 0) {
		m_socket->GetIntegratedPlugin()->SetPasswordData((const BYTE*)plainPassword, strlen(plainPassword));
	}

	int nSequenceNumber = 0;
	bool bSendChallenge = true;
	do
	{
		BYTE* pChallenge = NULL;
		int nChallengeLength = 0;
		if (!m_socket->GetIntegratedPlugin()->GetChallenge(pChallenge, nChallengeLength, nSequenceNumber)) {
			m_socket->GetIntegratedPlugin()->FreeMemory(pChallenge);
			return FALSE;
		}

		WORD wChallengeLength = (WORD)nChallengeLength;

		if (!m_socket->SendExact((const char*)&wChallengeLength, sizeof(wChallengeLength))) {
			m_socket->GetIntegratedPlugin()->FreeMemory(pChallenge);
			return FALSE;
		}

		if (!m_socket->SendExact((const char*)pChallenge, nChallengeLength)) {
			m_socket->GetIntegratedPlugin()->FreeMemory(pChallenge);
			return FALSE;
		}

		m_socket->GetIntegratedPlugin()->FreeMemory(pChallenge);
		WORD wResponseLength = 0;
		if (!m_socket->ReadExact((char*)&wResponseLength, sizeof(wResponseLength))) {
			return FALSE;
		}

		BYTE* pResponseData = new BYTE[wResponseLength];
		
		if (!m_socket->ReadExact((char*)pResponseData, wResponseLength)) {
			delete[] pResponseData;
			return FALSE;
		}

		if (!m_socket->GetIntegratedPlugin()->HandleResponse(pResponseData, (int)wResponseLength, nSequenceNumber, bSendChallenge)) {
			auth_ok = FALSE;
			bSendChallenge = false;
		} else if (!bSendChallenge) {
			auth_ok = TRUE;
			}

		delete[] pResponseData;
		nSequenceNumber++;
	} while (bSendChallenge);
	if (auth_ok && m_ms_logon) 
	{
		//adzm 2010-05-10 - this will set HandshakeComplete after sending the MsLogon authentication type packet
		vnclog.Print(LL_INTINFO, "MS-Logon (DH) authentication");
		CARD32 auth_val = Swap32IfLE(rfbMsLogon);
		if (!m_socket->SendExact((char *)&auth_val, sizeof(auth_val)))
		{
			auth_ok = FALSE;
			return FALSE;
		}

		int result;
		result = AuthMSLOGON();
		CARD32 authmsg = Swap32IfLE(result ? rfbVncAuthOK : rfbVncAuthFailed);

		if (!m_socket->SendExact((char *)&authmsg, sizeof(authmsg)))
		{
			auth_ok = FALSE;
			return FALSE;
		}
		
		if (!result) {
			vnclog.Print(LL_CONNERR, VNCLOG("authentication failed\n"));
			auth_ok = FALSE;
			return FALSE;
		}

	} else {
		// Did the authentication work?
		CARD32 authmsg;
		if (!auth_ok)
		{
			Logging(false);
			//adzm 2010-05-11 - Send an explanatory message for the failure (if any)
			const char* errmsg = m_socket->GetIntegratedPlugin()->GetLastErrorString();
			CARD32 errlen = Swap32IfLE(strlen(errmsg));
			if (errlen > 0) {
				authmsg = Swap32IfLE(rfbVncAuthFailedEx);
				if (!m_socket->SendExact((char *)&authmsg, sizeof(authmsg)))
					return FALSE;
				if (!m_socket->SendExact((char *)&errlen, sizeof(errlen)))
					return FALSE;
				if (!m_socket->SendExact(errmsg, strlen(errmsg)))
					return FALSE;
			} else {
				authmsg = Swap32IfLE(rfbVncAuthFailed);
				if (!m_socket->SendExact((char *)&authmsg, sizeof(authmsg)))
					return FALSE;
			}
			return FALSE;
		}
		else
		{
			// Tell the client we're ok
			authmsg = Swap32IfLE(rfbVncAuthOK);
			//////////////////
			// LOG it also in the event
			//////////////////
			if (!m_ms_logon){
				Logging(true);
			}
			if (!m_socket->SendExact((char *)&authmsg, sizeof(authmsg)))
				return FALSE;
		}			
		if (auth_ok) {			
			m_socket->GetIntegratedPlugin()->SetHandshakeComplete();
		}
	}
	return TRUE;
}


BOOL
vncClientThread::InitAuthenticate()
{
	vnclog.Print(LL_INTINFO, "Entered InitAuthenticate\n");
	char password[MAXPWLEN];
	m_server->GetPassword(password);
	vncPasswd::ToText plain(password);

	if (!FilterClients()) return FALSE;
	if (!CheckEmptyPasswd()) return FALSE;
	if (!CheckLoopBack()) return FALSE;

	CARD32 AuthenticationType=rfbConnFailed;
	if (minor>=8)
	{
	// WE now need to send the supported Auth types
		if ((m_socket->IsUsePluginEnabled() && m_server->GetDSMPluginPointer()->IsEnabled() && m_socket->GetIntegratedPlugin() != NULL) ||  m_ms_logon )
		{
			//Need rfbUltraVNC
			CARD8 value=1;
			if (!m_socket->SendExact((char *)&value, sizeof(value)))
				return FALSE;
			value=rfbUltraVNC;
			if (!m_socket->SendExact((char *)&value, sizeof(value)))
				return FALSE;
			if (!m_socket->ReadExact((char*)&value, sizeof(value)))
				return FALSE;
			//Viewer need to support rfbUltraVNC
			if (value!=rfbUltraVNC)
				return FALSE;
			AuthenticationType=value;
		}
		else if (m_auth || (strlen(plain) == 0))
		{
			CARD8 value=2;
			if (!m_socket->SendExact((char *)&value, sizeof(value)))
				return FALSE;
			CARD8 myarray[2];
			myarray[0]=rfbUltraVNC;
			myarray[1]=rfbNoAuth;
			if (!m_socket->SendExact((char *)&myarray, sizeof(myarray)))
				return FALSE;
			if (!m_socket->ReadExact((char*)&value, sizeof(value)))
				return FALSE;
			//Viewer need to answer with one of the sended value's
			if (value!=rfbUltraVNC && value!=rfbNoAuth)
				return FALSE;
			AuthenticationType=value;
		}
		else
		{
			CARD8 value=2;
			if (!m_socket->SendExact((char *)&value, sizeof(value)))
				return FALSE;
			CARD8 myarray[2];
			myarray[0]=rfbUltraVNC;
			myarray[1]=rfbVncAuth;
			if (!m_socket->SendExact((char *)&myarray, sizeof(myarray)))
				return FALSE;
			if (!m_socket->ReadExact((char*)&value, sizeof(value)))
				return FALSE;
			//Viewer need to answer with one of the sended value's
			if (value!=rfbUltraVNC && value!=rfbVncAuth)
				return FALSE;
			AuthenticationType=value;
		}
	}
	else
	{
		m_client->SetUltraViewer(false);
		if ((m_socket->IsUsePluginEnabled() && m_server->GetDSMPluginPointer()->IsEnabled() && m_socket->GetIntegratedPlugin() != NULL) ||  m_ms_logon )
		{
			
				//This option require rfb 3.8
				CARD8 value=0;
				if (!m_socket->SendExact((char *)&value, sizeof(value)))
					return FALSE;
				char *errmsg = "Server require rfb 3.8.";
				CARD32 errlen = Swap32IfLE(strlen(errmsg));
				if (!m_socket->SendExact((char *)&errlen, sizeof(errlen)))
					return FALSE;
				m_socket->SendExact(errmsg, strlen(errmsg));
				return FALSE;
		}
		else if (m_auth || (strlen(plain) == 0))
			{
				// Send no-auth-required message
				CARD32 auth_val = Swap32IfLE(rfbNoAuth);
				if (!m_socket->SendExact((char *)&auth_val, sizeof(auth_val)))
					return FALSE;
				AuthenticationType=rfbNoAuth;
			}
		else
			{
				CARD32 auth_val = Swap32IfLE(rfbVncAuth);
				if (!m_socket->SendExact((char *)&auth_val, sizeof(auth_val)))
				return FALSE;
				AuthenticationType=rfbVncAuth;
			}
	}

	//Now we can have 3 value's
	// rfbNoAuth,rfbVncAuth,rfbUltraVNC
	if (AuthenticationType==rfbVncAuth)
	{
		m_client->SetUltraViewer(false);
		CARD32 authmsg;
		if (VNCAUTH())
		{
			authmsg = Swap32IfLE(rfbVncAuthOK);
			Logging(true);
			if (!m_socket->SendExact((char *)&authmsg, sizeof(authmsg)))
				return FALSE;
		}
		else
		{
			authmsg = Swap32IfLE(rfbVncAuthFailed);
			Logging(false);
			if (!m_socket->SendExact((char *)&authmsg, sizeof(authmsg))) return FALSE;
			char *errmsg = "Wrong Password";
			CARD32 errlen = Swap32IfLE(strlen(errmsg));
			if (!m_socket->SendExact((char *)&errlen, sizeof(errlen))) return FALSE;
			m_socket->SendExact(errmsg, strlen(errmsg));
			return FALSE;
		}

	}
	else if (AuthenticationType==rfbNoAuth)
	{
		m_client->SetUltraViewer(false);
		if (minor>=8)
		{
			CARD32 authmsg;
			authmsg = Swap32IfLE(rfbVncAuthOK);
			if (!m_socket->SendExact((char *)&authmsg, sizeof(authmsg)))
				return FALSE;
		}
		Logging(true);
	}
	else if (AuthenticationType==rfbUltraVNC)
	{
		m_client->SetUltraViewer(true);
		CARD8 viewer_version=0;
		CARD8 server_version=1;
		if (!m_socket->ReadExact((char*)&viewer_version, sizeof(viewer_version)))
				return FALSE;
		if (viewer_version>1)
		{
			// Error, just return server_version=0 to indicate error
			CARD8 server_version=0;
			m_socket->SendExact((char *)&server_version, sizeof(server_version));
			return FALSE;
		}
		if (!m_socket->SendExact((char *)&server_version, sizeof(server_version)))
			return FALSE;
		// Send subauth type
		CARD8 SubAuth;
		CARD8 SpecialHandling=0;

		if (m_socket->IsUsePluginEnabled() && m_server->GetDSMPluginPointer()->IsEnabled() && m_socket->GetIntegratedPlugin() != NULL) SubAuth=rfbSubSecureVNCPlugin;
		else if (m_ms_logon) SubAuth=rfSubbMsLogon;
		else if (m_auth || (strlen(plain) == 0)) SubAuth=rfbSubNoAuth;
		else SubAuth=rfbSubVncAuth;
		if (!m_socket->SendExact((char *)&SubAuth, sizeof(SubAuth)))
			return FALSE;
		//Send SpecialHandling
		if (SPECIAL_SC_PROMPT)
		{
			SpecialHandling=rfbSubSpecialSC_PROMP;
		}
		if (!m_socket->SendExact((char *)&SpecialHandling, sizeof(SpecialHandling)))
			return FALSE;

		if (SPECIAL_SC_PROMPT)
		{
			char mytext[1024];
			getinfo(mytext);
			int size=strlen(mytext);
			if (!m_socket->SendExact((char *)&size, sizeof(int)))
				return FALSE;
			if (!m_socket->SendExact((char *)mytext, size))
				return FALSE;
			int nummer;
			if (!m_socket->ReadExact((char *)&nummer, sizeof(int)))
				return FALSE;
			if (nummer==0) return FALSE;
		}

		if (SubAuth==rfbSubNoAuth)
		{
			//Nothing to do
		}
		else if (SubAuth==rfbSubVncAuth)
		{
			CARD32 authmsg;
			if (VNCAUTH())
				{
					authmsg = Swap32IfLE(rfbVncAuthOK);
					Logging(true);
					if (!m_socket->SendExact((char *)&authmsg, sizeof(authmsg)))
					return FALSE;
				}
			else
				{
					authmsg = Swap32IfLE(rfbVncAuthFailed);
					Logging(false);
					if (!m_socket->SendExact((char *)&authmsg, sizeof(authmsg))) return FALSE;
					////
					char *errmsg = "Wrong Password";
					CARD32 errlen = Swap32IfLE(strlen(errmsg));
					if (!m_socket->SendExact((char *)&errlen, sizeof(errlen))) return FALSE;
					m_socket->SendExact(errmsg, strlen(errmsg));
					return FALSE;
				}			
		}
		else if (SubAuth==rfSubbMsLogon)
		{

			int result;
			result = AuthMSLOGON();
			CARD32 authmsg = Swap32IfLE(result ? rfbVncAuthOK : rfbVncAuthFailed);
			if (!m_socket->SendExact((char *)&authmsg, sizeof(authmsg)))
				return FALSE;
		
			if (!result) {
				char *errmsg = "Wrong user or Password";
				CARD32 errlen = Swap32IfLE(strlen(errmsg));
				if (!m_socket->SendExact((char *)&errlen, sizeof(errlen))) return FALSE;
				m_socket->SendExact(errmsg, strlen(errmsg));
				vnclog.Print(LL_CONNERR, VNCLOG("authentication failed\n"));
				Logging(false);
				return FALSE;
			}
			else
			{
				Logging(true);
			}

		}
		else if (SubAuth==rfbSubSecureVNCPlugin)
		{
			if (!SecureVNCPlugin()) return FALSE;
		}
		else
		{
			//should not happen
			return FALSE;
		}
	}
	else
	{
		//Can never happen
		CARD32 authmsg;
		authmsg = Swap32IfLE(rfbVncAuthFailed);
		if (!m_socket->SendExact((char *)&authmsg, sizeof(authmsg))) return FALSE;
		char *errmsg = "Unknown errer code 3";
		CARD32 errlen = Swap32IfLE(strlen(errmsg));
		if (!m_socket->SendExact((char *)&errlen, sizeof(errlen))) return FALSE;
		m_socket->SendExact(errmsg, strlen(errmsg));
		return FALSE;
	}	

	// Read the client's initialisation message
	rfbClientInitMsg client_ini;
	if (!m_socket->ReadExact((char *)&client_ini, sz_rfbClientInitMsg))
		return FALSE;

	// If the client wishes to have exclusive access then remove other clients
	if (m_server->ConnectPriority()==3 && !m_shared )
	{
		// Existing
			if (m_server->AuthClientCount() > 0)
			{
				vnclog.Print(LL_CLIENTS, VNCLOG("connections already exist - client rejected\n"));
				return FALSE;
			}
	}
	// adzm 2010-09
	if (!(client_ini.flags & clientInitShared) && !m_shared)
	{
		// Which client takes priority, existing or incoming?
		if (m_server->ConnectPriority() < 1)
		{
			// Incoming
			vnclog.Print(LL_INTINFO, VNCLOG("non-shared connection - disconnecting old clients\n"));
			m_server->KillAuthClients();
		} else if (m_server->ConnectPriority() > 1)
		{
			// Existing
			if (m_server->AuthClientCount() > 0)
			{
				vnclog.Print(LL_CLIENTS, VNCLOG("connections already exist - client rejected\n"));
				return FALSE;
			}
		}
	}

	vnclog.Print(LL_CLIENTS, VNCLOG("Leaving InitAuthenticate\n"));
	// Tell the server that this client is ok
	return m_server->Authenticated(m_client->GetClientId());
}