/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
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


#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
// vncMenu

// Implementation of a system Tray icon & menu for UltraVNC Server

#include "stdhdrs.h"
#include "winvnc.h"
#include "vncconndialog.h"
#include <lmcons.h>
#include <wininet.h>
#include <shlobj.h>

// Header

#include "vncmenu.h"
#include "HideDesktop.h"
#include "../common/win32_helpers.h"
#include "vncOSVersion.h"
#include "UltraVNCService.h"
#include "SettingsManager.h"
#include "dwmapi.h"
#include "credentials.h"
#include "ScSelect.h"
#include "Localization.h"

#ifndef __GNUC__
// [v1.0.2-jp1 fix]
#pragma comment(lib, "imm32.lib")
#endif
#include <WtsApi32.h>
#pragma comment(lib, "Dwmapi.lib")

// adzm 2009-07-05 - Tray icon balloon tips
// adzm 2010-02-10 - Changed this window message (added 2) to prevent receiving the same message from older UltraVNC builds
// which will send this message between processes with process-local pointers to strings as the wParam and lParam

BOOL g_restore_ActiveDesktop = FALSE;
// [v1.0.2-jp1 fix] Load resouce from dll
extern HINSTANCE	hInstResDLL;

extern bool	fShutdownOrdered;
extern wchar_t g_hookstring[16];

static BOOL AeroWasEnabled = FALSE;

static unsigned int WM_TASKBARCREATED = 0;
void Open_homepage();
void Open_forum();
void Open_github();
void Open_mastodon();
void Open_facebook();
void Open_xtwitter();
void Open_reddit();
void Open_openhub();

//HACK to use name in autoreconnect from service with dyn dns
extern char dnsname[255];

BOOL pfnDwmEnableCompositiond = FALSE;
static inline VOID DisableAero(VOID)
{
	if (!(SUCCEEDED(DwmIsCompositionEnabled(&pfnDwmEnableCompositiond))))
		return;

	if (SUCCEEDED(DwmEnableComposition(FALSE))) {
		AeroWasEnabled = pfnDwmEnableCompositiond;
	}
}

static inline VOID ResetAero(VOID)
{
	vnclog.Print(LL_INTINFO, VNCLOG("Reset %i \n"), AeroWasEnabled);
	if (AeroWasEnabled)
	{
		DwmEnableComposition(AeroWasEnabled);
	}
}

// adzm - 2010-07 - Disable more effects or font smoothing
static bool IsUserDesktop()
{
	//only kill wallpaper if desktop is user desktop
	HDESK desktop = GetThreadDesktop(GetCurrentThreadId());
	DWORD dummy;
	char new_name[256]{};
	if (GetUserObjectInformation(desktop, UOI_NAME, &new_name, 256, &dummy))
	{
		if (strcmp(new_name, "Default") == 0) {
			return true;
		}
	}

	return false;
}

// adzm - 2010-07 - Disable more effects or font smoothing
static void KillWallpaper()
{
#ifdef SC_20	
	if (!ScSelect::g_wallpaper_enabled) DisableAero();
	if (!ScSelect::g_wallpaper_enabled)HideDesktop();
	Sleep(200);
#else
	HideDesktop();
#endif // SC_20
}

static void RestoreWallpaper()
{
#ifdef SC_20
	if (!ScSelect::g_wallpaper_enabled)ResetAero();
	if (!ScSelect::g_wallpaper_enabled)RestoreDesktop();	
#else
	RestoreDesktop();
#endif // SC_20
}

// adzm - 2010-07 - Disable more effects or font smoothing
static void KillEffects()
{
	DisableEffects();
}

// adzm - 2010-07 - Disable more effects or font smoothing
static void RestoreEffects()
{
	EnableEffects();
}

// adzm - 2010-07 - Disable more effects or font smoothing
static void KillFontSmoothing()
{
	DisableFontSmoothing();
}

// adzm - 2010-07 - Disable more effects or font smoothing
static void RestoreFontSmoothing()
{
	EnableFontSmoothing();
}

// Implementation

