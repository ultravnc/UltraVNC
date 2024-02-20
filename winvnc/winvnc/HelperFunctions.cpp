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


#include <algorithm>
#include "stdhdrs.h"
#include "inifile.h"
#include <cctype>
#include <cassert>
#include "UltraVNCService.h"
#include <winvnc/winvnc.h>
#include "SettingsManager.h"
#include <lmcons.h>
#include "credentials.h"
//  We first use shellexecute with "runas"
//  This way we can use UAC and user/passwd
//	Runas is standard OS, so no security risk

HWND G_MENU_HWND = NULL;
char* MENU_CLASS_NAME = "ultravnc-server-tray-icon";
bool ClientTimerReconnect = false;
bool allowMultipleInstances = false;

namespace settingsHelpers {
	void Set_settings_as_admin(char* mycommand) {
		char exe_file_name[MAX_PATH];
		char commanline[MAX_PATH];
		GetModuleFileName(0, exe_file_name, MAX_PATH);

		strcpy_s(commanline, winvncSettings);
		strcat_s(commanline, ":");
		strcat_s(commanline, 260, mycommand);

		SHELLEXECUTEINFO shExecInfo{};
		shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		shExecInfo.fMask = NULL;
		shExecInfo.hwnd = GetForegroundWindow();
		shExecInfo.lpVerb = "runas";
		shExecInfo.lpFile = exe_file_name;
		shExecInfo.lpParameters = commanline;
		shExecInfo.lpDirectory = NULL;
		shExecInfo.nShow = SW_SHOWNORMAL;
		shExecInfo.hInstApp = NULL;
		ShellExecuteEx(&shExecInfo);
	}

	void Real_settings(char* mycommand) {
		Copy_to_Secure_from_temp_helper(mycommand);
	}

