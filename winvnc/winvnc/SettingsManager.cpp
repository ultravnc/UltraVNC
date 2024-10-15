/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
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


#include "SettingsManager.h"
#include <common/rfb.h>
#include "vncpasswd.h"
#include <VersionHelpers.h>
#include <userenv.h>
#include "credentials.h"
#include <tchar.h>
#include <shlobj.h>
#include <direct.h>
#define SODIUM_STATIC
#include <sodium.h>
#pragma comment(lib, "libsodium.lib")

SettingsManager* SettingsManager::s_instance = NULL;
SettingsManager* settings = SettingsManager::getInstance();

SettingsManager* SettingsManager::getInstance()
{
	if (!s_instance) {
		s_instance = new SettingsManager;
	}
	return s_instance;
}

SettingsManager::SettingsManager()
{
	DesktopUsersToken desktopUsersToken;
	HANDLE hPToken = desktopUsersToken.getDesktopUsersToken();
	int iImpersonateResult = 0;

	if (hPToken != NULL) {
		if (!ImpersonateLoggedOnUser(hPToken))
			iImpersonateResult = GetLastError();
	}

	sodium_init();
	setDefaults();

	char path[MAX_PATH];
	if (RunningFromExternalService()) {
		
		HRESULT result = SHGetFolderPathA(NULL, CSIDL_COMMON_APPDATA, NULL, 0, path);
		if (!SUCCEEDED(result)) {
			exit(0); //you can't run without a path to the ini file
		}
	}
	else {
		HRESULT result = SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path);
		if (!SUCCEEDED(result)) {
			exit(0); //you can't run without a path to the ini file
		}
	}

	strcpy_s(m_Inifile, "");
	strcat_s(m_Inifile, path);//set the directory
	strcat_s(m_Inifile, "\\UltraVNC");
	_mkdir(m_Inifile);
	strcat_s(m_Inifile, "\\");
	strcat_s(m_Inifile, INIFILE_NAME);
	myIniFile.setIniFile(m_Inifile);
	load();
	if (iImpersonateResult == ERROR_SUCCESS)
		RevertToSelf();
}

void SettingsManager::setRunningFromExternalService(BOOL fEnabled) 
{ 
	m_pref_fRunningFromExternalService = fEnabled; 
	char path[MAX_PATH];
	if (RunningFromExternalService()) {

		HRESULT result = SHGetFolderPathA(NULL, CSIDL_COMMON_APPDATA, NULL, 0, path);
		if (!SUCCEEDED(result)) {
			exit(0); //you can't run without a path to the ini file
		}
	}
	else {
		HRESULT result = SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path);
		if (!SUCCEEDED(result)) {
			exit(0); //you can't run without a path to the ini file
		}
	}

	strcpy_s(m_Inifile, "");
	strcat_s(m_Inifile, path);//set the directory
	strcat_s(m_Inifile, "\\UltraVNC");
	_mkdir(m_Inifile);
	strcat_s(m_Inifile, "\\");
	strcat_s(m_Inifile, INIFILE_NAME);
	myIniFile.setIniFile(m_Inifile);
};

#pragma comment(lib, "wtsapi32.lib")
#pragma comment(lib, "advapi32.lib")

#include <sddl.h>
#include <wtsapi32.h>

bool SettingsManager::IsDesktopUserAdmin()
{
	DesktopUsersToken desktopUsersToken;
	HANDLE hPToken = desktopUsersToken.getDesktopUsersToken();
	if (hPToken) {
		if (!ImpersonateLoggedOnUser(hPToken)) {
			return false;
		}
	}

	bool isAdmin = myIniFile.IsWritable();

	RevertToSelf();
	return isAdmin;
}

bool SettingsManager::getAllowUserSettingsWithPassword()
{
	return m_pref_AllowUserSettingsWithPassword;
}

void  SettingsManager::setAllowUserSettingsWithPassword(bool value)
{
	m_pref_AllowUserSettingsWithPassword = value;
}


