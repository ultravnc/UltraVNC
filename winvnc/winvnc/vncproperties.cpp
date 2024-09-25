/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
//  Copyright (C) 2000-2002 Const Kaplinsky. All Rights Reserved.
//  Copyright (C) 2002 TightVNC. All Rights Reserved.
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


// vncProperties.cpp

// Implementation of the Properties dialog!

#include "stdhdrs.h"
#include "lmcons.h"
#include "winvnc.h"
#include "vncproperties.h"
#include "vncserver.h"
#include "vncpasswd.h"
#include "vncOSVersion.h"
#include "common/win32_helpers.h"
#include "vncconndialog.h"

#include "Localization.h" // ACT : Add localization on messages
#include "shlwapi.h"
#include "SettingsManager.h"
#include "credentials.h"
#include <memory>

// [v1.0.2-jp1 fix] Load resouce from dll
extern HINSTANCE	hInstResDLL;

// Marscha@2004 - authSSP: Function pointer for dyn. linking
typedef void (*vncEditSecurityFn) (HWND hwnd, HINSTANCE hInstance);
vncEditSecurityFn vncEditSecurity = 0;

void Secure_Save_Plugin_Config(char* szPlugin);
void Secure_Plugin_elevated(char* szPlugin);
void Secure_Plugin(char* szPlugin);

// Constructor & Destructor
vncProperties::vncProperties()
{
	hBmpExpand = (HBITMAP)::LoadImage(hInstResDLL, MAKEINTRESOURCE(IDB_EXPAND), IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT);
	hBmpCollaps = (HBITMAP)::LoadImage(hInstResDLL, MAKEINTRESOURCE(IDB_COLLAPS), IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT);
	bExpanded = true;
	m_dlgvisible = FALSE;
	m_pref_Lock_service_helper = TRUE;
	cy = 0;
	cx = 0;
	m_server = NULL;
}

vncProperties::~vncProperties()
{
	DeleteObject(hBmpExpand);
	DeleteObject(hBmpCollaps);
}

// Initialisation
BOOL
vncProperties::Init(vncServer* server)
{
	// Save the server pointer
	m_server = server;

	LoadFromIniFile();

	{
		vncPasswd::ToText plain(settings->getPasswd(), settings->getSecure());
		if (strlen(plain) == 0)
			if (!settings->getAllowProperties() || !settings->IsRunninAsAdministrator()) {
				if (settings->getAuthRequired()) {
					MessageBoxSecure(NULL, sz_ID_NO_PASSWD_NO_OVERRIDE_ERR,
						sz_ID_WINVNC_ERROR,
						MB_OK | MB_ICONSTOP);
					PostQuitMessage(0);
				}
			}
			else {
				// If null passwords are not allowed, ensure that one is entered!
				if (settings->getAuthRequired()) {
					char username[UNLEN + 1];
					if (!processHelper::CurrentUser(username, sizeof(username)))
						return FALSE;
					if (strcmp(username, "") == 0) {
						m_pref_Lock_service_helper = true;
						MessageBoxSecure(NULL, sz_ID_NO_PASSWD_NO_LOGON_WARN,
							sz_ID_WINVNC_ERROR,
							MB_OK | MB_ICONEXCLAMATION);
						ShowAdmin();
						m_pref_Lock_service_helper = false;
					}
					else {
						ShowAdmin();
					}
				}
			}
	}
	m_pref_Lock_service_helper = false;
	return TRUE;
}

// Dialog box handling functions
void vncProperties::ShowAdmin()
{
	DesktopUsersToken desktopUsersToken;
	HANDLE hPToken = desktopUsersToken.getDesktopUsersToken();
	int iImpersonateResult = 0;
	if (hPToken) {
		if (!ImpersonateLoggedOnUser(hPToken))
			iImpersonateResult = GetLastError();
	}

	if (!settings->getAllowProperties()) {
		if (iImpersonateResult == ERROR_SUCCESS)
			RevertToSelf();
		return;
	}

	if (!m_dlgvisible) {
		for (;;) {
			bExpanded = true;
			cy = 0;
			cx = 0;
			int result = (int)DialogBoxParam(hInstResDLL, MAKEINTRESOURCE(IDD_PROPERTIES1), NULL, (DLGPROC)DialogProc, (LONG_PTR)this);
			vnclog.Print(LL_INTINFO, VNCLOG("dialog result = %d\n"), result);
			if (result == -1) {
				// Dialog box failed, so quit
				PostQuitMessage(0);
				if (iImpersonateResult == ERROR_SUCCESS)
					RevertToSelf();
				return;
			}
			// We're allowed to exit if the password is not empty
			{
				vncPasswd::ToText plain(settings->getPasswd(), settings->getSecure());
				if ((strlen(plain) != 0) || !settings->getAuthRequired())
					break;
			}
			vnclog.Print(LL_INTERR, VNCLOG("warning - empty password\n"));
			// If we reached here then OK was used & there is no password!
			MessageBoxSecure(NULL, sz_ID_NO_PASSWORD_WARN, sz_ID_WINVNC_WARNIN, MB_OK | MB_ICONEXCLAMATION);

			// The password is empty, so if OK was used then redisplay the box,
			// otherwise, if CANCEL was used, close down UltraVNC Server
			if (result == IDCANCEL) {
				vnclog.Print(LL_INTERR, VNCLOG("no password - QUITTING\n"));
				PostQuitMessage(0);
				if (iImpersonateResult == ERROR_SUCCESS)
					RevertToSelf();
				fShutdownOrdered = true;
				return;
			}
			omni_thread::sleep(4);
		}
		// Load in all the settings
		// If you run as service, you reload the saved settings before they are actual saved
		// via runas.....

		if (!settings->RunningFromExternalService())
			LoadFromIniFile();

	}

	if (iImpersonateResult == ERROR_SUCCESS)
		RevertToSelf();
}

BOOL CALLBACK
vncProperties::DialogProc(HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	// We use the dialog-box's USERDATA to store a _this pointer
	// This is set only once WM_INITDIALOG has been recieved, though!
	vncProperties* _this = helper::SafeGetWindowUserData<vncProperties>(hwnd);

	switch (uMsg) {
	case WM_INITDIALOG:
	{
		HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_WINVNC));
		SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		vnclog.Print(LL_INTINFO, VNCLOG("INITDIALOG properties\n"));
		// Retrieve the Dialog box parameter and use it as a pointer
		// to the calling vncProperties object
		helper::SafeSetWindowUserData(hwnd, lParam);

		_this = (vncProperties*)lParam;
		_this->m_dlgvisible = TRUE;

		_this->LoadFromIniFile();		

		// Initialise the properties controls
		HWND hConnectSock = GetDlgItem(hwnd, IDC_CONNECT_SOCK);

		// Tight 1.2.7 method
		BOOL bConnectSock = _this->m_server->SockConnected();
		SendMessage(hConnectSock, BM_SETCHECK, bConnectSock, 0);
#ifndef SC_20
		// Set the content of the password field to a predefined string.
		SetDlgItemText(hwnd, IDC_PASSWORD, "~~~~~~~~");
		EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD), bConnectSock);

		// Set the content of the view-only password field to a predefined string. //PGM
		SetDlgItemText(hwnd, IDC_PASSWORD2, "~~~~~~~~"); //PGM
		EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD2), bConnectSock); //PGM