	void Copy_to_Secure_from_temp_helper(char* lpCmdLine) {
		IniFile myIniFile_In;
		IniFile myIniFile_Out;
		myIniFile_Out.IniFileSetSecure();
		myIniFile_In.IniFileSetTemp(lpCmdLine);

		TCHAR* group1 = new char[150];
		TCHAR* group2 = new char[150];
		TCHAR* group3 = new char[150];
		BOOL BUseRegistry;
		LONG Secure;
		LONG MSLogonRequired;
		LONG NewMSLogon;
		LONG ReverseAuthRequired;
		LONG locdom1;
		LONG locdom2;
		LONG locdom3;
		LONG DebugMode = 2;
		LONG Avilog = 0;
		LONG DebugLevel = 10;
		LONG DisableTrayIcon = 0;
		LONG Rdpmode = 0;
		LONG NoScreensaver = 0;
		LONG LoopbackOnly = 0;
		LONG UseDSMPlugin;
		LONG AllowLoopback = 1;
		LONG AuthRequired;
		LONG ConnectPriority;

		char DSMPlugin[128];
		char* authhosts = new char[150];

		LONG AllowShutdown = 1;
		LONG AllowProperties = 1;
		LONG AllowInjection = 0;
		LONG AllowEditClients = 1;

		LONG FileTransferEnabled = 0;
		LONG FTUserImpersonation = 1;
		LONG BlankMonitorEnabled = 1;
		LONG BlankInputsOnly = 0; //PGM
		LONG DefaultScale = 1;

		LONG SocketConnect = 1;
		LONG HTTPConnect;
		LONG AutoPortSelect = 1;
		LONG PortNumber;
		LONG HttpPortNumber;
		LONG IdleTimeout;
		LONG IdleInputTimeout;

		LONG RemoveWallpaper = 0;

		LONG QuerySetting = 1;
		LONG QueryTimeout = 10;
		LONG QueryDisableTime = 0;
		LONG QueryAccept;
		LONG QueryIfNoLogon;

		LONG MaxViewerSettings = 0;
		LONG MaxViewers = 128;
		LONG Collabo = 0;

		LONG Frame = 0;
		LONG Notification = 0;
		LONG OSD = 0;
		LONG NotificationSelection = 0;
		LONG EnableRemoteInputs = 1;
		LONG LockSettings = 0;
		LONG DisableLocalInputs = 0;
		LONG EnableJapInput = 0;
		LONG EnableUnicodeInput = 0;
		LONG EnableWin8Helper = 0;
		LONG kickrdp = 0;
		LONG clearconsole = 0;
		char cloudServer[MAX_HOST_NAME_LEN];
		LONG cloudEnabled = 0;

#define MAXPWLEN 8
		char passwd[MAXPWLEN];

		LONG TurboMode = 1;
		LONG PollUnderCursor = 0;
		LONG PollForeground = 0;
		LONG PollFullScreen = 1;
		LONG PollConsoleOnly = 0;
		LONG PollOnEventOnly = 0;
		LONG Driver = 0;
		LONG Hook = 1;
		LONG Virtual;
		LONG SingleWindow = 0;
		char SingleWindowName[32];
		char path[512];
		char accept_reject_mesg[512];
		char service_commandline[1024];
		LONG MaxCpu = 100;
		LONG MaxFPS = 25;

		//adzm 2010-05-30 - dsmplugin config
		char DSMPluginConfig[512]{};
		*DSMPluginConfig = '\0';

		LONG Primary = 1;
		LONG Secondary = 0;

		BUseRegistry = myIniFile_In.ReadInt("admin", "UseRegistry", 99);
		if (BUseRegistry == 99) {
			delete[] group1;
			delete[] group2;
			delete[] group3;
			delete[] authhosts;
			return;
		}

		if (!myIniFile_Out.WriteInt("admin", "UseRegistry", BUseRegistry)) {
			//error
			MessageBoxSecure(NULL, "Permission denied:Uncheck [_] Protect my computer... in run as dialog or use user with write permission.", myIniFile_Out.myInifile, MB_ICONERROR);
			delete[] group1;
			delete[] group2;
			delete[] group3;
			delete[] authhosts;
			return;
		}

		BOOL setextramouse = myIniFile_In.ReadInt("admin", "SendExtraMouse", 1);
		myIniFile_Out.WriteInt("admin", "SendExtraMouse", setextramouse);

		Secure = myIniFile_In.ReadInt("admin", "Secure", false);
		myIniFile_Out.WriteInt("admin", "Secure", Secure);
		MSLogonRequired = myIniFile_In.ReadInt("admin", "MSLogonRequired", false);
		myIniFile_Out.WriteInt("admin", "MSLogonRequired", MSLogonRequired);
		NewMSLogon = myIniFile_In.ReadInt("admin", "NewMSLogon", false);
		myIniFile_Out.WriteInt("admin", "NewMSLogon", NewMSLogon);

		ReverseAuthRequired = myIniFile_In.ReadInt("admin", "ReverseAuthRequired", true);
		myIniFile_Out.WriteInt("admin", "ReverseAuthRequired", ReverseAuthRequired);

		myIniFile_In.ReadString("admin_auth", "group1", group1, 150);
		myIniFile_In.ReadString("admin_auth", "group2", group2, 150);
		myIniFile_In.ReadString("admin_auth", "group3", group3, 150);
		myIniFile_Out.WriteString("admin_auth", "group1", group1);
		myIniFile_Out.WriteString("admin_auth", "group2", group2);
		myIniFile_Out.WriteString("admin_auth", "group3", group3);

		locdom1 = myIniFile_In.ReadInt("admin_auth", "locdom1", 0);
		locdom2 = myIniFile_In.ReadInt("admin_auth", "locdom2", 0);
		locdom3 = myIniFile_In.ReadInt("admin_auth", "locdom3", 0);
		myIniFile_Out.WriteInt("admin_auth", "locdom1", locdom1);
		myIniFile_Out.WriteInt("admin_auth", "locdom2", locdom2);
		myIniFile_Out.WriteInt("admin_auth", "locdom3", locdom3);

		DebugMode = myIniFile_In.ReadInt("admin", "DebugMode", 0);
		Avilog = myIniFile_In.ReadInt("admin", "Avilog", 0);
		myIniFile_In.ReadString("admin", "path", path, 512);
		myIniFile_In.ReadString("admin", "accept_reject_mesg", accept_reject_mesg, 512);
		myIniFile_In.ReadString("admin", "service_commandline", service_commandline, 1024);
		DebugLevel = myIniFile_In.ReadInt("admin", "DebugLevel", 0);
		DisableTrayIcon = myIniFile_In.ReadInt("admin", "DisableTrayIcon", false);
		Rdpmode = myIniFile_In.ReadInt("admin", "rdpmode", 0);
		NoScreensaver = myIniFile_In.ReadInt("admin", "noscreensaver", 0);
		LoopbackOnly = myIniFile_In.ReadInt("admin", "LoopbackOnly", false);

		myIniFile_Out.WriteInt("admin", "DebugMode", DebugMode);
		myIniFile_Out.WriteInt("admin", "Avilog", Avilog);
		myIniFile_Out.WriteString("admin", "path", path);
		myIniFile_Out.WriteString("admin", "accept_reject_mesg", accept_reject_mesg);
		myIniFile_Out.WriteString("admin", "service_commandline", service_commandline);
		myIniFile_Out.WriteInt("admin", "DebugLevel", DebugLevel);
		myIniFile_Out.WriteInt("admin", "DisableTrayIcon", DisableTrayIcon);
		myIniFile_Out.WriteInt("admin", "rdpmode", Rdpmode);
		myIniFile_Out.WriteInt("admin", "noscreensaver", NoScreensaver);
		myIniFile_Out.WriteInt("admin", "LoopbackOnly", LoopbackOnly);

		UseDSMPlugin = myIniFile_In.ReadInt("admin", "UseDSMPlugin", false);
		AllowLoopback = myIniFile_In.ReadInt("admin", "AllowLoopback", true);
		AuthRequired = myIniFile_In.ReadInt("admin", "AuthRequired", true);
		ConnectPriority = myIniFile_In.ReadInt("admin", "ConnectPriority", 0);

		myIniFile_Out.WriteInt("admin", "UseDSMPlugin", UseDSMPlugin);
		myIniFile_Out.WriteInt("admin", "AllowLoopback", AllowLoopback);
		myIniFile_Out.WriteInt("admin", "AuthRequired", AuthRequired);
		myIniFile_Out.WriteInt("admin", "ConnectPriority", ConnectPriority);

		myIniFile_In.ReadString("admin", "DSMPlugin", DSMPlugin, 128);
		myIniFile_In.ReadString("admin", "AuthHosts", authhosts, 150);

		myIniFile_Out.WriteString("admin", "DSMPlugin", DSMPlugin);
		myIniFile_Out.WriteString("admin", "AuthHosts", authhosts);

		//adzm 2010-05-30 - dsmplugin config
		myIniFile_In.ReadString("admin", "DSMPluginConfig", DSMPluginConfig, 512);
		myIniFile_Out.WriteString("admin", "DSMPluginConfig", DSMPluginConfig);

		AllowShutdown = myIniFile_In.ReadInt("admin", "AllowShutdown", true);
		AllowProperties = myIniFile_In.ReadInt("admin", "AllowProperties", true);
		AllowInjection = myIniFile_In.ReadInt("admin", "AllowInjection", false);
		AllowEditClients = myIniFile_In.ReadInt("admin", "AllowEditClients", true);
		myIniFile_Out.WriteInt("admin", "AllowShutdown", AllowShutdown);
		myIniFile_Out.WriteInt("admin", "AllowProperties", AllowProperties);
		myIniFile_Out.WriteInt("admin", "AllowInjection", AllowInjection);
		myIniFile_Out.WriteInt("admin", "AllowEditClients", AllowEditClients);

		FileTransferEnabled = myIniFile_In.ReadInt("admin", "FileTransferEnabled", true);
		FTUserImpersonation = myIniFile_In.ReadInt("admin", "FTUserImpersonation", true);
		BlankMonitorEnabled = myIniFile_In.ReadInt("admin", "BlankMonitorEnabled", true);
		BlankInputsOnly = myIniFile_In.ReadInt("admin", "BlankInputsOnly", false); //PGM
		DefaultScale = myIniFile_In.ReadInt("admin", "DefaultScale", 1);

		Primary = myIniFile_In.ReadInt("admin", "primary", true);
		Secondary = myIniFile_In.ReadInt("admin", "secondary", false);

		myIniFile_Out.WriteInt("admin", "FileTransferEnabled", FileTransferEnabled);
		myIniFile_Out.WriteInt("admin", "FTUserImpersonation", FTUserImpersonation);
		myIniFile_Out.WriteInt("admin", "BlankMonitorEnabled", BlankMonitorEnabled);
		myIniFile_Out.WriteInt("admin", "BlankInputsOnly", BlankInputsOnly); //PGM
		myIniFile_Out.WriteInt("admin", "DefaultScale", DefaultScale);

		myIniFile_Out.WriteInt("admin", "primary", Primary);
		myIniFile_Out.WriteInt("admin", "secondary", Secondary);

		// Connection prefs
		SocketConnect = myIniFile_In.ReadInt("admin", "SocketConnect", true);
		HTTPConnect = myIniFile_In.ReadInt("admin", "HTTPConnect", true);
		AutoPortSelect = myIniFile_In.ReadInt("admin", "AutoPortSelect", true);
		PortNumber = myIniFile_In.ReadInt("admin", "PortNumber", 0);
		HttpPortNumber = myIniFile_In.ReadInt("admin", "HTTPPortNumber", 0);
		IdleTimeout = myIniFile_In.ReadInt("admin", "IdleTimeout", 0);
		IdleInputTimeout = myIniFile_In.ReadInt("admin", "IdleInputTimeout", 0);
		myIniFile_Out.WriteInt("admin", "SocketConnect", SocketConnect);
		myIniFile_Out.WriteInt("admin", "HTTPConnect", HTTPConnect);
		myIniFile_Out.WriteInt("admin", "AutoPortSelect", AutoPortSelect);
		myIniFile_Out.WriteInt("admin", "PortNumber", PortNumber);
		myIniFile_Out.WriteInt("admin", "HTTPPortNumber", HttpPortNumber);
		myIniFile_Out.WriteInt("admin", "IdleTimeout", IdleTimeout);
		myIniFile_Out.WriteInt("admin", "IdleInputTimeout", IdleInputTimeout);

		RemoveWallpaper = myIniFile_In.ReadInt("admin", "RemoveWallpaper", 0);
		myIniFile_Out.WriteInt("admin", "RemoveWallpaper", RemoveWallpaper);

		// Connection querying settings
		QuerySetting = myIniFile_In.ReadInt("admin", "QuerySetting", 0);
		QueryTimeout = myIniFile_In.ReadInt("admin", "QueryTimeout", 0);
		QueryDisableTime = myIniFile_In.ReadInt("admin", "QueryDisableTime", 0);
		QueryAccept = myIniFile_In.ReadInt("admin", "QueryAccept", 0);
		QueryIfNoLogon = myIniFile_In.ReadInt("admin", "QueryIfNoLogon", 1);
		myIniFile_Out.WriteInt("admin", "QuerySetting", QuerySetting);
		myIniFile_Out.WriteInt("admin", "QueryTimeout", QueryTimeout);
		myIniFile_Out.WriteInt("admin", "QueryDisableTime", QueryDisableTime);
		myIniFile_Out.WriteInt("admin", "QueryAccept", QueryAccept);
		myIniFile_Out.WriteInt("admin", "QueryIfNoLogon", QueryIfNoLogon);

		MaxViewerSettings = myIniFile_In.ReadInt("admin", "MaxViewerSetting", MaxViewerSettings);
		myIniFile_Out.WriteInt("admin", "MaxViewerSetting", MaxViewerSettings);

		Collabo = myIniFile_In.ReadInt("admin", "Collabo", Collabo);
		myIniFile_Out.WriteInt("admin", "Collabo", Collabo);

		Frame = myIniFile_In.ReadInt("admin", "Frame", Frame);
		Notification = myIniFile_In.ReadInt("admin", "Notification", Notification);
		OSD = myIniFile_In.ReadInt("admin", "OSD", OSD);
		NotificationSelection = myIniFile_In.ReadInt("admin", "NotificationSelection", NotificationSelection);
		myIniFile_Out.WriteInt("admin", "Frame", Frame);
		myIniFile_Out.WriteInt("admin", "Notification", Notification);
		myIniFile_Out.WriteInt("admin", "OSD", OSD);
		myIniFile_Out.WriteInt("admin", "NotificationSelection", NotificationSelection);

		MaxViewers = myIniFile_In.ReadInt("admin", "MaxViewers", MaxViewers);
		myIniFile_Out.WriteInt("admin", "MaxViewers", MaxViewers);

		myIniFile_In.ReadPassword(passwd, MAXPWLEN);
		myIniFile_Out.WritePassword(passwd);
		memset(passwd, '\0', MAXPWLEN); //PGM
		myIniFile_In.ReadPassword2(passwd, MAXPWLEN); //PGM
		myIniFile_Out.WritePassword2(passwd); //PGM

		myIniFile_In.ReadString("admin", "cloudServer", cloudServer, 64);
		myIniFile_Out.WriteString("admin", "cloudServer", cloudServer);
		cloudEnabled = myIniFile_In.ReadInt("admin", "cloudEnabled", cloudEnabled);
		myIniFile_Out.WriteInt("admin", "cloudEnabled", cloudEnabled);

		EnableRemoteInputs = myIniFile_In.ReadInt("admin", "InputsEnabled", 0);
		LockSettings = myIniFile_In.ReadInt("admin", "LockSetting", 0);
		DisableLocalInputs = myIniFile_In.ReadInt("admin", "LocalInputsDisabled", 0);
		EnableJapInput = myIniFile_In.ReadInt("admin", "EnableJapInput", 0);
		EnableUnicodeInput = myIniFile_In.ReadInt("admin", "EnableUnicodeInput", 0);
		EnableWin8Helper = myIniFile_In.ReadInt("admin", "EnableWin8Helper", 0);
		kickrdp = myIniFile_In.ReadInt("admin", "kickrdp", 0);
		clearconsole = myIniFile_In.ReadInt("admin", "clearconsole", 0);

		myIniFile_Out.WriteInt("admin", "InputsEnabled", EnableRemoteInputs);
		myIniFile_Out.WriteInt("admin", "LockSetting", LockSettings);
		myIniFile_Out.WriteInt("admin", "LocalInputsDisabled", DisableLocalInputs);
		myIniFile_Out.WriteInt("admin", "EnableJapInput", EnableJapInput);
		myIniFile_Out.WriteInt("admin", "EnableUnicodeInput", EnableUnicodeInput);
		myIniFile_Out.WriteInt("admin", "EnableWin8Helper", EnableWin8Helper);
		myIniFile_Out.WriteInt("admin", "kickrdp", kickrdp);
		myIniFile_Out.WriteInt("admin", "clearconsole", clearconsole);

		TurboMode = myIniFile_In.ReadInt("poll", "TurboMode", 0);
		PollUnderCursor = myIniFile_In.ReadInt("poll", "PollUnderCursor", 0);
		PollForeground = myIniFile_In.ReadInt("poll", "PollForeground", 0);
		PollFullScreen = myIniFile_In.ReadInt("poll", "PollFullScreen", 0);
		PollConsoleOnly = myIniFile_In.ReadInt("poll", "OnlyPollConsole", 0);
		PollOnEventOnly = myIniFile_In.ReadInt("poll", "OnlyPollOnEvent", 0);
		MaxCpu = myIniFile_In.ReadInt("poll", "MaxCpu2", 0);
		MaxFPS = myIniFile_In.ReadInt("poll", "MaxFPS", 0);
		Driver = myIniFile_In.ReadInt("poll", "EnableDriver", 0);
		Hook = myIniFile_In.ReadInt("poll", "EnableHook", 0);
		Virtual = myIniFile_In.ReadInt("poll", "EnableVirtual", 0);

		SingleWindow = myIniFile_In.ReadInt("poll", "SingleWindow", SingleWindow);
		myIniFile_In.ReadString("poll", "SingleWindowName", SingleWindowName, 32);

		myIniFile_Out.WriteInt("poll", "TurboMode", TurboMode);
		myIniFile_Out.WriteInt("poll", "PollUnderCursor", PollUnderCursor);
		myIniFile_Out.WriteInt("poll", "PollForeground", PollForeground);
		myIniFile_Out.WriteInt("poll", "PollFullScreen", PollFullScreen);
		myIniFile_Out.WriteInt("poll", "OnlyPollConsole", PollConsoleOnly);
		myIniFile_Out.WriteInt("poll", "OnlyPollOnEvent", PollOnEventOnly);
		myIniFile_Out.WriteInt("poll", "MaxCpu2", MaxCpu);
		myIniFile_Out.WriteInt("poll", "MaxFPS", MaxFPS);
		myIniFile_Out.WriteInt("poll", "EnableDriver", Driver);
		myIniFile_Out.WriteInt("poll", "EnableHook", Hook);
		myIniFile_Out.WriteInt("poll", "EnableVirtual", Virtual);

		myIniFile_Out.WriteInt("poll", "SingleWindow", SingleWindow);
		myIniFile_Out.WriteString("poll", "SingleWindowName", SingleWindowName);

		myIniFile_Out.WriteInt("poll", "MaxCpu2", MaxCpu);
		myIniFile_Out.WriteInt("poll", "MaxFPS", MaxFPS);

		DeleteFile(lpCmdLine);
		delete[] group1;
		delete[] group2;
		delete[] group3;
		delete[] authhosts;
	}
}