void SettingsManager::setDefaults()
{
	memset(reinterpret_cast<void*>(m_pref_DSMPluginConfig), 0, sizeof(m_pref_DSMPluginConfig));
#ifdef SC_20
	strcpy_s(m_pref_DSMPluginConfig, "DSMPluginConfig = SecureVNC; 0; 0x00104001;");
#endif // SC_20
	memset(reinterpret_cast<void*>(m_pref_service_commandline), 0, sizeof(m_pref_service_commandline));
	memset(reinterpret_cast<void*>(m_pref_accept_reject_mesg), 0, sizeof(m_pref_accept_reject_mesg));
	memset(reinterpret_cast<void*>(m_pref_passwd), 0, sizeof(m_pref_passwd));
	memset(reinterpret_cast<void*>(m_pref_passwd2), 0, sizeof(m_pref_passwd2));
#ifndef SC_20
	memset(reinterpret_cast<void*>(m_pref_szDSMPlugin), 0, sizeof(m_pref_szDSMPlugin));
#else
	strcpy_s(m_pref_szDSMPlugin, "SecureVNCPlugin.dsm");
#endif // SC_20
	
	memset(reinterpret_cast<void*>(m_pref_authhosts), 0, sizeof(m_pref_authhosts));

	m_pref_alloweditclients = TRUE;
	m_pref_allowproperties = TRUE;
	m_pref_allowInjection = FALSE;
	m_pref_allowshutdown = TRUE;
	m_pref_ftTimeout = FT_RECV_TIMEOUT;
	m_pref_keepAliveInterval = KEEPALIVE_INTERVAL;
	m_pref_IdleInputTimeout = 0;
	m_pref_Primary = true;
	m_pref_Secondary = false;
#ifndef SC_20
	m_pref_AutoPortSelect = TRUE;
	m_pref_EnableHTTPConnect = TRUE;
	m_pref_PortNumber = RFB_PORT_OFFSET;
	m_pref_EnableConnection = TRUE;
	m_pref_HttpPortNumber = DISPLAY_TO_HPORT(PORT_TO_DISPLAY(m_pref_PortNumber));
#else
	m_pref_AutoPortSelect = false;
	m_pref_EnableHTTPConnect = false;
	m_pref_PortNumber = RFB_PORT_OFFSET;
	m_pref_EnableConnection = false;
	m_pref_HttpPortNumber = DISPLAY_TO_HPORT(PORT_TO_DISPLAY(m_pref_PortNumber));
#endif // SC_20

	m_pref_QuerySetting = 2;
	m_pref_QueryTimeout = 10;
	m_pref_QueryDisableTime = 0;
	m_pref_QueryAccept = 0;
	m_pref_IdleTimeout = 0;
	m_pref_MaxViewerSetting = 0;
	m_pref_MaxViewers = 128;
	m_pref_EnableRemoteInputs = TRUE;
	m_pref_DisableLocalInputs = FALSE;
	m_pref_EnableJapInput = FALSE;
	m_pref_EnableUnicodeInput = FALSE;
	m_pref_EnableWin8Helper = FALSE;
	m_pref_clearconsole = FALSE;
	m_pref_LockSettings = -1;
	m_pref_Collabo = false;
#ifndef SC_20
	m_pref_Frame = FALSE;
	m_pref_Notification = FALSE;
#else
	m_pref_Frame = true;
	m_pref_Notification = true;
#endif // SC_20
	m_pref_OSD = FALSE;
	m_pref_NotificationSelection = 0;
	m_pref_RemoveWallpaper = TRUE;
	m_pref_RemoveEffects = FALSE;
	m_pref_RemoveFontSmoothing = FALSE;
	m_pref_alloweditclients = TRUE;
	m_pref_allowshutdown = TRUE;
	m_pref_allowproperties = TRUE;
	m_pref_allowInjection = FALSE;
	m_pref_UseDSMPlugin = FALSE;
	m_pref_EnableFileTransfer = TRUE;
	m_pref_FTUserImpersonation = TRUE;
	m_pref_EnableBlankMonitor = TRUE;
	m_pref_BlankInputsOnly = FALSE;
	m_pref_QueryIfNoLogon = 1;
	m_pref_DefaultScale = 1;
	m_pref_RequireMSLogon = false;
	m_pref_Secure = false;
	m_pref_NewMSLogon = false;
	m_pref_ReverseAuthRequired = true;

	m_pref_DisableTrayIcon = false;
	m_pref_Rdpmode = 0;
	m_pref_Noscreensaver = 0;
#ifdef SC_20
	m_pref_Noscreensaver = 1;	
#endif // SC_20
	m_pref_LoopbackOnly = false;
	m_pref_AllowLoopback = true;
	m_pref_AuthRequired = true;
#ifdef SC_20
	m_pref_AuthRequired = false;
#endif // SC_20
	m_pref_ConnectPriority = 0;

	m_pref_DebugMode = 0;
	memset(reinterpret_cast<void*>(m_pref_DebugPath), 0, sizeof(m_pref_DebugPath));
	m_pref_DebugLevel = 0;
	m_pref_Avilog = 0;
	m_pref_UseIpv6 = 0;
	// ethernet packet 1500 - 40 tcp/ip header - 8 PPPoE info
//unsigned int G_SENDBUFFER=8192;
	G_SENDBUFFER_EX = 1452;

	m_pref_fEnableStateUpdates = false;
	m_pref_fEnableKeepAlive = false;
	m_pref_fRunningFromExternalService = false;
	m_pref_fRunningFromExternalServiceRdp = false;
	m_pref_fAutoRestart = false;
#ifndef SC_20
	m_pref_ScExit = false;
	m_pref_ScPrompt = false;
#else
	m_pref_ScExit = true;
	m_pref_ScPrompt = true;

#endif // SC_20

	m_pref_ddEngine = IsWindows8OrGreater();
	m_pref_TurboMode = TRUE;
	m_pref_PollUnderCursor = FALSE;
	m_pref_PollForeground = FALSE;
	m_pref_PollFullScreen = TRUE;
	m_pref_PollConsoleOnly = FALSE;
	m_pref_PollOnEventOnly = FALSE;
	m_pref_MaxCpu = 100;
	m_pref_MaxFPS = 25;
	m_pref_Driver = CheckVideoDriver(0);
	m_pref_Hook = TRUE;
	m_pref_Virtual = FALSE;
	m_pref_autocapt = 1;
	m_pref_ipv6_allowed = false;

	m_pref_RunninAsAdministrator = false;

	memset(reinterpret_cast<void*>(m_pref_group1), 0, sizeof(m_pref_group1));
	memset(reinterpret_cast<void*>(m_pref_group2), 0, sizeof(m_pref_group2));
	memset(reinterpret_cast<void*>(m_pref_group3), 0, sizeof(m_pref_group3));
	m_pref_locdom1 = false;
	m_pref_locdom2 = false;
	m_pref_locdom3 = false;

	memset(m_pref_cloudServer, 0, MAX_HOST_NAME_LEN);
	m_pref_cloudEnabled = false;
	m_pref_AllowUserSettingsWithPassword = false;

};

