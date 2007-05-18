//  Copyright (C) 2002 Ultr@VNC Team Members. All Rights Reserved.
//  Copyright (C) 2000-2002 Const Kaplinsky. All Rights Reserved.
//  Copyright (C) 2002 TightVNC. All Rights Reserved.
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


// vncProperties.cpp

// Implementation of the Properties dialog!

#include "stdhdrs.h"
#include "lmcons.h"
#include "vncService.h"

#include "WinVNC.h"
#include "vncProperties.h"
#include "vncServer.h"
//#include "vncPasswd.h"
#include "vncOSVersion.h"
//#include "vncSecurityEditor.h"

#include "localization.h" // ACT : Add localization on messages

//extern HINSTANCE g_hInst;

const char WINVNC_REGISTRY_KEY [] = "Software\\ORL\\WinVNC3";

// Marscha@2004 - authSSP: Function pointer for dyn. linking
typedef void (*vncEditSecurityFn) (HWND hwnd, HINSTANCE hInstance);
vncEditSecurityFn vncEditSecurity = 0;
extern BOOL g_plugin;

// Constructor & Destructor
vncProperties::vncProperties()
{
  m_alloweditclients = TRUE;
	m_allowproperties = TRUE;
	m_allowshutdown = TRUE;
	m_dlgvisible = FALSE;
	m_usersettings = TRUE;
}

vncProperties::~vncProperties()
{
}

// Initialisation
BOOL
vncProperties::Init(vncServer *server)
{
	// Save the server pointer
	m_server = server;

	vnclog.Print(LL_INTINFO, VNCLOG("loading local-only settings\n"));
	vnclog.SetMode(0);
	vnclog.SetLevel(0);
	m_pref_RequireMSLogon=false;
	m_pref_NewMSLogon = false;

	m_pref_AutoPortSelect=TRUE;
	m_pref_PortNumber = RFB_PORT_OFFSET; 
	m_pref_SockConnect=TRUE;
	m_pref_QuerySetting=2;
	m_pref_QueryTimeout=10;
	m_pref_QueryAccept=0;
	m_pref_IdleTimeout=0;
	m_pref_EnableRemoteInputs=TRUE;
	m_pref_DisableLocalInputs=FALSE;
	m_pref_LockSettings=-1;
	m_pref_RemoveWallpaper=TRUE;
    m_alloweditclients = TRUE;
	m_allowshutdown = TRUE;
	m_allowproperties = TRUE;
	m_pref_SingleWindow = FALSE;
	m_pref_UseDSMPlugin = g_plugin;
	*m_pref_szDSMPlugin = '\0';
	m_pref_EnableFileTransfer = TRUE;
	m_pref_EnableBlankMonitor = TRUE;
	m_pref_DefaultScale = 1;
	if(OSVersion()==1)
	{
	m_pref_CaptureAlphaBlending = TRUE; 
	m_pref_BlackAlphaBlending = TRUE; 
	}
	else
	{
		m_pref_CaptureAlphaBlending = false; 
		m_pref_BlackAlphaBlending = false; 
	}

	// Make the loaded settings active..
	m_server->SetDisableTrayIcon(0);
	m_server->SetLoopbackOnly(true);
	m_server->SetLoopbackOk(true);
	m_server->SetLoopbackOk(true);
	m_server->SetAuthRequired(false);
	m_server->SetConnectPriority(3);
	m_server->SetAuthHosts(0);;
	m_server->SetNewMSLogon(m_pref_NewMSLogon);
	m_server->RequireMSLogon(m_pref_RequireMSLogon);
	m_server->EnableFileTransfer(m_pref_EnableFileTransfer);
	m_server->BlankMonitorEnabled(m_pref_EnableBlankMonitor);
	m_server->SetDefaultScale(m_pref_DefaultScale);
	m_server->SetQuerySetting(4);
	m_server->SetQueryTimeout(10);
	m_server->SetQueryAccept(1);
	m_server->SetAutoIdleDisconnectTimeout(m_pref_IdleTimeout);
	m_server->EnableRemoveWallpaper(m_pref_RemoveWallpaper);
	m_server->SockConnect(0);
	m_server->BlackAlphaBlending(m_pref_BlackAlphaBlending);
	m_server->CaptureAlphaBlending(m_pref_CaptureAlphaBlending);

	// Are inputs being disabled?
	if (!m_pref_EnableRemoteInputs)
		m_server->EnableRemoteInputs(m_pref_EnableRemoteInputs);
	if (m_pref_DisableLocalInputs)
		m_server->DisableLocalInputs(m_pref_DisableLocalInputs);

	// Update the password
//	m_server->SetPassword(m_pref_passwd);

	// Now change the listening port settings
	m_server->SetAutoPortSelect(m_pref_AutoPortSelect);
	if (!m_pref_AutoPortSelect)
		// m_server->SetPort(m_pref_PortNumber);
	m_server->SetPorts(m_pref_PortNumber, m_pref_HttpPortNumber); // Tight 1.2.7

	m_server->SockConnect(0);


	// Remote access prefs
	m_server->EnableRemoteInputs(m_pref_EnableRemoteInputs);
	m_server->SetLockSettings(m_pref_LockSettings);
	m_server->DisableLocalInputs(m_pref_DisableLocalInputs);

	// DSM Plugin prefs
	m_server->EnableDSMPlugin(m_pref_UseDSMPlugin);
	m_server->SetDSMPluginName("MSRC4Plugin.dsm");
	if (m_server->IsDSMPluginEnabled()) m_server->SetDSMPlugin();

	return TRUE;
}