void Open_homepage()
{
	ShellExecute(0, "open", "https://uvnc.com/", 0, 0, 1);
}

void Open_forum()
{
	ShellExecute(0, "open", "https://forum.uvnc.com/", 0, 0, 1);
}

void Open_github()
{
	ShellExecute(0, "open", "https://github.com/ultravnc", 0, 0, 1);
}

void Open_mastodon()
{
	ShellExecute(0, "open", "https://mastodon.social/@ultravnc", 0, 0, 1);
}

void Open_facebook()
{
	ShellExecute(0, "open", "https://www.facebook.com/ultravnc1", 0, 0, 1);
}

void Open_xtwitter()
{
	ShellExecute(0, "open", "https://twitter.com/ultravnc1", 0, 0, 1);
}

void Open_reddit()
{
	ShellExecute(0, "open", "https://www.reddit.com/r/ultravnc", 0, 0, 1);
}

void Open_openhub()
{
	ShellExecute(0, "open", "https://openhub.net/p/ultravnc", 0, 0, 1);
}

#ifndef SC_20
namespace serviceHelpers {
	void Set_stop_service_as_admin() {
		char exe_file_name[MAX_PATH];
		GetModuleFileName(0, exe_file_name, MAX_PATH);

		SHELLEXECUTEINFO shExecInfo{};

		shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		shExecInfo.fMask = NULL;
		shExecInfo.hwnd = GetForegroundWindow();
		shExecInfo.lpVerb = "runas";
		shExecInfo.lpFile = exe_file_name;
		shExecInfo.lpParameters = winvncStopservice;
		shExecInfo.lpDirectory = NULL;
		shExecInfo.nShow = SW_SHOWNORMAL;
		shExecInfo.hInstApp = NULL;
		ShellExecuteEx(&shExecInfo);
	}