#endif // SC_20
			// Set the initial keyboard focus
		if (bConnectSock) {
			SetFocus(GetDlgItem(hwnd, IDC_PASSWORD));
			SendDlgItemMessage(hwnd, IDC_PASSWORD, EM_SETSEL, 0, (LPARAM)-1);
		}
		else
			SetFocus(hConnectSock);
		// Set display/ports settings
		_this->InitPortSettings(hwnd);

		HWND hConnectHTTP = GetDlgItem(hwnd, IDC_CONNECT_HTTP);
		SendMessage(hConnectHTTP, BM_SETCHECK, settings->getHTTPConnect(), 0);

		// Modif sf@2002 - v1.1.0
		HWND hFileTransfer = GetDlgItem(hwnd, IDC_FILETRANSFER);
		SendMessage(hFileTransfer, BM_SETCHECK, settings->getEnableFileTransfer(), 0);

		HWND hFileTransferUserImp = GetDlgItem(hwnd, IDC_FTUSERIMPERSONATION_CHECK);
		SendMessage(hFileTransferUserImp, BM_SETCHECK, settings->getFTUserImpersonation(), 0);

		HWND hBlank = GetDlgItem(hwnd, IDC_BLANK);
		SendMessage(hBlank, BM_SETCHECK, settings->getEnableBlankMonitor(), 0);
		if (!VNC_OSVersion::getInstance()->OS_WIN10_TRANS && VNC_OSVersion::getInstance()->OS_WIN10)
			SetDlgItemText(hwnd, IDC_BLANK, "Enable Blank Monitor on Viewer Request require Min Win10 build 19041 ");
		if (VNC_OSVersion::getInstance()->OS_WIN8)
			SetDlgItemText(hwnd, IDC_BLANK, "Enable Blank Monitor on Viewer Not supported on windows 8 ");
		HWND hBlank2 = GetDlgItem(hwnd, IDC_BLANK2); //PGM
		SendMessage(hBlank2, BM_SETCHECK, settings->getBlankInputsOnly(), 0); //PGM

		HWND hLoopback = GetDlgItem(hwnd, IDC_ALLOWLOOPBACK);
		BOOL fLoopback = settings->getAllowLoopback();
		SendMessage(hLoopback, BM_SETCHECK, fLoopback, 0);

		HWND hIPV6 = GetDlgItem(hwnd, IDC_IPV6);
		BOOL fIPV6 = settings->getIPV6();
		SendMessage(hIPV6, BM_SETCHECK, fIPV6, 0);

		HWND hLoopbackonly = GetDlgItem(hwnd, IDC_LOOPBACKONLY);
		BOOL fLoopbackonly = settings->getLoopbackOnly();
		SendMessage(hLoopbackonly, BM_SETCHECK, fLoopbackonly, 0);

		HWND hTrayicon = GetDlgItem(hwnd, IDC_DISABLETRAY);
		BOOL fTrayicon = settings->getDisableTrayIcon();
		SendMessage(hTrayicon, BM_SETCHECK, fTrayicon, 0);

		HWND hrdpmode = GetDlgItem(hwnd, IDC_RDPMODE);
		BOOL frdpmode = settings->getRdpmode();
		SendMessage(hrdpmode, BM_SETCHECK, frdpmode, 0);

		HWND hNoScreensaver = GetDlgItem(hwnd, IDC_NOSCREENSAVER);
		BOOL fNoScrensaver = settings->getnoscreensaver();
		SendMessage(hNoScreensaver, BM_SETCHECK, fNoScrensaver, 0);

		HWND hAllowshutdown = GetDlgItem(hwnd, IDC_ALLOWSHUTDOWN);
		SendMessage(hAllowshutdown, BM_SETCHECK, !settings->getAllowShutdown(), 0);

		HWND hm_alloweditclients = GetDlgItem(hwnd, IDC_ALLOWEDITCLIENTS);
		SendMessage(hm_alloweditclients, BM_SETCHECK, !settings->getAllowEditClients(), 0);

		if (vnclog.GetMode() >= 2)
			CheckDlgButton(hwnd, IDC_LOG, BST_CHECKED);
		else
			CheckDlgButton(hwnd, IDC_LOG, BST_UNCHECKED);

#ifndef AVILOG
		ShowWindow(GetDlgItem(hwnd, IDC_CLEAR), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_VIDEO), SW_HIDE);
#endif
		if (vnclog.GetVideo()) {
			SetDlgItemText(hwnd, IDC_EDIT_PATH, vnclog.GetPath());
			//EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PATH), true);
			CheckDlgButton(hwnd, IDC_VIDEO, BST_CHECKED);
		}
		else {
			SetDlgItemText(hwnd, IDC_EDIT_PATH, vnclog.GetPath());
			//EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PATH), false);
			CheckDlgButton(hwnd, IDC_VIDEO, BST_UNCHECKED);
		}

		// Marscha@2004 - authSSP: moved MS-Logon checkbox back to admin props page
		// added New MS-Logon checkbox
		// only enable New MS-Logon checkbox and Configure MS-Logon groups when MS-Logon
		// is checked.
		HWND hSecure = GetDlgItem(hwnd, IDC_SAVEPASSWORDSECURE);
		SendMessage(hSecure, BM_SETCHECK, settings->getSecure(), 0);

		HWND hMSLogon = GetDlgItem(hwnd, IDC_MSLOGON_CHECKD);
		SendMessage(hMSLogon, BM_SETCHECK, settings->getRequireMSLogon(), 0);

		HWND hNewMSLogon = GetDlgItem(hwnd, IDC_NEW_MSLOGON);
		SendMessage(hNewMSLogon, BM_SETCHECK, settings->getNewMSLogon(), 0);

		HWND hReverseAuth = GetDlgItem(hwnd, IDC_REVERSEAUTH);
		SendMessage(hReverseAuth, BM_SETCHECK, settings->getReverseAuthRequired(), 0);

		EnableWindow(GetDlgItem(hwnd, IDC_NEW_MSLOGON), settings->getRequireMSLogon());
		EnableWindow(GetDlgItem(hwnd, IDC_MSLOGON), settings->getRequireMSLogon());
		// Marscha@2004 - authSSP: end of change

		SetDlgItemInt(hwnd, IDC_SCALE, settings->getDefaultScale(), false);

		// Remote input settings
		HWND hEnableRemoteInputs = GetDlgItem(hwnd, IDC_DISABLE_INPUTS);
		SendMessage(hEnableRemoteInputs, BM_SETCHECK, !(settings->getEnableRemoteInputs()), 0);

		// Local input settings
		HWND hDisableLocalInputs = GetDlgItem(hwnd, IDC_DISABLE_LOCAL_INPUTS);
		SendMessage(hDisableLocalInputs, BM_SETCHECK, settings->getDisableLocalInputs(), 0);

		// japanese keybaord
		HWND hJapInputs = GetDlgItem(hwnd, IDC_JAP_INPUTS);
		SendMessage(hJapInputs, BM_SETCHECK, settings->getEnableJapInput(), 0);

		HWND hUnicodeInputs = GetDlgItem(hwnd, IDC_UNICODE_INPUTS);
		SendMessage(hUnicodeInputs, BM_SETCHECK, settings->getEnableUnicodeInput(), 0);

		HWND hwinhelper = GetDlgItem(hwnd, IDC_WIN8_HELPER);
		SendMessage(hwinhelper, BM_SETCHECK, settings->getEnableWin8Helper(), 0);

		// Remove the wallpaper
		HWND hRemoveWallpaper = GetDlgItem(hwnd, IDC_REMOVE_WALLPAPER);
		SendMessage(hRemoveWallpaper, BM_SETCHECK, settings->getRemoveWallpaper(), 0);

		// Lock settings
		HWND hLockSetting{};
		switch (settings->getLockSettings()) {
		case 1:
			hLockSetting = GetDlgItem(hwnd, IDC_LOCKSETTING_LOCK);
			break;
		case 2:
			hLockSetting = GetDlgItem(hwnd, IDC_LOCKSETTING_LOGOFF);
			break;
		default:
			hLockSetting = GetDlgItem(hwnd, IDC_LOCKSETTING_NOTHING);
		};
		SendMessage(hLockSetting,
			BM_SETCHECK,
			TRUE,
			0);

		HWND hNotificationSelection{};
		switch (settings->getNotificationSelection()) {
		case 1:
			hNotificationSelection = GetDlgItem(hwnd, IDC_RADIONOTIFICATIONIFPROVIDED);
			break;
		default:
			hNotificationSelection = GetDlgItem(hwnd,
				IDC_RADIONOTIFICATIONON);
			break;
		};
		SendMessage(hNotificationSelection,
			BM_SETCHECK,
			TRUE,
			0);

		HWND hmvSetting = 0;
		switch (settings->getConnectPriority()) {
		case 0:
			hmvSetting = GetDlgItem(hwnd, IDC_MV1);
			break;
		case 1:
			hmvSetting = GetDlgItem(hwnd, IDC_MV2);
			break;
		case 2:
			hmvSetting = GetDlgItem(hwnd, IDC_MV3);
			break;
		case 3:
			hmvSetting = GetDlgItem(hwnd, IDC_MV4);
			break;
		};
		SendMessage(hmvSetting,
			BM_SETCHECK,
			TRUE,
			0);

		HWND hQuerySetting{};
		switch (_this->m_server->QueryAccept()) {
		case 0:
			hQuerySetting = GetDlgItem(hwnd, IDC_DREFUSE);
			break;
		case 1:
			hQuerySetting = GetDlgItem(hwnd, IDC_DACCEPT);
			break;
		case 2:
			hQuerySetting = GetDlgItem(hwnd, IDC_DRefuseOnly);
			break;
		default:
			hQuerySetting = GetDlgItem(hwnd, IDC_DREFUSE);
		};
		SendMessage(hQuerySetting,
			BM_SETCHECK,
			TRUE,
			0);

		HWND hMaxViewerSetting = NULL;
		switch (settings->getMaxViewerSetting()) {
		case 0:
			hMaxViewerSetting = GetDlgItem(hwnd, IDC_MAXREFUSE);
			break;
		case 1:
			hMaxViewerSetting = GetDlgItem(hwnd, IDC_MAXDISCONNECT);
			break;
		default:
			hMaxViewerSetting = GetDlgItem(hwnd, IDC_MAXREFUSE);
		};
		SendMessage(hMaxViewerSetting,
			BM_SETCHECK,
			TRUE,
			0);

		HWND hCollabo = GetDlgItem(hwnd, IDC_COLLABO);
		SendMessage(hCollabo, BM_SETCHECK, settings->getCollabo(), 0);

		HWND hwndDlg = GetDlgItem(hwnd, IDC_FRAME);
		SendMessage(hwndDlg, BM_SETCHECK, settings->getFrame(), 0);

		hwndDlg = GetDlgItem(hwnd, IDC_NOTIFOCATION);
		SendMessage(hwndDlg, BM_SETCHECK, settings->getNotification(), 0);

		hwndDlg = GetDlgItem(hwnd, IDC_OSD);
		SendMessage(hwndDlg, BM_SETCHECK, settings->getOSD(), 0);

		char maxviewersChar[128]{};
		UINT maxviewers = settings->getMaxViewers();
		sprintf_s(maxviewersChar, "%d", (int)maxviewers);
		SetDlgItemText(hwnd, IDC_MAXVIEWERS, (const char*)maxviewersChar);

		// sf@2002 - List available DSM Plugins
		HWND hPlugins = GetDlgItem(hwnd, IDC_PLUGINS_COMBO);
		int nPlugins = _this->m_server->GetDSMPluginPointer()->ListPlugins(hPlugins);
