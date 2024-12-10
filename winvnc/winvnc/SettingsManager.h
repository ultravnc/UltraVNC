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


#pragma once
#include "stdhdrs.h"
#include "common/inifile.h"
#include "rfb.h"

#define MAXPWLEN 8
#define MAXMSPWLEN 32
#define MAX_HOST_NAME_LEN 250

class SettingsManager
{
public:
	static SettingsManager* getInstance();

	void load();
	void save();
	void savePassword();
	void saveViewOnlyPassword();
	BOOL getAllowProperties() { return m_pref_allowproperties; };
	BOOL getAllowInjection() { return m_pref_allowInjection; };
	BOOL getAllowShutdown() { return m_pref_allowshutdown; };
	BOOL getAllowEditClients() { return m_pref_alloweditclients; };

	BOOL getSecure() { return m_pref_Secure; };
	char* getService_commandline() { return m_pref_service_commandline; };
	char* getAccept_reject_mesg() { return m_pref_accept_reject_mesg; };
	BOOL getEnableFileTransfer() { return m_pref_EnableFileTransfer; };
	BOOL getFTUserImpersonation() { return m_pref_FTUserImpersonation; };
	BOOL getPrimary() { return m_pref_Primary; };
	BOOL getSecondary() { return m_pref_Secondary; };
	void setPrimary(BOOL value) { m_pref_Primary = value;};
	void setSecondary(BOOL value) { m_pref_Secondary = value;};
	BOOL getEnableBlankMonitor() { return m_pref_EnableBlankMonitor; };
	BOOL getBlankInputsOnly() { return m_pref_BlankInputsOnly; };
	int getDefaultScale() { return m_pref_DefaultScale; };
	UINT getQuerySetting() { return m_pref_QuerySetting; };
	UINT getQueryTimeout() { return m_pref_QueryTimeout; };
	UINT getQueryDisableTime() { return m_pref_QueryDisableTime; };
	UINT getQueryAccept() { return m_pref_QueryAccept; };
	UINT getMaxViewerSetting() { return m_pref_MaxViewerSetting; };
	UINT getMaxViewers() { return m_pref_MaxViewers; };
	BOOL getCollabo() { return m_pref_Collabo; };
	BOOL getFrame() { return m_pref_Frame; };
	BOOL getNotification() { return m_pref_Notification; };
	BOOL getOSD() { return m_pref_OSD; };
	int getNotificationSelection() { return m_pref_NotificationSelection; };
	UINT getIdleTimeout() { return m_pref_IdleTimeout; };
	BOOL getRemoveWallpaper() { return m_pref_RemoveWallpaper; };
	BOOL getRemoveFontSmoothing() { return m_pref_RemoveFontSmoothing; };
	BOOL getRemoveEffects() { return m_pref_RemoveEffects; };
	BOOL getEnableConnections() { return m_pref_EnableConnection; };
	BOOL getHTTPConnect() { return m_pref_EnableHTTPConnect; };
	void setEnableConnections(bool value) { m_pref_EnableConnection = value; };
	void setHTTPConnect(bool value) { m_pref_EnableHTTPConnect = value; };
	BOOL getEnableRemoteInputs() { return m_pref_EnableRemoteInputs; };
	BOOL getDisableLocalInputs() { return m_pref_DisableLocalInputs; };
	BOOL getEnableJapInput() { return m_pref_EnableJapInput; };
	BOOL getEnableUnicodeInput() { return m_pref_EnableUnicodeInput; };
	BOOL getEnableWin8Helper() { return m_pref_EnableWin8Helper; };
	BOOL getClearconsole() { return m_pref_clearconsole; };
	char* getPasswd() { return m_pref_passwd; };
	char* getPasswdViewOnly() { return m_pref_passwdViewOnly; };
	BOOL getAutoPortSelect() { return m_pref_AutoPortSelect; };
	LONG getPortNumber() { return m_pref_PortNumber; };
	LONG getHttpPortNumber() { return m_pref_HttpPortNumber; };
	int getLockSettings() { return m_pref_LockSettings; };
	char* getDSMPluginConfig() { return m_pref_DSMPluginConfig; };
	char* getSzDSMPlugin() { 
		return m_pref_szDSMPlugin; };
	BOOL getUseDSMPlugin() { return m_pref_UseDSMPlugin; };
	void setService_commandline(char* value) { strcpy_s(m_pref_service_commandline, value); };
	void setAccept_reject_mesg(char* value) { strcpy_s(m_pref_accept_reject_mesg, value); };
	BOOL getDebugMode() { return m_pref_DebugMode; };
	void setDebugMode(BOOL value) { m_pref_DebugMode = value; };

