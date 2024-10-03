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


#include "inifile.h"
#define MAX_HOST_NAME_LEN 250
DWORD MessageBoxSecure(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType);

bool do_copy(IniFile& myIniFile_In, IniFile& myIniFile_Out)
{
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
	LONG DebugMode;
	LONG Avilog;
	LONG DebugLevel;
	LONG DisableTrayIcon;
	LONG Rdpmode = 0;
	LONG NoScreensaver = 0;
	LONG LoopbackOnly;
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
	LONG PortNumber = 5900;
	LONG HttpPortNumber = 5800;
	LONG IdleTimeout = 0;
	LONG IdleInputTimeout = 0;

	LONG RemoveWallpaper = 0;

	LONG QuerySetting = 1;
	LONG QueryTimeout = 10;
	LONG QueryDisableTime = 0;
	LONG QueryAccept = 4;
	LONG QueryIfNoLogon = 1;

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
	LONG Virtual = 0;
	LONG SingleWindow = 0;
	char SingleWindowName[32];
	LONG FTTimeout = 30;
	char path[512];
	char service_commandline[1024];
	char accept_reject_mesg[512];
	LONG MaxCpu = 100;
	LONG MaxFPS = 25;

	char DSMPluginConfig[512];
	*DSMPluginConfig = '\0';

	LONG Primary = 1;
	LONG Secondary = 0;
	//Beep(100,20000);
	BUseRegistry = myIniFile_In.ReadInt("admin", "UseRegistry", 0);
	if (!myIniFile_Out.WriteInt("admin", "UseRegistry", BUseRegistry))
	{
		//error
		char temp[10];
		DWORD error = GetLastError();
		MessageBoxSecure(NULL, myIniFile_Out.myInifile, _itoa(error, temp, 10), MB_ICONERROR);
		return false;
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
	myIniFile_In.ReadString("admin", "service_commandline", service_commandline, 1024);

	myIniFile_In.ReadString("admin", "accept_reject_mesg", accept_reject_mesg, 512);
	DebugLevel = myIniFile_In.ReadInt("admin", "DebugLevel", 0);
	DisableTrayIcon = myIniFile_In.ReadInt("admin", "DisableTrayIcon", false);
	Rdpmode = myIniFile_In.ReadInt("admin", "rdpmode", 0);
	NoScreensaver = myIniFile_In.ReadInt("admin", "noscreensaver", 0);
	LoopbackOnly = myIniFile_In.ReadInt("admin", "LoopbackOnly", false);

	myIniFile_Out.WriteInt("admin", "DebugMode", DebugMode);
	myIniFile_Out.WriteInt("admin", "Avilog", Avilog);
	myIniFile_Out.WriteString("admin", "path", path);
	myIniFile_Out.WriteString("admin", "service_commandline", service_commandline);
	myIniFile_Out.WriteString("admin", "accept_reject_mesg", accept_reject_mesg);
	myIniFile_Out.WriteInt("admin", "DebugLevel", DebugLevel);
	myIniFile_Out.WriteInt("admin", "DisableTrayIcon", DisableTrayIcon);
	myIniFile_Out.WriteInt("admin", "rdpmode", Rdpmode);
	myIniFile_Out.WriteInt("admin", "noscreensaver", NoScreensaver);
	myIniFile_Out.WriteInt("admin", "LoopbackOnly", LoopbackOnly);

	UseDSMPlugin = myIniFile_In.ReadInt("admin", "UseDSMPlugin", false);
	myIniFile_In.ReadString("admin", "DSMPluginConfig", DSMPluginConfig, 512);
	AllowLoopback = myIniFile_In.ReadInt("admin", "AllowLoopback", true);
	AuthRequired = myIniFile_In.ReadInt("admin", "AuthRequired", true);
	ConnectPriority = myIniFile_In.ReadInt("admin", "ConnectPriority", 0);

	myIniFile_Out.WriteInt("admin", "UseDSMPlugin", UseDSMPlugin);
	myIniFile_Out.WriteString("admin", "DSMPluginConfig", DSMPluginConfig);
	myIniFile_Out.WriteInt("admin", "AllowLoopback", AllowLoopback);
	myIniFile_Out.WriteInt("admin", "AuthRequired", AuthRequired);
	myIniFile_Out.WriteInt("admin", "ConnectPriority", ConnectPriority);

	myIniFile_In.ReadString("admin", "DSMPlugin", DSMPlugin, 128);
	myIniFile_In.ReadString("admin", "AuthHosts", authhosts, 1280);

	myIniFile_Out.WriteString("admin", "DSMPlugin", DSMPlugin);
	myIniFile_Out.WriteString("admin", "AuthHosts", authhosts);

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
	FTTimeout = myIniFile_In.ReadInt("admin", "FileTransferTimeout", 30);

	Primary = myIniFile_In.ReadInt("admin", "primary", true);
	Secondary = myIniFile_In.ReadInt("admin", "secondary", false);

	myIniFile_Out.WriteInt("admin", "FileTransferEnabled", FileTransferEnabled);
	myIniFile_Out.WriteInt("admin", "FTUserImpersonation", FTUserImpersonation);
	myIniFile_Out.WriteInt("admin", "BlankMonitorEnabled", BlankMonitorEnabled);
	myIniFile_Out.WriteInt("admin", "BlankInputsOnly", BlankInputsOnly); //PGM
	myIniFile_Out.WriteInt("admin", "DefaultScale", DefaultScale);
	myIniFile_Out.WriteInt("admin", "FileTransferTimeout", 30);

	myIniFile_Out.WriteInt("admin", "primary", Primary);
	myIniFile_Out.WriteInt("admin", "secondary", Secondary);

	// Connection prefs
	SocketConnect = myIniFile_In.ReadInt("admin", "SocketConnect", true);
	HTTPConnect = myIniFile_In.ReadInt("admin", "HTTPConnect", true);
	AutoPortSelect = myIniFile_In.ReadInt("admin", "AutoPortSelect", true);
	PortNumber = myIniFile_In.ReadInt("admin", "PortNumber", PortNumber);
	HttpPortNumber = myIniFile_In.ReadInt("admin", "HTTPPortNumber", HttpPortNumber);
	IdleTimeout = myIniFile_In.ReadInt("admin", "IdleTimeout", IdleTimeout);
	IdleInputTimeout = myIniFile_In.ReadInt("admin", "IdleInputTimeout", IdleInputTimeout);
	myIniFile_Out.WriteInt("admin", "SocketConnect", SocketConnect);
	myIniFile_Out.WriteInt("admin", "HTTPConnect", HTTPConnect);
	myIniFile_Out.WriteInt("admin", "AutoPortSelect", AutoPortSelect);
	myIniFile_Out.WriteInt("admin", "PortNumber", PortNumber);
	myIniFile_Out.WriteInt("admin", "HTTPPortNumber", HttpPortNumber);
	myIniFile_Out.WriteInt("admin", "IdleTimeout", IdleTimeout);
	myIniFile_Out.WriteInt("admin", "IdleInputTimeout", IdleInputTimeout);

	RemoveWallpaper = myIniFile_In.ReadInt("admin", "RemoveWallpaper", RemoveWallpaper);
	myIniFile_Out.WriteInt("admin", "RemoveWallpaper", RemoveWallpaper);

	// Connection querying settings
	QuerySetting = myIniFile_In.ReadInt("admin", "QuerySetting", QuerySetting);
	QueryTimeout = myIniFile_In.ReadInt("admin", "QueryTimeout", QueryTimeout);
	QueryTimeout = myIniFile_In.ReadInt("admin", "QueryDisableTime", QueryDisableTime);
	QueryAccept = myIniFile_In.ReadInt("admin", "QueryAccept", QueryAccept);
	QueryIfNoLogon = myIniFile_In.ReadInt("admin", "QueryIfNoLogon", QueryIfNoLogon);
	myIniFile_Out.WriteInt("admin", "QuerySetting", QuerySetting);
	myIniFile_Out.WriteInt("admin", "QueryTimeout", QueryTimeout);
	myIniFile_Out.WriteInt("admin", "QueryDisableTime", QueryDisableTime);
	myIniFile_Out.WriteInt("admin", "QueryAccept", QueryAccept);
	myIniFile_Out.WriteInt("admin", "QueryIfNoLogon", QueryIfNoLogon);

	MaxViewerSettings = myIniFile_In.ReadInt("admin", "MaxViewerSetting", MaxViewerSettings);
	myIniFile_Out.WriteInt("admin", "MaxViewerSetting", MaxViewerSettings);
	MaxViewers = myIniFile_In.ReadInt("admin", "MaxViewers", MaxViewers);
	myIniFile_Out.WriteInt("admin", "MaxViewers", MaxViewers);
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
	myIniFile_In.ReadPassword(passwd, MAXPWLEN);
	myIniFile_Out.WritePassword(passwd);
	memset(passwd, '\0', MAXPWLEN); //PGM
	myIniFile_In.ReadPassword2(passwd, MAXPWLEN); //PGM
	myIniFile_Out.WritePassword2(passwd); //PGM
	myIniFile_In.ReadString("admin", "cloudServer", cloudServer, MAX_HOST_NAME_LEN);
	myIniFile_Out.WriteString("admin", "cloudServer", cloudServer);
	cloudEnabled = myIniFile_In.ReadInt("admin", "cloudEnabled", cloudEnabled);
	myIniFile_Out.WriteInt("admin", "cloudEnabled", cloudEnabled);
	

	EnableRemoteInputs = myIniFile_In.ReadInt("admin", "InputsEnabled", EnableRemoteInputs);
	LockSettings = myIniFile_In.ReadInt("admin", "LockSetting", LockSettings);
	DisableLocalInputs = myIniFile_In.ReadInt("admin", "LocalInputsDisabled", DisableLocalInputs);
	EnableJapInput = myIniFile_In.ReadInt("admin", "EnableJapInput", EnableJapInput);
	EnableUnicodeInput = myIniFile_In.ReadInt("admin", "EnableUnicodeInput", EnableUnicodeInput);
	EnableWin8Helper = myIniFile_In.ReadInt("admin", "EnableWin8Helper", EnableWin8Helper);
	kickrdp = myIniFile_In.ReadInt("admin", "kickrdp", kickrdp);
	clearconsole = myIniFile_In.ReadInt("admin", "clearconsole", clearconsole);

	myIniFile_Out.WriteInt("admin", "InputsEnabled", EnableRemoteInputs);
	myIniFile_Out.WriteInt("admin", "LockSetting", LockSettings);
	myIniFile_Out.WriteInt("admin", "LocalInputsDisabled", DisableLocalInputs);
	myIniFile_Out.WriteInt("admin", "EnableJapInput", EnableJapInput);
	myIniFile_Out.WriteInt("admin", "EnableUnicodeInput", EnableUnicodeInput);
	myIniFile_Out.WriteInt("admin", "EnableWin8Helper", EnableWin8Helper);
	myIniFile_Out.WriteInt("admin", "kickrdp", kickrdp);
	myIniFile_Out.WriteInt("admin", "clearconsole", clearconsole);

	TurboMode = myIniFile_In.ReadInt("poll", "TurboMode", TurboMode);
	PollUnderCursor = myIniFile_In.ReadInt("poll", "PollUnderCursor", PollUnderCursor);
	PollForeground = myIniFile_In.ReadInt("poll", "PollForeground", PollForeground);
	PollFullScreen = myIniFile_In.ReadInt("poll", "PollFullScreen", PollFullScreen);
	PollConsoleOnly = myIniFile_In.ReadInt("poll", "OnlyPollConsole", PollConsoleOnly);
	PollOnEventOnly = myIniFile_In.ReadInt("poll", "OnlyPollOnEvent", PollOnEventOnly);
	MaxCpu = myIniFile_In.ReadInt("poll", "MaxCpu2", MaxCpu);
	MaxFPS = myIniFile_In.ReadInt("poll", "MaxFPS", MaxFPS);
	Driver = myIniFile_In.ReadInt("poll", "EnableDriver", Driver);
	Hook = myIniFile_In.ReadInt("poll", "EnableHook", Hook);
	Virtual = myIniFile_In.ReadInt("poll", "EnableVirtual", Virtual);

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
	return true;
}

bool Copy_to_Temp(char* tempfile)
{
	IniFile myIniFile_In;
	IniFile myIniFile_Out;
	myIniFile_In.IniFileSetSecure();
	myIniFile_Out.IniFileSetTemp(tempfile);

	return do_copy(myIniFile_In, myIniFile_Out);
}

bool Copy_to_Secure_from_temp(char* tempfile)
{
	IniFile myIniFile_In;
	IniFile myIniFile_Out;
	myIniFile_Out.IniFileSetSecure();
	myIniFile_In.IniFileSetTemp(tempfile);

	return do_copy(myIniFile_In, myIniFile_Out);
}