#ifndef SC_20
		if (!nPlugins) {
			SendMessage(hPlugins, CB_ADDSTRING, 0, (LPARAM)sz_ID_NO_PLUGIN_DETECT);
			SendMessage(hPlugins, CB_SETCURSEL, 0, 0);
		}
		else
			SendMessage(hPlugins, CB_SELECTSTRING, 0, (LPARAM)settings->getSzDSMPlugin());
#else
		SendMessage(hPlugins, CB_ADDSTRING, 0, (LPARAM)settings->getSzDSMPlugin());
		SendMessage(hPlugins, CB_SETCURSEL, 0, 0);
		SendMessage(hPlugins, CB_SELECTSTRING, 0, (LPARAM)settings->getSzDSMPlugin());
#endif // SC_20

		// Modif sf@2002
		SendMessage(GetDlgItem(hwnd, IDC_PLUGIN_CHECK), BM_SETCHECK, settings->getUseDSMPlugin(), 0);
		EnableWindow(GetDlgItem(hwnd, IDC_PLUGIN_BUTTON), (_this->m_server->AuthClientCount() == 0 ? settings->getUseDSMPlugin() : false));

		// Query window option - Taken from TightVNC advanced properties
		BOOL queryEnabled = (settings->getQuerySetting() == 4);
		SendMessage(GetDlgItem(hwnd, IDQUERY), BM_SETCHECK, queryEnabled, 0);
		EnableWindow(GetDlgItem(hwnd, IDQUERYTIMEOUT), queryEnabled);
		EnableWindow(GetDlgItem(hwnd, IDC_QUERYDISABLETIME), queryEnabled);
		EnableWindow(GetDlgItem(hwnd, IDC_DREFUSE), queryEnabled);
		EnableWindow(GetDlgItem(hwnd, IDC_DACCEPT), queryEnabled);

		SetDlgItemText(hwnd, IDC_SERVICE_COMMANDLINE, settings->getService_commandline());
		SetDlgItemText(hwnd, IDC_EDITQUERYTEXT, settings->getAccept_reject_mesg());

		char timeout[128]{};
		UINT t = settings->getQueryTimeout();
		sprintf_s(timeout, "%d", (int)t);
		SetDlgItemText(hwnd, IDQUERYTIMEOUT, (const char*)timeout);

		char disableTime[128]{};
		UINT tt = settings->getQueryDisableTime();
		sprintf_s(disableTime, "%d", (int)tt);
		SetDlgItemText(hwnd, IDC_QUERYDISABLETIME, (const char*)disableTime);

		_this->ExpandBox(hwnd, !_this->bExpanded);

		SetForegroundWindow(hwnd);		
		return FALSE; // Because we've set the focus
	}
	case WM_DESTROY :
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_SHOWOPTIONS:
		case IDC_BUTTON_EXPAND:
			_this->ExpandBox(hwnd, !_this->bExpanded);
			return TRUE;
		case IDOK:
		case IDC_APPLY:
		{
			char path[512];
			int lenpath = GetDlgItemText(hwnd, IDC_EDIT_PATH, (LPSTR)&path, 512);
			if (lenpath != 0)
			{
				vnclog.SetPath(path);
			}
			else
			{
				strcpy_s(path, "");
				vnclog.SetPath(path);
			}

			bool Secure_old = settings->getSecure();
			HWND hSecure = GetDlgItem(hwnd, IDC_SAVEPASSWORDSECURE);
			settings->setSecure(SendMessage(hSecure, BM_GETCHECK, 0, 0) == BST_CHECKED);

			// Save the password
			char passwd[MAXPWLEN + 1];
			char passwd2[MAXPWLEN + 1];
			memset(passwd, '\0', MAXPWLEN + 1); //PGM
			memset(passwd2, '\0', MAXPWLEN + 1); //PGM
			// TightVNC method
			int lenPassword = GetDlgItemText(hwnd, IDC_PASSWORD, (LPSTR)&passwd, MAXPWLEN + 1);
			int lenPassword2 = GetDlgItemText(hwnd, IDC_PASSWORD2, (LPSTR)&passwd2, MAXPWLEN + 1); //PGM

			bool bSecure = settings->getSecure() ? true : false;
			if (Secure_old != bSecure) {
				//We changed the method to save the password
				//load passwords and encrypt the other method
				vncPasswd::ToText plain(settings->getPasswd(), Secure_old);
				vncPasswd::ToText plain2(settings->getPasswd2(), Secure_old);
				memset(passwd, '\0', MAXPWLEN + 1); //PGM
				memset(passwd2, '\0', MAXPWLEN + 1); //PGM
				strcpy_s(passwd, plain);
				strcpy_s(passwd2, plain2);
				lenPassword = (int)strlen(passwd);
				lenPassword2 = (int)strlen(passwd2);
			}

			if (strcmp(passwd, "~~~~~~~~") != 0) {
				if (lenPassword == 0)
				{
					vncPasswd::FromClear crypt(settings->getSecure());
					settings->setPasswd(crypt);
				}
				else
				{
					vncPasswd::FromText crypt(passwd, settings->getSecure());
					settings->setPasswd(crypt);
				}
			}

			if (strcmp(passwd2, "~~~~~~~~") != 0) { //PGM
				if (lenPassword2 == 0) { //PGM
					vncPasswd::FromClear crypt2(settings->getSecure()); //PGM
					settings->setPasswd2(crypt2); //PGM
				} //PGM
				else { //PGM
					vncPasswd::FromText crypt2(passwd2, settings->getSecure()); //PGM
					settings->setPasswd2(crypt2); //PGM
				} //PGM
			} //PGM

			//avoid readonly and full passwd being set the same
			if (strcmp(passwd, "~~~~~~~~") != 0 && strcmp(passwd2, "~~~~~~~~") != 0) {
				if (strcmp(passwd, passwd2) == 0)
					MessageBox(NULL, "View only and full password are the same\nView only ignored", "Warning", 0);
			}

			// Save the new settings to the server
			int state = (int)SendDlgItemMessage(hwnd, IDC_PORTNO_AUTO, BM_GETCHECK, 0, 0);
			_this->m_server->SetAutoPortSelect(state == BST_CHECKED);
			settings->setAutoPortSelect(state == BST_CHECKED);

			// Save port numbers if we're not auto selecting
			if (!settings->getAutoPortSelect()) {
				if (SendDlgItemMessage(hwnd, IDC_SPECDISPLAY, BM_GETCHECK, 0, 0) == BST_CHECKED) {
					// Display number was specified
					BOOL ok;
					UINT display = GetDlgItemInt(hwnd, IDC_DISPLAYNO, &ok, TRUE);
					if (ok)
						_this->m_server->SetPorts(DISPLAY_TO_PORT(display),
							DISPLAY_TO_HPORT(display));
				}
				else {
					// Assuming that port numbers were specified
					BOOL ok1{}, ok2{};
					UINT port_rfb = GetDlgItemInt(hwnd, IDC_PORTRFB, &ok1, TRUE);
					UINT port_http = GetDlgItemInt(hwnd, IDC_PORTHTTP, &ok2, TRUE);
					if (ok1 && ok2) {
						_this->m_server->SetPorts(port_rfb, port_http);
						settings->setPortNumber(port_rfb);
						settings->setHttpPortNumber(port_http);
					}
				}
			}
			HWND hConnectSock = GetDlgItem(hwnd, IDC_CONNECT_SOCK);
			_this->m_server->EnableConnections(SendMessage(hConnectSock, BM_GETCHECK, 0, 0) == BST_CHECKED);
			settings->setEnableConnections(SendMessage(hConnectSock, BM_GETCHECK, 0, 0) == BST_CHECKED);			

			// Update display/port controls on pressing the "Apply" button
			if (LOWORD(wParam) == IDC_APPLY)
				_this->InitPortSettings(hwnd);

			HWND hConnectHTTP = GetDlgItem(hwnd, IDC_CONNECT_HTTP);
			_this->m_server->EnableHTTPConnect(SendMessage(hConnectHTTP, BM_GETCHECK, 0, 0) == BST_CHECKED);
			settings->setHTTPConnect(SendMessage(hConnectHTTP, BM_GETCHECK, 0, 0) == BST_CHECKED);

			// Remote input stuff
			HWND hEnableRemoteInputs = GetDlgItem(hwnd, IDC_DISABLE_INPUTS);
			_this->m_server->EnableRemoteInputs(SendMessage(hEnableRemoteInputs, BM_GETCHECK, 0, 0) != BST_CHECKED);

			// Local input stuff
			HWND hDisableLocalInputs = GetDlgItem(hwnd, IDC_DISABLE_LOCAL_INPUTS);
			settings->setDisableLocalInputs(SendMessage(hDisableLocalInputs, BM_GETCHECK, 0, 0) == BST_CHECKED
			);

			// japanese keyboard
			HWND hJapInputs = GetDlgItem(hwnd, IDC_JAP_INPUTS);
			_this->m_server->EnableJapInput(SendMessage(hJapInputs, BM_GETCHECK, 0, 0) == BST_CHECKED);

			// japanese keyboard
			HWND hUnicodeInputs = GetDlgItem(hwnd, IDC_UNICODE_INPUTS);
			_this->m_server->EnableUnicodeInput(SendMessage(hUnicodeInputs, BM_GETCHECK, 0, 0) == BST_CHECKED);

			HWND hwinhelper = GetDlgItem(hwnd, IDC_WIN8_HELPER);
			settings->setEnableWin8Helper(SendMessage(hwinhelper, BM_GETCHECK, 0, 0) == BST_CHECKED);

			// Wallpaper handling
			HWND hRemoveWallpaper = GetDlgItem(hwnd, IDC_REMOVE_WALLPAPER);
			settings->setRemoveWallpaper(SendMessage(hRemoveWallpaper, BM_GETCHECK, 0, 0) == BST_CHECKED);

			// Lock settings handling
			if (SendMessage(GetDlgItem(hwnd, IDC_LOCKSETTING_LOCK), BM_GETCHECK, 0, 0) == BST_CHECKED) {
				settings->setLockSettings(1);
			}
			else if (SendMessage(GetDlgItem(hwnd, IDC_LOCKSETTING_LOGOFF), BM_GETCHECK, 0, 0) == BST_CHECKED) {
				settings->setLockSettings(2);
			}
			else {
				settings->setLockSettings(0);
			}

			if (SendMessage(GetDlgItem(hwnd, IDC_RADIONOTIFICATIONON), BM_GETCHECK, 0, 0) == BST_CHECKED) {
				settings->setNotificationSelection(0);
			}
			else if (SendMessage(GetDlgItem(hwnd, IDC_RADIONOTIFICATIONIFPROVIDED), BM_GETCHECK, 0, 0) == BST_CHECKED) {
				settings->setNotificationSelection(1);
			}

			if (SendMessage(GetDlgItem(hwnd, IDC_DREFUSE), BM_GETCHECK, 0, 0) == BST_CHECKED) {
				settings->getQueryIfNoLogon() == 0
					? settings->setQueryAccept(2)
					: settings->setQueryAccept(0);
			}
			else if (SendMessage(GetDlgItem(hwnd, IDC_DACCEPT), BM_GETCHECK, 0, 0) == BST_CHECKED) {
				settings->setQueryAccept(1);
			}

			if (SendMessage(GetDlgItem(hwnd, IDC_MAXREFUSE), BM_GETCHECK, 0, 0) == BST_CHECKED) {
				settings->setMaxViewerSetting(0);
			}
			else if (SendMessage(GetDlgItem(hwnd, IDC_MAXDISCONNECT), BM_GETCHECK, 0, 0) == BST_CHECKED) {
				settings->setMaxViewerSetting(1);
			}

			char maxViewerChar[256];
			strcpy_s(maxViewerChar, "128");
			if (GetDlgItemText(hwnd, IDC_MAXVIEWERS, (LPSTR)&maxViewerChar, 256) == 0) {
				int value = atoi(maxViewerChar);
				if (value > 128)
					value = 128;
				settings->setMaxViewers(value);
			}
			else
				settings->setMaxViewers(atoi(maxViewerChar));

			HWND hCollabo = GetDlgItem(hwnd, IDC_COLLABO);
			settings->setCollabo(SendMessage(hCollabo, BM_GETCHECK, 0, 0) == BST_CHECKED);

			HWND hwndDlg = GetDlgItem(hwnd, IDC_FRAME);
			settings->setFrame(SendMessage(hwndDlg, BM_GETCHECK, 0, 0) == BST_CHECKED);

			hwndDlg = GetDlgItem(hwnd, IDC_NOTIFOCATION);
			settings->setNotification(SendMessage(hwndDlg, BM_GETCHECK, 0, 0) == BST_CHECKED);

			hwndDlg = GetDlgItem(hwnd, IDC_OSD);
			settings->setOSD(SendMessage(hwndDlg, BM_GETCHECK, 0, 0) == BST_CHECKED);

			if (SendMessage(GetDlgItem(hwnd, IDC_MV1), BM_GETCHECK, 0, 0) == BST_CHECKED) {
				settings->setConnectPriority(0);
			}
			else if (SendMessage(GetDlgItem(hwnd, IDC_MV2), BM_GETCHECK, 0, 0) == BST_CHECKED) {
				settings->setConnectPriority(1);
			}
			else if (SendMessage(GetDlgItem(hwnd, IDC_MV3), BM_GETCHECK, 0, 0) == BST_CHECKED) {
				settings->setConnectPriority(2);
			}
			else if (SendMessage(GetDlgItem(hwnd, IDC_MV4), BM_GETCHECK, 0, 0) == BST_CHECKED) {
				settings->setConnectPriority(3);
			}

			// Modif sf@2002 - v1.1.0
			HWND hFileTransfer = GetDlgItem(hwnd, IDC_FILETRANSFER);
			settings->setEnableFileTransfer(SendMessage(hFileTransfer, BM_GETCHECK, 0, 0) == BST_CHECKED);

			HWND hFileTransferUserImp = GetDlgItem(hwnd, IDC_FTUSERIMPERSONATION_CHECK);
			settings->setFTUserImpersonation(SendMessage(hFileTransferUserImp, BM_GETCHECK, 0, 0) == BST_CHECKED);

			HWND hBlank = GetDlgItem(hwnd, IDC_BLANK);
			settings->setEnableBlankMonitor(SendMessage(hBlank, BM_GETCHECK, 0, 0) == BST_CHECKED);
			HWND hBlank2 = GetDlgItem(hwnd, IDC_BLANK2); //PGM
			settings->setBlankInputsOnly(SendMessage(hBlank2, BM_GETCHECK, 0, 0) == BST_CHECKED); //PGM

			settings->setAllowLoopback(IsDlgButtonChecked(hwnd, IDC_ALLOWLOOPBACK));
			settings->setIPV6(IsDlgButtonChecked(hwnd, IDC_IPV6));
			_this->m_server->SetLoopbackOnly(IsDlgButtonChecked(hwnd, IDC_LOOPBACKONLY));

			settings->setDisableTrayIcon(IsDlgButtonChecked(hwnd, IDC_DISABLETRAY));
			settings->setRdpmode(IsDlgButtonChecked(hwnd, IDC_RDPMODE));
			settings->setNoScreensaver(IsDlgButtonChecked(hwnd, IDC_NOSCREENSAVER));
			_this->m_server->SetNoScreensaver(IsDlgButtonChecked(hwnd, IDC_NOSCREENSAVER));
			settings->setAllowShutdown(!IsDlgButtonChecked(hwnd, IDC_ALLOWSHUTDOWN));
			settings->setAllowEditClients(!IsDlgButtonChecked(hwnd, IDC_ALLOWEDITCLIENTS));

			if (IsDlgButtonChecked(hwnd, IDC_LOG)) {
				vnclog.SetMode(2);
				vnclog.SetLevel(10);
			}
			else
				vnclog.SetMode(0);

			if (IsDlgButtonChecked(hwnd, IDC_VIDEO))
				vnclog.SetVideo(true);
			else
				vnclog.SetVideo(false);

			// Modif sf@2002 - v1.1.0
			// Marscha@2004 - authSSP: moved MS-Logon checkbox back to admin props page
			// added New MS-Logon checkbox

			HWND hMSLogon = GetDlgItem(hwnd, IDC_MSLOGON_CHECKD);
			settings->setRequireMSLogon(SendMessage(hMSLogon, BM_GETCHECK, 0, 0) == BST_CHECKED);

			HWND hNewMSLogon = GetDlgItem(hwnd, IDC_NEW_MSLOGON);
			settings->setNewMSLogon(SendMessage(hNewMSLogon, BM_GETCHECK, 0, 0) == BST_CHECKED);
			// Marscha@2004 - authSSP: end of change

			HWND hReverseAuth = GetDlgItem(hwnd, IDC_REVERSEAUTH);
			settings->setReverseAuthRequired(SendMessage(hReverseAuth, BM_GETCHECK, 0, 0) == BST_CHECKED);

			int nDScale = GetDlgItemInt(hwnd, IDC_SCALE, NULL, FALSE);
			if (nDScale < 1 || nDScale > 9) nDScale = 1;
			settings->setDefaultScale(nDScale);

			// sf@2002 - DSM Plugin loading
			// If Use plugin is checked, load the plugin if necessary
			if (SendMessage(GetDlgItem(hwnd, IDC_PLUGIN_CHECK), BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				TCHAR szPlugin[MAX_PATH];
				GetDlgItemText(hwnd, IDC_PLUGINS_COMBO, szPlugin, MAX_PATH);
				settings->setSzDSMPlugin(szPlugin);
				settings->setUseDSMPlugin(true);
			}
			else // If Use plugin unchecked but the plugin is loaded, unload it
			{
				settings->setUseDSMPlugin(false);
				if (_this->m_server->GetDSMPluginPointer()->IsLoaded())
				{
					_this->m_server->GetDSMPluginPointer()->UnloadPlugin();
					_this->m_server->GetDSMPluginPointer()->SetEnabled(false);
				}
			}

			// Query Window options - Taken from TightVNC advanced properties
			char timeout[256];
			strcpy_s(timeout, "5");
			if (GetDlgItemText(hwnd, IDQUERYTIMEOUT, (LPSTR)&timeout, 256) == 0)
				settings->setQueryTimeout(atoi(timeout));
			else
				settings->setQueryTimeout(atoi(timeout));

			char disabletime[256];
			strcpy_s(disabletime, "5");
			if (GetDlgItemText(hwnd, IDC_QUERYDISABLETIME, (LPSTR)&disabletime, 256) == 0)
				settings->setQueryDisableTime(atoi(disabletime));
			else
				settings->setQueryDisableTime(atoi(disabletime));

			char temp[1024]{};
			char temp2[512]{};
			GetDlgItemText(hwnd, IDC_SERVICE_COMMANDLINE, temp, 1024);
			GetDlgItemText(hwnd, IDC_EDITQUERYTEXT, temp2, 512);
			settings->setService_commandline(temp);
			settings->setAccept_reject_mesg(temp2);

			HWND hQuery = GetDlgItem(hwnd, IDQUERY);
			settings->setQuerySetting((SendMessage(hQuery, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 4 : 2);
#ifndef SC_20
			_this->SaveToIniFile();
#endif // SC_20
			// Was ok pressed?
			if (LOWORD(wParam) == IDOK) {
				// Yes, so close the dialog
				vnclog.Print(LL_INTINFO, VNCLOG("enddialog (OK)\n"));
				EndDialog(hwnd, IDOK);
				_this->m_dlgvisible = FALSE;
			}
			_this->m_server->SetHookings();
			return TRUE;
		}

		case IDCANCEL:
			vnclog.Print(LL_INTINFO, VNCLOG("enddialog (CANCEL)\n"));
			EndDialog(hwnd, IDCANCEL);
			_this->m_dlgvisible = FALSE;
			return TRUE;

			// Added Jef Fix - 5 March 2008 paquette@atnetsend.net
		case IDC_BLANK:
		{
			// only enable alpha blanking if blanking is enabled
			HWND hBlank = ::GetDlgItem(hwnd, IDC_BLANK);
			HWND hBlank2 = ::GetDlgItem(hwnd, IDC_BLANK2); //PGM
			::EnableWindow(hBlank2, ::SendMessage(hBlank, BM_GETCHECK, 0, 0) == BST_CHECKED); //PGM
		}
		break;

		case IDC_VIDEO:
		{
			if (IsDlgButtonChecked(hwnd, IDC_VIDEO))
				EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PATH), true);
			else
				EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PATH), false);
			break;
		}

		case IDC_CLEAR:
		{
			vnclog.ClearAviConfig();
			break;
		}

		case IDC_CONNECT_SOCK:
			// TightVNC 1.2.7 method
			// The user has clicked on the socket connect tickbox
		{
			BOOL bConnectSock =
				(SendDlgItemMessage(hwnd, IDC_CONNECT_SOCK,
					BM_GETCHECK, 0, 0) == BST_CHECKED);

			EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD), bConnectSock);

			HWND hPortNoAuto = GetDlgItem(hwnd, IDC_PORTNO_AUTO);
			EnableWindow(hPortNoAuto, bConnectSock);
			HWND hSpecDisplay = GetDlgItem(hwnd, IDC_SPECDISPLAY);
			EnableWindow(hSpecDisplay, bConnectSock);
			HWND hSpecPort = GetDlgItem(hwnd, IDC_SPECPORT);
			EnableWindow(hSpecPort, bConnectSock);

			EnableWindow(GetDlgItem(hwnd, IDC_DISPLAYNO), bConnectSock &&
				(SendMessage(hSpecDisplay, BM_GETCHECK, 0, 0) == BST_CHECKED));
			EnableWindow(GetDlgItem(hwnd, IDC_PORTRFB), bConnectSock &&
				(SendMessage(hSpecPort, BM_GETCHECK, 0, 0) == BST_CHECKED));
			EnableWindow(GetDlgItem(hwnd, IDC_PORTHTTP), bConnectSock &&
				(SendMessage(hSpecPort, BM_GETCHECK, 0, 0) == BST_CHECKED));
		}
		return TRUE;

		// TightVNC 1.2.7 method
		case IDC_PORTNO_AUTO:
		{
			EnableWindow(GetDlgItem(hwnd, IDC_DISPLAYNO), FALSE);
			EnableWindow(GetDlgItem(hwnd, IDC_PORTRFB), FALSE);
			EnableWindow(GetDlgItem(hwnd, IDC_PORTHTTP), FALSE);

			SetDlgItemText(hwnd, IDC_DISPLAYNO, "");
			SetDlgItemText(hwnd, IDC_PORTRFB, "");
			SetDlgItemText(hwnd, IDC_PORTHTTP, "");
		}
		return TRUE;

		case IDC_SPECDISPLAY:
		{
			EnableWindow(GetDlgItem(hwnd, IDC_DISPLAYNO), TRUE);
			EnableWindow(GetDlgItem(hwnd, IDC_PORTRFB), FALSE);
			EnableWindow(GetDlgItem(hwnd, IDC_PORTHTTP), FALSE);

			int display = PORT_TO_DISPLAY(settings->getPortNumber());
			if (display < 0 || display > 99)
				display = 0;
			SetDlgItemInt(hwnd, IDC_DISPLAYNO, display, FALSE);
			SetDlgItemInt(hwnd, IDC_PORTRFB, settings->getPortNumber(), FALSE);
			SetDlgItemInt(hwnd, IDC_PORTHTTP, settings->getHttpPortNumber(), FALSE);

			SetFocus(GetDlgItem(hwnd, IDC_DISPLAYNO));
			SendDlgItemMessage(hwnd, IDC_DISPLAYNO, EM_SETSEL, 0, (LPARAM)-1);
		}
		return TRUE;

		case IDC_SPECPORT:
		{
			EnableWindow(GetDlgItem(hwnd, IDC_DISPLAYNO), FALSE);
			EnableWindow(GetDlgItem(hwnd, IDC_PORTRFB), TRUE);
			EnableWindow(GetDlgItem(hwnd, IDC_PORTHTTP), TRUE);

			int d1 = PORT_TO_DISPLAY(settings->getPortNumber());
			int d2 = HPORT_TO_DISPLAY(settings->getHttpPortNumber());
			if (d1 == d2 && d1 >= 0 && d1 <= 99) {
				SetDlgItemInt(hwnd, IDC_DISPLAYNO, d1, FALSE);
			}
			else {
				SetDlgItemText(hwnd, IDC_DISPLAYNO, "");
			}
			SetDlgItemInt(hwnd, IDC_PORTRFB, settings->getPortNumber(), FALSE);
			SetDlgItemInt(hwnd, IDC_PORTHTTP, settings->getHttpPortNumber(), FALSE);

			SetFocus(GetDlgItem(hwnd, IDC_PORTRFB));
			SendDlgItemMessage(hwnd, IDC_PORTRFB, EM_SETSEL, 0, (LPARAM)-1);
		}
		return TRUE;

		// Query window option - Taken from TightVNC advanced properties code
		case IDQUERY:
		{
			HWND hQuery = GetDlgItem(hwnd, IDQUERY);
			BOOL queryon = (SendMessage(hQuery, BM_GETCHECK, 0, 0) == BST_CHECKED);
			EnableWindow(GetDlgItem(hwnd, IDQUERYTIMEOUT), queryon);
			EnableWindow(GetDlgItem(hwnd, IDC_QUERYDISABLETIME), queryon);
			EnableWindow(GetDlgItem(hwnd, IDC_DREFUSE), queryon);
			EnableWindow(GetDlgItem(hwnd, IDC_DACCEPT), queryon);
		}
		return TRUE;

		case IDC_STARTREP:
		{
			auto newconn = std::make_unique<vncConnDialog>(_this->m_server);
			if (newconn)
			{
				newconn->DoDialog(true);
				// delete newconn; // NO ! Already done in vncConnDialog.
			}
		}

		// sf@2002 - DSM Plugin
		case IDC_PLUGIN_CHECK:
		{
			EnableWindow(GetDlgItem(hwnd, IDC_PLUGIN_BUTTON), _this->m_server->AuthClientCount() == 0
				? SendMessage(GetDlgItem(hwnd, IDC_PLUGIN_CHECK), BM_GETCHECK, 0, 0) == BST_CHECKED
				: BST_UNCHECKED);
		}
		return TRUE;
		// Marscha@2004 - authSSP: moved MSLogon checkbox back to admin props page
		// Reason: Different UI for old and new mslogon group config.
		case IDC_MSLOGON_CHECKD:
		{
			BOOL bMSLogonChecked =
				(SendDlgItemMessage(hwnd, IDC_MSLOGON_CHECKD,
					BM_GETCHECK, 0, 0) == BST_CHECKED);

			EnableWindow(GetDlgItem(hwnd, IDC_NEW_MSLOGON), bMSLogonChecked);
			EnableWindow(GetDlgItem(hwnd, IDC_MSLOGON), bMSLogonChecked);
		}
		return TRUE;