	BOOL getDebugLevel() { return m_pref_DebugLevel; };
	BOOL getAvilog() { return m_pref_Avilog; };
	char* getDebugPath() { return m_pref_DebugPath; };
	BOOL getAllowLoopback() { return m_pref_AllowLoopback; };
	BOOL getAuthRequired() { return m_pref_AuthRequired; };
	void setAuthRequired(BOOL value) { m_pref_AuthRequired = value; };
	char* getAuthhosts() { return m_pref_authhosts; };
	BOOL getDisableTrayIcon() { return m_pref_DisableTrayIcon; };
	BOOL getRdpmode() { return m_pref_Rdpmode; };
	BOOL getnoscreensaver() { return m_pref_Noscreensaver; };
	BOOL getLoopbackOnly() { return m_pref_LoopbackOnly; };
	int getConnectPriority() { return m_pref_ConnectPriority; };
	BOOL getRequireMSLogon() { return m_pref_RequireMSLogon; };
	BOOL getNewMSLogon() { return m_pref_NewMSLogon; };
	BOOL getReverseAuthRequired() { return m_pref_ReverseAuthRequired; };
	int getftTimeout() { return m_pref_ftTimeout; };
	int getkeepAliveInterval() { return m_pref_keepAliveInterval; };
	int getIdleInputTimeout() { return m_pref_IdleInputTimeout; };
	BOOL getQueryIfNoLogon() { return m_pref_QueryIfNoLogon; };
	UINT getSENDBUFFER_EX() { return G_SENDBUFFER_EX; };

	void setQueryIfNoLogon(BOOL value) { m_pref_QueryIfNoLogon = value; };
	void setAllowProperties(BOOL value) { m_pref_allowproperties = value; };
	void setAllowInjection(BOOL value) { m_pref_allowInjection = value; };
	void setAllowShutdown(BOOL value) { m_pref_allowshutdown = value; };
	void setAllowEditClients(BOOL value) { m_pref_alloweditclients = value; };
	void setSecure(BOOL value) { m_pref_Secure = value; };
	void setDisableTrayIcon(BOOL value) { m_pref_DisableTrayIcon = value; };
	void setRdpmode(BOOL value) { m_pref_Rdpmode = value; };
	void setNoScreensaver(BOOL value) { m_pref_Noscreensaver = value; };
	void setEnableFileTransfer(BOOL value) { m_pref_EnableFileTransfer = value; };
	void setFTUserImpersonation(BOOL value) { m_pref_FTUserImpersonation = value; };
	void setAutoPortSelect(BOOL value) { m_pref_AutoPortSelect = value; };
	void setPortNumber(LONG port) { m_pref_PortNumber = port; };
	void setHttpPortNumber(LONG port) { m_pref_HttpPortNumber = port; };


	void setAuthhosts(char* value) { strcpy_s(m_pref_authhosts, value); };

	void setEnableRemoteInputs(BOOL value) { m_pref_EnableRemoteInputs = value; };
	void setDisableLocalInputs(BOOL value) { m_pref_DisableLocalInputs = value; };
	void setEnableJapInput(BOOL value) { m_pref_EnableJapInput = value; };
	void setEnableUnicodeInput(BOOL value) { m_pref_EnableUnicodeInput = value; };
	void setLoopbackOnly(BOOL value) { m_pref_LoopbackOnly = value; if (value) setAllowLoopback(true);};
	void setAllowLoopback(BOOL value) { m_pref_AllowLoopback = value; };
	void setQuerySetting(UINT value) { m_pref_QuerySetting = value; };
	void setConnectPriority(int value) { m_pref_ConnectPriority = value; };
	void setQueryDisableTime(UINT value) { m_pref_QueryDisableTime = value; };
	void setQueryTimeout(UINT value) { m_pref_QueryTimeout = value; };
	void setDebugPath(char* value) { strcpy_s(m_pref_DebugPath, value);};