void SettingsManager::load()
{
	m_pref_RemoveWallpaper = myIniFile.ReadInt("admin", "RemoveWallpaper", m_pref_RemoveWallpaper);
	m_pref_RemoveEffects = myIniFile.ReadInt("admin", "RemoveEffects", m_pref_RemoveEffects);
	m_pref_RemoveFontSmoothing = myIniFile.ReadInt("admin", "RemoveFontSmoothing", m_pref_RemoveFontSmoothing);
#ifndef SC_20
	// Disable Tray icon
	m_pref_DisableTrayIcon = myIniFile.ReadInt("admin", "DisableTrayIcon", m_pref_DisableTrayIcon);
	m_pref_AllowUserSettingsWithPassword = myIniFile.ReadInt("admin", "AllowUserSettingsWithPassword", m_pref_AllowUserSettingsWithPassword);
	m_pref_Rdpmode = myIniFile.ReadInt("admin", "rdpmode", m_pref_Rdpmode);
	m_pref_Noscreensaver = myIniFile.ReadInt("admin", "noscreensaver", m_pref_Noscreensaver);
	m_pref_LoopbackOnly = myIniFile.ReadInt("admin", "LoopbackOnly", m_pref_LoopbackOnly);
	m_pref_Secure = myIniFile.ReadInt("admin", "Secure", m_pref_Secure);
	m_pref_RequireMSLogon = myIniFile.ReadInt("admin", "MSLogonRequired", m_pref_RequireMSLogon);
	m_pref_NewMSLogon = myIniFile.ReadInt("admin", "NewMSLogon", m_pref_NewMSLogon);
	m_pref_UseDSMPlugin = myIniFile.ReadInt("admin", "UseDSMPlugin", m_pref_UseDSMPlugin);
	myIniFile.ReadString("admin", "DSMPlugin", m_pref_szDSMPlugin, 128);
	m_pref_ReverseAuthRequired = myIniFile.ReadInt("admin", "ReverseAuthRequired", m_pref_ReverseAuthRequired);
	myIniFile.ReadString("admin", "DSMPluginConfig", m_pref_DSMPluginConfig, 512);
	m_pref_ipv6_allowed = myIniFile.ReadInt("admin", "UseIpv6", m_pref_ipv6_allowed);
	m_pref_AllowLoopback = myIniFile.ReadInt("admin", "AllowLoopback", m_pref_AllowLoopback);
	m_pref_AuthRequired = myIniFile.ReadInt("admin", "AuthRequired", m_pref_AuthRequired);
	m_pref_ConnectPriority = myIniFile.ReadInt("admin", "ConnectPriority", m_pref_ConnectPriority);
	myIniFile.ReadString("admin", "AuthHosts", m_pref_authhosts, 1280);
	myIniFile.ReadString("admin", "AuthHosts", m_pref_authhosts2, 1280);
	m_pref_allowshutdown = myIniFile.ReadInt("admin", "AllowShutdown", m_pref_allowshutdown);
	m_pref_allowproperties = myIniFile.ReadInt("admin", "AllowProperties", m_pref_allowproperties);
	m_pref_allowInjection = myIniFile.ReadInt("admin", "AllowInjection", m_pref_allowInjection);
	m_pref_alloweditclients = myIniFile.ReadInt("admin", "AllowEditClients", m_pref_alloweditclients);
	m_pref_ftTimeout = myIniFile.ReadInt("admin", "FileTransferTimeout", m_pref_ftTimeout);
	if (m_pref_ftTimeout > 600)
		m_pref_ftTimeout = 600;
	m_pref_keepAliveInterval = myIniFile.ReadInt("admin", "KeepAliveInterval", m_pref_keepAliveInterval);
	m_pref_IdleInputTimeout = myIniFile.ReadInt("admin", "IdleInputTimeout", m_pref_IdleInputTimeout);
	if (m_pref_keepAliveInterval >= (m_pref_ftTimeout - KEEPALIVE_HEADROOM))
		m_pref_keepAliveInterval = m_pref_ftTimeout - KEEPALIVE_HEADROOM;
	myIniFile.ReadString("admin", "service_commandline", m_pref_service_commandline, 1024);
	myIniFile.ReadString("admin", "accept_reject_mesg", m_pref_accept_reject_mesg, 512);
	vncPasswd::FromClear crypt(m_pref_Secure);
	memcpy(m_pref_passwd, crypt, MAXPWLEN);
	m_pref_DebugMode = myIniFile.ReadInt("admin", "DebugMode", m_pref_DebugMode);
	myIniFile.ReadString("admin", "path", m_pref_DebugPath, 512);
	m_pref_DebugLevel = myIniFile.ReadInt("admin", "DebugLevel", m_pref_DebugLevel);
	m_pref_Avilog = myIniFile.ReadInt("admin", "Avilog", m_pref_Avilog);
	m_pref_UseIpv6 = myIniFile.ReadInt("admin", "UseIpv6", m_pref_UseIpv6);
	m_pref_EnableFileTransfer = myIniFile.ReadInt("admin", "FileTransferEnabled", m_pref_EnableFileTransfer);
	m_pref_FTUserImpersonation = myIniFile.ReadInt("admin", "FTUserImpersonation", m_pref_FTUserImpersonation); // sf@2005
	m_pref_EnableBlankMonitor = myIniFile.ReadInt("admin", "BlankMonitorEnabled", m_pref_EnableBlankMonitor);
	m_pref_BlankInputsOnly = myIniFile.ReadInt("admin", "BlankInputsOnly", m_pref_BlankInputsOnly); //PGM
	m_pref_DefaultScale = myIniFile.ReadInt("admin", "DefaultScale", m_pref_DefaultScale);
	m_pref_UseDSMPlugin = myIniFile.ReadInt("admin", "UseDSMPlugin", m_pref_UseDSMPlugin);
	myIniFile.ReadString("admin", "DSMPlugin", m_pref_szDSMPlugin, 128);
	myIniFile.ReadString("admin", "DSMPluginConfig", m_pref_DSMPluginConfig, 512);
	m_pref_Primary = myIniFile.ReadInt("admin", "primary", m_pref_Primary);
	m_pref_Secondary = myIniFile.ReadInt("admin", "secondary", m_pref_Secondary);
	m_pref_EnableConnection = myIniFile.ReadInt("admin", "SocketConnect", m_pref_EnableConnection);
	m_pref_EnableHTTPConnect = myIniFile.ReadInt("admin", "HTTPConnect", m_pref_EnableHTTPConnect);
	m_pref_AutoPortSelect = myIniFile.ReadInt("admin", "AutoPortSelect", m_pref_AutoPortSelect);
	m_pref_PortNumber = myIniFile.ReadInt("admin", "PortNumber", m_pref_PortNumber);
	m_pref_HttpPortNumber = myIniFile.ReadInt("admin", "HTTPPortNumber", DISPLAY_TO_HPORT(PORT_TO_DISPLAY(m_pref_PortNumber)));
	m_pref_IdleTimeout = myIniFile.ReadInt("admin", "IdleTimeout", m_pref_IdleTimeout);
	m_pref_QuerySetting = myIniFile.ReadInt("admin", "QuerySetting", m_pref_QuerySetting);
	m_pref_QueryTimeout = myIniFile.ReadInt("admin", "QueryTimeout", m_pref_QueryTimeout);
	m_pref_QueryDisableTime = myIniFile.ReadInt("admin", "QueryDisableTime", m_pref_QueryDisableTime);
	m_pref_QueryAccept = myIniFile.ReadInt("admin", "QueryAccept", m_pref_QueryAccept);
	m_pref_MaxViewerSetting = myIniFile.ReadInt("admin", "MaxViewerSetting", m_pref_MaxViewerSetting);
	m_pref_MaxViewers = myIniFile.ReadInt("admin", "MaxViewers", m_pref_MaxViewers);
	m_pref_Collabo = myIniFile.ReadInt("admin", "Collabo", m_pref_Collabo);
	m_pref_Frame = myIniFile.ReadInt("admin", "Frame", m_pref_Frame);
	m_pref_Notification = myIniFile.ReadInt("admin", "Notification", m_pref_Notification);
	m_pref_OSD = myIniFile.ReadInt("admin", "OSD", m_pref_OSD);
	m_pref_NotificationSelection = myIniFile.ReadInt("admin", "NotificationSelection", m_pref_NotificationSelection);
	m_pref_QueryIfNoLogon = myIniFile.ReadInt("admin", "QueryIfNoLogon", m_pref_QueryIfNoLogon);
	myIniFile.ReadPassword(m_pref_passwd, MAXPWLEN);
	myIniFile.ReadPassword2(m_pref_passwd2, MAXPWLEN); //PGM
	m_pref_EnableRemoteInputs = myIniFile.ReadInt("admin", "InputsEnabled", m_pref_EnableRemoteInputs);
	m_pref_LockSettings = myIniFile.ReadInt("admin", "LockSetting", m_pref_LockSettings);
	m_pref_DisableLocalInputs = myIniFile.ReadInt("admin", "LocalInputsDisabled", m_pref_DisableLocalInputs);
	m_pref_EnableJapInput = myIniFile.ReadInt("admin", "EnableJapInput", m_pref_EnableJapInput);
	m_pref_EnableUnicodeInput = myIniFile.ReadInt("admin", "EnableUnicodeInput", m_pref_EnableUnicodeInput);
	m_pref_EnableWin8Helper = myIniFile.ReadInt("admin", "EnableWin8Helper", m_pref_EnableWin8Helper);
	m_pref_clearconsole = myIniFile.ReadInt("admin", "clearconsole", m_pref_clearconsole);
	G_SENDBUFFER_EX = myIniFile.ReadInt("admin", "sendbuffer", G_SENDBUFFER_EX);	
	_tcscpy_s(m_pref_group1, "VNCACCESS");
	myIniFile.ReadString("admin_auth", "group1", m_pref_group1, 150);
	_tcscpy_s(m_pref_group2, "Administrators");
	myIniFile.ReadString("admin_auth", "group2", m_pref_group2, 150);	
	_tcscpy_s(m_pref_group3, "VNCVIEWONLY");
	myIniFile.ReadString("admin_auth", "group3", m_pref_group3, 150);

	myIniFile.ReadString("admin", "cloudServer", m_pref_cloudServer, MAX_HOST_NAME_LEN);
	m_pref_cloudEnabled = myIniFile.ReadInt("admin", "cloudEnabled", m_pref_cloudEnabled);


	m_pref_locdom1 = myIniFile.ReadInt("admin_auth", "locdom1", m_pref_locdom1);
	m_pref_locdom2 = myIniFile.ReadInt("admin_auth", "locdom2", m_pref_locdom2);
	m_pref_locdom3 = myIniFile.ReadInt("admin_auth", "locdom3", m_pref_locdom3);
#endif // SC_20
	m_pref_TurboMode = myIniFile.ReadInt("poll", "TurboMode", m_pref_TurboMode);
	m_pref_PollUnderCursor = myIniFile.ReadInt("poll", "PollUnderCursor", m_pref_PollUnderCursor);
	m_pref_PollForeground = myIniFile.ReadInt("poll", "PollForeground", m_pref_PollForeground);
	m_pref_PollFullScreen = myIniFile.ReadInt("poll", "PollFullScreen", m_pref_PollFullScreen);
	m_pref_PollConsoleOnly = myIniFile.ReadInt("poll", "OnlyPollConsole", m_pref_PollConsoleOnly);
	m_pref_PollOnEventOnly = myIniFile.ReadInt("poll", "OnlyPollOnEvent", m_pref_PollOnEventOnly);
	m_pref_MaxCpu = myIniFile.ReadInt("poll", "MaxCpu2", m_pref_MaxCpu);
	if (m_pref_MaxCpu == 0)
		m_pref_MaxCpu = 100;
	m_pref_MaxFPS = myIniFile.ReadInt("poll", "MaxFPS", m_pref_MaxFPS);
	m_pref_Driver = myIniFile.ReadInt("poll", "EnableDriver", m_pref_Driver);
	if (m_pref_Driver)
		m_pref_Driver = CheckVideoDriver(0);
	m_pref_Hook = myIniFile.ReadInt("poll", "EnableHook", m_pref_Hook);
	m_pref_Virtual = myIniFile.ReadInt("poll", "EnableVirtual", m_pref_Virtual);
	m_pref_autocapt = myIniFile.ReadInt("poll", "autocapt", m_pref_autocapt);
}