	void Real_stop_service() {
		char command[MAX_PATH + 32]; // 29 January 2008 jdp
		_snprintf_s(command, sizeof command, "net stop \"%s\"", UltraVNCService::service_name);
		WinExec(command, SW_HIDE);
	}

	void Set_start_service_as_admin() {
		char exe_file_name[MAX_PATH];
		GetModuleFileName(0, exe_file_name, MAX_PATH);

		SHELLEXECUTEINFO shExecInfo{};

		shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		shExecInfo.fMask = NULL;
		shExecInfo.hwnd = GetForegroundWindow();
		shExecInfo.lpVerb = "runas";
		shExecInfo.lpFile = exe_file_name;
		shExecInfo.lpParameters = winvncStartservice;
		shExecInfo.lpDirectory = NULL;
		shExecInfo.nShow = SW_SHOWNORMAL;
		shExecInfo.hInstApp = NULL;
		ShellExecuteEx(&shExecInfo);
	}

	void Real_start_service() {
		char command[MAX_PATH + 32]; // 29 January 2008 jdp
		_snprintf_s(command, sizeof command, "net start \"%s\"", UltraVNCService::service_name);
		WinExec(command, SW_HIDE);
	}

	void Set_install_service_as_admin() {
		char exe_file_name[MAX_PATH];
		GetModuleFileName(0, exe_file_name, MAX_PATH);

		SHELLEXECUTEINFO shExecInfo{};

		shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		shExecInfo.fMask = NULL;
		shExecInfo.hwnd = GetForegroundWindow();
		shExecInfo.lpVerb = "runas";
		shExecInfo.lpFile = exe_file_name;
		shExecInfo.lpParameters = winvncInstallService;
		shExecInfo.lpDirectory = NULL;
		shExecInfo.nShow = SW_SHOWNORMAL;
		shExecInfo.hInstApp = NULL;
		ShellExecuteEx(&shExecInfo);
	}