	void setMaxViewers(UINT value) { m_pref_MaxViewers = value; };
	void setCollabo(BOOL value) { m_pref_Collabo = value; };
	void setMaxViewerSetting(UINT value) { m_pref_MaxViewerSetting = value; };
	void setQueryAccept(UINT value) { m_pref_QueryAccept = value; };

	void setLockSettings(int value) { m_pref_LockSettings = value; };
	void setRemoveWallpaper(BOOL value) { m_pref_RemoveWallpaper = value; };
	void setEnableWin8Helper(BOOL value) { m_pref_EnableWin8Helper = value; };
	void setFrame(BOOL value) { m_pref_Frame = value; };
	void setNotification(BOOL value) { m_pref_Notification = value; };
	void setNotificationSelection(int value) { m_pref_NotificationSelection = value; };
	void setOSD(BOOL value) { m_pref_OSD = value; };

	void setRequireMSLogon(BOOL value) { m_pref_RequireMSLogon = value; };
	void setNewMSLogon(BOOL value) { m_pref_NewMSLogon = value; };
	void setReverseAuthRequired(BOOL value) { m_pref_ReverseAuthRequired = value; };

	void setEnableBlankMonitor(BOOL value) { m_pref_EnableBlankMonitor = value; };
	void setBlankInputsOnly(BOOL value) { m_pref_BlankInputsOnly = value; };

	void setPasswd(const char* passwd)
	{
		memcpy(m_pref_passwd, passwd, MAXPWLEN);
	}
	void setPasswdViewOnly(const char* passwd)
	{
		memcpy(m_pref_passwdViewOnly, passwd, MAXPWLEN);
	}

	void setDSMPluginConfig(char* value) { strncpy_s(m_pref_DSMPluginConfig, sizeof(m_pref_DSMPluginConfig) - 1, value, _TRUNCATE); };
	void setSzDSMPlugin(char* value) { 
		strncpy_s(m_pref_szDSMPlugin, sizeof(m_pref_szDSMPlugin) - 1, value, _TRUNCATE); 
	};
	void setUseDSMPlugin(BOOL value) { m_pref_UseDSMPlugin = value; };
	void setDefaultScale(int value) { m_pref_DefaultScale = value; };

	void setkeepAliveInterval(int secs);
	void setIdleTimeout(int secs);
	void setIdleInputTimeout(int secs);
	void setftTimeout(int value) { m_pref_ftTimeout = value; };

	void EnableServerStateUpdates(bool newstate) { m_pref_fEnableStateUpdates = newstate; }
	bool DoServerStateUpdates() { return m_pref_fEnableStateUpdates; }
	void EnableKeepAlives(bool newstate) { m_pref_fEnableKeepAlive = newstate; }
	bool DoKeepAlives() { return m_pref_fEnableKeepAlive; }
	BOOL RunningFromExternalService() { return m_pref_fRunningFromExternalService; };
	void setRunningFromExternalService(BOOL fEnabled);
	BOOL RunningFromExternalServiceRdp() { return m_pref_fRunningFromExternalServiceRdp; };
	void setRunningFromExternalServiceRdp(BOOL fEnabled) { m_pref_fRunningFromExternalServiceRdp = fEnabled; };
	void AutoRestartFlag(BOOL fOn) { m_pref_fAutoRestart = fOn; };
	BOOL AutoRestartFlag() { return m_pref_fAutoRestart; };

	void setScExit(BOOL value) { m_pref_ScExit = value; };
	void setScPrompt(BOOL value) { m_pref_ScPrompt = value; };
	BOOL getScExit() { return m_pref_ScExit; };
	BOOL getScPrompt() { return m_pref_ScPrompt; };

	void setTurboMode(BOOL value) { m_pref_TurboMode = value; };
	BOOL getTurboMode() { return m_pref_TurboMode; };
	void setPollUnderCursor(BOOL value) { m_pref_PollUnderCursor = value; };
	BOOL getPollUnderCursor() { return m_pref_PollUnderCursor; };
	void setPollForeground(BOOL value) { m_pref_PollForeground = value; };
	BOOL getPollForeground() { return m_pref_PollForeground; };