#ifndef SC_20
		case IDC_MSLOGON:
		{
			// Marscha@2004 - authSSP: if "New MS-Logon" is checked,
			// call vncEditSecurity from SecurityEditor.dll,
			// else call "old" dialog.
			BOOL bNewMSLogonChecked = (SendDlgItemMessage(hwnd, IDC_NEW_MSLOGON, BM_GETCHECK, 0, 0) == BST_CHECKED);
			if (bNewMSLogonChecked) {
				DesktopUsersToken desktopUsersToken;
				HANDLE hPToken = desktopUsersToken.getDesktopUsersToken();
				if (!hPToken)
					break;

				char dir[MAX_PATH];
				char exe_file_name[MAX_PATH];
				GetModuleFileName(0, exe_file_name, MAX_PATH);
				strcpy_s(dir, exe_file_name);
				strcat_s(dir, " -securityeditorhelper");

				STARTUPINFO          StartUPInfo;
				PROCESS_INFORMATION  ProcessInfo;
				ZeroMemory(&StartUPInfo, sizeof(STARTUPINFO));
				ZeroMemory(&ProcessInfo, sizeof(PROCESS_INFORMATION));
				StartUPInfo.wShowWindow = SW_SHOW;
				StartUPInfo.lpDesktop = "Winsta0\\Default";
				StartUPInfo.cb = sizeof(STARTUPINFO);

				CreateProcessAsUser(hPToken, NULL, dir, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &StartUPInfo, &ProcessInfo);
				DWORD errorcode = GetLastError();
				if (ProcessInfo.hThread)
					CloseHandle(ProcessInfo.hThread);
				if (ProcessInfo.hProcess)
					CloseHandle(ProcessInfo.hProcess);
				if (errorcode == 1314)
					goto error;
				break;
			error:
				serviceHelpers::winvncSecurityEditorHelper_as_admin();
			}
			else {
				// Marscha@2004 - authSSP: end of change
				_this->m_vncauth.Init(_this->m_server);
				_this->m_vncauth.Show(TRUE);
			}
		}
		return TRUE;