	void Set_uninstall_service_as_admin() {
		char exe_file_name[MAX_PATH];
		GetModuleFileName(0, exe_file_name, MAX_PATH);

		SHELLEXECUTEINFO shExecInfo{};

		shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		shExecInfo.fMask = NULL;
		shExecInfo.hwnd = GetForegroundWindow();
		shExecInfo.lpVerb = "runas";
		shExecInfo.lpFile = exe_file_name;
		shExecInfo.lpParameters = winvncUnInstallService;
		shExecInfo.lpDirectory = NULL;
		shExecInfo.nShow = SW_SHOWNORMAL;
		shExecInfo.hInstApp = NULL;
		ShellExecuteEx(&shExecInfo);
	}

	void winvncSecurityEditorHelper_as_admin() {
		char exe_file_name[MAX_PATH];
		GetModuleFileName(0, exe_file_name, MAX_PATH);

		SHELLEXECUTEINFO shExecInfo{};

		shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		shExecInfo.fMask = NULL;
		shExecInfo.hwnd = GetForegroundWindow();
		shExecInfo.lpVerb = "runas";
		shExecInfo.lpFile = exe_file_name;
		shExecInfo.lpParameters = winvncSecurityEditor;
		shExecInfo.lpDirectory = NULL;
		shExecInfo.nShow = SW_SHOWNORMAL;
		shExecInfo.hInstApp = NULL;
		ShellExecuteEx(&shExecInfo);
	}

	void make_upper(std::string& str)
	{
		// convert to uppercase
		std::transform(str.begin(), str.end(), str.begin(), toupper);//(int(*)(int))
	}

