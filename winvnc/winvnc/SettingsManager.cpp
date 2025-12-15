// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


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
#include "common/win32_helpers.h"
#include <fstream>

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
	sodium_init();
	setDefaults();
}

void SettingsManager::Initialize(char *configFile)
{
	iniFile.setIniFile(configFile);
	load();

	/*HANDLE hPToken = DesktopUsersToken::getInstance()->getDesktopUsersToken();
	int iImpersonateResult = 0;

	if (hPToken != NULL) {
		if (!ImpersonateLoggedOnUser(hPToken)) {
			iImpersonateResult = GetLastError();
			vnclog.Print(LL_INTWARN, VNCLOG("ImpersonateLoggedOnUser failed error %i\n"), iImpersonateResult);
		}
	}

	if (iImpersonateResult == ERROR_SUCCESS)
		RevertToSelf();*/
}

void SettingsManager::setRunningFromExternalService(BOOL fEnabled)
{ 
	m_pref_fRunningFromExternalService = fEnabled; 
};

#pragma comment(lib, "wtsapi32.lib")
#pragma comment(lib, "advapi32.lib")

#include <sddl.h>
#include <wtsapi32.h>

bool SettingsManager::IsDesktopUserAdmin()
{
	HANDLE hPToken = DesktopUsersToken::getInstance()->getDesktopUsersToken();
	if (hPToken) {
		if (!ImpersonateLoggedOnUser(hPToken)) {
			vnclog.Print(LL_LOGSCREEN, "ImpersonateLoggedOnUser Failed");
			vnclog.Print(LL_INTWARN, VNCLOG("ImpersonateLoggedOnUser Failed\n"));
			return false;
		}
	}
	vnclog.Print(LL_LOGSCREEN, "ImpersonateLoggedOnUser OK");
	vnclog.Print(LL_INTWARN, VNCLOG("ImpersonateLoggedOnUser OK\n"));
	bool isAdmin = Credentials::RunningAsAdministrator(RunningFromExternalService());

	if (isAdmin) {
		vnclog.Print(LL_LOGSCREEN, "Desktop user is Administrator");
		vnclog.Print(LL_INTWARN, VNCLOG("Desktop user is Administrator\n"));
	}
	else {
		vnclog.Print(LL_LOGSCREEN, "Desktop user is no Administrator");
		vnclog.Print(LL_INTWARN, VNCLOG("Desktop user is no Administrator\n"));
	}

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
	memset(reinterpret_cast<void*>(m_pref_passwdViewOnly), 0, sizeof(m_pref_passwdViewOnly));
#ifndef SC_20
	memset(reinterpret_cast<void*>(m_pref_szDSMPlugin), 0, sizeof(m_pref_szDSMPlugin));
#else
	strcpy_s(m_pref_szDSMPlugin, "SecureVNCPlugin.dsm");
#endif // SC_20
	
	memset(reinterpret_cast<void*>(m_pref_authhosts), 0, sizeof(m_pref_authhosts));
	strcpy(m_pref_authhosts, "?");

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
	m_pref_EnableUnicodeInput = TRUE;
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
#ifdef SC_20
	m_pref_ReverseAuthRequired = false;
#else
	m_pref_ReverseAuthRequired = true;
#endif

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
	memset(m_pref_alternateShell, 0, 129);
	m_pref_cloudEnabled = false;
	m_pref_AllowUserSettingsWithPassword = false;

};