	void setPollFullScreen(BOOL value) { m_pref_PollFullScreen = value; };
	BOOL getPollFullScreen() { return m_pref_PollFullScreen; };

	void setPollConsoleOnly(BOOL value) { m_pref_PollConsoleOnly = value; };
	BOOL getPollConsoleOnly() { return m_pref_PollConsoleOnly; };

	void setPollOnEventOnly(BOOL value) { m_pref_PollOnEventOnly = value; };
	BOOL getPollOnEventOnly() { return m_pref_PollOnEventOnly; };

	void setVirtual(BOOL value) { m_pref_Virtual = value; };
	BOOL getVirtual() { return m_pref_Virtual; };

	void setHook(BOOL value) { m_pref_Hook = value; };
	BOOL getHook() { return m_pref_Hook; };

	void setDriver(BOOL value) { m_pref_Driver = value; };
	BOOL getDriver() { return m_pref_Driver; };

	void setddEngine(BOOL value) { m_pref_ddEngine = value; };
	BOOL getddEngine() { return m_pref_ddEngine; };

	void setMaxCpu(BOOL value) { m_pref_MaxCpu = value; };
	BOOL getMaxCpu() { return m_pref_MaxCpu; };
	void setMaxFPS(BOOL value) { m_pref_MaxFPS = value; };
	BOOL getMaxFPS() { return m_pref_MaxFPS; };
	void setAutocapt(BOOL value) { m_pref_autocapt = value; };
	BOOL getAutocapt() { return m_pref_autocapt; };

	void setGroup1(TCHAR* value) { strcpy_s(m_pref_group1, value); };
	TCHAR* getGroup1() { return m_pref_group1; };
	void setGroup2(TCHAR* value) { strcpy_s(m_pref_group2, value); };
	TCHAR* getGroup2() { return m_pref_group2; };
	void setGroup3(TCHAR* value) { strcpy_s(m_pref_group3, value); };
	TCHAR* getGroup3() { return m_pref_group3; };

	void setLocdom1(BOOL value) { m_pref_locdom1 = value; };
	BOOL getLocdom1() { return m_pref_locdom1; };
	void setLocdom2(BOOL value) { m_pref_locdom2 = value; };
	BOOL getLocdom2() { return m_pref_locdom2; };
	void setLocdom3(BOOL value) { m_pref_locdom3 = value; };
	BOOL getLocdom3() { return m_pref_locdom3; };

	void setCloudEnabled(BOOL value) { m_pref_cloudEnabled = value; };
	BOOL getCloudEnabled() { return m_pref_cloudEnabled; };

	void setCloudServer(TCHAR* value) { strcpy_s(m_pref_cloudServer, value); };
	TCHAR* getCloudServer() { return m_pref_cloudServer; };


	// Whether or not to allow connections from the local machine
	void setIPV6(BOOL ok) { m_pref_ipv6_allowed = ok; };
	BOOL getIPV6() { return m_pref_ipv6_allowed; };

	bool IsRunninAsAdministrator();
	bool IsDesktopUserAdmin();
	bool getAllowUserSettingsWithPassword();
	void setAllowUserSettingsWithPassword(bool value);
	bool checkAdminPassword();
	void setAdminPasswordHash(char *password);
	bool isAdminPasswordSet();
	void setShowAllLogs(bool value) { showAllLogs = value; }
	bool getShowAllLogs() { return showAllLogs; }

private:
	SettingsManager();
	static SettingsManager* s_instance;
	void setDefaults();
	void initTemp();
	IniFile myIniFile;

	BOOL	m_pref_allowproperties;
	BOOL	m_pref_allowInjection;
	BOOL	m_pref_allowshutdown;
	BOOL	m_pref_alloweditclients;
	int     m_pref_ftTimeout;
	int     m_pref_keepAliveInterval;
	int		m_pref_IdleInputTimeout;
	char m_pref_service_commandline[1024];
	char m_pref_accept_reject_mesg[512];
	BOOL m_pref_EnableConnection;
	BOOL m_pref_EnableHTTPConnect;