	//**************************************************************************
	// ExistServiceName() looks up service by application path. If found, the function
	// fills pszServiceName (must be at least 256+1 characters long).
	bool ExistServiceName(TCHAR* pszAppPath, TCHAR* pszServiceName)
	{
		// prepare given application path for matching against service list
		std::string appPath(pszAppPath);
		// convert to uppercase
		make_upper(appPath);

		// connect to serice control manager
		SC_HANDLE hManager = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
		if (!hManager)
			return false;

		DWORD dwBufferSize = 0;
		DWORD dwCount = 0;
		DWORD dwPosition = 0;
		bool bResult = false;

		// call EnumServicesStatus() the first time to receive services array size
		BOOL bOK = EnumServicesStatus(
			hManager,
			SERVICE_WIN32,
			SERVICE_STATE_ALL,
			NULL,
			0,
			&dwBufferSize,
			&dwCount,
			&dwPosition);
		if (!bOK && GetLastError() == ERROR_MORE_DATA)
		{
			// allocate space per results from the first call
			ENUM_SERVICE_STATUS* pServices = (ENUM_SERVICE_STATUS*) new UCHAR[dwBufferSize];
			if (pServices)
			{
				// call EnumServicesStatus() the second time to actually get the services array
				bOK = EnumServicesStatus(
					hManager,
					SERVICE_WIN32,
					SERVICE_STATE_ALL,
					pServices,
					dwBufferSize,
					&dwBufferSize,
					&dwCount,
					&dwPosition);
				if (bOK)
				{
					// iterate through all services returned by EnumServicesStatus()
					for (DWORD i = 0; i < dwCount && !bResult; i++)
					{
						// open service
						SC_HANDLE hService = OpenService(hManager,
							pServices[i].lpServiceName,
							GENERIC_READ);
						if (!hService)
							break;

						// call QueryServiceConfig() the first time to receive buffer size
						bOK = QueryServiceConfig(
							hService,
							NULL,
							0,
							&dwBufferSize);
						if (!bOK && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
						{
							// allocate space per results from the first call
							QUERY_SERVICE_CONFIG* pServiceConfig = (QUERY_SERVICE_CONFIG*) new UCHAR[dwBufferSize];
							if (pServiceConfig)
							{
								// call EnumServicesStatus() the second time to actually get service config
								bOK = QueryServiceConfig(
									hService,
									pServiceConfig,
									dwBufferSize,
									&dwBufferSize);
								if (bOK)
								{
									// match given application name against executable path in the service config
									std::string servicePath(pServiceConfig->lpBinaryPathName);
									make_upper(servicePath);
									if (servicePath.find(appPath.c_str()) != -1)
									{
										bResult = true;
										strncpy_s(pszServiceName, 256, pServices[i].lpServiceName, 256);
										pszServiceName[255] = 0;
									}
								}

								delete[](UCHAR*) pServiceConfig;
							}
						}

						CloseServiceHandle(hService);
					}
				}

				delete[](UCHAR*) pServices;
			}
		}

		// disconnect from serice control manager
		CloseServiceHandle(hManager);

		return bResult;
	}
}
#endif

DWORD MessageBoxSecure(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType)
{
	DWORD retunvalue;
	if (settings->RunningFromExternalService())
	{
		HDESK desktop = NULL;
		HDESK old_desktop;
		desktop = OpenInputDesktop(0, FALSE, DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW | DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL | DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS | DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
		old_desktop = GetThreadDesktop(GetCurrentThreadId());
		if (desktop && old_desktop && old_desktop != desktop)
		{
			SetThreadDesktop(desktop);
			retunvalue = MessageBox(hWnd, lpText, lpCaption, uType);
			SetThreadDesktop(old_desktop);
			CloseDesktop(desktop);
		}
		else retunvalue = 0;
	}
	else
	{
		retunvalue = MessageBox(hWnd, lpText, lpCaption, uType);
	}
	return retunvalue;
}

namespace desktopSelector {
	int closeHandlesAndReturn(HDESK threaddesktop, HDESK inputdesktop, int value) {
		if(threaddesktop)
			CloseDesktop(threaddesktop);
		if (inputdesktop)
			CloseDesktop(inputdesktop);
		return value;
	}
	int InputDesktopSelected() {
		// Get the input and thread desktops
		HDESK threaddesktop = GetThreadDesktop(GetCurrentThreadId());
		HDESK inputdesktop = OpenInputDesktop(0, FALSE,
			DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
			DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
			DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
			DESKTOP_SWITCHDESKTOP);

		if (inputdesktop == NULL) {
			if (!settings->RunningFromExternalService())
				return closeHandlesAndReturn(threaddesktop, inputdesktop, 2); 			//Running as SC we want to keep the viewer open in case UAC or screensaver jump in
			DWORD lasterror;
			lasterror = GetLastError();
			if (lasterror == 170)
				return closeHandlesAndReturn(threaddesktop, inputdesktop, 1);
			if (lasterror == 624)
				return closeHandlesAndReturn(threaddesktop, inputdesktop, 1);
			return closeHandlesAndReturn(threaddesktop, inputdesktop, 0);
		}

		DWORD dummy;
		char threadname[256]{};
		char inputname[256]{};

		if (!GetUserObjectInformation(threaddesktop, UOI_NAME, &threadname, 256, &dummy)) {
			if (!settings->RunningFromExternalService())
				return closeHandlesAndReturn(threaddesktop, inputdesktop, 2);
			return closeHandlesAndReturn(threaddesktop, inputdesktop, 0);
		}
		assert(dummy <= 256);
		if (!GetUserObjectInformation(inputdesktop, UOI_NAME, &inputname, 256, &dummy)) {
			if (!settings->RunningFromExternalService())
				return closeHandlesAndReturn(threaddesktop, inputdesktop, 2);
			return closeHandlesAndReturn(threaddesktop, inputdesktop, 0);
		}
		assert(dummy <= 256);

		if (strcmp(threadname, inputname) != 0) {
			if (!settings->RunningFromExternalService())
				return closeHandlesAndReturn(threaddesktop, inputdesktop, 2);
			return closeHandlesAndReturn(threaddesktop, inputdesktop, 0);
		}
		return closeHandlesAndReturn(threaddesktop, inputdesktop, 1);
	}

	BOOL SelectHDESK(HDESK new_desktop) {
		HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());

		DWORD dummy;
		char new_name[256]{};

		if (!GetUserObjectInformation(new_desktop, UOI_NAME, &new_name, 256, &dummy)) {
			vnclog.Print(LL_INTERR, VNCLOG("!GetUserObjectInformation \n"));
			return FALSE;
		}

		vnclog.Print(LL_INTERR, VNCLOG("SelectHDESK to %s (%x) from %x\n"), new_name, new_desktop, old_desktop);

		// Switch the desktop
		if (!SetThreadDesktop(new_desktop)) {
			vnclog.Print(LL_INTERR, VNCLOG("SelectHDESK:!SetThreadDesktop \n"));
			return FALSE;
		}
		return TRUE;
	}

	BOOL SelectDesktop(char* name, HDESK* new_desktop) {
		HDESK desktop;
		vnclog.Print(LL_INTERR, VNCLOG("SelectDesktop \n"));
		if (name != NULL)
		{
			vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop2 named\n"));
			// Attempt to open the named desktop
			desktop = OpenDesktop(name, 0, FALSE,
				DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
				DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
				DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
				DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
		}
		else
		{
			vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop2 NULL\n"));
			// No, so open the input desktop
			desktop = OpenInputDesktop(0, FALSE,
				DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
				DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
				DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
				DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
		}

		// Did we succeed?
		if (desktop == NULL) {
			vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop2 \n"));
			return FALSE;
		}
		else vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop2 OK\n"));

		// Switch to the new desktop
		if (!SelectHDESK(desktop)) {
			// Failed to enter the new desktop, so free it!
			if (!CloseDesktop(desktop))
				vnclog.Print(LL_INTERR, VNCLOG("SelectDesktop failed to close desktop\n"));
			return FALSE;
		}

		if (new_desktop)
		{
			if (*new_desktop)
				CloseDesktop(*new_desktop);
			*new_desktop = desktop;
		}

		// We successfully switched desktops!
		return TRUE;
	}
}

namespace postHelper {
	UINT MENU_ADD_CLIENT_MSG = RegisterWindowMessage("WinVNC.AddClient.Message");
	UINT MENU_ADD_CLOUD_MSG = RegisterWindowMessage("WinVNC.AddCloud.Message");
	UINT MENU_REPEATER_ID_MSG = RegisterWindowMessage("WinVNC.AddRepeaterID.Message");
	UINT MENU_AUTO_RECONNECT_MSG = RegisterWindowMessage("WinVNC.AddAutoClient.Message");
	UINT MENU_STOP_RECONNECT_MSG = RegisterWindowMessage("WinVNC.AddStopClient.Message");
	UINT MENU_STOP_ALL_RECONNECT_MSG = RegisterWindowMessage("WinVNC.AddStopAllClient.Message");
	UINT MENU_ADD_CLIENT_MSG_INIT = RegisterWindowMessage("WinVNC.AddClient.Message.Init");
	UINT MENU_ADD_CLIENT6_MSG_INIT = RegisterWindowMessage("WinVNC.AddClient6.Message.Init");
	UINT MENU_ADD_CLIENT6_MSG = RegisterWindowMessage("WinVNC.AddClient6.Message");
	UINT MENU_TRAYICON_BALLOON_MSG = RegisterWindowMessage("WinVNC.TrayIconBalloon2.Message");
	UINT FileTransferSendPacketMessage = RegisterWindowMessage("UltraVNC.Viewer.FileTransferSendPacketMessage");

	in6_addr G_LPARAM_IN6 = {};

	BOOL PostAddAutoConnectClient(const char* pszId) {
		ATOM aId = INVALID_ATOM;
		if (pszId)
			aId = GlobalAddAtom(pszId);
		return (PostToWinVNC(MENU_AUTO_RECONNECT_MSG, 0, (LPARAM)aId));
	}

	BOOL PostAddNewRepeaterClient() {
		// assumes the -repeater command line set the repeater global variable.
		// Post to the UltraVNC Server menu window (usually expected to fail at program startup)
		if (!PostToWinVNC(MENU_ADD_CLIENT_MSG, (WPARAM)0xFFFFFFFF, (LPARAM)0xFFFFFFFF))
			return FALSE;
		return TRUE;
	}

	BOOL PostAddNewCloudClient() {
		// assumes the -repeater command line set the repeater global variable.
		// Post to the UltraVNC Server menu window (usually expected to fail at program startup)
		if (!PostToWinVNC(MENU_ADD_CLOUD_MSG, (WPARAM)0xFFFFFFFF, (LPARAM)0xFFFFFFFF))
			return FALSE;
		return TRUE;
	}

	BOOL PostAddStopConnectClient() {
		return (PostToWinVNC(MENU_STOP_RECONNECT_MSG, 0, 0));
	}

	BOOL PostAddStopConnectClientAll() {
		PostToWinVNC(MENU_STOP_RECONNECT_MSG, 0, 0); // stop running reconnect in server class
		return (PostToWinVNC(MENU_STOP_ALL_RECONNECT_MSG, 0, 0)); // disable reconnect for tunning clients
	}

	BOOL PostAddNewClientInit(unsigned long ipaddress, unsigned short port) {
		// Post to the UltraVNC Server menu window
		if (!PostToWinVNC(MENU_ADD_CLIENT_MSG_INIT, (WPARAM)port, (LPARAM)ipaddress))
		{
			//MessageBoxSecure(NULL, sz_ID_NO_EXIST_INST, szAppName, MB_ICONEXCLAMATION | MB_OK);
			//Little hack, seems postmessage fail in some cases on some os.
			//permission proble
			//use G_var + WM_time to reconnect
			vnclog.Print(LL_INTERR, VNCLOG("PostAddNewClient failed\n"));
			if (port == 1111 && ipaddress == 1111) ClientTimerReconnect = true;
			return FALSE;
		}

		return TRUE;
	}

	BOOL PostAddNewClient4(unsigned long ipaddress, unsigned short port) {
		// Post to the UltraVNC Server menu window
		if (!PostToWinVNC(MENU_ADD_CLIENT_MSG, (WPARAM)port, (LPARAM)ipaddress))
		{
			//MessageBoxSecure(NULL, sz_ID_NO_EXIST_INST, szAppName, MB_ICONEXCLAMATION | MB_OK);
			//Little hack, seems postmessage fail in some cases on some os.
			//permission proble
			//use G_var + WM_time to reconnect
			vnclog.Print(LL_INTERR, VNCLOG("PostAddNewClient failed\n"));
			if (port == 1111 && ipaddress == 1111) ClientTimerReconnect = true;
			return FALSE;
		}

		return TRUE;
	}

	BOOL PostAddNewClientInit4(unsigned long ipaddress, unsigned short port) {
		// Post to the UltraVNC Server menu window
		if (!PostToWinVNC(MENU_ADD_CLIENT_MSG_INIT, (WPARAM)port, (LPARAM)ipaddress)) {
			//MessageBoxSecure(NULL, sz_ID_NO_EXIST_INST, szAppName, MB_ICONEXCLAMATION | MB_OK);
			//Little hack, seems postmessage fail in some cases on some os.
			//permission proble
			//use G_var + WM_time to reconnect
			vnclog.Print(LL_INTERR, VNCLOG("PostAddNewClient failed\n"));
			if (port == 1111 && ipaddress == 1111) ClientTimerReconnect = true;
			return FALSE;
		}
		return TRUE;
	}
	BOOL PostAddNewClient6(in6_addr* ipaddress, unsigned short port) {
		// Post to the UltraVNC Server menu window
		// We can not sen a IPv6 address with a LPARAM, so we fake the message by copying in a gloobal var.
		memcpy(&G_LPARAM_IN6, ipaddress, sizeof(in6_addr));
		if (!PostToWinVNC(MENU_ADD_CLIENT6_MSG, (WPARAM)port, (LPARAM)ipaddress))
			return FALSE;
		return TRUE;
	}

	BOOL PostAddNewClientInit6(in6_addr* ipaddress, unsigned short port) {
		// Post to the UltraVNC Server menu window
		// We can not sen a IPv6 address with a LPARAM, so we fake the message by copying in a gloobal var.
		memcpy(&G_LPARAM_IN6, ipaddress, sizeof(in6_addr));
		if (!PostToWinVNC(MENU_ADD_CLIENT6_MSG_INIT, (WPARAM)port, (LPARAM)ipaddress))
			return FALSE;
		return TRUE;
	}

	BOOL PostAddNewClient(unsigned long ipaddress, unsigned short port) {
		// Post to the UltraVNC Server menu window
		if (!PostToWinVNC(MENU_ADD_CLIENT_MSG, (WPARAM)port, (LPARAM)ipaddress))
		{
			//MessageBoxSecure(NULL, sz_ID_NO_EXIST_INST, szAppName, MB_ICONEXCLAMATION | MB_OK);
			//Little hack, seems postmessage fail in some cases on some os.
			//permission proble
			//use G_var + WM_time to reconnect
			vnclog.Print(LL_INTERR, VNCLOG("PostAddNewClient failed\n"));
			if (port == 1111 && ipaddress == 1111) ClientTimerReconnect = true;
			return FALSE;
		}
		return TRUE;
	}

	BOOL PostAddConnectClient(const char* pszId) {
		ATOM aId = INVALID_ATOM;
		if (pszId) {
			aId = GlobalAddAtom(pszId);
		}
		return (PostToWinVNC(MENU_REPEATER_ID_MSG, 0, (LPARAM)aId));
	}

	BOOL PostToThisWinVNC(UINT message, WPARAM wParam, LPARAM lParam) {
		//adzm 2010-02-10 - Finds the appropriate VNC window for this process
		HWND hservwnd = FindWinVNCWindow(true);
		if (hservwnd == NULL)
			return FALSE;

		// Post the message to UltraVNC Server
		PostMessage(hservwnd, message, wParam, lParam);
		return TRUE;
	}

	BOOL PostToWinVNC(UINT message, WPARAM wParam, LPARAM lParam) {
		// Locate the hidden UltraVNC Server menu window
		// adzm 2010-02-10 - If we are in SC mode, then we know we want to only post messages to our own instance. This prevents
		// conflicts if the user already has another copy of a UltraVNC Server derived application running.
		if (allowMultipleInstances || settings->getScExit() || settings->getScPrompt()) {
			return PostToThisWinVNC(message, wParam, lParam);
		}

		//adzm 2010-02-10 - Finds the appropriate VNC window
		HWND hservwnd = FindWinVNCWindow(false);
		if (hservwnd == NULL)
			return FALSE;

		// Post the message to UltraVNC Server
		PostMessage(hservwnd, message, wParam, lParam);
		return TRUE;
	}

	HWND FindWinVNCWindow(bool bThisProcess) {
		// Locate the hidden UltraVNC Server menu window
		if (!bThisProcess) {
			// Find any window with the MENU_CLASS_NAME window class
			HWND returnvalue = FindWindow(MENU_CLASS_NAME, NULL);
			if (returnvalue == NULL) goto nullreturn;
			return returnvalue;
		}
		else {
			// Find one that matches the class and is the same process
			HWND hwndZ = NULL;
			HWND hwndServer = NULL;
			while (!hwndServer) {
				hwndServer = FindWindowEx(NULL, hwndZ, MENU_CLASS_NAME, NULL);

				if (hwndServer != NULL) {
					DWORD dwProcessId = 0;
					GetWindowThreadProcessId(hwndServer, &dwProcessId);

					if (dwProcessId == GetCurrentProcessId()) {
						return hwndServer;
					}
					else {
						hwndZ = hwndServer;
						hwndServer = NULL;
					}
				}
				else {
					goto nullreturn;
				}
			}
		}

	nullreturn:
		return G_MENU_HWND;
	}
}

namespace processHelper {
	DWORD GetExplorerLogonPid()
	{
		char alternate_shell[129];
		IniFile myIniFile;
		strcpy_s(alternate_shell, "");
		myIniFile.ReadString("admin", "alternate_shell", alternate_shell, 256);
		DWORD dwSessionId;
		DWORD dwExplorerLogonPid = 0;
		PROCESSENTRY32 procEntry{};
		dwSessionId = WTSGetActiveConsoleSessionId();
		if (GetSystemMetrics(SM_REMOTESESSION)) {
			DWORD dw = GetCurrentProcessId();
			DWORD pSessionId = 0xFFFFFFFF;
			ProcessIdToSessionId(dw, &pSessionId);
			dwSessionId = pSessionId;
		}

		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnap == INVALID_HANDLE_VALUE) {
			return 0;
		}
		procEntry.dwSize = sizeof(PROCESSENTRY32);
		if (!Process32First(hSnap, &procEntry)) {
			CloseHandle(hSnap);
			return 0;
		}

		do {
			if ((_stricmp(procEntry.szExeFile, "explorer.exe") == 0) || (strlen(alternate_shell) != 0 && (_stricmp(procEntry.szExeFile, alternate_shell) == 0))) {
				DWORD dwExplorerSessId = 0;
				if (ProcessIdToSessionId(procEntry.th32ProcessID, &dwExplorerSessId)
					&& dwExplorerSessId == dwSessionId) {
					dwExplorerLogonPid = procEntry.th32ProcessID;
					break;
				}
			}
		} while (Process32Next(hSnap, &procEntry));
		CloseHandle(hSnap);
		return dwExplorerLogonPid;
	}

	bool GetConsoleUser(char* buffer, UINT size)
	{
		DesktopUsersToken desktopUsersToken;
		HANDLE hPToken = desktopUsersToken.getDesktopUsersToken();
		if (hPToken == NULL) {
			strcpy_s(buffer, UNLEN + 1, "");
			return 0;
		}

		char aa[16384]{};
		// token user
		TOKEN_USER* ptu;
		DWORD needed;
		ptu = (TOKEN_USER*)aa;//malloc( 16384 );
		if (GetTokenInformation(hPToken, TokenUser, ptu, 16384, &needed))
		{
			char  DomainName[64];
			memset(DomainName, 0, sizeof(DomainName));
			DWORD DomainSize;
			DomainSize = sizeof(DomainName) - 1;
			SID_NAME_USE SidType;
			DWORD dwsize = size;
			LookupAccountSid(NULL, ptu->User.Sid, buffer, &dwsize, DomainName, &DomainSize, &SidType);
			//free(ptu);
			return 1;
		}
		//free(ptu);
		strcpy_s(buffer, UNLEN + 1, "");
		return 0;
	}

	BOOL GetCurrentUser(char* buffer, UINT size) // RealVNC 336 change
	{
		BOOL	g_impersonating_user = 0;
		if (settings->RunningFromExternalService())
			g_impersonating_user = TRUE;

		// How to obtain the name of the current user depends upon the OS being used
		if (settings->RunningFromExternalService()) {
			// Get the current Window station
			HWINSTA station = GetProcessWindowStation();
			if (station == NULL)
				return FALSE;

			DWORD usersize;
			GetUserObjectInformation(station, UOI_USER_SID, NULL, 0, &usersize);

			// Check the required buffer size isn't zero
			if (usersize == 0) {
				// No user is logged in - ensure we're not impersonating anyone
				RevertToSelf();
				g_impersonating_user = FALSE;
				if (strlen("") >= size)
					return FALSE;
				strcpy_s(buffer, UNLEN + 1, "");
				return TRUE;
			}

			if (!g_impersonating_user) {
				if (strlen("") >= size)
					return FALSE;
				strcpy_s(buffer, UNLEN + 1, "");
				return TRUE;
			}
		}

		DWORD length = size;
		if (GetConsoleUser(buffer, size) == 0) {
			if (GetUserName(buffer, &length) == 0) {
				UINT error = GetLastError();
				if (error == ERROR_NOT_LOGGED_ON) {
					// No user logged on
					if (strlen("") >= size)
						return FALSE;
					strcpy_s(buffer, UNLEN + 1, "");
					return TRUE;
				}
				else {
					// Genuine error...
					vnclog.Print(LL_INTERR, VNCLOG("getusername error %d\n"), GetLastError());
					return FALSE;
				}
			}
		}
		return TRUE;
	}

	BOOL CurrentUser(char* buffer, UINT size)
	{
		BOOL result = GetCurrentUser(buffer, size);
		if (result && (strcmp(buffer, "") == 0) && !settings->RunningFromExternalService()) {
			strncpy_s(buffer, size, "Default", size);
		}
		return result;
	}

	bool IsServiceInstalled()
	{
		BOOL bResult = FALSE;
#ifndef SC_20
		SC_HANDLE hSCM = ::OpenSCManager(NULL, // local machine
			NULL, // ServicesActive database
			SC_MANAGER_ENUMERATE_SERVICE); // full access
		if (hSCM) {
			SC_HANDLE hService = ::OpenService(hSCM,
				UltraVNCService::service_name,
				SERVICE_QUERY_CONFIG);
			if (hService) {
				bResult = TRUE;
				::CloseServiceHandle(hService);
			}
			::CloseServiceHandle(hSCM);
		}
#endif
		return (FALSE != bResult);
	}

	DWORD GetCurrentConsoleSessionID()
	{
		return WTSGetActiveConsoleSessionId();
	}

	BOOL IsWSLocked()
	{
		bool bLocked = false;

		// Original code does not work if running as a service... apparently no access to the desktop.
		// Alternative is to check for a running LogonUI.exe (if present, system is either not logged in or locked)
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

		PROCESSENTRY32W procentry{};
		procentry.dwSize = sizeof(procentry);

		if (Process32FirstW(hSnap, &procentry)) {
			do {
				if (!_wcsicmp(procentry.szExeFile, L"LogonUI.exe")) {
					bLocked = true;
					break;
				}
			} while (Process32NextW(hSnap, &procentry));
		}
		return bLocked;
	}
}