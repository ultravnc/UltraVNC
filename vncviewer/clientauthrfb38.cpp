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
// whence you received this file, check http://www.uvnc.com or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.


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

#include "Exception.h"
extern "C" {
	#include "vncauth.h"
}

#include <rdr/FdInStream.h>
#include <rdr/ZlibInStream.h>
#include <rdr/Exception.h>

#include <rfb/dh.h>

#include <DSMPlugin/DSMPlugin.h> // sf@2002
#include "common/win32_helpers.h"

extern char sz_L89[64];
extern char sz_L92[64];
extern char sz_L51[64];
extern char sz_L52[64];
extern char sz_L53[64];
extern char sz_L54[64];
extern char sz_L55[64];
extern char sz_L56[64];
extern char sz_L57[64];
extern char sz_L58[64];
extern char sz_L59[64];
extern char sz_L91[64];
extern char sz_L90[64];
extern bool g_passwordfailed;


//Replacement for NegotiateProtocolVersion()
// and Authenticate()
#ifdef rfb38

void ClientConnection::NegotiateProtocolVersionrfb38()
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
		if (m_fUsePlugin)
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
		if (m_fUsePlugin)
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

				int returnvalue=MessageBox(m_hwndMain, "You have specified an encryption plugin, however this connection in unencrypted! Do you want to continue?", "Accept insecure connection", MB_YESNO | MB_ICONEXCLAMATION | MB_TOPMOST);
				if (returnvalue==IDNO) 
				{
					throw WarningException("You refused the insecure connection.");
				}
			}
		}
	}


    if (sscanf(pv,rfbProtocolVersionFormat,&m_majorVersion,&m_minorVersion) != 2)
	{
		SetEvent(KillEvent);
		if (m_fUsePlugin)
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

    vnclog.Print(0, _T("RFB server supports protocol version %d.%d\n"), m_majorVersion,m_minorVersion);

	
	//14 and 16 are not official supported
	//To stay compateble with UltraVnc 1082 we check <14, this will break if vnc proto
	// reach level 14
	if (m_majorVersion == 3 && (m_minorVersion >= 8 && m_minorVersion<14)) {
		m_minorVersion = 8;
	} else if (m_majorVersion == 3 && m_minorVersion == 7) {
		m_minorVersion = 7;
	} else if (m_majorVersion == 3 && m_minorVersion == 4) {
		m_minorVersion = 4;
	}else if (m_majorVersion == 3 && m_minorVersion == 6) {
		m_minorVersion = 6;
	}else if (m_majorVersion == 3 && m_minorVersion == 14) {
		m_minorVersion = 14;
	}else if (m_majorVersion == 3 && m_minorVersion == 16) {
		m_minorVersion = 16;
	}
	else {
		m_minorVersion = 3;
	}
	// send to server
	sprintf(pv,rfbProtocolVersionFormat, m_majorVersion, m_minorVersion);
	WriteExact(pv, sz_rfbProtocolVersionMsg);
	// Set special protocols

	if (m_minorVersion == 4)
			{
				m_ms_logon = true;
				m_fServerKnowsFileTransfer = true;
			}
	else if (m_minorVersion == 6) // 6 because 5 already used in TightVNC viewer for some reason
			{
				m_ms_logon = false;
				m_fServerKnowsFileTransfer = true;
			}
			// Added for SC so we can do something before actual data transfer start
	else if (m_minorVersion == 14 )
			{
				m_ms_logon = true;
				m_fServerKnowsFileTransfer = true;
			}
	else if ( m_minorVersion == 16)
			{
				m_fServerKnowsFileTransfer = true;
			}

	// SC special, request viewer permission to connect
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

				//adzm 2009-06-21 - auto-accept if specified
				if (!m_opts.m_fAutoAcceptIncoming) {
					int returnvalue=MessageBox(m_hwndMain,   mytext,"Accept Incoming SC connection", MB_YESNO |  MB_TOPMOST);
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
				} else {
					int nummer=1;
					WriteExact((char *)&nummer,sizeof(int));
				}		
			}
	vnclog.Print(0, _T("Connected to RFB server, using protocol version %d.%d\n"),rfbProtocolMajorVersion, m_minorVersion);
}