#endif // SC_20
		case IDC_CHECKDRIVER:
			CheckVideoDriver(1);
			return TRUE;
		case IDC_PLUGIN_BUTTON:
		{
			HWND hPlugin = GetDlgItem(hwnd, IDC_PLUGIN_CHECK);
			if (SendMessage(hPlugin, BM_GETCHECK, 0, 0) == BST_CHECKED) {
				TCHAR szPlugin[MAX_PATH];
				GetDlgItemText(hwnd, IDC_PLUGINS_COMBO, szPlugin, MAX_PATH);
				PathStripPathA(szPlugin);

				if (!_this->m_server->GetDSMPluginPointer()->IsLoaded())
					_this->m_server->GetDSMPluginPointer()->LoadPlugin(szPlugin, false);
				else {
					// sf@2003 - We check if the loaded plugin is the same than
					// the currently selected one or not
					_this->m_server->GetDSMPluginPointer()->DescribePlugin();
					if (_stricmp(_this->m_server->GetDSMPluginPointer()->GetPluginFileName(), szPlugin)) {
						_this->m_server->GetDSMPluginPointer()->UnloadPlugin();
						_this->m_server->GetDSMPluginPointer()->LoadPlugin(szPlugin, false);
					}
				}

				if (_this->m_server->GetDSMPluginPointer()->IsLoaded())
					Secure_Save_Plugin_Config(szPlugin);
				else
					MessageBoxSecure(NULL, sz_ID_PLUGIN_NOT_LOAD, sz_ID_PLUGIN_LOADIN, MB_OK | MB_ICONEXCLAMATION);
			}
			return TRUE;
		}
		}
		break;
	case WM_SIZE:
			_this->SD_OnSize(hwnd, wParam, LOWORD(lParam), HIWORD(lParam));			
		break;
	case WM_HSCROLL:
		_this->SD_OnHScroll(hwnd, LOWORD(wParam));
		break;
	case  WM_VSCROLL:
		_this->SD_OnVScroll(hwnd, LOWORD(wParam));
		break;
	case WM_GETMINMAXINFO:
		MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
		if (_this->maxWidth != 0 && _this->maxHeight != 0) {
			mmi->ptMaxSize.x = _this->maxWidth;
			mmi->ptMaxSize.y = _this->maxHeight;
			mmi->ptMaxTrackSize.x = _this->maxWidth;
			mmi->ptMaxTrackSize.y = _this->maxHeight;
		}
		break;
	}
	return 0;
}