void SettingsManager::save()
{
	if (!getAllowProperties())
		return;

	// SAVE PER-USER PREFS IF ALLOWED	
	// Modif sf@2002
	myIniFile.WriteInt("admin", "AllowUserSettingsWithPassword", m_pref_AllowUserSettingsWithPassword);
	myIniFile.WriteInt("admin", "FileTransferEnabled", m_pref_EnableFileTransfer);
	myIniFile.WriteInt("admin", "FTUserImpersonation", m_pref_FTUserImpersonation); // sf@2005
	myIniFile.WriteInt("admin", "BlankMonitorEnabled", m_pref_EnableBlankMonitor);
	myIniFile.WriteInt("admin", "BlankInputsOnly", m_pref_BlankInputsOnly); //PGM
	myIniFile.WriteInt("admin", "DefaultScale", m_pref_DefaultScale);
	myIniFile.WriteInt("admin", "UseDSMPlugin", m_pref_UseDSMPlugin);
	myIniFile.WriteString("admin", "DSMPlugin", m_pref_szDSMPlugin);
	myIniFile.WriteString("admin", "AuthHosts", m_pref_authhosts);
	myIniFile.WriteInt("admin", "primary", m_pref_Primary);
	myIniFile.WriteInt("admin", "secondary", m_pref_Secondary);
	myIniFile.WriteInt("admin", "SocketConnect", m_pref_EnableConnection);
	myIniFile.WriteInt("admin", "HTTPConnect", m_pref_EnableHTTPConnect);
	myIniFile.WriteInt("admin", "AutoPortSelect", m_pref_AutoPortSelect);
	if (!m_pref_AutoPortSelect) {
		myIniFile.WriteInt("admin", "PortNumber", m_pref_PortNumber);
		myIniFile.WriteInt("admin", "HTTPPortNumber", m_pref_HttpPortNumber);
	}
	myIniFile.WriteInt("admin", "InputsEnabled", m_pref_EnableRemoteInputs);
	myIniFile.WriteInt("admin", "LocalInputsDisabled", m_pref_DisableLocalInputs);
	myIniFile.WriteInt("admin", "IdleTimeout", m_pref_IdleTimeout);
	myIniFile.WriteInt("admin", "EnableJapInput", m_pref_EnableJapInput);
	myIniFile.WriteInt("admin", "EnableUnicodeInput", m_pref_EnableUnicodeInput);
	myIniFile.WriteInt("admin", "EnableWin8Helper", m_pref_EnableWin8Helper);
	myIniFile.WriteInt("admin", "QuerySetting", m_pref_QuerySetting);
	myIniFile.WriteInt("admin", "QueryTimeout", m_pref_QueryTimeout);
	myIniFile.WriteInt("admin", "QueryDisableTime", m_pref_QueryDisableTime);
	myIniFile.WriteInt("admin", "QueryAccept", m_pref_QueryAccept);
	myIniFile.WriteInt("admin", "MaxViewerSetting", m_pref_MaxViewerSetting);
	myIniFile.WriteInt("admin", "MaxViewers", m_pref_MaxViewers);
	myIniFile.WriteInt("admin", "Collabo", m_pref_Collabo);
	myIniFile.WriteInt("admin", "Frame", m_pref_Frame);
	myIniFile.WriteInt("admin", "Notification", m_pref_Notification);
	myIniFile.WriteInt("admin", "OSD", m_pref_OSD);
	myIniFile.WriteInt("admin", "NotificationSelection", m_pref_NotificationSelection);
	myIniFile.WriteInt("admin", "QueryIfNoLogon", m_pref_QueryIfNoLogon);
	myIniFile.WriteInt("admin", "LockSetting", m_pref_LockSettings);
	myIniFile.WriteInt("admin", "RemoveWallpaper", m_pref_RemoveWallpaper);
	myIniFile.WriteInt("admin", "RemoveEffects", m_pref_RemoveEffects);
	myIniFile.WriteInt("admin", "RemoveFontSmoothing", m_pref_RemoveFontSmoothing);

	myIniFile.WritePassword(m_pref_passwd);
	myIniFile.WritePassword2(m_pref_passwd2);

	myIniFile.WriteInt("admin", "DebugMode", vnclog.GetMode());
	myIniFile.WriteInt("admin", "Avilog", vnclog.GetVideo());
	myIniFile.WriteString("admin", "path", vnclog.GetPath());
	myIniFile.WriteInt("admin", "DebugLevel", vnclog.GetLevel());
	myIniFile.WriteInt("admin", "AllowLoopback", m_pref_AllowLoopback);
	myIniFile.WriteInt("admin", "UseIpv6", settings->getIPV6());
	myIniFile.WriteInt("admin", "LoopbackOnly", m_pref_LoopbackOnly);
	myIniFile.WriteInt("admin", "AllowShutdown", m_pref_allowshutdown);
	myIniFile.WriteInt("admin", "AllowProperties", m_pref_allowproperties);
	myIniFile.WriteInt("admin", "AllowInjection", m_pref_allowInjection);
	myIniFile.WriteInt("admin", "AllowEditClients", m_pref_alloweditclients);
	myIniFile.WriteInt("admin", "FileTransferTimeout", m_pref_ftTimeout);
	myIniFile.WriteInt("admin", "KeepAliveInterval", m_pref_keepAliveInterval);
	myIniFile.WriteInt("admin", "IdleInputTimeout", m_pref_IdleInputTimeout);
	myIniFile.WriteInt("admin", "DisableTrayIcon", m_pref_DisableTrayIcon);
	myIniFile.WriteInt("admin", "rdpmode", m_pref_Rdpmode);
	myIniFile.WriteInt("admin", "noscreensaver", m_pref_Noscreensaver);
	myIniFile.WriteInt("admin", "Secure", m_pref_Secure);
	myIniFile.WriteInt("admin", "MSLogonRequired", m_pref_RequireMSLogon);
	myIniFile.WriteInt("admin", "NewMSLogon", m_pref_NewMSLogon);
	myIniFile.WriteInt("admin", "ReverseAuthRequired", m_pref_ReverseAuthRequired);
	myIniFile.WriteInt("admin", "ConnectPriority", m_pref_ConnectPriority);
	myIniFile.WriteInt("admin", "AuthRequired", m_pref_AuthRequired);
	myIniFile.WriteString("admin", "service_commandline", m_pref_service_commandline);
	myIniFile.WriteString("admin", "accept_reject_mesg", m_pref_accept_reject_mesg);
	myIniFile.WriteInt("poll", "TurboMode", m_pref_TurboMode);
	myIniFile.WriteInt("poll", "PollUnderCursor", m_pref_PollUnderCursor);
	myIniFile.WriteInt("poll", "PollForeground", m_pref_PollForeground);
	myIniFile.WriteInt("poll", "PollFullScreen", m_pref_PollFullScreen);
	myIniFile.WriteInt("poll", "OnlyPollConsole", m_pref_PollConsoleOnly);
	myIniFile.WriteInt("poll", "OnlyPollOnEvent", m_pref_PollOnEventOnly);
	myIniFile.WriteInt("poll", "MaxCpu2", m_pref_MaxCpu);
	myIniFile.WriteInt("poll", "MaxFPS", m_pref_MaxFPS);
	myIniFile.WriteInt("poll", "EnableDriver", m_pref_Driver);
	myIniFile.WriteInt("poll", "EnableHook", m_pref_Hook);
	myIniFile.WriteInt("poll", "EnableVirtual", m_pref_Virtual);
	myIniFile.WriteInt("poll", "autocapt", m_pref_autocapt);

	myIniFile.WriteString("admin", "cloudServer", m_pref_cloudServer);
	myIniFile.WriteInt("admin", "cloudEnabled", m_pref_cloudEnabled);

	myIniFile.WriteString("admin_auth", "group1", m_pref_group1);
	myIniFile.WriteString("admin_auth", "group2", m_pref_group2);
	myIniFile.WriteString("admin_auth", "group3", m_pref_group3);

	myIniFile.WriteInt("admin_auth", "locdom1", m_pref_locdom1);
	myIniFile.WriteInt("admin_auth", "locdom2", m_pref_locdom2);
	myIniFile.WriteInt("admin_auth", "locdom3", m_pref_locdom3);	
}