void ClientConnection::ReadServerFailureMessage()
{
		CARD32 reasonLen;
		ReadExact((char *)&reasonLen, 4);
		reasonLen = Swap32IfLE(reasonLen);		
		CheckBufferSize(reasonLen+1);
		ReadString(m_netbuf, reasonLen);		
		vnclog.Print(0, _T("RFB connection failed, reason: %s\n"), m_netbuf);
		if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L91);
		throw WarningException(m_netbuf);
}


void ClientConnection::Authenticaterfb38_Part1()
{
	CARD32 authScheme;
	AuthSecureVNCPluginUsed=false;

	if (m_minorVersion >= 7 && m_minorVersion<14) 
		{
			CARD8 SecArrayLen;
			ReadExact((char *)&SecArrayLen, sizeof(SecArrayLen));
			if (SecArrayLen == 0)
				ReadServerFailureMessage();
			CARD8 *SecArray = new CARD8[SecArrayLen];
			ReadExact((char *)SecArray, SecArrayLen);
			if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,"Array supported Security received");

			CARD8 Sec=rfbConnFailed;
			int j;
			for (j = 0; j < (int)SecArrayLen; j++) 
				{
					if (SecArray[j] == rfbUltraVNC) 
						{
						Sec = rfbUltraVNC;
						WriteExact((char *)&Sec, sizeof(Sec));
						if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,"UltraVnc Protocol Extension enabled");
						goto found;
						}
				}

			for (j = 0; j < (int)SecArrayLen; j++) 
				{		
					if (SecArray[j] ==rfbNoAuth)
					{
						Sec=rfbNoAuth;
						WriteExact((char *)&Sec, sizeof(Sec));
						if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,"rfbNoAuth");
						goto found;
					}
					if (SecArray[j] ==rfbVncAuth)
					{
						Sec=rfbVncAuth;
						WriteExact((char *)&Sec, sizeof(Sec));
						if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,"rfbVncAuth");
						goto found;
					}
				}


			found:
			authScheme=Sec;
			if(Sec==rfbConnFailed)
			{
				vnclog.Print(0, _T("Unknwon Security Type rfb3.8 \n"));
				if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,"Unknwon Security Type rfb3.8");
				throw WarningException("Unknwon Security Type rfb3.8");
			}


		} 
	else 
		{
			ReadExact((char *)&authScheme, 4);
			authScheme = Swap32IfLE(authScheme);
			if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L90);
	
			if ( authScheme==rfbConnFailed)
			{
				ReadServerFailureMessage();
			}

			if ( authScheme!=rfbNoAuth && authScheme!=rfbVncAuth)
			{
				vnclog.Print(0, _T("Unknwon Security Type \n"));
				if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,"Unknwon Security Type");
				throw WarningException("Unknwon Security Type");
			}

			if ((m_minorVersion == 4 || m_minorVersion == 14 ) &&  (authScheme==rfbVncAuth))//hack for old ultravnc1082
			{
				authScheme=rfbMsLogon;
			}
		}


	if (m_fUsePlugin && m_pIntegratedPluginInterface && authScheme != rfbConnFailed && authScheme!=rfbUltraVNC) 
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
				int returnvalue=MessageBox(m_hwndMain, "You have specified an encryption plugin, however this connection in unencrypted! Do you want to continue?", "Accept insecure connection", MB_YESNO | MB_ICONEXCLAMATION | MB_TOPMOST);
				if (returnvalue==IDNO) 
				{
					throw WarningException("You refused the insecure connection.");
				}
			}
		}
	}

	switch (authScheme) 
	{
    case rfbNoAuth:
		Authenticaterfb38_Part2(rfbNoAuth);
		break;
    case rfbVncAuth:
		Authenticaterfb38_Part2(rfbVncAuth);
		break;

	case rfbUltraVNC:
		Authenticaterfb38_Part2(rfbUltraVNC);
		break;
    case rfbMsLogon:
		Authenticaterfb38_Part2(rfbMsLogon);
		break;

	default:	// should never happen
		vnclog.Print(0, _T("Unknwon Security Type \n"));
		if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,"Unknwon Security Type");
		throw WarningException("Unknwon Security Type");
    }

}