// TightVNC 1.2.7
// Set display/port settings to the correct state
void
vncProperties::InitPortSettings(HWND hwnd)
{
	BOOL bConnectSock = m_server->SockConnected();
	BOOL bAutoPort = settings->getAutoPortSelect();
	UINT port_rfb = settings->getPortNumber();
	UINT port_http = settings->getHttpPortNumber();
	int d1 = PORT_TO_DISPLAY(port_rfb);
	int d2 = HPORT_TO_DISPLAY(port_http);
	BOOL bValidDisplay = (d1 == d2 && d1 >= 0 && d1 <= 99);

	CheckDlgButton(hwnd, IDC_PORTNO_AUTO,
		(bAutoPort) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwnd, IDC_SPECDISPLAY,
		(!bAutoPort && bValidDisplay) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwnd, IDC_SPECPORT,
		(!bAutoPort && !bValidDisplay) ? BST_CHECKED : BST_UNCHECKED);

	EnableWindow(GetDlgItem(hwnd, IDC_PORTNO_AUTO), bConnectSock);
	EnableWindow(GetDlgItem(hwnd, IDC_SPECDISPLAY), bConnectSock);
	EnableWindow(GetDlgItem(hwnd, IDC_SPECPORT), bConnectSock);

	if (bValidDisplay) {
		SetDlgItemInt(hwnd, IDC_DISPLAYNO, d1, FALSE);
	}
	else {
		SetDlgItemText(hwnd, IDC_DISPLAYNO, "");
	}
	SetDlgItemInt(hwnd, IDC_PORTRFB, port_rfb, FALSE);
	SetDlgItemInt(hwnd, IDC_PORTHTTP, port_http, FALSE);

	EnableWindow(GetDlgItem(hwnd, IDC_DISPLAYNO),
		bConnectSock && !bAutoPort && bValidDisplay);
	EnableWindow(GetDlgItem(hwnd, IDC_PORTRFB),
		bConnectSock && !bAutoPort && !bValidDisplay);
	EnableWindow(GetDlgItem(hwnd, IDC_PORTHTTP),
		bConnectSock && !bAutoPort && !bValidDisplay);
}