	BOOL m_pref_AutoPortSelect;
	LONG m_pref_PortNumber;
	LONG m_pref_HttpPortNumber;
	char m_pref_passwd[MAXPWLEN];
	char m_pref_passwdViewOnly[MAXPWLEN];
	UINT m_pref_QuerySetting;
	UINT m_pref_QueryIfNoLogon;
	UINT m_pref_QueryAccept;
	UINT m_pref_QueryTimeout;
	UINT m_pref_QueryDisableTime;
	UINT m_pref_IdleTimeout;
	UINT m_pref_MaxViewerSetting;
	UINT m_pref_MaxViewers;
	BOOL m_pref_Collabo;
	BOOL m_pref_RemoveWallpaper;
	BOOL m_pref_RemoveEffects;
	BOOL m_pref_RemoveFontSmoothing;
	BOOL m_pref_EnableRemoteInputs;
	int m_pref_LockSettings;
	BOOL m_pref_DisableLocalInputs;
	BOOL m_pref_EnableJapInput;
	BOOL m_pref_EnableUnicodeInput;
	BOOL m_pref_EnableWin8Helper;
	BOOL m_pref_clearconsole;
	BOOL m_pref_EnableFileTransfer;
	BOOL m_pref_FTUserImpersonation;
	BOOL m_pref_EnableBlankMonitor;
	BOOL m_pref_BlankInputsOnly;
	int  m_pref_DefaultScale;
	BOOL m_pref_RequireMSLogon;
	BOOL m_pref_Secure;
	BOOL m_pref_NewMSLogon;
	BOOL m_pref_ReverseAuthRequired;
	BOOL m_pref_UseDSMPlugin;
	char m_pref_szDSMPlugin[128];
	char m_pref_DSMPluginConfig[512];
	BOOL m_pref_Primary;
	BOOL m_pref_Secondary;
	BOOL m_pref_Frame;
	BOOL m_pref_Notification;
	BOOL m_pref_OSD;
	int	m_pref_NotificationSelection;
	BOOL m_pref_DisableTrayIcon;
	BOOL m_pref_Rdpmode;
	BOOL m_pref_Noscreensaver;
	BOOL m_pref_LoopbackOnly;
	BOOL m_pref_AllowLoopback;
	BOOL m_pref_AuthRequired;
	int m_pref_ConnectPriority;
	char m_pref_authhosts[1280];
	char m_pref_authhosts2[1280];

	BOOL m_pref_DebugMode;
	char m_pref_DebugPath[512];
	BOOL m_pref_DebugLevel;
	BOOL m_pref_Avilog;
	BOOL m_pref_UseIpv6;
	unsigned int G_SENDBUFFER_EX;
	char m_Inifile[MAX_PATH];
	bool m_pref_fEnableStateUpdates;
	bool m_pref_fEnableKeepAlive;
	BOOL m_pref_fRunningFromExternalService;
	BOOL m_pref_fRunningFromExternalServiceRdp;
	BOOL m_pref_fAutoRestart;

	bool m_pref_ScExit;
	bool m_pref_ScPrompt;

	BOOL m_pref_TurboMode;
	BOOL m_pref_PollUnderCursor;
	BOOL m_pref_PollForeground;
	BOOL m_pref_PollFullScreen;
	BOOL m_pref_PollConsoleOnly;
	BOOL m_pref_PollOnEventOnly;
	LONG m_pref_MaxCpu;
	LONG m_pref_MaxFPS;
	BOOL m_pref_Driver;
	BOOL m_pref_Hook;
	BOOL m_pref_Virtual;
	bool m_pref_ddEngine;
	int m_pref_autocapt;
	BOOL m_pref_ipv6_allowed;
	bool m_pref_RunninAsAdministrator;

	char m_pref_group1[150];
	char m_pref_group2[150];
	char m_pref_group3[150];

	BOOL m_pref_locdom1;
	BOOL m_pref_locdom2;
	BOOL m_pref_locdom3;

	TCHAR m_pref_cloudServer[MAX_HOST_NAME_LEN];
	bool m_pref_cloudEnabled;
	bool m_pref_AllowUserSettingsWithPassword;
	bool showAllLogs = false;
};

extern SettingsManager* settings;