void SettingsManager::setkeepAliveInterval(int secs) {
	m_pref_keepAliveInterval = secs;
	if (m_pref_keepAliveInterval >= (m_pref_ftTimeout - KEEPALIVE_HEADROOM))
		m_pref_keepAliveInterval = m_pref_ftTimeout - KEEPALIVE_HEADROOM;
}

void SettingsManager::setIdleTimeout(int secs) {
	m_pref_IdleTimeout = secs;
}

void SettingsManager::setIdleInputTimeout(int secs) {
	m_pref_IdleInputTimeout= secs;
}
static bool notset = false;
bool SettingsManager::IsRunninAsAdministrator()
{
	if (!notset)
		m_pref_RunninAsAdministrator = Credentials::RunningAsAdministrator();
	notset = true;
	return m_pref_RunninAsAdministrator;
};

#include <windows.h>
#include "resource.h"
extern HINSTANCE	hInstResDLL;
char password[1024] = "";

INT_PTR CALLBACK PasswordDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_INITDIALOG:
	{
		HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_WINVNC));
		SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	}
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK) {
			// Get the password entered in the edit box
			GetDlgItemText(hDlg, IDC_ADMINPASSWORD, password, 1024);

			// Close the dialog with OK
			EndDialog(hDlg, IDOK);
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDCANCEL) {
			// Close the dialog with Cancel
			EndDialog(hDlg, IDCANCEL);
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

bool SettingsManager::checkAdminPassword()
{
	memset(password, 0, 1024);
	INT_PTR ret = DialogBox(hInstResDLL, MAKEINTRESOURCE(IDD_ADMINPASSWORD), NULL, PasswordDlgProc);
	if (ret == IDOK) {
		char hashed_password[crypto_pwhash_STRBYTES];
		myIniFile.ReadHash(hashed_password, crypto_pwhash_STRBYTES);
		if (crypto_pwhash_str_verify(hashed_password, password, strlen(password)) == 0) {
			return true;
		}
		else {
			return false;
		}
	}
	return false;
}

void SettingsManager::setAdminPasswordHash(char* password)
{
	char hashed_password[crypto_pwhash_STRBYTES];
	crypto_pwhash_str(
		hashed_password,
		password,
		strlen(password),
		crypto_pwhash_OPSLIMIT_INTERACTIVE,
		crypto_pwhash_MEMLIMIT_INTERACTIVE);

	myIniFile.WriteHash(hashed_password, crypto_pwhash_STRBYTES);
}