vncMenu::vncMenu(vncServer* server)
{
	vnclog.Print(LL_INTERR, VNCLOG("vncmenu(server)\n"));
	ports_set = false;
	CoInitialize(0);
	IsIconSet = false;
	IconFaultCounter = 0;

	ChangeWindowMessageFilter(postHelper::MENU_AUTO_RECONNECT_MSG, MSGFLT_ADD);
	ChangeWindowMessageFilter(postHelper::MENU_STOP_RECONNECT_MSG, MSGFLT_ADD);
	ChangeWindowMessageFilter(postHelper::MENU_STOP_ALL_RECONNECT_MSG, MSGFLT_ADD);
	ChangeWindowMessageFilter(postHelper::MENU_REPEATER_ID_MSG, MSGFLT_ADD);
	ChangeWindowMessageFilter(postHelper::MENU_TRAYICON_BALLOON_MSG, MSGFLT_ADD);

	// adzm 2009-07-05 - Tray icon balloon tips
	m_BalloonInfo = NULL;
	m_BalloonTitle = NULL;

	// Save the server pointer
	m_server = server;

	// Set the initial user name to something sensible...
	processHelper::CurrentUser((char*)&m_username, sizeof(m_username));

	//if (strcmp(m_username, "") == 0)
	//	strcpy_s((char *)&m_username, "SYSTEM");
	//vnclog.Print(LL_INTERR, VNCLOG("########### vncMenu::vncMenu - UserName = %s\n"), m_username);

	// Create a dummy window to handle Tray icon messages
	WNDCLASSEX wndclass{};

	wndclass.cbSize = sizeof(wndclass);
	wndclass.style = 0;
	wndclass.lpfnWndProc = vncMenu::WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hAppInstance;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = (const char*)NULL;
	wndclass.lpszClassName = MENU_CLASS_NAME;
	wndclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	RegisterClassEx(&wndclass);

	m_hwnd = CreateWindow(MENU_CLASS_NAME,
		MENU_CLASS_NAME,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		200, 200,
		NULL,
		NULL,
		hAppInstance,
		NULL);
	G_MENU_HWND = m_hwnd;
	if (m_hwnd == NULL)
	{
		PostQuitMessage(0);
		return;
	}

	// record which client created this window
	helper::SafeSetWindowUserData(m_hwnd, (LONG_PTR)this);

	// Ask the server object to notify us of stuff
	server->AddNotify(m_hwnd);

	// Initialise the properties dialog object
	if (!m_properties.Init(m_server))
	{
		PostQuitMessage(0);
		return;
	}
	if (!m_propertiesPoll.Init(m_server))
	{
		PostQuitMessage(0);
		return;
	}

	m_server->setVNcPort();
	if (settings->getAllowInjection()) {
		ChangeWindowMessageFilter(postHelper::MENU_ADD_CLIENT_MSG, MSGFLT_ADD);
		ChangeWindowMessageFilter(postHelper::MENU_ADD_CLIENT_MSG_INIT, MSGFLT_ADD);
		ChangeWindowMessageFilter(postHelper::MENU_ADD_CLOUD_MSG, MSGFLT_ADD);
#ifdef IPV6V4
		ChangeWindowMessageFilter(postHelper::MENU_ADD_CLIENT6_MSG, MSGFLT_ADD);
		ChangeWindowMessageFilter(postHelper::MENU_ADD_CLIENT6_MSG_INIT, MSGFLT_ADD);
#endif
	}

	SetTimer(m_hwnd, 1, 5000, NULL);

	// sf@2002
	if (!m_ListDlg.Init(m_server))
	{
		PostQuitMessage(0);
		return;
	}

	// Load the icons for the tray
//	m_winvnc_icon = LoadIcon(hAppInstance, MAKEINTRESOURCE(IDI_WINVNC));
//	m_flash_icon = LoadIcon(hAppInstance, MAKEINTRESOURCE(IDI_FLASH));
	{
		m_winvnc_icon = (HICON)LoadImage(NULL, "icon1.ico", IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
			GetSystemMetrics(SM_CYSMICON), LR_LOADFROMFILE | LR_DEFAULTCOLOR);
		m_flash_icon = (HICON)LoadImage(NULL, "icon2.ico", IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
			GetSystemMetrics(SM_CYSMICON), LR_LOADFROMFILE | LR_DEFAULTCOLOR);
		// [v1.0.2-jp1 fix]
		//if (!m_winvnc_icon) m_winvnc_icon=(HICON)LoadImage(hAppInstance, MAKEINTRESOURCE(IDI_WINVNC), IMAGE_ICON,
		if (!m_winvnc_icon) m_winvnc_icon = (HICON)LoadImage(hInstResDLL, MAKEINTRESOURCE(IDI_WINVNC), IMAGE_ICON,
			GetSystemMetrics(SM_CXSMICON),
			GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
		// [v1.0.2-jp1 fix]
		//if (!m_flash_icon) m_flash_icon=(HICON)LoadImage(hAppInstance, MAKEINTRESOURCE(IDI_FLASH), IMAGE_ICON,
		if (!m_flash_icon) m_flash_icon = (HICON)LoadImage(hInstResDLL, MAKEINTRESOURCE(IDI_FLASH), IMAGE_ICON,
			GetSystemMetrics(SM_CXSMICON),
			GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	}

	// Load the popup menu
	// [v1.0.2-jp1 fix]

#ifdef SC_20
	m_hmenu = LoadMenu(hInstResDLL, MAKEINTRESOURCE(IDR_TRAYMENU1));
#else
	m_hmenu = LoadMenu(hInstResDLL, MAKEINTRESOURCE(IDR_TRAYMENU));
#endif // SC_20

	// Install the Tray icon!
	AddTrayIcon();
	CoUninitialize();
}

vncMenu::~vncMenu()
{
#ifdef SC_20
	if (ScSelect::g_dis_uac)
		ScSelect::Restore_UAC_for_admin_elevated();
	ScSelect::g_dis_uac = false;
#endif // SC_20

	KillTimer(m_hwnd, 1);
	vnclog.Print(LL_INTERR, VNCLOG("vncmenu killed\n"));

	// adzm 2009-07-05 - Tray icon balloon tips
	if (m_BalloonInfo) {
		free(m_BalloonInfo);
		m_BalloonInfo = NULL;
	}
	if (m_BalloonTitle) {
		free(m_BalloonTitle);
		m_BalloonTitle = NULL;
	}

	if (m_winvnc_icon)
		DestroyIcon(m_winvnc_icon);
	if (m_flash_icon)
		DestroyIcon(m_flash_icon);

	// Remove the Tray icon
	DelTrayIcon();

	// Destroy the loaded menu
	if (m_hmenu != NULL)
		DestroyMenu(m_hmenu);

	// Tell the server to stop notifying us!
	if (m_server != NULL)
		m_server->RemNotify(m_hwnd);

	if (settings->getRemoveWallpaper())
		RestoreWallpaper();
	// adzm - 2010-07 - Disable more effects or font smoothing
	if (settings->getRemoveEffects())
		RestoreEffects();
	if (settings->getRemoveFontSmoothing())
		RestoreFontSmoothing();
	if (settings->getRemoveWallpaper())
		ResetAero();
	CoUninitialize();
}

void
vncMenu::AddTrayIcon()
{
	// If the user name is non-null then we have a user!
	if (strcmp(m_username, "") != 0 && strcmp(m_username, "SYSTEM") != 0)
	{
		// Make sure the server has not been configured to
		// suppress the Tray icon.
		HWND tray = FindWindow(("Shell_TrayWnd"), 0);
		if (!tray) {
			IsIconSet = false;
			IconFaultCounter++;
			//m_server->TriggerUpdate();
			return;
		}

		if (!IsIconSet)
			AddNotificationIcon();
		setToolTip();
		wcscpy_s(m_nid.szTip, m_tooltip);
		Shell_NotifyIconW(NIM_MODIFY, &m_nid);

		/*if (m_server->AuthClientCount() != 0) { //PGM @ Advantig
			// adzm - 2010-07 - Disable more effects or font smoothing
			if (IsUserDesktop()) {
				if (m_server->RemoveWallpaperEnabled()) //PGM @ Advantig
					KillWallpaper(); //PGM @ Advantig
				if (m_server->RemoveEffectsEnabled())
					KillEffects();
				if (m_server->RemoveFontSmoothingEnabled())
					KillFontSmoothing();
			}
			if (m_server->RemoveWallpaperEnabled()) //PGM @ Advantig
				DisableAero(); //PGM @ Advantig
			VNC_OSVersion::getInstance()->SetAeroState();
		} //PGM @ Advantig*/
	}
}

void
vncMenu::DelTrayIcon()
{
	//vnclog.Print(LL_INTERR, VNCLOG("########### vncMenu::DelTrayIcon - DEL Tray icon call\n"));
	SendTrayMsg(NIM_DELETE, false, FALSE);
}

void
vncMenu::FlashTrayIcon(BOOL flash)
{
	//vnclog.Print(LL_INTERR, VNCLOG("########### vncMenu::FlashTrayIcon - FLASH Tray icon call\n"));
	SendTrayMsg(NIM_MODIFY, false, flash);
}

// Get the local ip addresses as a human-readable string.
// If more than one, then with \n between them.
// If not available, then gets a message to that effect.
// The ip address is not likely to change while running
// this function is an overhead, each time calculating the ip
// the ip is just used in the tray tip
char old_buffer[512];
char old_buflen = 0;
int dns_counter = 0; // elimate to many dns requests once every 250s is ok
void
vncMenu::GetIPAddrString(char* buffer, int buflen) {
	if (old_buflen != 0 && dns_counter < 12)
	{
		dns_counter++;
		strcpy_s(buffer, buflen, old_buffer);
		return;
	}
	dns_counter = 0;
	char namebuf[256];

	if (gethostname(namebuf, 256) != 0) {
		strncpy_s(buffer, buflen, "Host name unavailable", buflen);
		return;
	};

#ifdef IPV6V4
	* buffer = '\0';

	LPSOCKADDR sockaddr_ip;
	struct addrinfo hint;
	struct addrinfo* serverinfo = 0;
	memset(&hint, 0, sizeof(hint));
	hint.ai_family = AF_UNSPEC;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_protocol = IPPROTO_TCP;
	struct sockaddr_in6* pIpv6Addr;
	struct sockaddr_in* pIpv4Addr;
	struct sockaddr_in6 Ipv6Addr;
	struct sockaddr_in Ipv4Addr;
	memset(&Ipv6Addr, 0, sizeof(Ipv6Addr));
	memset(&Ipv4Addr, 0, sizeof(Ipv4Addr));

	//make sure the buffer is not overwritten

	if (getaddrinfo(namebuf, 0, &hint, &serverinfo) == 0)
	{
		struct addrinfo* p;
		if (!settings->getIPV6())
		{
			p = serverinfo;
			for (p = serverinfo; p != NULL; p = p->ai_next) {
				switch (p->ai_family) {
				case AF_INET:
				{
					pIpv4Addr = (struct sockaddr_in*)p->ai_addr;
					memcpy(&Ipv4Addr, pIpv4Addr, sizeof(Ipv4Addr));
					Ipv4Addr.sin_family = AF_INET;
					char			szText[256];
					sprintf_s(szText, "%s-", inet_ntoa(Ipv4Addr.sin_addr));
					int len = strlen(buffer);
					int len2 = strlen(szText);
					if (len + len2 < buflen) strcat_s(buffer, buflen, szText);
					break;
				}
				case AF_INET6:
				{
					break;
				}
				default:
					break;
				}
			}
		}

		if (settings->getIPV6())
		{
			p = serverinfo;
			for (p = serverinfo; p != NULL; p = p->ai_next) {
				switch (p->ai_family) {
				case AF_INET:
				{
					break;
				}
				case AF_INET6:
				{
					char ipstringbuffer[46];
					DWORD ipbufferlength = 46;
					ipbufferlength = 46;
					memset(ipstringbuffer, 0, 46);
					pIpv6Addr = (struct sockaddr_in6*)p->ai_addr;
					memcpy(&Ipv6Addr, pIpv6Addr, sizeof(Ipv6Addr));
					Ipv6Addr.sin6_family = AF_INET6;
					sockaddr_ip = (LPSOCKADDR)p->ai_addr;
					WSAAddressToString(sockaddr_ip, (DWORD)p->ai_addrlen, NULL, ipstringbuffer, &ipbufferlength);
					char			szText[256];
					memset(szText, 0, 256);
					strncpy_s(szText, ipstringbuffer, ipbufferlength - 4);
					strcat_s(szText, "-");
					int len = strlen(buffer);
					int len2 = strlen(szText);
					if (len + len2 < buflen)strcat_s(buffer, buflen, szText);
					break;
				}
				default:
					break;
				}
			}
		}
	}
	freeaddrinfo(serverinfo);
#else
	HOSTENT* ph = gethostbyname(namebuf);
	if (!ph) {
		strncpy_s(buffer, buflen, "IP address unavailable", buflen);
		return;
	};

	*buffer = '\0';
	char digtxt[5];
	for (int i = 0; ph->h_addr_list[i]; i++) {
		for (int j = 0; j < ph->h_length; j++) {
			sprintf_s(digtxt, "%d.", (unsigned char)ph->h_addr_list[i][j]);
			strncat_s(buffer, buflen, digtxt, (buflen - 1) - strlen(buffer));
		}
		buffer[strlen(buffer) - 1] = '\0';
		if (ph->h_addr_list[i + 1] != 0)
			strncat_s(buffer, buflen, ", ", (buflen - 1) - strlen(buffer));
	}

#endif
}

BOOL vncMenu::AddNotificationIcon()
{
	if (IsIconSet == true)
		return true;
	memset(&m_nid, 0, sizeof(m_nid));
	// Create the Tray icon message
	m_nid.hWnd = m_hwnd;
	m_nid.cbSize = sizeof(m_nid);
	m_nid.uID = IDI_WINVNC;			// never changes after construction
	m_nid.hIcon = settings->getDisableTrayIcon() ? NULL : m_winvnc_icon;

	m_nid.uFlags = settings->getDisableTrayIcon()
		? NIF_MESSAGE | NIF_STATE | NIF_TIP | NIF_SHOWTIP
		: NIF_ICON | NIF_MESSAGE | NIF_STATE | NIF_TIP | NIF_SHOWTIP;
	m_nid.uTimeout = 29000;
	m_nid.uCallbackMessage = WM_TRAYNOTIFY;
	if (settings->getDisableTrayIcon()) {
		m_nid.dwState = NIS_HIDDEN;
		m_nid.dwStateMask = NIS_HIDDEN;
	}
	else {
		m_nid.dwState = 0;
		m_nid.dwStateMask = NIS_HIDDEN;
	}

	if (Shell_NotifyIconW(NIM_ADD, &m_nid)) {
		m_nid.uVersion = NOTIFYICON_VERSION_4;
		Shell_NotifyIconW(NIM_SETVERSION, &m_nid);
		IsIconSet = true;
		IconFaultCounter = 0;
		if (!settings->getDisableTrayIcon())
			addMenus();
		return true;
	}
	if (!settings->RunningFromExternalService()) {
		// The Tray icon couldn't be created, so use the Properties dialog
		// as the main program window
		// sf@2007 - Do not display Properties pages when running in Application0 mode
		if (!settings->RunningFromExternalService()) {
			m_properties.ShowAdmin();
			PostQuitMessage(0);
		}
	}
	else {
		IsIconSet = false;
		IconFaultCounter++;
		m_server->TriggerUpdate();
		vnclog.Print(LL_INTINFO, VNCLOG("Failed IsIconSet \n"));
	}
	return false;
}

void vncMenu::addMenus()
{
	EnableMenuItem(m_hmenu, ID_ADMIN_PROPERTIES,
		settings->getAllowProperties() ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(m_hmenu, ID_PROPERTIES,
		settings->getAllowProperties() ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(m_hmenu, ID_CLOSE,
		settings->getAllowShutdown() ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(m_hmenu, ID_KILLCLIENTS,
		settings->getAllowEditClients() ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(m_hmenu, ID_OUTGOING_CONN,
		settings->getAllowEditClients() ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(m_hmenu, ID_CLOSE_SERVICE, (settings->RunningFromExternalService() && settings->getAllowShutdown()) ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(m_hmenu, ID_START_SERVICE, (processHelper::IsServiceInstalled() && !settings->RunningFromExternalService() && settings->getAllowShutdown()) ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(m_hmenu, ID_RUNASSERVICE, (!processHelper::IsServiceInstalled() && !settings->RunningFromExternalService() && settings->getAllowShutdown()) ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(m_hmenu, ID_UNINSTALL_SERVICE, (processHelper::IsServiceInstalled() && settings->getAllowShutdown()) ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(m_hmenu, ID_REBOOTSAFEMODE, (settings->RunningFromExternalService() && settings->getAllowShutdown()) ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(m_hmenu, ID_REBOOT_FORCE, (settings->RunningFromExternalService() && settings->getAllowShutdown()) ? MF_ENABLED : MF_GRAYED);
	// adzm 2009-07-05
	if (settings->getScPrompt()) {
		RemoveMenu(m_hmenu, ID_ADMIN_PROPERTIES, MF_BYCOMMAND);
		RemoveMenu(m_hmenu, ID_CLOSE_SERVICE, MF_BYCOMMAND);
		RemoveMenu(m_hmenu, ID_START_SERVICE, MF_BYCOMMAND);
		RemoveMenu(m_hmenu, ID_RUNASSERVICE, MF_BYCOMMAND);
		RemoveMenu(m_hmenu, ID_UNINSTALL_SERVICE, MF_BYCOMMAND);
		RemoveMenu(m_hmenu, ID_REBOOTSAFEMODE, MF_BYCOMMAND);
		RemoveMenu(m_hmenu, ID_REBOOT_FORCE, MF_BYCOMMAND);
	}
}

BOOL vncMenu::DeleteNotificationIcon()
{
	IsIconSet = false;
	IconFaultCounter = 0;
	return Shell_NotifyIconW(NIM_DELETE, &m_nid);
}

void vncMenu::setToolTip()
{
	LoadStringW(hInstResDLL, IDI_WINVNC, m_tooltip, sizeof(m_tooltip));
	wcsncat_s(m_tooltip, L" - ", _TRUNCATE);
	if (m_server->SockConnected()) {
		char ipstring[100];
		wchar_t wipstring[100];
		GetIPAddrString(ipstring, 100);
		mbstowcs(wipstring, ipstring, 100);
		wcsncat_s(m_tooltip, wipstring, _TRUNCATE);
	}
	else
		wcsncat_s(m_tooltip, L"Not listening", _TRUNCATE);
	wchar_t namebufw[256]{};
	char namebuf[256]{};

	if (gethostname(namebuf, 256) == 0) {
		mbstowcs(namebufw, namebuf, strlen(namebuf));
		wcsncat_s(m_tooltip, L" - ", _TRUNCATE);
		wcsncat_s(m_tooltip, namebufw, _TRUNCATE);
	}

	if (settings->RunningFromExternalService())
		wcsncat_s(m_tooltip, L" - service - ", _TRUNCATE);
	else
		wcsncat_s(m_tooltip, L" - application - ", _TRUNCATE);
	wcsncat_s(m_tooltip, g_hookstring, _TRUNCATE);
}

void vncMenu::RestoreTooltip()
{
	m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_STATE | NIF_TIP | NIF_SHOWTIP;
	if (settings->getDisableTrayIcon()) {
		m_nid.dwState = NIS_HIDDEN;
		m_nid.dwStateMask = NIS_HIDDEN;
	}
	else {
		m_nid.dwState = 0;
		m_nid.dwStateMask = NIS_HIDDEN;
	}
	Shell_NotifyIconW(NIM_MODIFY, &m_nid);
	balloonset = false;
}

void vncMenu::setBalloonInfo()
{
	m_nid.uTimeout = 29000; // minimum
	if (settings->getDisableTrayIcon()) {
		m_nid.dwState = 0;
		m_nid.dwStateMask = NIS_HIDDEN;
	}
	if (m_BalloonInfo && (wcslen(m_BalloonInfo) > 0)) {
		m_nid.uFlags = settings->getDisableTrayIcon()
			? NIF_MESSAGE | NIF_STATE | NIF_TIP | NIF_INFO
			: NIF_ICON | NIF_MESSAGE | NIF_STATE | NIF_TIP | NIF_INFO;
		wcsncpy_s(m_nid.szInfo, m_BalloonInfo, 255);
		m_nid.szInfo[255] = '\0';
		wcscpy_s(m_nid.szInfoTitle, L"UltraVNC Connection...");
		m_nid.dwInfoFlags = NIIF_INFO;
		balloonset = true;
	}

	if (m_BalloonTitle && (wcslen(m_BalloonTitle) > 0)) {
		wcsncpy_s(m_nid.szInfoTitle, m_BalloonTitle, 63);
		m_nid.szInfoTitle[63] = '\0';
	}

	if (m_BalloonInfo) {
		free(m_BalloonInfo);
		m_BalloonInfo = NULL;
	}
	if (m_BalloonTitle) {
		free(m_BalloonTitle);
		m_BalloonTitle = NULL;
	}
}

void
vncMenu::SendTrayMsg(DWORD msg, bool balloon, BOOL flash)
{
	if (!IsIconSet) //We only need to continue if we have an TryIcon
		return;
	omni_mutex_lock sync(m_mutexTrayIcon, 69);

	if (msg == NIM_DELETE) {
		DeleteNotificationIcon();
		return;
	}

	if (!balloonset && balloon) {
		setBalloonInfo();
		Shell_NotifyIconW(NIM_MODIFY, &m_nid);
	}
	else if (balloonset && !flash && msg == NIM_MODIFY)
		RestoreTooltip();
	else {
		setToolTip();
		if (!balloonset && msg == NIM_MODIFY && (m_nid.hIcon != (settings->getDisableTrayIcon() ? NULL : flash ? m_flash_icon : m_winvnc_icon)
			|| wcscmp(m_nid.szTip, m_tooltip) != 0)) { //balloonset = 1 , ballon is showing, don't update icon and tip
			m_nid.hIcon = settings->getDisableTrayIcon() ? NULL : flash ? m_flash_icon : m_winvnc_icon;
			wcscpy_s(m_nid.szTip, m_tooltip);
			Shell_NotifyIconW(NIM_MODIFY, &m_nid);
		}
	}
}

// sf@2007
void vncMenu::Shutdown(bool kill_client)
{
	vnclog.Print(LL_INTERR, VNCLOG("vncMenu::Shutdown: Close menu - Disconnect all - Shutdown server\n"));
	SendMessage(m_hwnd, WM_CLOSE, 0, 0);
	if (kill_client) m_server->KillAuthClients();
	G_MENU_HWND = NULL;
}
bool vncMenu::OpenWebpageFromService( char * cmdline)
{
	char dir[MAX_PATH];
	strcpy_s(dir, cmdline);
	WTS_SESSION_INFO* pSessionInfo;
	DWORD n_sessions = 0;
	if (WTSEnumerateSessions(WTS_CURRENT_SERVER, 0, 1, &pSessionInfo, &n_sessions)) {
		DWORD SessionId = 0;
		for (DWORD i = 0; i < n_sessions; ++i) {
			if (pSessionInfo[i].State == WTSActive) {
				SessionId = pSessionInfo[i].SessionId;
				break;
			}
		}
		WTSFreeMemory(pSessionInfo);
		if (SessionId != 0) {
			HANDLE hToken;
			if (WTSQueryUserToken(SessionId, &hToken)) {
				void* environment = NULL;
				if (CreateEnvironmentBlock(&environment, hToken, TRUE)) {
					HANDLE hToken2;
					DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, NULL, SecurityIdentification, TokenPrimary, &hToken2);
					DWORD dwUIAccess = 1;
					SetTokenInformation(hToken2, TokenUIAccess, &dwUIAccess, sizeof(dwUIAccess));
					STARTUPINFO si = { 0 };
					PROCESS_INFORMATION pi = { 0 };
					si.cb = sizeof(STARTUPINFO);
					si.dwFlags = STARTF_USESHOWWINDOW;
					si.wShowWindow = FALSE;
					si.lpDesktop = "winsta0\\default";
					DWORD dwCreationFlags = NORMAL_PRIORITY_CLASS | CREATE_UNICODE_ENVIRONMENT;
					if (CreateProcessAsUser(hToken2, NULL, dir, NULL, NULL, FALSE,
						dwCreationFlags, environment, NULL, &si, &pi)) {
						CloseHandle(pi.hThread);
						CloseHandle(pi.hProcess);
					}
					DestroyEnvironmentBlock(environment);
					CloseHandle(hToken);
					CloseHandle(hToken2);
					return true;
				}
				else
					CloseHandle(hToken);
			}
		}
	}
	return false;
}

bool vncMenu::OpenWebpageFromApp(int iMsg)
{
	DesktopUsersToken desktopUsersToken;
	HANDLE hPToken = desktopUsersToken.getDesktopUsersToken();
	if (!hPToken)
		return false;

	char dir[MAX_PATH];
	char exe_file_name[MAX_PATH];
	GetModuleFileName(0, exe_file_name, MAX_PATH);
	strcpy_s(dir, exe_file_name);
	if (iMsg == ID_VISITUSONLINE_HOMEPAGE)
		strcat_s(dir, " -openhomepage");
	if (iMsg == ID_VISITUSONLINE_FORUM)
		strcat_s(dir, " -openforum");
	if (iMsg == ID_VISITUSONLINE_GITHUB)
		strcat_s(dir, " -opengithub");
	if (iMsg == ID_VISITUSONLINE_MASTODON)
		strcat_s(dir, " -openmastodon");
	if (iMsg == ID_VISITUSONLINE_FACEBOOK)
		strcat_s(dir, " -openfacebook");
	if (iMsg == ID_VISITUSONLINE_XTWITTER)
		strcat_s(dir, " -openxtwitter");
	if (iMsg == ID_VISITUSONLINE_REDDIT)
		strcat_s(dir, " -openreddit");
	if (iMsg == ID_VISITUSONLINE_OPENHUB)
		strcat_s(dir, " -openopenhub");

	STARTUPINFO          StartUPInfo;
	PROCESS_INFORMATION  ProcessInfo;
	ZeroMemory(&StartUPInfo, sizeof(STARTUPINFO));
	ZeroMemory(&ProcessInfo, sizeof(PROCESS_INFORMATION));
	StartUPInfo.wShowWindow = SW_SHOW;
	StartUPInfo.lpDesktop = "Winsta0\\Default";
	StartUPInfo.cb = sizeof(STARTUPINFO);
	PVOID                lpEnvironment = NULL;
	if (CreateEnvironmentBlock(&lpEnvironment, hPToken, FALSE))
		CreateProcessAsUser(hPToken, NULL, dir, NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT | DETACHED_PROCESS, lpEnvironment, NULL, &StartUPInfo, &ProcessInfo);
	else
		CreateProcessAsUser(hPToken, NULL, dir, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &StartUPInfo, &ProcessInfo);

	if (ProcessInfo.hThread)
		CloseHandle(ProcessInfo.hThread);
	if (ProcessInfo.hProcess)
		CloseHandle(ProcessInfo.hProcess);
	if (lpEnvironment)
		DestroyEnvironmentBlock(lpEnvironment);
	return true;
}

char newuser[UNLEN + 1];
// Process window messages
LRESULT CALLBACK vncMenu::WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	// This is a static method, so we don't know which instantiation we're
	// dealing with. We use Allen Hadden's (ahadden@taratec.com) suggestion
	// from a newsgroup to get the pseudo-this.
	vncMenu* _this = helper::SafeGetWindowUserData<vncMenu>(hwnd);
	//	Beep(100,10);
	//	vnclog.Print(LL_INTINFO, VNCLOG("iMsg 0x%x \n"),iMsg);

	if (iMsg == WM_TASKBARCREATED && settings->RunningFromExternalService()) {
		Sleep(1000);
		vnclog.Print(LL_INTINFO, VNCLOG("WM_TASKBARCREATED \n"));
		// User has changed!
		strcpy_s(_this->m_username, newuser);
		// Order impersonation thread killing
		KillTimer(hwnd, 1);
		PostQuitMessage(0);
	}

	switch (iMsg) {
		// Every five seconds, a timer message causes the icon to update
	case WM_TIMER:
		if (wParam == 1) {
			if (ClientTimerReconnect == true && _this->IsIconSet == true) { // ClientTimerReconnect==true --> reconnect
				vnclog.Print(LL_INTERR, VNCLOG("Add client reconnect from timer\n"));
				ClientTimerReconnect = false;
				PostMessage(hwnd, postHelper::MENU_ADD_CLIENT_MSG, 1111, 1111);
			}

			if (settings->RunningFromExternalService()) {
				strcpy_s(newuser, "");
				if (processHelper::CurrentUser((char*)&newuser, sizeof(newuser))) {
					// Check whether the user name has changed!
					if (_stricmp(newuser, _this->m_username) != 0 || (_this->IconFaultCounter > 2)) {
						Sleep(1000);
						vnclog.Print(LL_INTINFO, VNCLOG("user name has changed\n"));
						// User has changed!
						strcpy_s(_this->m_username, newuser);
						// Order impersonation thread killing
						PostQuitMessage(0);
						break;
					}
				}
			}

			// *** HACK for running servicified
			if (settings->RunningFromExternalService()) {
				// Attempt to add the icon if it's not already there
				_this->AddTrayIcon();
				// Trigger a check of the current user
				PostMessage(hwnd, WM_USERCHANGED, 0, 0);
			}
			// Update the icon
			_this->FlashTrayIcon(_this->m_server->AuthClientCount() != 0);
		}
		else if (wParam == 2) {
			if (settings->RunningFromExternalService() && settings->getRdpmode()) {
				fShutdownOrdered = TRUE;
				vnclog.Print(LL_INTINFO, VNCLOG("RdpMode auto 30s auto reset \n"));
				_this->m_server->KillAuthClients();
				PostMessage(hwnd, WM_CLOSE, 0, 0);
			}
		}
		break;

		// DEAL WITH NOTIFICATIONS FROM THE SERVER:
	case WM_SRV_CLIENT_AUTHENTICATED:
	case WM_SRV_CLIENT_DISCONNECT:
		// Adjust the icon accordingly
		_this->FlashTrayIcon(_this->m_server->AuthClientCount() != 0);

		if (_this->m_server->AuthClientCount() != 0) {
			// adzm - 2010-07 - Disable more effects or font smoothing
			if (IsUserDesktop()) {
#ifdef SC_20
				if (ScSelect::g_dis_uac) 
					ScSelect::Disbale_UAC_for_admin_run_elevated();
#endif // SC_20
				if (settings->getRemoveWallpaper())
					KillWallpaper();
				if (settings->getRemoveEffects())
					KillEffects();
				if (settings->getRemoveFontSmoothing())
					KillFontSmoothing();
			}
			if (settings->getRemoveWallpaper()) // Moved, redundant if //PGM @ Advantig
				DisableAero(); // Moved, redundant if //PGM @ Advantig
			VNC_OSVersion::getInstance()->SetAeroState();
			KillTimer(hwnd, 2);
		}
		else {
#ifdef SC_20
			if (ScSelect::g_dis_uac) 
				ScSelect::Restore_UAC_for_admin_elevated();
#endif // SC_20
			if (settings->getRemoveWallpaper()) // Moved, redundant if //PGM @ Advantig
				ResetAero(); // Moved, redundant if //PGM @ Advantig
			if (settings->getRemoveWallpaper()) { // Added { //PGM @ Advantig
				Sleep(2000); // Added 2 second delay to help wallpaper restore //PGM @ Advantig
				RestoreWallpaper();
			} //PGM @ Advantig
			// adzm - 2010-07 - Disable more effects or font smoothing
			if (settings->getRemoveEffects()) {
				RestoreEffects();
			}
			if (settings->getRemoveFontSmoothing()) {
				RestoreFontSmoothing();
			}
			SetTimer(hwnd, 2, 30000, NULL);
		}
		return 0;

		// STANDARD MESSAGE HANDLING
	case WM_CREATE:
		WM_TASKBARCREATED = RegisterWindowMessage("TaskbarCreated");
		return 0;

	case WM_COMMAND:
		// User has clicked an item on the tray menu
		switch (LOWORD(wParam))
		{
		case ID_PROPERTIES:
			// Show the properties dialog, unless it is already displayed
			vnclog.Print(LL_INTINFO, VNCLOG("show user properties requested\n"));
			_this->m_propertiesPoll.Show();
			_this->FlashTrayIcon(_this->m_server->AuthClientCount() != 0);
			break;

		case ID_ADMIN_PROPERTIES:
			// Show the properties dialog, unless it is already displayed
			vnclog.Print(LL_INTINFO, VNCLOG("show user properties requested\n"));
			_this->m_properties.ShowAdmin();
			_this->FlashTrayIcon(_this->m_server->AuthClientCount() != 0);
			break;


		
		case ID_OUTGOING_CONN:
			// Connect out to a listening VNC Viewer
		{
			auto newconn = std::make_unique<vncConnDialog>(_this->m_server);
			if (newconn)
			{
				newconn->DoDialog();
				// delete newconn; // NO ! Already done in vncConnDialog.
			}
		}
		break;

		case ID_KILLCLIENTS:
			// Disconnect all currently connected clients
			vnclog.Print(LL_INTINFO, VNCLOG("KillAuthClients() ID_KILLCLIENTS \n"));
			_this->m_server->KillAuthClients();
			break;

			// sf@2002
		case ID_LISTCLIENTS:
			_this->m_ListDlg.Display();
			break;

		case ID_ABOUT:
			// Show the About box
			_this->m_about.Show(TRUE);
			break;

		case ID_VISITUSONLINE_HOMEPAGE:
			if (settings->RunningFromExternalService() && OpenWebpageFromService("cmd /c start https://uvnc.com/"));
			else 
				OpenWebpageFromApp(ID_VISITUSONLINE_HOMEPAGE);
			break;

		case ID_VISITUSONLINE_FORUM:
			if (settings->RunningFromExternalService() && OpenWebpageFromService("cmd /c start https://forum.uvnc.com/"));
			else
				OpenWebpageFromApp(ID_VISITUSONLINE_FORUM);
			break;

		case ID_VISITUSONLINE_GITHUB:
			if (settings->RunningFromExternalService() && OpenWebpageFromService("cmd /c start https://github.com/ultravnc"));
			else 
				OpenWebpageFromApp(ID_VISITUSONLINE_GITHUB);
			break;

		case ID_VISITUSONLINE_MASTODON:
			if (settings->RunningFromExternalService() && OpenWebpageFromService("cmd /c start https://mastodon.social/@ultravnc"));
			else
				OpenWebpageFromApp(ID_VISITUSONLINE_MASTODON);
			break;

		case ID_VISITUSONLINE_FACEBOOK:
			if (settings->RunningFromExternalService() && OpenWebpageFromService("cmd /c start https://www.facebook.com/ultravnc1"));
			else 
				OpenWebpageFromApp(ID_VISITUSONLINE_FACEBOOK);
			break;

		case ID_VISITUSONLINE_XTWITTER:
			if (settings->RunningFromExternalService() && OpenWebpageFromService("cmd /c start https://twitter.com/ultravnc1"));
			else
				OpenWebpageFromApp(ID_VISITUSONLINE_XTWITTER);
			break;

		case ID_VISITUSONLINE_REDDIT:
			if (settings->RunningFromExternalService() && OpenWebpageFromService("cmd /c start https://www.reddit.com/r/ultravnc"));
			else 
				OpenWebpageFromApp(ID_VISITUSONLINE_REDDIT);
			break;

		case ID_VISITUSONLINE_OPENHUB:
			if (settings->RunningFromExternalService() && OpenWebpageFromService("cmd /c start https://openhub.net/p/ultravnc"));
			else
				OpenWebpageFromApp(ID_VISITUSONLINE_OPENHUB);
			break;


		case ID_CLOSE:
			// User selected Close from the tray menu
			fShutdownOrdered = TRUE;
			//Sleep(1000);
			vnclog.Print(LL_INTINFO, VNCLOG("KillAuthClients() ID_CLOSE \n"));
			_this->m_server->KillAuthClients();
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			break;
#ifndef SC_20
		case ID_REBOOTSAFEMODE:
		{
			DesktopUsersToken desktopUsersToken;
			HANDLE hPToken = desktopUsersToken.getDesktopUsersToken();
			if (!hPToken) {
				UltraVNCService::Reboot_in_safemode_elevated();
				break;
			}

			char dir[MAX_PATH];
			char exe_file_name[MAX_PATH];
			GetModuleFileName(0, exe_file_name, MAX_PATH);
			strcpy_s(dir, exe_file_name);
			strcat_s(dir, " -rebootsafemodehelper");

			STARTUPINFO          StartUPInfo{};
			PROCESS_INFORMATION  ProcessInfo{};
			ZeroMemory(&StartUPInfo, sizeof(STARTUPINFO));
			ZeroMemory(&ProcessInfo, sizeof(PROCESS_INFORMATION));
			StartUPInfo.wShowWindow = SW_SHOW;
			StartUPInfo.lpDesktop = "Winsta0\\Default";
			StartUPInfo.cb = sizeof(STARTUPINFO);

			CreateProcessAsUser(hPToken, NULL, dir, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &StartUPInfo, &ProcessInfo);
			DWORD errorcode = GetLastError();
			if (ProcessInfo.hProcess)
				CloseHandle(ProcessInfo.hProcess);
			if (ProcessInfo.hThread)
				CloseHandle(ProcessInfo.hThread);
			if (errorcode == 1314)
				UltraVNCService::Reboot_in_safemode_elevated();
			break;
		}

		case ID_REBOOT_FORCE:
		{
			DesktopUsersToken desktopUsersToken;
			HANDLE hPToken = desktopUsersToken.getDesktopUsersToken();
			if (!hPToken) {
				UltraVNCService::Reboot_with_force_reboot_elevated();
				break;
			}

			char dir[MAX_PATH];
			char exe_file_name[MAX_PATH];
			GetModuleFileName(0, exe_file_name, MAX_PATH);
			strcpy_s(dir, exe_file_name);
			strcat_s(dir, " -rebootforcedehelper");

			STARTUPINFO          StartUPInfo{};
			PROCESS_INFORMATION  ProcessInfo{};
			ZeroMemory(&StartUPInfo, sizeof(STARTUPINFO));
			ZeroMemory(&ProcessInfo, sizeof(PROCESS_INFORMATION));
			StartUPInfo.wShowWindow = SW_SHOW;
			StartUPInfo.lpDesktop = "Winsta0\\Default";
			StartUPInfo.cb = sizeof(STARTUPINFO);

			CreateProcessAsUser(hPToken, NULL, dir, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &StartUPInfo, &ProcessInfo);
			DWORD errorcode = GetLastError();
			if (ProcessInfo.hProcess)
				CloseHandle(ProcessInfo.hProcess);
			if (ProcessInfo.hThread)
				CloseHandle(ProcessInfo.hThread);
			if (errorcode == 1314)
				UltraVNCService::Reboot_with_force_reboot_elevated();
			break;
		}
		break;

		case ID_UNINSTALL_SERVICE:
		{
			HWND hwnd = postHelper::FindWinVNCWindow(true);
			if (hwnd) SendMessage(hwnd, WM_COMMAND, ID_CLOSE, 0);
			DesktopUsersToken desktopUsersToken;
			HANDLE hPToken = desktopUsersToken.getDesktopUsersToken();
			if (!hPToken) {
				serviceHelpers::Set_uninstall_service_as_admin();
				break;
			}
			char dir[MAX_PATH];
			char exe_file_name[MAX_PATH];
			GetModuleFileName(0, exe_file_name, MAX_PATH);
			strcpy_s(dir, exe_file_name);
			strcat_s(dir, " -uninstallhelper");

			STARTUPINFO          StartUPInfo{};
			PROCESS_INFORMATION  ProcessInfo{};
			ZeroMemory(&StartUPInfo, sizeof(STARTUPINFO));
			ZeroMemory(&ProcessInfo, sizeof(PROCESS_INFORMATION));
			StartUPInfo.wShowWindow = SW_SHOW;
			StartUPInfo.lpDesktop = "Winsta0\\Default";
			StartUPInfo.cb = sizeof(STARTUPINFO);

			CreateProcessAsUser(hPToken, NULL, dir, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &StartUPInfo, &ProcessInfo);
			DWORD errorcode = GetLastError();
			if (ProcessInfo.hProcess)
				CloseHandle(ProcessInfo.hProcess);
			if (ProcessInfo.hThread)
				CloseHandle(ProcessInfo.hThread);
			if (errorcode == 1314)
				serviceHelpers::Set_uninstall_service_as_admin();
		}
		break;

		case ID_RUNASSERVICE:
		{
			DWORD errorcode = 0;
			DesktopUsersToken desktopUsersToken;
			HANDLE hPToken = desktopUsersToken.getDesktopUsersToken();
			if (!hPToken)
				goto error6;

			char dir[MAX_PATH];
			char exe_file_name[MAX_PATH];
			GetModuleFileName(0, exe_file_name, MAX_PATH);
			strcpy_s(dir, exe_file_name);
			strcat_s(dir, " -installhelper");

			STARTUPINFO          StartUPInfo;
			PROCESS_INFORMATION  ProcessInfo;
			ZeroMemory(&StartUPInfo, sizeof(STARTUPINFO));
			ZeroMemory(&ProcessInfo, sizeof(PROCESS_INFORMATION));
			StartUPInfo.wShowWindow = SW_SHOW;
			StartUPInfo.lpDesktop = "Winsta0\\Default";
			StartUPInfo.cb = sizeof(STARTUPINFO);

			CreateProcessAsUser(hPToken, NULL, dir, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &StartUPInfo, &ProcessInfo);
			errorcode = GetLastError();
			if (ProcessInfo.hProcess)
				CloseHandle(ProcessInfo.hProcess);
			if (ProcessInfo.hThread)
				CloseHandle(ProcessInfo.hThread);
			if (errorcode == 1314)
				goto error6;

			fShutdownOrdered = TRUE;
			Sleep(1000);
			vnclog.Print(LL_INTINFO, VNCLOG("KillAuthClients() ID_CLOSE \n"));
			_this->m_server->KillAuthClients();
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		error6:
			serviceHelpers::Set_install_service_as_admin();
			fShutdownOrdered = TRUE;
			Sleep(1000);
			vnclog.Print(LL_INTINFO, VNCLOG("KillAuthClients() ID_CLOSE \n"));
			_this->m_server->KillAuthClients();
			PostMessage(hwnd, WM_CLOSE, 0, 0);
		}
		break;
		case ID_CLOSE_SERVICE:
		{
			HANDLE hProcess, hPToken;
			DWORD id = processHelper::GetExplorerLogonPid();
			if (id != 0)
			{
				DWORD errorcode = 0;
				STARTUPINFO          StartUPInfo;
				PROCESS_INFORMATION  ProcessInfo;
				HANDLE Token=NULL;
				HANDLE process=NULL;
				ZeroMemory(&StartUPInfo,sizeof(STARTUPINFO));
				ZeroMemory(&ProcessInfo,sizeof(PROCESS_INFORMATION));

				hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, id);
				if (!hProcess) goto error7;
				if (!OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY
					| TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_ADJUST_SESSIONID
					| TOKEN_READ | TOKEN_WRITE, &hPToken))
				{
					CloseHandle(hProcess);
					goto error7;
				}

				char dir[MAX_PATH];
				char exe_file_name[MAX_PATH];
				GetModuleFileName(0, exe_file_name, MAX_PATH);
				strcpy_s(dir, exe_file_name);
				strcat_s(dir, " -stopservicehelper");

				StartUPInfo.wShowWindow = SW_SHOW;
				StartUPInfo.lpDesktop = "Winsta0\\Default";
				StartUPInfo.cb = sizeof(STARTUPINFO);

				CreateProcessAsUser(hPToken, NULL, dir, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &StartUPInfo, &ProcessInfo);
				errorcode = GetLastError();
				if (process) CloseHandle(process);
				if (Token) CloseHandle(Token);
				if (hProcess) CloseHandle(hProcess);
				if (ProcessInfo.hProcess) CloseHandle(ProcessInfo.hProcess);
				if (ProcessInfo.hThread) CloseHandle(ProcessInfo.hThread);
				if (errorcode == 1314) goto error7;
				break;
			error7:
				serviceHelpers::Set_stop_service_as_admin();
			}
		}
		break;
		case ID_START_SERVICE:
		{
			HANDLE hProcess{}, hPToken{};
			const DWORD id = processHelper::GetExplorerLogonPid();
			if (id != 0)
			{
				DWORD errorcode = 0;
				hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, id);
				if (!hProcess) {
					serviceHelpers::Set_start_service_as_admin();
					fShutdownOrdered = TRUE;
					Sleep(1000);
					vnclog.Print(LL_INTINFO, VNCLOG("KillAuthClients() ID_CLOSE \n"));
					_this->m_server->KillAuthClients();
					PostMessage(hwnd, WM_CLOSE, 0, 0);
					break;
				}
				if (!OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY
					| TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_ADJUST_SESSIONID
					| TOKEN_READ | TOKEN_WRITE, &hPToken))
				{
					CloseHandle(hProcess);
					serviceHelpers::Set_start_service_as_admin();
					fShutdownOrdered = TRUE;
					Sleep(1000);
					vnclog.Print(LL_INTINFO, VNCLOG("KillAuthClients() ID_CLOSE \n"));
					_this->m_server->KillAuthClients();
					PostMessage(hwnd, WM_CLOSE, 0, 0);
					break;
				}

				char dir[MAX_PATH];
				char exe_file_name[MAX_PATH];
				GetModuleFileName(0, exe_file_name, MAX_PATH);
				strcpy_s(dir, exe_file_name);
				strcat_s(dir, " -startservicehelper");

				STARTUPINFO          StartUPInfo{};
				PROCESS_INFORMATION  ProcessInfo{};
				HANDLE Token = NULL;
				HANDLE process = NULL;
				ZeroMemory(&StartUPInfo, sizeof(STARTUPINFO));
				ZeroMemory(&ProcessInfo, sizeof(PROCESS_INFORMATION));
				StartUPInfo.wShowWindow = SW_SHOW;
				StartUPInfo.lpDesktop = "Winsta0\\Default";
				StartUPInfo.cb = sizeof(STARTUPINFO);

				CreateProcessAsUser(hPToken, NULL, dir, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &StartUPInfo, &ProcessInfo);
				errorcode = GetLastError();
				if (hPToken) CloseHandle(hPToken);
				if (process) CloseHandle(process);
				if (Token) CloseHandle(Token);
				if (hProcess) CloseHandle(hProcess);
				if (ProcessInfo.hProcess) CloseHandle(ProcessInfo.hProcess);
				if (ProcessInfo.hThread) CloseHandle(ProcessInfo.hThread);
				if (errorcode == 1314)
					serviceHelpers::Set_start_service_as_admin();
				fShutdownOrdered = TRUE;
				Sleep(1000);
				vnclog.Print(LL_INTINFO, VNCLOG("KillAuthClients() ID_CLOSE \n"));
				_this->m_server->KillAuthClients();
				PostMessage(hwnd, WM_CLOSE, 0, 0);
				break;
			}
		}
		break;
#endif // SC_20
		}
		return 0;

	case WM_TRAYNOTIFY:
		// User has clicked on the Tray icon or the menu
	{
		// Get the submenu to use as a pop-up menu
		HMENU submenu = GetSubMenu(_this->m_hmenu, 0);

		switch (LOWORD(lParam))
		{
			// for NOTIFYICON_VERSION_4 clients, NIN_SELECT is prerable to listening to mouse clicks and key presses
			// directly.
		case NIN_SELECT:
			break;

		case NIN_BALLOONTIMEOUT:
			_this->RestoreTooltip();
			break;

		case NIN_BALLOONUSERCLICK:
			_this->RestoreTooltip();
			break;

		case WM_CONTEXTMENU:
		{
			POINT const pt = { LOWORD(wParam), HIWORD(wParam) };
			if (submenu)
			{
				SetMenuDefaultItem(submenu, 0, TRUE);
				SetForegroundWindow(hwnd);
				// respect menu drop alignment
				UINT uFlags = TPM_RIGHTBUTTON;
				if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0)
				{
					uFlags |= TPM_RIGHTALIGN;
				}
				else
				{
					uFlags |= TPM_LEFTALIGN;
				}

				TrackPopupMenuEx(submenu, uFlags, pt.x, pt.y, hwnd, NULL);
			}
		}
		}
		break;

		// What event are we responding to, RMB click?
		if (lParam == WM_RBUTTONUP)
		{
			if (submenu == NULL)
			{
				vnclog.Print(LL_INTERR, VNCLOG("no submenu available\n"));
				return 0;
			}

			// Make the first menu item the default (bold font)
			SetMenuDefaultItem(submenu, 0, TRUE);

			// Get the current cursor position, to display the menu at
			POINT mouse;
			GetCursorPos(&mouse);

			// There's a "bug"
			// (Microsoft calls it a feature) in Windows 95 that requires calling
			// SetForegroundWindow. To find out more, search for Q135788 in MSDN.
			//
			SetForegroundWindow(_this->m_nid.hWnd);

			// Display the menu at the desired position
			TrackPopupMenu(submenu,
				0, mouse.x, mouse.y, 0,
				_this->m_nid.hWnd, NULL);

			PostMessage(hwnd, WM_NULL, 0, 0);

			return 0;
		}

		// Or was there a LMB double click?
		if (lParam == WM_LBUTTONDBLCLK)
		{
			// double click: execute first menu item
			SendMessage(_this->m_nid.hWnd,
				WM_COMMAND,
				GetMenuItemID(submenu, 0),
				0);
		}

		return 0;
	}

	case WM_CLOSE:
		// tnatsni Wallpaper fix
		if (settings->getRemoveWallpaper())
			RestoreWallpaper();
		// adzm - 2010-07 - Disable more effects or font smoothing
		if (settings->getRemoveEffects())
			RestoreEffects();
		if (settings->getRemoveFontSmoothing())
			RestoreFontSmoothing();
		if (settings->getRemoveWallpaper())
			ResetAero();

		vnclog.Print(LL_INTERR, VNCLOG("vncMenu WM_CLOSE call - All cleanup done\n"));
		//Sleep(2000);
		DestroyWindow(hwnd);
		break;

	case WM_DESTROY:
		// The user wants UltraVNC Server to quit cleanly...
		_this->DeleteNotificationIcon();
		vnclog.Print(LL_INTINFO, VNCLOG("quitting from WM_DESTROY\n"));
		PostQuitMessage(0);
		return 0;

	case WM_QUERYENDSESSION:
	{
		//shutdown or reboot
		if ((lParam & ENDSESSION_LOGOFF) != ENDSESSION_LOGOFF) {
			fShutdownOrdered = TRUE;
			Sleep(1000);
			vnclog.Print(LL_INTERR, VNCLOG("SHUTDOWN OS detected\n"));
			vnclog.Print(LL_INTINFO, VNCLOG("KillAuthClients() ID_CLOSE \n"));
			_this->m_server->OS_Shutdown = true;
			_this->m_server->KillAuthClients();
			_this->DelTrayIcon();
			HANDLE hEndSessionEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, "Global\\EndSessionEvent");
			if (hEndSessionEvent != NULL) {
				SetEvent(hEndSessionEvent);
				CloseHandle(hEndSessionEvent);
			}
		}
	}
	return 1;

	case WM_ENDSESSION:
		fShutdownOrdered = TRUE;
		vnclog.Print(LL_INTERR, VNCLOG("WM_ENDSESSION\n"));
		DestroyWindow(hwnd);
		return 0;

	case WM_USERCHANGED:
		// The current user may have changed.
	{
		strcpy_s(newuser, "");
		if (processHelper::CurrentUser((char*)&newuser, sizeof(newuser)))
		{
			//vnclog.Print(LL_INTINFO,
			//	VNCLOG("############### Usernames change: old=\"%s\", new=\"%s\"\n"),
			//	_this->m_username, newuser);

			// Check whether the user name has changed!
			if (_stricmp(newuser, _this->m_username) != 0)
			{
				vnclog.Print(LL_INTINFO,
					VNCLOG("user name has changed\n"));

				// User has changed!
				strcpy_s(_this->m_username, newuser);

				// Redraw the Tray icon and set it's state
				_this->DelTrayIcon();
				_this->AddTrayIcon();
				_this->FlashTrayIcon(_this->m_server->AuthClientCount() != 0);
				// We should load in the prefs for the new user

				_this->m_properties.LoadFromIniFile();
				_this->m_propertiesPoll.LoadFromIniFile();

			}
		}
	}
	return 0;

	// [v1.0.2-jp1 fix] Don't show IME toolbar on right click menu.
	case WM_INITMENU:
	case WM_INITMENUPOPUP:
		SendMessage(ImmGetDefaultIMEWnd(hwnd), WM_IME_CONTROL, IMC_CLOSESTATUSWINDOW, 0);
		return 0;

	default:
		// Deal with any of our custom message types
		// wa@2005 -- added support for the AutoReconnectId
		// removed the previous code that used 999,999
		if (iMsg == postHelper::MENU_STOP_RECONNECT_MSG)
		{
			_this->m_server->AutoReconnect(false);
		}
		if (iMsg == postHelper::MENU_STOP_ALL_RECONNECT_MSG)
		{
			_this->m_server->StopReconnectAll();
		}
		if (iMsg == postHelper::MENU_AUTO_RECONNECT_MSG)
		{
			char szId[MAX_PATH] = { 0 };
			UINT ret = 0;
			if (lParam != 0)
			{
				ret = GlobalGetAtomName((ATOM)lParam, szId, sizeof(szId));
				GlobalDeleteAtom((ATOM)lParam);
			}
			_this->m_server->AutoReconnect(true);

			if (ret > 0)
				_this->m_server->AutoReconnectId(szId);

			return 0;
		}
		if (iMsg == postHelper::MENU_REPEATER_ID_MSG)
		{
			char szId[MAX_PATH] = { 0 };
			UINT ret = 0;
			if (lParam != 0)
			{
				ret = GlobalGetAtomName((ATOM)lParam, szId, sizeof(szId));
				GlobalDeleteAtom((ATOM)lParam);
			}
			_this->m_server->IdReconnect(true);

			if (ret > 0)
				_this->m_server->AutoReconnectId(szId);

			return 0;
		}

#ifdef IPV6V4

		if (iMsg == postHelper::MENU_ADD_CLIENT6_MSG || iMsg == postHelper::MENU_ADD_CLIENT6_MSG_INIT)
		{
			if (iMsg == postHelper::MENU_ADD_CLIENT6_MSG_INIT)
				_this->m_server->AutoReconnectAdr("");

			// Add Client message. This message includes an IP address
			// of a listening client, to which we should connect.

			//adzm 2009-06-20 - Check for special add repeater client message
			if (wParam == 0xFFFFFFFF && (ULONG)lParam == 0xFFFFFFFF) {
				auto newconn = std::make_unique<vncConnDialog>(_this->m_server);
				if (newconn)
				{
					if (IDOK != newconn->DoDialog()) {
						if (settings->getScPrompt() && _this->m_server->AuthClientCount() == 0 && _this->m_server->UnauthClientCount() == 0) {
							PostMessage(hwnd, WM_COMMAND, ID_CLOSE, 0);
						}
					}
				}
				return 0;
			}

			// If there is no IP address then show the connection dialog
			if (!lParam) {
				auto newconn = std::make_unique<vncConnDialog>(_this->m_server);
				if (newconn)
				{
					newconn->DoDialog();
					// winvnc -connect fixed
					//CHECH memeory leak
					//			delete newconn;
				}
				return 0;
			}

			unsigned short nport = 0;
			char* nameDup = 0;
			char szAdrName[64];
			char szId[MAX_PATH] = { 0 };
			// sf@2003 - Values are already converted

			if (WaitForSingleObject(_this->m_server->retryThreadHandle, 0) == WAIT_OBJECT_0 && fShutdownOrdered)
				Sleep(5000);

			if ((_this->m_server->AutoReconnect() || _this->m_server->IdReconnect()) && strlen(_this->m_server->AutoReconnectAdr()) > 0)
			{
				struct in6_addr address;
				memset(&address, 0, sizeof(address));
				nport = _this->m_server->AutoReconnectPort();
				VCard32 ipaddress = VSocket::Resolve6(_this->m_server->AutoReconnectAdr(), &address);
				char straddr[INET6_ADDRSTRLEN];
				memset(straddr, 0, INET6_ADDRSTRLEN);
				PCSTR test = inet_ntop(AF_INET6, &address, straddr, sizeof(straddr));
				if (strlen(straddr) == 0) return 0;
				nameDup = _strdup(straddr);
				if (nameDup == 0)
					return 0;
				strcpy_s(szAdrName, nameDup);
				// Free the duplicate name
				if (nameDup != 0) free(nameDup);
			}
			else
			{
				// Get the IP address stringified
				struct in6_addr address;
				memset(&address, 0, sizeof(address));
				char straddr[INET6_ADDRSTRLEN];
				memset(straddr, 0, INET6_ADDRSTRLEN);
				memcpy(&address, &postHelper::G_LPARAM_IN6, sizeof(in6_addr));
				PCSTR test = inet_ntop(AF_INET6, &address, straddr, sizeof(straddr));
				if (strlen(straddr) == 0) return 0;
				nameDup = _strdup(straddr);
				if (nameDup == 0) return 0;
				strcpy_s(szAdrName, nameDup);
				// Free the duplicate name
				if (nameDup != 0) free(nameDup);
				// Get the port number
				nport = (unsigned short)wParam;
				if (nport == 0) nport = INCOMING_PORT_OFFSET;
			}
			// wa@2005 -- added support for the AutoReconnectId
			// (but it's not required)
			bool bId = (strlen(_this->m_server->AutoReconnectId()) > 0);
			if (bId)
				strcpy_s(szId, _this->m_server->AutoReconnectId());

			// sf@2003
			// Stores the client adr/ports the first time we try to connect
			// This way we can call this message again later to reconnect with the same values
			if ((_this->m_server->AutoReconnect() || _this->m_server->IdReconnect()) && strlen(_this->m_server->AutoReconnectAdr()) == 0)
			{
				if (strlen(dnsname) > 0) _this->m_server->AutoReconnectAdr(dnsname);
				else
					_this->m_server->AutoReconnectAdr(szAdrName);
				strcpy_s(dnsname, "");

				_this->m_server->AutoReconnectPort(nport);
			}

			if (_this->m_server->AutoReconnect())
			{
				_this->m_server->AutoConnectRetry();
			}
			else
			{
				// Attempt to create a new socket
				VSocket* tmpsock;
				tmpsock = new VSocket;
				if (tmpsock) {
					// Connect out to the specified host on the UltraVNC Viewer listen port
#ifdef IPV6V4
					if (tmpsock->CreateConnect(szAdrName, nport))
#else
					tmpsock->Create();
					if (tmpsock->Connect(szAdrName, nport))
#endif
					{
						if (bId)
						{
							// wa@2005 -- added support for the AutoReconnectId
							// Set the ID for this client -- code taken from vncconndialog.cpp (ln:142)
							tmpsock->Send(szId, 250);
							tmpsock->SetTimeout(0);

							// adzm 2009-07-05 - repeater IDs
							// Add the new client to this server
							// adzm 2009-08-02
							_this->m_server->AddClient(tmpsock, TRUE, TRUE, 0, NULL, szId, szAdrName, nport, true);
						}
						else {
							// Add the new client to this server
							// adzm 2009-08-02
							_this->m_server->AddClient(tmpsock, TRUE, TRUE, 0, NULL, NULL, szAdrName, nport, true);
						}
					}
					else {
						delete tmpsock;
					}
				}
			}

			return 0;
		}

		if (iMsg == postHelper::MENU_ADD_CLIENT_MSG || iMsg == postHelper::MENU_ADD_CLIENT_MSG_INIT)
		{
			if (iMsg == postHelper::MENU_ADD_CLIENT_MSG_INIT)
				_this->m_server->AutoReconnectAdr("");

			// Add Client message. This message includes an IP address
			// of a listening client, to which we should connect.

			//adzm 2009-06-20 - Check for special add repeater client message
			if (wParam == 0xFFFFFFFF && (ULONG)lParam == 0xFFFFFFFF) {
				auto newconn = std::make_unique<vncConnDialog>(_this->m_server);
				if (newconn)
				{
					if (IDOK != newconn->DoDialog()) {
						if (settings->getScPrompt() && _this->m_server->AuthClientCount() == 0 && _this->m_server->UnauthClientCount() == 0) {
							PostMessage(hwnd, WM_COMMAND, ID_CLOSE, 0);
						}
					}
				}
				return 0;
			}

			// If there is no IP address then show the connection dialog
			if (!lParam) {
				auto newconn = std::make_unique<vncConnDialog>(_this->m_server);
				if (newconn)
				{
					newconn->DoDialog();
					// winvnc -connect fixed
					//CHECH memeory leak
					//			delete newconn;
				}
				return 0;
			}

			unsigned short nport = 0;
			char* nameDup = 0;
			char szAdrName[64];
			char szId[MAX_PATH] = { 0 };
			// sf@2003 - Values are already converted

			if (WaitForSingleObject(_this->m_server->retryThreadHandle, 0) == WAIT_OBJECT_0 && fShutdownOrdered)
				Sleep(5000);
			if ((_this->m_server->AutoReconnect() || _this->m_server->IdReconnect()) && strlen(_this->m_server->AutoReconnectAdr()) > 0)
			{
				struct in_addr address;
				nport = _this->m_server->AutoReconnectPort();
				VCard32 ipaddress = VSocket::Resolve4(_this->m_server->AutoReconnectAdr());
				unsigned long ipaddress_long = ipaddress;
				address.S_un.S_addr = ipaddress_long;
				char* name = inet_ntoa(address);
				if (name == 0)
					return 0;
				nameDup = _strdup(name);
				if (nameDup == 0)
					return 0;
				strcpy_s(szAdrName, nameDup);
				// Free the duplicate name
				if (nameDup != 0) free(nameDup);
			}
			else
			{
				// Get the IP address stringified
				struct in_addr address;
				address.S_un.S_addr = lParam;
				char* name = inet_ntoa(address);
				if (name == 0)
					return 0;
				nameDup = _strdup(name);
				if (nameDup == 0)
					return 0;
				strcpy_s(szAdrName, nameDup);
				// Free the duplicate name
				if (nameDup != 0) free(nameDup);

				// Get the port number
				nport = (unsigned short)wParam;
				if (nport == 0)
					nport = INCOMING_PORT_OFFSET;
			}
			// wa@2005 -- added support for the AutoReconnectId
			// (but it's not required)
			bool bId = (strlen(_this->m_server->AutoReconnectId()) > 0);
			if (bId)
				strcpy_s(szId, _this->m_server->AutoReconnectId());

			// sf@2003
			// Stores the client adr/ports the first time we try to connect
			// This way we can call this message again later to reconnect with the same values
			if ((_this->m_server->AutoReconnect() || _this->m_server->IdReconnect()) && strlen(_this->m_server->AutoReconnectAdr()) == 0)
			{
				if (strlen(dnsname) > 0) _this->m_server->AutoReconnectAdr(dnsname);
				else
					_this->m_server->AutoReconnectAdr(szAdrName);
				strcpy_s(dnsname, "");

				_this->m_server->AutoReconnectPort(nport);
			}

			if (_this->m_server->AutoReconnect())
			{
				_this->m_server->AutoConnectRetry();
			}
			else
			{
				// Attempt to create a new socket
				VSocket* tmpsock;
				tmpsock = new VSocket;
				if (tmpsock) {
					// Connect out to the specified host on the UltraVNC Viewer listen port
#ifdef IPV6V4
					if (tmpsock->CreateConnect(szAdrName, nport))
#else
					tmpsock->Create();
					if (tmpsock->Connect(szAdrName, nport))
#endif
					{
						if (bId)
						{
							// wa@2005 -- added support for the AutoReconnectId
							// Set the ID for this client -- code taken from vncconndialog.cpp (ln:142)
							tmpsock->Send(szId, 250);
							tmpsock->SetTimeout(0);

							// adzm 2009-07-05 - repeater IDs
							// Add the new client to this server
							// adzm 2009-08-02
							_this->m_server->AddClient(tmpsock, TRUE, TRUE, 0, NULL, szId, szAdrName, nport, true);
						}
						else {
							// Add the new client to this server
							// adzm 2009-08-02
							_this->m_server->AddClient(tmpsock, TRUE, TRUE, 0, NULL, NULL, szAdrName, nport, true);
						}
					}
					else {
						delete tmpsock;
					}
				}
			}

			return 0;
		}
#else
		if (iMsg == postHelper::MENU_ADD_CLIENT_MSG || iMsg == postHelper::MENU_ADD_CLIENT_MSG_INIT)
		{
			if (iMsg == postHelper::MENU_ADD_CLIENT_MSG_INIT)
				_this->m_server->AutoReconnectAdr("");

			// Add Client message. This message includes an IP address
			// of a listening client, to which we should connect.

			//adzm 2009-06-20 - Check for special add repeater client message
			if (wParam == 0xFFFFFFFF && (ULONG)lParam == 0xFFFFFFFF) {


//////////////////////////////////////////////////////////////////////////////////////
				if (strlen(g_szRepeaterHost) > 0 && (strlen(_this->m_server->AutoReconnectId()) > 0)) {
					int port;
					char actualhostname[_MAX_PATH];
					strcpy_s(actualhostname, g_szRepeaterHost);
					char* portp = strchr(actualhostname, ':');
					char finalidcode[_MAX_PATH];
					char idcode[MAX_PATH];
					strcpy_s(idcode, _this->m_server->AutoReconnectId());

					//adzm 2010-08 - this was sending uninitialized data over the wire
					ZeroMemory(finalidcode, sizeof(finalidcode));

					size_t i = 0;
					for (i = 0; i < strlen(_this->m_server->AutoReconnectId()); i++) {
						finalidcode[i] = toupper(idcode[i]);
					}
					finalidcode[i] = 0;
					if (0 != strncmp("ID:", idcode, 3)) {
						strcpy_s(finalidcode, "ID:");
						for (i = 0; i < strlen(idcode); i++) {
							finalidcode[i + 3] = toupper(idcode[i]);
						}
						finalidcode[i + 3] = 0;
					}


					port = INCOMING_PORT_OFFSET;
					if (portp)
					{
						*portp++ = '\0';
						if (*portp == ':') // Tight127 method
						{
							port = atoi(++portp);		// Port number after "::"
						}
						else // RealVNC method
						{
							if (atoi(portp) < 100)		// If < 100 after ":" -> display number
								port += atoi(portp);
							else
								port = atoi(portp);	    // If > 100 after ":" -> Port number
						}
					}
					VSocket* tmpsock;
					tmpsock = new VSocket;
					if (!tmpsock)
						return TRUE;
#ifdef IPV6V4
					if (tmpsock->CreateConnect(actualhostname, port)) {
#else
					tmpsock->Create();
					if (tmpsock->Connect(actualhostname, port)) {
#endif				
						tmpsock->Send(finalidcode, 250);
						tmpsock->SetTimeout(0);
						_this->m_server->AddClient(tmpsock, !settings->getReverseAuthRequired(), TRUE, 0, NULL, finalidcode, actualhostname, port, true);

					}
					else {
						// Print up an error message
						MessageBoxSecure(NULL,
							sz_ID_FAILED_CONNECT_LISTING_VIEW,
							sz_ID_OUTGOING_CONNECTION,
							MB_OK | MB_ICONEXCLAMATION);
						delete tmpsock;
					}
				}
				else {
					auto newconn = std::make_unique<vncConnDialog>(_this->m_server);
					if (newconn) {
						if (IDOK != newconn->DoDialog()) {
							if (settings->getScPrompt() && _this->m_server->AuthClientCount() == 0 && _this->m_server->UnauthClientCount() == 0) {
								PostMessage(hwnd, WM_COMMAND, ID_CLOSE, 0);
							}
						}
					}
				}
				return 0;
			}

			// If there is no IP address then show the connection dialog
			if (!lParam) {
				auto newconn = std::make_unique<vncConnDialog>(_this->m_server);
				if (newconn) {
					newconn->DoDialog();
					// winvnc -connect fixed
					// CHECH memeory leak
					// delete newconn;
				}
				return 0;
			}

			unsigned short nport = 0;
			char* nameDup = 0;
			char szAdrName[64]{};
			char szId[MAX_PATH] = { 0 };
			// sf@2003 - Values are already converted

			if (WaitForSingleObject(_this->m_server->retryThreadHandle, 0) == WAIT_OBJECT_0 && fShutdownOrdered)
				Sleep(5000);

			if ((_this->m_server->AutoReconnect() || _this->m_server->IdReconnect()) && strlen(_this->m_server->AutoReconnectAdr()) > 0)
			{
				struct in_addr address {};
				nport = _this->m_server->AutoReconnectPort();
				VCard32 ipaddress = VSocket::Resolve(_this->m_server->AutoReconnectAdr());
				unsigned long ipaddress_long = ipaddress;
				address.S_un.S_addr = ipaddress_long;
				char* name = inet_ntoa(address);
				if (name == 0)
					return 0;
				nameDup = _strdup(name);
				if (nameDup == 0)
					return 0;
				strcpy_s(szAdrName, nameDup);
				// Free the duplicate name
				if (nameDup != 0) free(nameDup);
			}
			else
			{
				// Get the IP address stringified
				struct in_addr address {};
				address.S_un.S_addr = (ULONG)lParam;
				char* name = inet_ntoa(address);
				if (name == 0)
					return 0;
				nameDup = _strdup(name);
				if (nameDup == 0)
					return 0;
				strcpy_s(szAdrName, nameDup);
				// Free the duplicate name
				if (nameDup != 0) free(nameDup);

				// Get the port number
				nport = (unsigned short)wParam;
				if (nport == 0)
					nport = INCOMING_PORT_OFFSET;
			}

			// wa@2005 -- added support for the AutoReconnectId
			// (but it's not required)
			bool bId = (strlen(_this->m_server->AutoReconnectId()) > 0);
			if (bId)
				strcpy_s(szId, _this->m_server->AutoReconnectId());

			// sf@2003
			// Stores the client adr/ports the first time we try to connect
			// This way we can call this message again later to reconnect with the same values
			if ((_this->m_server->AutoReconnect() || _this->m_server->IdReconnect()) && strlen(_this->m_server->AutoReconnectAdr()) == 0)
			{
				if (strlen(dnsname) > 0) _this->m_server->AutoReconnectAdr(dnsname);
				else
					_this->m_server->AutoReconnectAdr(szAdrName);
				strcpy_s(dnsname, "");

				_this->m_server->AutoReconnectPort(nport);
			}

			if (!fShutdownOrdered && _this->m_server->AutoReconnect())
			{
				_this->m_server->AutoConnectRetry();
			}
			else if (!fShutdownOrdered)
			{
				// Attempt to create a new socket
				VSocket* tmpsock;
				tmpsock = new VSocket;
				if (tmpsock) {
					{
						tmpsock->Create();
						if (tmpsock->Connect(szAdrName, nport)) {
							if (bId)
							{
								// wa@2005 -- added support for the AutoReconnectId
								// Set the ID for this client -- code taken from vncconndialog.cpp (ln:142)
								tmpsock->Send(szId, 250);
								tmpsock->SetTimeout(0);

								// adzm 2009-07-05 - repeater IDs
								// Add the new client to this server
								// adzm 2009-08-02
								_this->m_server->AddClient(tmpsock, TRUE, TRUE, 0, NULL, szId, szAdrName, nport, true);
							}
							else {
								// Add the new client to this server
								// adzm 2009-08-02
								_this->m_server->AddClient(tmpsock, TRUE, TRUE, 0, NULL, NULL, szAdrName, nport, true);
							}
						}
						else {
							delete tmpsock;
						}
					}
				}
			}

			return 0;
		}
#endif

		// Process File Transfer asynchronous Send Packet Message
		if (iMsg == postHelper::FileTransferSendPacketMessage)
		{
			vncClient* pClient = (vncClient*)wParam;
			if (_this->m_server->IsClient(pClient)) pClient->SendFileChunk();
		}

		// adzm 2009-07-05 - Tray icon balloon tips
		if (iMsg == postHelper::MENU_TRAYICON_BALLOON_MSG && _this->IsIconSet) {
			try {
				omni_mutex_lock sync(_this->m_mutexTrayIcon, 70);

				wchar_t* szTitle = (wchar_t*)lParam;
				wchar_t* szInfo = (wchar_t*)wParam;

				if (szInfo && (wcslen(szInfo) > 0))
					_this->m_BalloonInfo = _wcsdup(szInfo);
				if (szTitle && (wcslen(szTitle) > 0))
					_this->m_BalloonTitle = _wcsdup(szTitle);

				if (szInfo)
					free(szInfo);
				if (szTitle)
					free(szTitle);

				if (_this->IsIconSet)
					_this->SendTrayMsg(NIM_MODIFY, true, _this->m_nid.hIcon == _this->m_winvnc_icon ? FALSE : TRUE);
			}
			catch (...) {
				// just in case
				vnclog.Print(LL_INTWARN,
					VNCLOG("Warning: exception handling balloon message\n"));
			}
		}
	}

	// Message not recognised
	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

// adzm 2009-07-05 - Tray icon balloon tips
void  vncMenu::NotifyBalloon(wchar_t* szInfo, wchar_t* szTitle)
{
	wchar_t* szInfoCopy = _wcsdup(szInfo);
	wchar_t* szTitleCopy = _wcsdup(szTitle);
	if (!postHelper::PostToThisWinVNC(postHelper::MENU_TRAYICON_BALLOON_MSG, (WPARAM)szInfoCopy, (LPARAM)szTitleCopy)) {
		if (szInfoCopy)
			free(szInfoCopy);
		if (szTitleCopy)
			free(szTitleCopy);
	}
}