void ClientConnection::Authenticaterfb38_Part2(CARD32 authScheme)
{
	CARD32 authResult=rfbVncAuthFailed;
	switch (authScheme) 
	{
    case rfbNoAuth:
		if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L92);
		vnclog.Print(0, _T("No authentication needed\n"));
		if (m_minorVersion < 8)
			{
				authResult=rfbVncAuthOK;
			}
		else 
			{
				ReadExact((char *) &authResult, 4);
				authResult = Swap32IfLE(authResult);
			}
		break;
    case rfbVncAuth:
		authResult=AuthVnc();
		break;

	case rfbUltraVNC:
		authResult=AuthUltraVnc();
		break;
    case rfbMsLogon:
		authResult=AuthMSLOGON();
		break;

	default:	// should never happen
		vnclog.Print(0, _T("Unknwon Security Type \n"));
		if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,"Unknwon Security Type");
		throw WarningException("Unknwon Security Type");

	}
	HandleAuthResult(authResult);
}


CARD32 ClientConnection::AuthVnc()
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
			if (ad.DoDialog(false))
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
		ReadExact((char *) &authResult, 4);			
		authResult = Swap32IfLE(authResult);
		return authResult;
}

CARD32 ClientConnection::AuthMSLOGON()
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
		strcpy(m_clearPasswd, m_pApp->m_options.m_clearPassword);
		strncpy(passwd, m_clearPasswd, 64);
		strncpy(user, m_cmdlnUser, 254);
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
		strncpy(passwd, ad.m_passwd, 64);
		strncpy(user, ad.m_user, 254);
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


	CARD32 authResult=rfbVncAuthFailed;
	ReadExact((char *) &authResult, 4);
	
	authResult = Swap32IfLE(authResult);

	return authResult;
}

CARD32  ClientConnection::AuthSecureVNCPlugin()
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
					} else {
						bTriedNoPassword = true;

						AuthDialog ad;
						//adzm 2010-05-12 - passphrase
						ad.m_bPassphraseMode = bPassphraseRequired;
						
						if (ad.DoDialog(false))
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

		CARD32 authResult=rfbVncAuthFailed;	
		ReadExact((char *) &authResult, 4);		
		authResult = Swap32IfLE(authResult);
		return authResult;
}


CARD32  ClientConnection::AuthUltraVnc()
{
	CARD8 viewer_version=1;
	CARD8 server_version=0;
	WriteExact((char *)&viewer_version, sizeof(viewer_version));
	ReadExact((char *) &server_version, sizeof(server_version));

	if (server_version<1)
	{
		vnclog.Print(0, _T("Server not supported to old \n"));
		if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,"Server not supported to old");
		throw WarningException("Server not supported to old");
	}

	// Read Requested server Auth, we support NO,VNC,MSLOGON,SecureVNCPlugin
	// Read Special Handling SC_PROMP, SessionSelect 1=SC_PROMP, 2=SessionSelect

	CARD8 SubAuth=rfbConnFailed;
	CARD8 SpecialHandling=0;
	ReadExact((char *) &SubAuth, sizeof(SubAuth));
	ReadExact((char *) &SpecialHandling, sizeof(SpecialHandling));

	//Activate SecurePlugin
	if (m_fUsePlugin && m_pIntegratedPluginInterface && SubAuth != rfbConnFailed && SubAuth!=rfbSubSecureVNCPlugin) 
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
				int returnvalue=MessageBox(m_hwndMain, "You have specified an encryption plugin, however this connection in unencrypted! Do you want to continue?", "Accept insecure connection", MB_YESNO | MB_ICONEXCLAMATION | MB_TOPMOST);
				if (returnvalue==IDNO) 
				{
					throw WarningException("You refused the insecure connection.");
				}
			}
		}
	}

	if (rfbSubSpecialSC_PROMP==1)
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

		//adzm 2009-06-21 - auto-accept if specified
		if (!m_opts.m_fAutoAcceptIncoming) 
		{
		int returnvalue=MessageBox(m_hwndMain,   mytext,"Accept Incoming SC connection", MB_YESNO |  MB_TOPMOST);
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
		else
		{
			int nummer=1;
			WriteExact((char *)&nummer,sizeof(int));
		}		
	}

	CARD32 authResult=rfbVncAuthFailed;
	switch (SubAuth)
	{
		case rfbSubNoAuth:
			if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L92);
			vnclog.Print(0, _T("No authentication needed\n"));
			authResult=rfbVncAuthOK;
			break;

		case rfbSubVncAuth:
			authResult=AuthVnc();
			break;

		case rfbSubSecureVNCPlugin:
			AuthSecureVNCPluginUsed=true;
			authResult=AuthSecureVNCPlugin();
			break;

		case rfSubbMsLogon:
			authResult=AuthMSLOGON();
			break;
	}

	// For V2 only
	if (rfbSubSpecialSessionSelect==1 && (server_version>=2) && authResult!=rfbVncAuthFailed)
	{
		//TODO
	}

	return authResult;
}