void SettingsManager::load()
{
	m_pref_RemoveWallpaper = iniFile.ReadInt("admin", "RemoveWallpaper", m_pref_RemoveWallpaper);
	m_pref_RemoveEffects = iniFile.ReadInt("admin", "RemoveEffects", m_pref_RemoveEffects);
	m_pref_RemoveFontSmoothing = iniFile.ReadInt("admin", "RemoveFontSmoothing", m_pref_RemoveFontSmoothing);
#ifndef SC_20
	// Disable Tray icon
	m_pref_DisableTrayIcon = iniFile.ReadInt("admin", "DisableTrayIcon", m_pref_DisableTrayIcon);
	m_pref_AllowUserSettingsWithPassword = iniFile.ReadInt("admin", "AllowUserSettingsWithPassword", m_pref_AllowUserSettingsWithPassword);
	m_pref_Rdpmode = iniFile.ReadInt("admin", "rdpmode", m_pref_Rdpmode);
	m_pref_Noscreensaver = iniFile.ReadInt("admin", "noscreensaver", m_pref_Noscreensaver);
	m_pref_LoopbackOnly = iniFile.ReadInt("admin", "LoopbackOnly", m_pref_LoopbackOnly);
	m_pref_Secure = iniFile.ReadInt("admin", "Secure", m_pref_Secure);
	m_pref_RequireMSLogon = iniFile.ReadInt("admin", "MSLogonRequired", m_pref_RequireMSLogon);
	m_pref_NewMSLogon = iniFile.ReadInt("admin", "NewMSLogon", m_pref_NewMSLogon);
	m_pref_UseDSMPlugin = iniFile.ReadInt("admin", "UseDSMPlugin", m_pref_UseDSMPlugin);
	iniFile.ReadString("admin", "DSMPlugin", m_pref_szDSMPlugin, 128);
	m_pref_ReverseAuthRequired = iniFile.ReadInt("admin", "ReverseAuthRequired", m_pref_ReverseAuthRequired);
	iniFile.ReadString("admin", "DSMPluginConfig", m_pref_DSMPluginConfig, 512);
	m_pref_ipv6_allowed = iniFile.ReadInt("admin", "UseIpv6", m_pref_ipv6_allowed);
	m_pref_AllowLoopback = iniFile.ReadInt("admin", "AllowLoopback", m_pref_AllowLoopback);
	m_pref_AuthRequired = iniFile.ReadInt("admin", "AuthRequired", m_pref_AuthRequired);
	m_pref_ConnectPriority = iniFile.ReadInt("admin", "ConnectPriority", m_pref_ConnectPriority);
	iniFile.ReadString("admin", "AuthHosts", m_pref_authhosts, 1280);
	iniFile.ReadString("admin", "AuthHosts", m_pref_authhosts2, 1280);
	m_pref_allowshutdown = iniFile.ReadInt("admin", "AllowShutdown", m_pref_allowshutdown);
	m_pref_allowproperties = iniFile.ReadInt("admin", "AllowProperties", m_pref_allowproperties);
	m_pref_allowInjection = iniFile.ReadInt("admin", "AllowInjection", m_pref_allowInjection);
	m_pref_alloweditclients = iniFile.ReadInt("admin", "AllowEditClients", m_pref_alloweditclients);
	m_pref_ftTimeout = iniFile.ReadInt("admin", "FileTransferTimeout", m_pref_ftTimeout);
	if (m_pref_ftTimeout > 600)
		m_pref_ftTimeout = 600;
	m_pref_keepAliveInterval = iniFile.ReadInt("admin", "KeepAliveInterval", m_pref_keepAliveInterval);
	m_pref_IdleInputTimeout = iniFile.ReadInt("admin", "IdleInputTimeout", m_pref_IdleInputTimeout);
	if (m_pref_keepAliveInterval >= (m_pref_ftTimeout - KEEPALIVE_HEADROOM))
		m_pref_keepAliveInterval = m_pref_ftTimeout - KEEPALIVE_HEADROOM;
	iniFile.ReadString("admin", "service_commandline", m_pref_service_commandline, 1024);
	iniFile.ReadString("admin", "accept_reject_mesg", m_pref_accept_reject_mesg, 512);
	//vncPasswd::FromClear crypt(m_pref_Secure);
	//memcpy(m_pref_passwd, crypt, MAXPWLEN);
	m_pref_DebugMode = iniFile.ReadInt("admin", "DebugMode", m_pref_DebugMode);
	iniFile.ReadString("admin", "path", m_pref_DebugPath, 512);
	m_pref_DebugLevel = iniFile.ReadInt("admin", "DebugLevel", m_pref_DebugLevel);
	m_pref_Avilog = iniFile.ReadInt("admin", "Avilog", m_pref_Avilog);
	m_pref_UseIpv6 = iniFile.ReadInt("admin", "UseIpv6", m_pref_UseIpv6);
	m_pref_EnableFileTransfer = iniFile.ReadInt("admin", "FileTransferEnabled", m_pref_EnableFileTransfer);
	m_pref_FTUserImpersonation = iniFile.ReadInt("admin", "FTUserImpersonation", m_pref_FTUserImpersonation); // sf@2005
	m_pref_EnableBlankMonitor = iniFile.ReadInt("admin", "BlankMonitorEnabled", m_pref_EnableBlankMonitor);
	m_pref_BlankInputsOnly = iniFile.ReadInt("admin", "BlankInputsOnly", m_pref_BlankInputsOnly); //PGM
	m_pref_DefaultScale = iniFile.ReadInt("admin", "DefaultScale", m_pref_DefaultScale);
	m_pref_UseDSMPlugin = iniFile.ReadInt("admin", "UseDSMPlugin", m_pref_UseDSMPlugin);
	iniFile.ReadString("admin", "DSMPlugin", m_pref_szDSMPlugin, 128);
	iniFile.ReadString("admin", "DSMPluginConfig", m_pref_DSMPluginConfig, 512);
	m_pref_Primary = iniFile.ReadInt("admin", "primary", m_pref_Primary);
	m_pref_Secondary = iniFile.ReadInt("admin", "secondary", m_pref_Secondary);
	m_pref_EnableConnection = iniFile.ReadInt("admin", "SocketConnect", m_pref_EnableConnection);
	m_pref_EnableHTTPConnect = iniFile.ReadInt("admin", "HTTPConnect", m_pref_EnableHTTPConnect);
	m_pref_AutoPortSelect = iniFile.ReadInt("admin", "AutoPortSelect", m_pref_AutoPortSelect);
	m_pref_PortNumber = iniFile.ReadInt("admin", "PortNumber", m_pref_PortNumber);
	m_pref_HttpPortNumber = iniFile.ReadInt("admin", "HTTPPortNumber", DISPLAY_TO_HPORT(PORT_TO_DISPLAY(m_pref_PortNumber)));
	m_pref_IdleTimeout = iniFile.ReadInt("admin", "IdleTimeout", m_pref_IdleTimeout);
	m_pref_QuerySetting = iniFile.ReadInt("admin", "QuerySetting", m_pref_QuerySetting);
	m_pref_QueryTimeout = iniFile.ReadInt("admin", "QueryTimeout", m_pref_QueryTimeout);
	m_pref_QueryDisableTime = iniFile.ReadInt("admin", "QueryDisableTime", m_pref_QueryDisableTime);
	m_pref_QueryAccept = iniFile.ReadInt("admin", "QueryAccept", m_pref_QueryAccept);
	m_pref_MaxViewerSetting = iniFile.ReadInt("admin", "MaxViewerSetting", m_pref_MaxViewerSetting);
	m_pref_MaxViewers = iniFile.ReadInt("admin", "MaxViewers", m_pref_MaxViewers);
	m_pref_Collabo = iniFile.ReadInt("admin", "Collabo", m_pref_Collabo);
	m_pref_Frame = iniFile.ReadInt("admin", "Frame", m_pref_Frame);
	m_pref_Notification = iniFile.ReadInt("admin", "Notification", m_pref_Notification);
	m_pref_OSD = iniFile.ReadInt("admin", "OSD", m_pref_OSD);
	m_pref_NotificationSelection = iniFile.ReadInt("admin", "NotificationSelection", m_pref_NotificationSelection);
	m_pref_QueryIfNoLogon = iniFile.ReadInt("admin", "QueryIfNoLogon", m_pref_QueryIfNoLogon);
	strcpy_s(m_pref_passwd, "");
	strcpy_s(m_pref_passwdViewOnly, "");
	iniFile.ReadPassword(m_pref_passwd, MAXPWLEN);
	iniFile.ReadPasswordViewOnly(m_pref_passwdViewOnly, MAXPWLEN); //PGM
	m_pref_EnableRemoteInputs = iniFile.ReadInt("admin", "InputsEnabled", m_pref_EnableRemoteInputs);
	m_pref_LockSettings = iniFile.ReadInt("admin", "LockSetting", m_pref_LockSettings);
	m_pref_DisableLocalInputs = iniFile.ReadInt("admin", "LocalInputsDisabled", m_pref_DisableLocalInputs);
	m_pref_EnableJapInput = iniFile.ReadInt("admin", "EnableJapInput", m_pref_EnableJapInput);
	m_pref_EnableUnicodeInput = iniFile.ReadInt("admin", "EnableUnicodeInput", m_pref_EnableUnicodeInput);
	m_pref_EnableWin8Helper = iniFile.ReadInt("admin", "EnableWin8Helper", m_pref_EnableWin8Helper);
	m_pref_clearconsole = iniFile.ReadInt("admin", "clearconsole", m_pref_clearconsole);
	G_SENDBUFFER_EX = iniFile.ReadInt("admin", "sendbuffer", G_SENDBUFFER_EX);	
	_tcscpy_s(m_pref_group1, "VNCACCESS");
	iniFile.ReadString("admin_auth", "group1", m_pref_group1, 150);
	_tcscpy_s(m_pref_group2, "Administrators");
	iniFile.ReadString("admin_auth", "group2", m_pref_group2, 150);	
	_tcscpy_s(m_pref_group3, "VNCVIEWONLY");
	iniFile.ReadString("admin_auth", "group3", m_pref_group3, 150);

	iniFile.ReadString("admin", "cloudServer", m_pref_cloudServer, MAX_HOST_NAME_LEN);
	m_pref_cloudEnabled = iniFile.ReadInt("admin", "cloudEnabled", m_pref_cloudEnabled);

	iniFile.ReadString("admin", "alternate_shell", m_pref_alternateShell, 1024);


	m_pref_locdom1 = iniFile.ReadInt("admin_auth", "locdom1", m_pref_locdom1);
	m_pref_locdom2 = iniFile.ReadInt("admin_auth", "locdom2", m_pref_locdom2);
	m_pref_locdom3 = iniFile.ReadInt("admin_auth", "locdom3", m_pref_locdom3);
#endif // SC_20
	m_pref_TurboMode = iniFile.ReadInt("poll", "TurboMode", m_pref_TurboMode);
	m_pref_PollUnderCursor = iniFile.ReadInt("poll", "PollUnderCursor", m_pref_PollUnderCursor);
	m_pref_PollForeground = iniFile.ReadInt("poll", "PollForeground", m_pref_PollForeground);
	m_pref_PollFullScreen = iniFile.ReadInt("poll", "PollFullScreen", m_pref_PollFullScreen);
	m_pref_PollConsoleOnly = iniFile.ReadInt("poll", "OnlyPollConsole", m_pref_PollConsoleOnly);
	m_pref_PollOnEventOnly = iniFile.ReadInt("poll", "OnlyPollOnEvent", m_pref_PollOnEventOnly);
	m_pref_MaxCpu = iniFile.ReadInt("poll", "MaxCpu2", m_pref_MaxCpu);
	if (m_pref_MaxCpu == 0)
		m_pref_MaxCpu = 100;
	m_pref_MaxFPS = iniFile.ReadInt("poll", "MaxFPS", m_pref_MaxFPS);
	m_pref_Driver = iniFile.ReadInt("poll", "EnableDriver", m_pref_Driver);
	if (m_pref_Driver)
		m_pref_Driver = CheckVideoDriver(0);
	m_pref_Hook = iniFile.ReadInt("poll", "EnableHook", m_pref_Hook);
	m_pref_Virtual = iniFile.ReadInt("poll", "EnableVirtual", m_pref_Virtual);
	m_pref_autocapt = iniFile.ReadInt("poll", "autocapt", m_pref_autocapt);
}