// ********************************************************************
// Ini file part - Wwill replace registry access completely, some day
// WARNING: until then, when adding/modifying a config parameter
//          don't forget to modify both ini file & registry parts !
// ********************************************************************
void vncProperties::LoadFromIniFile()
{
#ifndef SC_20
	settings->load();
#endif // SC_20
	vnclog.SetMode(settings->getDebugMode());
	vnclog.SetPath(settings->getDebugPath());
	vnclog.SetLevel(settings->getDebugLevel());
	vnclog.SetVideo(settings->getAvilog());

	if (settings->getLoopbackOnly())
		settings->setAllowLoopback(true);

	if (!settings->getLoopbackOnly()) {
		if (settings->getAuthhosts() != 0) {
			m_server->SetAuthHosts(settings->getAuthhosts());
		}
		else {
			m_server->SetAuthHosts(0);
		}
	}
	else {
		m_server->SetAuthHosts(0);
	}

	m_server->SetNoScreensaver(settings->getnoscreensaver());
	m_server->SetLoopbackOnly(settings->getLoopbackOnly());

	// adzm - 2010-07 - Disable more effects or font smoothing
	//m_server->EnableRemoveFontSmoothing(settings->getRemoveFontSmoothing());
	//m_server->EnableRemoveEffects(settings->getRemoveEffects());
	m_server->EnableHTTPConnect(settings->getHTTPConnect());
	m_server->EnableRemoteInputs(settings->getEnableRemoteInputs());
	m_server->EnableJapInput(settings->getEnableJapInput());
	m_server->EnableUnicodeInput(settings->getEnableUnicodeInput());

	// Update the password

	// Now change the listening port settings
	m_server->SetAutoPortSelect(settings->getAutoPortSelect());
	if (!settings->getAutoPortSelect())
		m_server->SetPorts(settings->getPortNumber(), settings->getHttpPortNumber());

	m_server->EnableConnections(settings->getEnableConnections());

	// DSM Plugin prefs
	if (settings->getUseDSMPlugin())
		m_server->SetDSMPlugin(false);

	// adzm - 2010-07 - Disable more effects or font smoothing
	m_server->EnableHTTPConnect(settings->getHTTPConnect());
	m_server->EnableRemoteInputs(settings->getEnableRemoteInputs());
	m_server->EnableJapInput(settings->getEnableJapInput());
	m_server->EnableUnicodeInput(settings->getEnableUnicodeInput());

	// Now change the listening port settings
	m_server->SetAutoPortSelect(settings->getAutoPortSelect());
	if (!settings->getAutoPortSelect()) {
		m_server->SetPorts(settings->getPortNumber(), settings->getHttpPortNumber());
	}

	m_server->EnableConnections(settings->getEnableConnections());

	// DSM Plugin prefs
	if (settings->getUseDSMPlugin())
		m_server->SetDSMPlugin(false);
}

void vncProperties::SaveToIniFile()
{
#ifndef SC_20
	settings->save();
#endif // SC_20
}


void Secure_Save_Plugin_Config(char* szPlugin)
{
	DesktopUsersToken desktopUsersToken;
	HANDLE hPToken = desktopUsersToken.getDesktopUsersToken();
	if (!hPToken)
		return;

	char dir[MAX_PATH];
	char exe_file_name[MAX_PATH];
	GetModuleFileName(0, exe_file_name, MAX_PATH);
	strcpy_s(dir, exe_file_name);
	strcat_s(dir, " -dsmpluginhelper ");
	strcat_s(dir, szPlugin);

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
		Secure_Plugin(szPlugin);
	return;
}

void Secure_Plugin_elevated(char* szPlugin)
{
	char dir[MAX_PATH];
	char exe_file_name[MAX_PATH];
	strcpy_s(dir, " -dsmplugininstance ");
	strcat_s(dir, szPlugin);

	GetModuleFileName(0, exe_file_name, MAX_PATH);
	SHELLEXECUTEINFO shExecInfo;
	shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	shExecInfo.fMask = NULL;
	shExecInfo.hwnd = GetForegroundWindow();
	shExecInfo.lpVerb = "runas";
	shExecInfo.lpFile = exe_file_name;
	shExecInfo.lpParameters = dir;
	shExecInfo.lpDirectory = NULL;
	shExecInfo.nShow = SW_HIDE;
	shExecInfo.hInstApp = NULL;
	ShellExecuteEx(&shExecInfo);
}