void ClientConnection::HandleAuthResult(CARD32 authResult)
{

	switch (authResult) 
		{		
		case rfbVncAuthOK:
			if (m_hwndStatus)vnclog.Print(0, _T("VNC authentication succeeded\n"));
			if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L55);
		
			//adzm 2010-05-10
			if(AuthSecureVNCPluginUsed)
			{
			AuthSecureVNCPluginUsed=false;
			m_pIntegratedPluginInterface->SetHandshakeComplete();
			if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_PLUGIN_STATUS,m_pIntegratedPluginInterface->DescribeCurrentSettings());
			}

			g_passwordfailed=false;
			break;

		case rfbVncAuthFailed:
			vnclog.Print(0, _T("VNC authentication failed!"));

			if(!AuthSecureVNCPluginUsed)
			{
			 m_pApp->m_options.m_NoMoreCommandLineUserPassword = TRUE;					
			 g_passwordfailed=true;
			 if (m_minorVersion >= 8) ReadServerFailureMessage();
			}
			if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,sz_L56);
			SetEvent(KillEvent);
			throw WarningException(sz_L57,IDS_L57);
			break;

		case rfbVncAuthFailedEx: 
			{
				if(AuthSecureVNCPluginUsed)
					{
						AuthSecureVNCPluginUsed=false;
						CARD32 reasonLen;
						vnclog.Print(0, _T("VNC authentication failed! Extended information available."));	
						//adzm 2010-05-11 - Send an explanatory message for the failure (if any)
						ReadExact((char *)&reasonLen, 4);
						reasonLen = Swap32IfLE(reasonLen);					
						CheckBufferSize(reasonLen+1);
						ReadString(m_netbuf, reasonLen);			
						vnclog.Print(0, _T("VNC authentication failed! Extended information: %s\n"), m_netbuf);
						if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_STATUS,m_netbuf);
						throw WarningException(m_netbuf);
					}
			}
			break;

		case rfbVncAuthTooMany:
			SetEvent(KillEvent);
			throw WarningException(sz_L58);
			break;

		case rfbMsLogon:
			if(AuthSecureVNCPluginUsed)
				{
					//adzm 2010-05-10
					AuthSecureVNCPluginUsed=false;
					m_pIntegratedPluginInterface->SetHandshakeComplete();
					if (m_hwndStatus)SetDlgItemText(m_hwndStatus,IDC_PLUGIN_STATUS,m_pIntegratedPluginInterface->DescribeCurrentSettings());
					// ????
					//Call this function for a second time with authresult2
					// AuthSecureVNCPluginUsed is needed to prevent a second m_pIntegratedPluginInterface->SetHandshakeComplete();
					CARD32 authResult2;
					authResult2=AuthMSLOGON();
					HandleAuthResult(authResult2);
				}
			break;

		default:
			vnclog.Print(0, _T("Unknown VNC authentication result: %d\n"),(int)authResult);
			throw ErrorException(sz_L59,IDS_L59);
			break;
		}
	AuthSecureVNCPluginUsed=false;
}
#endif