void SettingsManager::savePassword() {
	if (strlen(m_pref_passwd) == 0) {
		iniFile.WriteString("UltraVNC", "passwd", m_pref_passwd);
		return;
	}
	iniFile.WritePassword(m_pref_passwd);
}

void SettingsManager::saveViewOnlyPassword() {
	if (strlen(m_pref_passwdViewOnly) == 0) {
		iniFile.WriteString("UltraVNC", "passwd2", m_pref_passwdViewOnly);
		return;
	}	
	iniFile.WritePasswordViewOnly(m_pref_passwdViewOnly);
}

void SettingsManager::save()
{
	if (!getAllowProperties())
		return;

	// SAVE PER-USER PREFS IF ALLOWED	
	// Modif sf@2002
	iniFile.WriteInt("admin", "AllowUserSettingsWithPassword", m_pref_AllowUserSettingsWithPassword);
	iniFile.WriteInt("admin", "FileTransferEnabled", m_pref_EnableFileTransfer);
	iniFile.WriteInt("admin", "FTUserImpersonation", m_pref_FTUserImpersonation); // sf@2005
	iniFile.WriteInt("admin", "BlankMonitorEnabled", m_pref_EnableBlankMonitor);
	iniFile.WriteInt("admin", "BlankInputsOnly", m_pref_BlankInputsOnly); //PGM
	iniFile.WriteInt("admin", "DefaultScale", m_pref_DefaultScale);
	iniFile.WriteInt("admin", "UseDSMPlugin", m_pref_UseDSMPlugin);
	iniFile.WriteString("admin", "DSMPlugin", m_pref_szDSMPlugin);
	iniFile.WriteString("admin", "DSMPluginConfig", m_pref_DSMPluginConfig);
	iniFile.WriteString("admin", "AuthHosts", m_pref_authhosts);
	iniFile.WriteInt("admin", "primary", m_pref_Primary);
	iniFile.WriteInt("admin", "secondary", m_pref_Secondary);
	iniFile.WriteInt("admin", "SocketConnect", m_pref_EnableConnection);
	iniFile.WriteInt("admin", "HTTPConnect", m_pref_EnableHTTPConnect);
	iniFile.WriteInt("admin", "AutoPortSelect", m_pref_AutoPortSelect);
	if (!m_pref_AutoPortSelect) {
		iniFile.WriteInt("admin", "PortNumber", m_pref_PortNumber);
		iniFile.WriteInt("admin", "HTTPPortNumber", m_pref_HttpPortNumber);
	}
	iniFile.WriteInt("admin", "InputsEnabled", m_pref_EnableRemoteInputs);
	iniFile.WriteInt("admin", "LocalInputsDisabled", m_pref_DisableLocalInputs);
	iniFile.WriteInt("admin", "IdleTimeout", m_pref_IdleTimeout);
	iniFile.WriteInt("admin", "EnableJapInput", m_pref_EnableJapInput);
	iniFile.WriteInt("admin", "EnableUnicodeInput", m_pref_EnableUnicodeInput);
	iniFile.WriteInt("admin", "EnableWin8Helper", m_pref_EnableWin8Helper);
	iniFile.WriteInt("admin", "QuerySetting", m_pref_QuerySetting);
	iniFile.WriteInt("admin", "QueryTimeout", m_pref_QueryTimeout);
	iniFile.WriteInt("admin", "QueryDisableTime", m_pref_QueryDisableTime);
	iniFile.WriteInt("admin", "QueryAccept", m_pref_QueryAccept);
	iniFile.WriteInt("admin", "MaxViewerSetting", m_pref_MaxViewerSetting);
	iniFile.WriteInt("admin", "MaxViewers", m_pref_MaxViewers);
	iniFile.WriteInt("admin", "Collabo", m_pref_Collabo);
	iniFile.WriteInt("admin", "Frame", m_pref_Frame);
	iniFile.WriteInt("admin", "Notification", m_pref_Notification);
	iniFile.WriteInt("admin", "OSD", m_pref_OSD);
	iniFile.WriteInt("admin", "NotificationSelection", m_pref_NotificationSelection);
	iniFile.WriteInt("admin", "QueryIfNoLogon", m_pref_QueryIfNoLogon);
	iniFile.WriteInt("admin", "LockSetting", m_pref_LockSettings);
	iniFile.WriteInt("admin", "RemoveWallpaper", m_pref_RemoveWallpaper);
	iniFile.WriteInt("admin", "RemoveEffects", m_pref_RemoveEffects);
	iniFile.WriteInt("admin", "RemoveFontSmoothing", m_pref_RemoveFontSmoothing);
	iniFile.WriteInt("admin", "DebugMode", vnclog.GetMode());
	iniFile.WriteInt("admin", "Avilog", vnclog.GetVideo());
	iniFile.WriteString("admin", "path", m_pref_DebugPath);
	iniFile.WriteInt("admin", "DebugLevel", vnclog.GetLevel());
	iniFile.WriteInt("admin", "AllowLoopback", m_pref_AllowLoopback);
	iniFile.WriteInt("admin", "UseIpv6", settings->getIPV6());
	iniFile.WriteInt("admin", "LoopbackOnly", m_pref_LoopbackOnly);
	iniFile.WriteInt("admin", "AllowShutdown", m_pref_allowshutdown);
	iniFile.WriteInt("admin", "AllowProperties", m_pref_allowproperties);
	iniFile.WriteInt("admin", "AllowInjection", m_pref_allowInjection);
	iniFile.WriteInt("admin", "AllowEditClients", m_pref_alloweditclients);
	iniFile.WriteInt("admin", "FileTransferTimeout", m_pref_ftTimeout);
	iniFile.WriteInt("admin", "KeepAliveInterval", m_pref_keepAliveInterval);
	iniFile.WriteInt("admin", "IdleInputTimeout", m_pref_IdleInputTimeout);
	iniFile.WriteInt("admin", "DisableTrayIcon", m_pref_DisableTrayIcon);
	iniFile.WriteInt("admin", "rdpmode", m_pref_Rdpmode);
	iniFile.WriteInt("admin", "noscreensaver", m_pref_Noscreensaver);
	iniFile.WriteInt("admin", "Secure", m_pref_Secure);
	iniFile.WriteInt("admin", "MSLogonRequired", m_pref_RequireMSLogon);
	iniFile.WriteInt("admin", "NewMSLogon", m_pref_NewMSLogon);
	iniFile.WriteInt("admin", "ReverseAuthRequired", m_pref_ReverseAuthRequired);
	iniFile.WriteInt("admin", "ConnectPriority", m_pref_ConnectPriority);
	iniFile.WriteInt("admin", "AuthRequired", m_pref_AuthRequired);
	iniFile.WriteString("admin", "service_commandline", m_pref_service_commandline);
	iniFile.WriteString("admin", "accept_reject_mesg", m_pref_accept_reject_mesg);
	iniFile.WriteInt("poll", "TurboMode", m_pref_TurboMode);
	iniFile.WriteInt("poll", "PollUnderCursor", m_pref_PollUnderCursor);
	iniFile.WriteInt("poll", "PollForeground", m_pref_PollForeground);
	iniFile.WriteInt("poll", "PollFullScreen", m_pref_PollFullScreen);
	iniFile.WriteInt("poll", "OnlyPollConsole", m_pref_PollConsoleOnly);
	iniFile.WriteInt("poll", "OnlyPollOnEvent", m_pref_PollOnEventOnly);
	iniFile.WriteInt("poll", "MaxCpu2", m_pref_MaxCpu);
	iniFile.WriteInt("poll", "MaxFPS", m_pref_MaxFPS);
	iniFile.WriteInt("poll", "EnableDriver", m_pref_Driver);
	iniFile.WriteInt("poll", "EnableHook", m_pref_Hook);
	iniFile.WriteInt("poll", "EnableVirtual", m_pref_Virtual);
	iniFile.WriteInt("poll", "autocapt", m_pref_autocapt);

	iniFile.WriteString("admin", "cloudServer", m_pref_cloudServer);
	iniFile.WriteInt("admin", "cloudEnabled", m_pref_cloudEnabled);

	iniFile.WriteString("admin_auth", "group1", m_pref_group1);
	iniFile.WriteString("admin_auth", "group2", m_pref_group2);
	iniFile.WriteString("admin_auth", "group3", m_pref_group3);

	iniFile.WriteInt("admin_auth", "locdom1", m_pref_locdom1);
	iniFile.WriteInt("admin_auth", "locdom2", m_pref_locdom2);
	iniFile.WriteInt("admin_auth", "locdom3", m_pref_locdom3);	
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
		m_pref_RunninAsAdministrator = Credentials::RunningAsAdministrator(RunningFromExternalService());
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
	while(true) {
		memset(password, 0, 1024);
		INT_PTR ret = DialogBox(hInstResDLL, MAKEINTRESOURCE(IDD_ADMINPASSWORD), NULL, PasswordDlgProc);
		if (ret == IDOK) {
			char hashed_password[crypto_pwhash_STRBYTES]{};
			iniFile.ReadHash(hashed_password, crypto_pwhash_STRBYTES);
			if (crypto_pwhash_str_verify(hashed_password, password, strlen(password)) == 0) {
				return true;
			}
			else {
				DWORD result = MessageBoxSecure(NULL, "Wrong password, do you want to retry?", "Error", MB_OK);
				if (result == 1)
					Sleep(2000);
				else
					return false;
			}
		}
		else
			return false;
	}
	return false;
}

bool SettingsManager::isAdminPasswordSet()
{
	char hashed_password[crypto_pwhash_STRBYTES]{};
	iniFile.ReadHash(hashed_password, crypto_pwhash_STRBYTES);
	if (strlen(hashed_password) == 0)
		return false;
	return true;
}

void SettingsManager::setAdminPasswordHash(char* password)
{
	if (strlen(password) == 0) {
		iniFile.WriteString("UltraVNC", "hash", "");
		return;
	}

	char hashed_password[crypto_pwhash_STRBYTES]{};
	crypto_pwhash_str(
		hashed_password,
		password,
		strlen(password),
		crypto_pwhash_OPSLIMIT_INTERACTIVE,
		crypto_pwhash_MEMLIMIT_INTERACTIVE);

	iniFile.WriteHash(hashed_password, crypto_pwhash_STRBYTES);
}

bool SettingsManager::getShowSettings()
{ 
	return showSettings; 
};