void Secure_Plugin(char* szPlugin)
{
	auto m_pDSMPlugin = std::make_unique<CDSMPlugin>();
	m_pDSMPlugin->LoadPlugin(szPlugin, false);
	if (m_pDSMPlugin->IsLoaded()) {
		char szParams[32];
		strcpy_s(szParams, "NoPassword,");
		strcat_s(szParams, "server-app");

		HDESK desktop;
		desktop = OpenInputDesktop(0, FALSE, DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW | DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
			DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS | DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);

		if (desktop == NULL)
			vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop Error \n"));
		else
			vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop OK\n"));

		HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());
		DWORD dummy{};

		char new_name[256]{};
		if (desktop) {
			if (!GetUserObjectInformation(desktop, UOI_NAME, &new_name, 256, &dummy)) {
				vnclog.Print(LL_INTERR, VNCLOG("!GetUserObjectInformation \n"));
			}

			vnclog.Print(LL_INTERR, VNCLOG("SelectHDESK to %s (%x) from %x\n"), new_name, desktop, old_desktop);

			if (!SetThreadDesktop(desktop)) {
				vnclog.Print(LL_INTERR, VNCLOG("SelectHDESK:!SetThreadDesktop \n"));
			}
		}

		CoInitialize(NULL);
		HWND hwnd2 = CreateWindowA("STATIC", "dummy", WS_VISIBLE, 0, 0, 100, 100, NULL, NULL, NULL, NULL);
		ShowWindow(hwnd2, SW_HIDE);
		char* szNewConfig = NULL;
		char DSMPluginConfig[512];
		DSMPluginConfig[0] = '\0';
		IniFile myIniFile;
		myIniFile.ReadString("admin", "DSMPluginConfig", DSMPluginConfig, 512);
		m_pDSMPlugin->SetPluginParams(hwnd2, szParams, DSMPluginConfig, &szNewConfig);

		if (szNewConfig != NULL && strlen(szNewConfig) > 0) {
			strcpy_s(DSMPluginConfig, 511, szNewConfig);
		}
		myIniFile.WriteString("admin", "DSMPluginConfig", DSMPluginConfig);

		CoUninitialize();
		SetThreadDesktop(old_desktop);
		if (desktop) CloseDesktop(desktop);
	}
}

void vncProperties::ExpandBox(HWND hDlg, BOOL fExpand)
{
	// if the dialog is already in the requested state, return
	// immediately.
	if (fExpand == bExpanded) return;

	RECT rcWnd{}, rcDefaultBox{}, rcChild{}, rcIntersection{};
	HWND wndChild = NULL;
	HWND wndDefaultBox = NULL;

	// get the window of the button
	HWND  pCtrl = GetDlgItem(hDlg, IDC_BUTTON_EXPAND);
	if (pCtrl == NULL) return;

	wndDefaultBox = GetDlgItem(hDlg, IDC_DEFAULTBOX);
	if (wndDefaultBox == NULL) return;

	// retrieve coordinates for the default child window
	GetWindowRect(wndDefaultBox, &rcDefaultBox);

	// enable/disable all of the child window outside of the default box.
	wndChild = GetTopWindow(hDlg);

	for (; wndChild != NULL; wndChild = GetWindow(wndChild, GW_HWNDNEXT))
	{
		// get rectangle occupied by child window in screen coordinates.
		GetWindowRect(wndChild, &rcChild);

		if (!IntersectRect(&rcIntersection, &rcChild, &rcDefaultBox))
		{
			EnableWindow(wndChild, fExpand);
		}
	}

	if (!fExpand)  // we are contracting
	{
		_ASSERT(bExpanded);
		GetWindowRect(hDlg, &rcWnd);

		// this is the first time we are being called to shrink the dialog
		// box. The dialog box is currently in its expanded size and we must
		// save the expanded width and height so that it can be restored
		// later when the dialog box is expanded.

		if (cx == 0 && cy == 0)
		{
			cx = rcDefaultBox.right - rcWnd.left;
			cy = rcWnd.bottom - rcWnd.top;

			// we also hide the default box here so that it is not visible
			ShowWindow(wndDefaultBox, SW_HIDE);
		}

		// shrink the dialog box so that it encompasses everything from the top,
		// left up to and including the default box.
		maxWidth = rcDefaultBox.right - rcWnd.left;
		maxHeight = rcDefaultBox.bottom - rcWnd.top;
		SetWindowPos(hDlg, NULL, 0, 0,
			rcDefaultBox.right - rcWnd.left,
			rcDefaultBox.bottom - rcWnd.top,
			SWP_NOZORDER | SWP_NOMOVE);

		// record that the dialog is contracted.
		bExpanded = FALSE;
	}
	else // we are expanding
	{
		_ASSERT(!bExpanded);
		maxWidth = cx;
		maxHeight =cy;
		SetWindowPos(hDlg, NULL, 0, 0, cx, cy, SWP_NOZORDER | SWP_NOMOVE);

		// make sure that the entire dialog box is visible on the user's
		// screen.
		SendMessage(hDlg, DM_REPOSITION, 0, 0);
		bExpanded = TRUE;
	}
	SD_OnInitDialog(hDlg);
}

BOOL vncProperties::SD_OnInitDialog(HWND hwnd)
{
	RECT rc = {};
	GetClientRect(hwnd, &rc);

	const SIZE sz = { rc.right - rc.left, rc.bottom - rc.top };

	SCROLLINFO si = {};
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
	si.nPos = si.nMin = 1;

	si.nMax = sz.cx;
	si.nPage = sz.cx;
	SetScrollInfo(hwnd, SB_HORZ, &si, FALSE);

	si.nMax = sz.cy;
	si.nPage = sz.cy;
	SetScrollInfo(hwnd, SB_VERT, &si, FALSE);
	return FALSE;
}

void vncProperties::SD_OnSize(HWND hwnd, UINT state, int cx, int cy)
{
	if (state != SIZE_RESTORED && state != SIZE_MAXIMIZED)
		return;

	SCROLLINFO si = {};
	si.cbSize = sizeof(SCROLLINFO);

	const int bar[] = { SB_HORZ, SB_VERT };
	const int page[] = { cx, cy };

	for (size_t i = 0; i < ARRAYSIZE(bar); ++i)
	{
		si.fMask = SIF_PAGE;
		si.nPage = page[i];
		SetScrollInfo(hwnd, bar[i], &si, TRUE);

		si.fMask = SIF_RANGE | SIF_POS;
		GetScrollInfo(hwnd, bar[i], &si);

		const int maxScrollPos = si.nMax - (page[i] - 1);

		// Scroll client only if scroll bar is visible and window's
		// content is fully scrolled toward right and/or bottom side.
		// Also, update window's content on maximize.
		const bool needToScroll =
			(si.nPos != si.nMin && si.nPos == maxScrollPos) ||
			(state == SIZE_MAXIMIZED);

		if (needToScroll)
		{
			SD_ScrollClient(hwnd, bar[i], si.nPos);
		}
	}
}

void vncProperties::SD_OnHScroll(HWND hwnd, UINT code)
{
	SD_OnHVScroll(hwnd, SB_HORZ, code);
}

void vncProperties::SD_OnVScroll(HWND hwnd,  UINT code)
{
	SD_OnHVScroll(hwnd, SB_VERT, code);
}

void vncProperties::SD_OnHVScroll(HWND hwnd, int bar, UINT code)
{
	const int scrollPos = SD_GetScrollPos(hwnd, bar, code);

	if (scrollPos == -1)
		return;

	SetScrollPos(hwnd, bar, scrollPos, TRUE);
	SD_ScrollClient(hwnd, bar, scrollPos);
}

void vncProperties::SD_ScrollClient(HWND hwnd, int bar, int pos)
{
	static int s_prevx = 1;
	static int s_prevy = 1;

	int cx = 0;
	int cy = 0;

	int& delta = (bar == SB_HORZ ? cx : cy);
	int& prev = (bar == SB_HORZ ? s_prevx : s_prevy);

	delta = prev - pos;
	prev = pos;

	if (cx || cy)
	{
		ScrollWindow(hwnd, cx, cy, NULL, NULL);
	}
}

int vncProperties::SD_GetScrollPos(HWND hwnd, int bar, UINT code)
{
	SCROLLINFO si = {};
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;
	GetScrollInfo(hwnd, bar, &si);

	const int minPos = si.nMin;
	const int maxPos = si.nMax - (si.nPage - 1);

	int result = -1;

	switch (code)
	{
	case SB_LINEUP /*SB_LINELEFT*/:
		result = std::max(si.nPos - 1, minPos);
		break;

	case SB_LINEDOWN /*SB_LINERIGHT*/:
		result = std::min(si.nPos + 1, maxPos);
		break;

	case SB_PAGEUP /*SB_PAGELEFT*/:
		result = std::max(si.nPos - (int)si.nPage, minPos);
		break;

	case SB_PAGEDOWN /*SB_PAGERIGHT*/:
		result = std::min(si.nPos + (int)si.nPage, maxPos);
		break;

	case SB_THUMBPOSITION:
		// do nothing
		break;

	case SB_THUMBTRACK:
		result = si.nTrackPos;
		break;

	case SB_TOP /*SB_LEFT*/:
		result = minPos;
		break;

	case SB_BOTTOM /*SB_RIGHT*/:
		result = maxPos;
		break;

	case SB_ENDSCROLL:
		// do nothing
		break;
	}

	return result;
}
