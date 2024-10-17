#include "PropertiesDialog.h"
#include <shlobj.h>
#include "resource.h"
#include "SettingsManager.h"
#include "vncServer.h"
#include "vncOSVersion.h"
#include "Localization.h"
#include "common/win32_helpers.h"
#include "RulesListView.h"
#include <commctrl.h>
#include "vncconndialog.h"
#include "credentials.h"
#include <shlwapi.h>
#include "DlgChangePassword.h"

extern HINSTANCE	hInstResDLL;


BOOL CALLBACK DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK PropertiesDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PropertiesDialog* _this;
	if (uMsg == WM_INITDIALOG) {
		_this = (PropertiesDialog*)lParam;
		helper::SafeSetWindowUserData(hwnd, lParam);
	}
	else
		_this = (PropertiesDialog*)helper::SafeGetWindowUserData<PropertiesDialog>(hwnd);

	switch (uMsg) {
		case WM_INITDIALOG:
			return _this->InitDialog(hwnd);

		case WM_NOTIFY:
			return _this->HandleNotify(hwnd, wParam, lParam);

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
			case IDCANCEL:				
				_this->onCancel(hwnd);
				return TRUE;
			case IDC_APPLY:
				_this->onApply(hwnd);
				return TRUE;
			case IDOK:
				_this->onOK(hwnd);				
				return TRUE;
			default:
				break;
			}
		case WM_CLOSE:
			return FALSE;
		case WM_DESTROY:
			EndDialog(hwnd, FALSE);
			return TRUE;
	}
	return 0;
}
bool PropertiesDialog::InitDialog(HWND hwnd)
{
	PropertiesDialogHwnd = hwnd;

	SetForegroundWindow(hwnd);
	HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_WINVNC));
	SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

	showAdminPanel = false;
	if (settings->RunningFromExternalService()) {
		if (settings->IsDesktopUserAdmin())
			showAdminPanel = true;
		else if (settings->getAllowUserSettingsWithPassword() && !settings->checkAdminPassword()) {
				EndDialog(hwnd, IDCANCEL);
				return true;
		}
	}

	

	hTabControl = GetDlgItem(hwnd, IDC_PROPERTIESTAB);
	TCITEM item;
	item.mask = TCIF_TEXT;
	item.pszText = "Security";
	TabCtrl_InsertItem(hTabControl, 0, &item);
	item.pszText = "Connection";
	TabCtrl_InsertItem(hTabControl, 1, &item);
	item.pszText = "Input/FT";
	TabCtrl_InsertItem(hTabControl, 2, &item);
	item.pszText = "Misc";
	TabCtrl_InsertItem(hTabControl, 3, &item);
	item.pszText = "Notifications";
	TabCtrl_InsertItem(hTabControl, 4, &item);
	item.pszText = "Reverse";
	TabCtrl_InsertItem(hTabControl, 5, &item);
	item.pszText = "Rules";
	TabCtrl_InsertItem(hTabControl, 6, &item);
	item.pszText = "Capture";
	TabCtrl_InsertItem(hTabControl, 7, &item);	


	if (showAdminPanel)
	{
		item.pszText = "Administration";
		TabCtrl_InsertItem(hTabControl, 8, &item);
	}

	hTabAuthentication = CreateDialogParam(hInstResDLL,
		MAKEINTRESOURCE(IDD_FORM_AUTHENTICATION),
		hwnd,
		(DLGPROC)DlgProc,
		(LONG_PTR)this);
	hTabIncoming = CreateDialogParam(hInstResDLL,
		MAKEINTRESOURCE(IDD_FORM_Incoming),
		hwnd,
		(DLGPROC)DlgProc,
		(LONG_PTR)this);
	hTabInput = CreateDialogParam(hInstResDLL,
		MAKEINTRESOURCE(IDD_FORM_input_FT),
		hwnd,
		(DLGPROC)DlgProc,
		(LONG_PTR)this);
	hTabMisc = CreateDialogParam(hInstResDLL,
		MAKEINTRESOURCE(IDD_FORM_Misc),
		hwnd,
		(DLGPROC)DlgProc,
		(LONG_PTR)this);
	hTabNotifications = CreateDialogParam(hInstResDLL,
		MAKEINTRESOURCE(IDD_FORM_Notifications),
		hwnd,
		(DLGPROC)DlgProc,
		(LONG_PTR)this);
	hTabReverse = CreateDialogParam(hInstResDLL,
		MAKEINTRESOURCE(IDD_FORM_Reverse),
		hwnd,
		(DLGPROC)DlgProc,
		(LONG_PTR)this);
	hTabRules = CreateDialogParam(hInstResDLL,
		MAKEINTRESOURCE(IDD_FORM_Rules),
		hwnd,
		(DLGPROC)DlgProc,
		(LONG_PTR)this);
	hTabCapture = CreateDialogParam(hInstResDLL,
		MAKEINTRESOURCE(IDD_FORM_capture),
		hwnd,
		(DLGPROC)DlgProc,
		(LONG_PTR)this);
	hTabAdministration = CreateDialogParam(hInstResDLL,
		MAKEINTRESOURCE(IDD_FORM_administration),
		hwnd,
		(DLGPROC)DlgProc,
		(LONG_PTR)this);
	RECT rc;
	GetWindowRect(hTabControl, &rc);
	MapWindowPoints(NULL, hwnd, (POINT*)&rc, 2);
	TabCtrl_AdjustRect(hTabControl, FALSE, &rc);
	SetWindowPos(hTabAuthentication, HWND_TOP, rc.left, rc.top,
		rc.right - rc.left, rc.bottom - rc.top,
		SWP_SHOWWINDOW);
	SetWindowPos(hTabIncoming, HWND_TOP, rc.left, rc.top,
		rc.right - rc.left, rc.bottom - rc.top,
		SWP_HIDEWINDOW);
	SetWindowPos(hTabInput, HWND_TOP, rc.left, rc.top,
		rc.right - rc.left, rc.bottom - rc.top,
		SWP_HIDEWINDOW);
	SetWindowPos(hTabMisc, HWND_TOP, rc.left, rc.top,
		rc.right - rc.left, rc.bottom - rc.top,
		SWP_HIDEWINDOW);
	SetWindowPos(hTabNotifications, HWND_TOP, rc.left, rc.top,
		rc.right - rc.left, rc.bottom - rc.top,
		SWP_HIDEWINDOW);
	SetWindowPos(hTabReverse, HWND_TOP, rc.left, rc.top,
		rc.right - rc.left, rc.bottom - rc.top,
		SWP_HIDEWINDOW);
	SetWindowPos(hTabRules, HWND_TOP, rc.left, rc.top,
		rc.right - rc.left, rc.bottom - rc.top,
		SWP_HIDEWINDOW);
	SetWindowPos(hTabCapture, HWND_TOP, rc.left, rc.top,
		rc.right - rc.left, rc.bottom - rc.top,
		SWP_HIDEWINDOW);
	SetWindowPos(hTabAdministration, HWND_TOP, rc.left, rc.top,
		rc.right - rc.left, rc.bottom - rc.top,
		SWP_HIDEWINDOW);
	return TRUE;
};
int PropertiesDialog::HandleNotify(HWND hwndDlg, WPARAM wParam, LPARAM lParam)
{
	LPNMHDR pn = (LPNMHDR)lParam;
	switch (pn->code) {
	case TCN_SELCHANGE:
		switch (pn->idFrom) {
		case IDC_PROPERTIESTAB:
			int i = TabCtrl_GetCurFocus(hTabControl);
			switch (i) {
			case 0:
				ShowWindow(hTabAuthentication, SW_SHOW);
				SetFocus(hTabAuthentication);
				return 0;
			case 1:
				ShowWindow(hTabIncoming, SW_SHOW);
				SetFocus(hTabIncoming);
				return 0;

			case 2:
				ShowWindow(hTabInput, SW_SHOW);
				SetFocus(hTabInput);
				return 0;

			case 3:
				ShowWindow(hTabMisc, SW_SHOW);
				SetFocus(hTabMisc);
				return 0;

			case 4:
				ShowWindow(hTabNotifications, SW_SHOW);
				SetFocus(hTabNotifications);
				return 0;

			case 5:
				ShowWindow(hTabReverse, SW_SHOW);
				SetFocus(hTabReverse);
				return 0;

			case 6:
				ShowWindow(hTabRules, SW_SHOW);
				SetFocus(hTabRules);
				return 0;
			case 7:
				ShowWindow(hTabCapture, SW_SHOW);
				SetFocus(hTabCapture);
				return 0;
			case 8:
				ShowWindow(hTabAdministration, SW_SHOW);
				SetFocus(hTabAdministration);
				return 0;				
			}
			return 0;
		}
		return 0;
	case TCN_SELCHANGING:
		switch (pn->idFrom) {
		case IDC_PROPERTIESTAB:
			int i = TabCtrl_GetCurFocus(hTabControl);
			switch (i) {
			case 0:
				ShowWindow(hTabAuthentication, SW_HIDE);
				break;
			case 1:
				ShowWindow(hTabIncoming, SW_HIDE);
				break;

			case 2:
				ShowWindow(hTabInput, SW_HIDE);
				break;

			case 3:
				ShowWindow(hTabMisc, SW_HIDE);
				break;

			case 4:
				ShowWindow(hTabNotifications, SW_HIDE);
				break;

			case 5:
				ShowWindow(hTabReverse, SW_HIDE);
				break;

			case 6:
				ShowWindow(hTabRules, SW_HIDE);
				break;
			case 7:
				ShowWindow(hTabCapture, SW_HIDE);
				break;
			case 8:
				ShowWindow(hTabAdministration, SW_HIDE);
				break;
			}
			return 0;
		}
		return 0;
	}
	return 0;
}
bool CheckVideoDriver(bool Box);


BOOL CALLBACK DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PropertiesDialog* _this;
	if (uMsg == WM_INITDIALOG) {
		_this = (PropertiesDialog*)lParam;
		helper::SafeSetWindowUserData(hwnd, lParam);
	}
	else
		_this = (PropertiesDialog*)helper::SafeGetWindowUserData<PropertiesDialog>(hwnd);

	switch (uMsg)
	{
	case WM_INITDIALOG: {
		_this = (PropertiesDialog*)lParam;
		return _this->DlgInitDialog(hwnd);
	}
	case WM_CTLCOLORDLG:
		return (INT_PTR)GetStockObject(WHITE_BRUSH);
	case WM_CTLCOLORSTATIC: {
		HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
		SetBkMode(hdcStatic, TRANSPARENT);
		return (LONG)(INT_PTR)GetStockObject(WHITE_BRUSH);
	}
	case WM_COMMAND:
		_this->onCommand(LOWORD(wParam), hwnd);
	}
	return (INT_PTR)FALSE;
}
bool PropertiesDialog::DlgInitDialog(HWND hwnd)
{
	vnclog.SetPath(settings->getDebugPath());
	vnclog.SetLevel(settings->getDebugLevel());
	vnclog.SetVideo(settings->getAvilog());
	vnclog.SetMode(settings->getDebugMode());

	m_dlgvisible = TRUE;
	bConnectSock = settings->getEnableConnections();

	if (GetDlgItem(hwnd, IDC_CONNECT_SOCK)) {
		SetFocus(GetDlgItem(hwnd, IDC_CONNECT_SOCK));
		SendMessage(GetDlgItem(hwnd, IDC_CONNECT_SOCK), BM_SETCHECK, bConnectSock, 0);
	}

	// Set display/ports settings
	BOOL bConnectHttp = settings->getHTTPConnect();
	BOOL bAutoPort = settings->getAutoPortSelect();
	UINT port_rfb = settings->getPortNumber();
	UINT port_http = settings->getHttpPortNumber();
	int d1 = PORT_TO_DISPLAY(port_rfb);
	int d2 = HPORT_TO_DISPLAY(port_http);
	BOOL bValidDisplay = (d1 == d2 && d1 >= 0 && d1 <= 99);

	if (GetDlgItem(hwnd, IDC_SPECPORT)) {
		CheckDlgButton(hwnd, IDC_SPECPORT,
			(!bAutoPort && !bValidDisplay) ? BST_CHECKED : BST_UNCHECKED);
		EnableWindow(GetDlgItem(hwnd, IDC_SPECPORT), bConnectSock);
	}

	if (GetDlgItem(hwnd, IDC_PORTRFB)) {
		SetDlgItemInt(hwnd, IDC_PORTRFB, port_rfb, FALSE);
		EnableWindow(GetDlgItem(hwnd, IDC_PORTRFB),
			bConnectSock && !bAutoPort && !bValidDisplay);
	}

	if (GetDlgItem(hwnd, IDC_PORTHTTP)) {
		SetDlgItemInt(hwnd, IDC_PORTHTTP, port_http, FALSE);
		EnableWindow(GetDlgItem(hwnd, IDC_PORTHTTP),
			bConnectSock && !bAutoPort && !bValidDisplay && bConnectHttp);
	}

	if (GetDlgItem(hwnd, IDC_CONNECT_HTTP))
		SendMessage(GetDlgItem(hwnd, IDC_CONNECT_HTTP), BM_SETCHECK, settings->getHTTPConnect(), 0);

	if (GetDlgItem(hwnd, IDC_FILETRANSFER))
		SendMessage(GetDlgItem(hwnd, IDC_FILETRANSFER), BM_SETCHECK, settings->getEnableFileTransfer(), 0);

	if (GetDlgItem(hwnd, IDC_FTUSERIMPERSONATION_CHECK))
		SendMessage(GetDlgItem(hwnd, IDC_FTUSERIMPERSONATION_CHECK), BM_SETCHECK, settings->getFTUserImpersonation(), 0);

	if (GetDlgItem(hwnd, IDC_BLANK)) {
		SendMessage(GetDlgItem(hwnd, IDC_BLANK), BM_SETCHECK, settings->getEnableBlankMonitor(), 0);
		if (!VNC_OSVersion::getInstance()->OS_WIN10_TRANS && VNC_OSVersion::getInstance()->OS_WIN10)
			SetDlgItemText(hwnd, IDC_BLANK, "Enable Blank Monitor on Viewer Request require Min Win10 build 19041 ");
		if (VNC_OSVersion::getInstance()->OS_WIN8)
			SetDlgItemText(hwnd, IDC_BLANK, "Enable Blank Monitor on Viewer Not supported on windows 8 ");
	}

	if (GetDlgItem(hwnd, IDC_BLANK2)) //PGM
		SendMessage(GetDlgItem(hwnd, IDC_BLANK2), BM_SETCHECK, settings->getBlankInputsOnly(), 0); //PGM

	if (GetDlgItem(hwnd, IDC_ALLOWLOOPBACK)) {
		BOOL fLoopback = settings->getAllowLoopback();
		SendMessage(GetDlgItem(hwnd, IDC_ALLOWLOOPBACK), BM_SETCHECK, fLoopback, 0);
	}

	if (GetDlgItem(hwnd, IDC_IPV6)) {
		BOOL fIPV6 = settings->getIPV6();
		SendMessage(GetDlgItem(hwnd, IDC_IPV6), BM_SETCHECK, fIPV6, 0);
	}

	if (GetDlgItem(hwnd, IDC_LOOPBACKONLY)) {
		BOOL fLoopbackonly = settings->getLoopbackOnly();
		SendMessage(GetDlgItem(hwnd, IDC_LOOPBACKONLY), BM_SETCHECK, fLoopbackonly, 0);
	}

	if (GetDlgItem(hwnd, IDC_DISABLETRAY)) {
		BOOL fTrayicon = settings->getDisableTrayIcon();
		SendMessage(GetDlgItem(hwnd, IDC_DISABLETRAY), BM_SETCHECK, fTrayicon, 0);
	}

	if (GetDlgItem(hwnd, IDC_RDPMODE)) {
		BOOL frdpmode = settings->getRdpmode();
		SendMessage(GetDlgItem(hwnd, IDC_RDPMODE), BM_SETCHECK, frdpmode, 0);
	}

	if (GetDlgItem(hwnd, IDC_NOSCREENSAVER)) {
		BOOL fNoScrensaver = settings->getnoscreensaver();
		SendMessage(GetDlgItem(hwnd, IDC_NOSCREENSAVER), BM_SETCHECK, fNoScrensaver, 0);
	}

	if (GetDlgItem(hwnd, IDC_ALLOWSHUTDOWN))
		SendMessage(GetDlgItem(hwnd, IDC_ALLOWSHUTDOWN), BM_SETCHECK, !settings->getAllowShutdown(), 0);

	if (GetDlgItem(hwnd, IDC_ALLOWEDITCLIENTS))
		SendMessage(GetDlgItem(hwnd, IDC_ALLOWEDITCLIENTS), BM_SETCHECK, !settings->getAllowEditClients(), 0);

	if (vnclog.GetMode() >= 2)
		CheckDlgButton(hwnd, IDC_LOG, BST_CHECKED);
	else
		CheckDlgButton(hwnd, IDC_LOG, BST_UNCHECKED);

	if (GetDlgItem(hwnd, IDC_VIDEO)) {
#ifndef AVILOG
		ShowWindow(GetDlgItem(hwnd, IDC_CLEAR), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_VIDEO), SW_HIDE);
#endif

		if (vnclog.GetVideo()) {
			SetDlgItemText(hwnd, IDC_EDIT_PATH, vnclog.GetPath());
			CheckDlgButton(hwnd, IDC_VIDEO, BST_CHECKED);
		}
		else {
			SetDlgItemText(hwnd, IDC_EDIT_PATH, vnclog.GetPath());
			CheckDlgButton(hwnd, IDC_VIDEO, BST_UNCHECKED);
		}
	}

	if (GetDlgItem(hwnd, IDC_SAVEPASSWORDSECURE))
		SendMessage(GetDlgItem(hwnd, IDC_SAVEPASSWORDSECURE), BM_SETCHECK, settings->getSecure(), 0);

	if (GetDlgItem(hwnd, IDC_MSLOGON)) {
		HWND hMSLogon = GetDlgItem(hwnd, IDC_MSLOGON_CHECKD);
		SendMessage(hMSLogon, BM_SETCHECK, settings->getRequireMSLogon(), 0);

		HWND hNewMSLogon = GetDlgItem(hwnd, IDC_NEW_MSLOGON);
		SendMessage(hNewMSLogon, BM_SETCHECK, settings->getNewMSLogon(), 0);

		EnableWindow(GetDlgItem(hwnd, IDC_NEW_MSLOGON), settings->getRequireMSLogon());
		EnableWindow(GetDlgItem(hwnd, IDC_MSLOGON), settings->getRequireMSLogon());
	}


	if (GetDlgItem(hwnd, IDC_REVERSEAUTH))
		SendMessage(GetDlgItem(hwnd, IDC_REVERSEAUTH), BM_SETCHECK, settings->getReverseAuthRequired(), 0);

	SetDlgItemInt(hwnd, IDC_SCALE, settings->getDefaultScale(), false);

	// Remote input settings
	if (GetDlgItem(hwnd, IDC_DISABLE_INPUTS))
		SendMessage(GetDlgItem(hwnd, IDC_DISABLE_INPUTS), BM_SETCHECK, !(settings->getEnableRemoteInputs()), 0);

	// Local input settings
	if (GetDlgItem(hwnd, IDC_DISABLE_LOCAL_INPUTS))
		SendMessage(GetDlgItem(hwnd, IDC_DISABLE_LOCAL_INPUTS), BM_SETCHECK, settings->getDisableLocalInputs(), 0);

	// japanese keybaord
	if (GetDlgItem(hwnd, IDC_JAP_INPUTS))
		SendMessage(GetDlgItem(hwnd, IDC_JAP_INPUTS), BM_SETCHECK, settings->getEnableJapInput(), 0);

	if (GetDlgItem(hwnd, IDC_UNICODE_INPUTS))
		SendMessage(GetDlgItem(hwnd, IDC_UNICODE_INPUTS), BM_SETCHECK, settings->getEnableUnicodeInput(), 0);

	if (GetDlgItem(hwnd, IDC_WIN8_HELPER))
		SendMessage(GetDlgItem(hwnd, IDC_WIN8_HELPER), BM_SETCHECK, settings->getEnableWin8Helper(), 0);

	// Remove the wallpaper
	if (GetDlgItem(hwnd, IDC_REMOVE_WALLPAPER))
		SendMessage(GetDlgItem(hwnd, IDC_REMOVE_WALLPAPER), BM_SETCHECK, settings->getRemoveWallpaper(), 0);

	if (GetDlgItem(hwnd, IDC_LOCKSETTING_LOCK)) {
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
	}

	if (GetDlgItem(hwnd, IDC_RADIONOTIFICATIONIFPROVIDED)) {
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
		SendMessage(hNotificationSelection,BM_SETCHECK,TRUE,0);
	}

	if (GetDlgItem(hwnd, IDC_MV1)) {
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
	}

	if (GetDlgItem(hwnd, IDC_DREFUSE)) {
		HWND hQuerySetting{};
		int QueryAccept = 0;
		if (settings->getQuerySetting() == 2)
			QueryAccept = 0;
		else
			QueryAccept = settings->getQueryAccept();

		switch (QueryAccept) {
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
	}

	if (GetDlgItem(hwnd, IDC_MAXREFUSE)) {
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
	}

	if (GetDlgItem(hwnd, IDC_COLLABO))
		SendMessage(GetDlgItem(hwnd, IDC_COLLABO), BM_SETCHECK, settings->getCollabo(), 0);

	if (GetDlgItem(hwnd, IDC_FRAME))
		SendMessage(GetDlgItem(hwnd, IDC_FRAME), BM_SETCHECK, settings->getFrame(), 0);

	if (GetDlgItem(hwnd, IDC_NOTIFOCATION))
		SendMessage(GetDlgItem(hwnd, IDC_NOTIFOCATION), BM_SETCHECK, settings->getNotification(), 0);

	if (GetDlgItem(hwnd, IDC_OSD))
		SendMessage(GetDlgItem(hwnd, IDC_OSD), BM_SETCHECK, settings->getOSD(), 0);

	char maxviewersChar[128]{};
	UINT maxviewers = settings->getMaxViewers();
	sprintf_s(maxviewersChar, "%d", (int)maxviewers);
	SetDlgItemText(hwnd, IDC_MAXVIEWERS, (const char*)maxviewersChar);

	if (GetDlgItem(hwnd, IDC_PLUGINS_COMBO)) {
		HWND hPlugins = GetDlgItem(hwnd, IDC_PLUGINS_COMBO);
		int nPlugins = ListPlugins(hPlugins);
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
	}

	if (GetDlgItem(hwnd, IDC_PLUGIN_CHECK))
		SendMessage(GetDlgItem(hwnd, IDC_PLUGIN_CHECK), BM_SETCHECK, settings->getUseDSMPlugin(), 0);

	// Query window option - Taken from TightVNC advanced properties
	BOOL queryEnabled = (settings->getQuerySetting() == 4);

	if (GetDlgItem(hwnd, IDQUERY))
		SendMessage(GetDlgItem(hwnd, IDQUERY), BM_SETCHECK, queryEnabled, 0);
	if (GetDlgItem(hwnd, IDQUERYTIMEOUT)) {
		EnableWindow(GetDlgItem(hwnd, IDQUERYTIMEOUT), queryEnabled);
		char timeout[128]{};
		UINT t = settings->getQueryTimeout();
		sprintf_s(timeout, "%d", (int)t);
		SetDlgItemText(hwnd, IDQUERYTIMEOUT, (const char*)timeout);
	}
	if (GetDlgItem(hwnd, IDC_QUERYDISABLETIME)) {
		EnableWindow(GetDlgItem(hwnd, IDC_QUERYDISABLETIME), queryEnabled);
		char disableTime[128]{};
		UINT tt = settings->getQueryDisableTime();
		sprintf_s(disableTime, "%d", (int)tt);
		SetDlgItemText(hwnd, IDC_QUERYDISABLETIME, (const char*)disableTime);
	}
	if (GetDlgItem(hwnd, IDC_DREFUSE))
		EnableWindow(GetDlgItem(hwnd, IDC_DREFUSE), queryEnabled);
	if (GetDlgItem(hwnd, IDC_DRefuseOnly))
		EnableWindow(GetDlgItem(hwnd, IDC_DRefuseOnly), queryEnabled);
	if (GetDlgItem(hwnd, IDC_DACCEPT))
		EnableWindow(GetDlgItem(hwnd, IDC_DACCEPT), queryEnabled);
	if (GetDlgItem(hwnd, IDC_QNOLOGON))
		EnableWindow(GetDlgItem(hwnd, IDC_QNOLOGON), queryEnabled);

	if (GetDlgItem(hwnd, IDC_SERVICE_COMMANDLINE))
		SetDlgItemText(hwnd, IDC_SERVICE_COMMANDLINE, settings->getService_commandline());
	if (GetDlgItem(hwnd, IDC_EDITQUERYTEXT))
		SetDlgItemText(hwnd, IDC_EDITQUERYTEXT, settings->getAccept_reject_mesg());

	if (GetDlgItem(hwnd, IDC_SLIDERFPS)) {
		HWND slider = GetDlgItem(hwnd, IDC_SLIDERFPS);
			SendMessage(slider, TBM_SETRANGE, (WPARAM)1, (LPARAM)MAKELONG(5, 60));
			SendMessage(slider, TBM_SETTICFREQ, 1, 10);
			SendMessage(slider, TBM_SETPOS, true, settings->getMaxFPS());
			if (settings->getddEngine())
				EnableWindow(slider, settings->getDriver());
			else
				EnableWindow(slider, false);
	}

	if (GetDlgItem(hwnd, IDC_STATICFPS)) {
		CHAR temp[250];
		sprintf_s(temp, "%d", settings->getMaxFPS());
		SetDlgItemText(hwnd, IDC_STATICFPS, temp);
	}

	if (settings->getddEngine()) {
		if (GetDlgItem(hwnd, IDC_CHECKDRIVER))
			ShowWindow(GetDlgItem(hwnd, IDC_CHECKDRIVER), false);
		if (GetDlgItem(hwnd, IDC_STATICELEVATED))
			ShowWindow(GetDlgItem(hwnd, IDC_STATICELEVATED), false);
		if (GetDlgItem(hwnd, IDC_STATICFPS60))
			EnableWindow(GetDlgItem(hwnd, IDC_STATICFPS60), settings->getDriver());
		if (GetDlgItem(hwnd, IDC_STATICFPS5))
			EnableWindow(GetDlgItem(hwnd, IDC_STATICFPS5), settings->getDriver());
		if (GetDlgItem(hwnd, IDC_DRIVER))
			SetWindowText(GetDlgItem(hwnd, IDC_DRIVER), "DDEngine (restart on change required)");
	}
	else {
		if (GetDlgItem(hwnd, IDC_STATICFPS60))
			EnableWindow(GetDlgItem(hwnd, IDC_STATICFPS60), false);
		if (GetDlgItem(hwnd, IDC_STATICFPS5))
			EnableWindow(GetDlgItem(hwnd, IDC_STATICFPS5), false);
	}
	if (GetDlgItem(hwnd, IDC_MAXCPU))
		SetDlgItemInt(hwnd, IDC_MAXCPU, settings->getMaxCpu(), false);

	if (GetDlgItem(hwnd, IDC_TURBOMODE))
		SendMessage(GetDlgItem(hwnd, IDC_TURBOMODE), BM_SETCHECK, settings->getTurboMode(), 0);

	if (GetDlgItem(hwnd, IDC_DRIVER))
		SendMessage(GetDlgItem(hwnd, IDC_DRIVER), BM_SETCHECK, settings->getDriver(), 0);

	if (GetDlgItem(hwnd, IDC_HOOK))
		SendMessage(GetDlgItem(hwnd, IDC_HOOK), BM_SETCHECK, settings->getHook(), 0);

	if (GetDlgItem(hwnd, IDC_POLL_FULLSCREEN))
		SendMessage(GetDlgItem(hwnd, IDC_POLL_FULLSCREEN), BM_SETCHECK, settings->getPollFullScreen(), 0);

	if (GetDlgItem(hwnd, IDC_POLL_FOREGROUND))
		SendMessage(GetDlgItem(hwnd, IDC_POLL_FOREGROUND), BM_SETCHECK, settings->getPollForeground(), 0);

	if (GetDlgItem(hwnd, IDC_POLL_UNDER_CURSOR))
		SendMessage(GetDlgItem(hwnd, IDC_POLL_UNDER_CURSOR), BM_SETCHECK, settings->getPollUnderCursor(), 0);

	if (GetDlgItem(hwnd, IDC_CONSOLE_ONLY)) {
		SendMessage(GetDlgItem(hwnd, IDC_CONSOLE_ONLY), BM_SETCHECK, settings->getPollConsoleOnly(), 0);
		EnableWindow(GetDlgItem(hwnd, IDC_CONSOLE_ONLY), settings->getPollUnderCursor() || settings->getPollForeground());
	}

	if (GetDlgItem(hwnd, IDC_ONEVENT_ONLY)) {
		SendMessage(GetDlgItem(hwnd, IDC_ONEVENT_ONLY), BM_SETCHECK, settings->getPollOnEventOnly(), 0);
		EnableWindow(GetDlgItem(hwnd, IDC_ONEVENT_ONLY), settings->getPollUnderCursor() || settings->getPollForeground());
	}

	if (GetDlgItem(hwnd, IDC_AUTOCAPT1))
		CheckDlgButton(hwnd, IDC_AUTOCAPT1, (settings->getAutocapt() == 1) ? BST_CHECKED : BST_UNCHECKED);
	if (GetDlgItem(hwnd, IDC_AUTOCAPT2))
		CheckDlgButton(hwnd, IDC_AUTOCAPT2, (settings->getAutocapt() == 2) ? BST_CHECKED : BST_UNCHECKED);
	if (GetDlgItem(hwnd, IDC_AUTOCAPT3))
		CheckDlgButton(hwnd, IDC_AUTOCAPT3, (settings->getAutocapt() == 3) ? BST_CHECKED : BST_UNCHECKED);

	if (GetDlgItem(hwnd, IDC_IDLETIME))
		SetDlgItemInt(hwnd, IDC_IDLETIME, settings->getIdleTimeout(), FALSE);
	if (GetDlgItem(hwnd, IDC_IDLETIMEINPUT))
		SetDlgItemInt(hwnd, IDC_IDLETIMEINPUT, settings->getIdleInputTimeout(), FALSE);
	if (GetDlgItem(hwnd, IDC_KINTERVAL))
		SetDlgItemInt(hwnd, IDC_KINTERVAL, settings->getkeepAliveInterval(), FALSE);

	if (GetDlgItem(hwnd, IDC_AUTHREQUIRED))
		CheckDlgButton(hwnd, IDC_AUTHREQUIRED, (settings->getAuthRequired() == 1) ? BST_CHECKED : BST_UNCHECKED);
	EnableWindow(GetDlgItem(hwnd, IDC_CLEARPASSWORD), settings->getAuthRequired() == 0);
	EnableWindow(GetDlgItem(hwnd, IDC_CLEARPASSWORDVO), settings->getAuthRequired() == 0);

	if (GetDlgItem(hwnd, IDC_ALLOWUSERSSETTINGS)) {
		CheckDlgButton(hwnd, IDC_ALLOWUSERSSETTINGS, (settings->getAllowUserSettingsWithPassword() == 1) ? BST_CHECKED : BST_UNCHECKED);
	}

	if (GetDlgItem(hwnd, IDC_QNOLOGON))
		SendMessage(GetDlgItem(hwnd, IDC_QNOLOGON), BM_SETCHECK, settings->getQueryIfNoLogon(), 0);

	if (!settings->getLoopbackOnly() && GetDlgItem(hwnd, IDC_IP_ACCESS_CONTROL_LIST))
		rulesListView->init(GetDlgItem(hwnd, IDC_IP_ACCESS_CONTROL_LIST));

	if (GetDlgItem(hwnd, IDC_PRIM))
		SendMessage(GetDlgItem(hwnd, IDC_PRIM), BM_SETCHECK, settings->getPrimary(), 0);
	if (GetDlgItem(hwnd, IDC_SEC))
		SendMessage(GetDlgItem(hwnd, IDC_SEC), BM_SETCHECK, settings->getSecondary(), 0);

	SetForegroundWindow(hwnd);

	return FALSE; // Because we've set the focus
}

PropertiesDialog::PropertiesDialog()
{
	hTabControl = NULL;
	rulesListView = new RulesListView();
	m_dlgvisible = FALSE;
	m_server = NULL;
};

PropertiesDialog::~PropertiesDialog()
{
	delete rulesListView;
};

BOOL PropertiesDialog::Init(vncServer* server)
{
	// Save the server pointer
	m_server = server;
	UpdateServer();
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
					MessageBoxSecure(NULL, sz_ID_NO_PASSWD_NO_LOGON_WARN,
						sz_ID_WINVNC_ERROR,
						MB_OK | MB_ICONEXCLAMATION);
					ShowDialog();
				}
				else {
					ShowDialog();
				}
			}
		}

	return TRUE;
}

// Dialog box handling functions
void PropertiesDialog::ShowImpersonateDialog()
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

		int result = (int)DialogBoxParam(hInstResDLL, MAKEINTRESOURCE(IDD_PROPERTIESDIALOG), NULL, (DLGPROC)PropertiesDlgProc, (LONG_PTR)this);
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
				return;
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

		// Load in all the settings
		// If you run as service, you reload the saved settings before they are actual saved
		// via runas.....

		if (!settings->RunningFromExternalService())
			UpdateServer();

	}
	if (iImpersonateResult == ERROR_SUCCESS)
		RevertToSelf();
}

int PropertiesDialog::ShowDialog()
{
	return DialogBoxParam(hInstResDLL, DIALOG_MAKEINTRESOURCE(IDD_PROPERTIESDIALOG),
		NULL, (DLGPROC)PropertiesDlgProc, (LONG_PTR)this);
};




void PropertiesDialog::UpdateServer()
{
	if (!m_server)
		return;
#ifndef SC_20
	settings->load();
#endif // SC_20	
	/*if (!settings->getLoopbackOnly()) {
		if (settings->getAuthhosts() != 0) {
			m_server->SetAuthHosts(settings->getAuthhosts());
		}
		else {
			m_server->SetAuthHosts(0);
		}
	}
	else {
		m_server->SetAuthHosts(0);
	}*/

	m_server->SetNoScreensaver(settings->getnoscreensaver());
	m_server->SetLoopbackOnly(settings->getLoopbackOnly());
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

	bConnectSock = m_server->SockConnected();
}

int PropertiesDialog::ListPlugins(HWND hComboBox)
{
	// TODO: Log events
	WIN32_FIND_DATA fd;
	HANDLE ff;
	int fRet = 1;
	int nFiles = 0;
	char szCurrentDir[MAX_PATH];

	if (GetModuleFileName(NULL, szCurrentDir, MAX_PATH))
	{
		char* p = strrchr(szCurrentDir, '\\');
		if (p == NULL)
			return 0;
		*p = '\0';
	}
	else
		return 0;

	if (szCurrentDir[strlen(szCurrentDir) - 1] != '\\') strcat_s(szCurrentDir, "\\");
	strcat_s(szCurrentDir, "*.dsm"); // The DSMplugin dlls must have this extension

	ff = FindFirstFile(szCurrentDir, &fd);
	if (ff == INVALID_HANDLE_VALUE)
	{
		// Todo: Log error here
		return 0;
	}

	while (fRet != 0)
	{
		SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)(fd.cFileName));
		nFiles++;
		fRet = FindNextFile(ff, &fd);
	}

	FindClose(ff);

	return nFiles;
}

bool PropertiesDialog::onCommand( int command, HWND hwnd)
{
	switch (command) {
	case IDC_REMOVE_BUTTON:
		rulesListView->removeSelectedItem();
		break;
	case IDC_MOVE_DOWN_BUTTON:
		rulesListView->moveDown();
		break;
	case IDC_MOVE_UP_BUTTON:
		rulesListView->moveUp();
		break;
	case IDC_ADD_BUTTON:
		rulesListView->add();
		break;
	case IDC_EDIT_BUTTON:
		rulesListView->edit();
		break;
	case IDC_DRIVER:
		if (m_server)
		if (SendMessage(GetDlgItem(hwnd, IDC_DRIVER), BM_GETCHECK, 0, 0) == BST_CHECKED)
			m_server->Driver(CheckVideoDriver(0));
		else
			m_server->Driver(false);
		if (settings->getddEngine()) {
			EnableWindow(GetDlgItem(hwnd, IDC_SLIDERFPS), settings->getDriver());
			EnableWindow(GetDlgItem(hwnd, IDC_STATICFPS60), settings->getDriver());
			EnableWindow(GetDlgItem(hwnd, IDC_STATICFPS5), settings->getDriver());
		}
		break;
	case IDCANCEL:
		EndDialog(PropertiesDialogHwnd, IDCANCEL);
		m_dlgvisible = FALSE;
		return TRUE;
	case IDC_APPLY:
		onTabsAPPLY(hwnd);
		return TRUE;
	case IDOK:
		onTabsAPPLY(hwnd);
		onTabsOK(hwnd);
		return TRUE;
	case IDC_POLL_FOREGROUND:
	case IDC_POLL_UNDER_CURSOR: {
		BOOL enabled = (SendMessage(GetDlgItem(hwnd, IDC_POLL_FOREGROUND), BM_GETCHECK, 0, 0) == BST_CHECKED) ||
			(SendMessage(GetDlgItem(hwnd, IDC_POLL_UNDER_CURSOR), BM_GETCHECK, 0, 0) == BST_CHECKED);
		EnableWindow(GetDlgItem(hwnd, IDC_CONSOLE_ONLY), enabled);
		EnableWindow(GetDlgItem(hwnd, IDC_ONEVENT_ONLY), enabled);
		return TRUE; }
	case IDC_CHECKDRIVER:
		CheckVideoDriver(1);
		return TRUE;
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

	case IDC_CONNECT_HTTP:
		EnableWindow(GetDlgItem(hwnd, IDC_PORTHTTP),
			(SendDlgItemMessage(hwnd, IDC_CONNECT_HTTP,
				BM_GETCHECK, 0, 0) == BST_CHECKED));
		break;

	case IDC_CONNECT_SOCK:
		// TightVNC 1.2.7 method
		// The user has clicked on the socket connect tickbox
	{
		BOOL bConnectSock =
			(SendDlgItemMessage(hwnd, IDC_CONNECT_SOCK,
				BM_GETCHECK, 0, 0) == BST_CHECKED);
		BOOL bConnectHttp =
			(SendDlgItemMessage(hwnd, IDC_CONNECT_HTTP,
				BM_GETCHECK, 0, 0) == BST_CHECKED);

		HWND hPortNoAuto = GetDlgItem(hwnd, IDC_PORTNO_AUTO);
		EnableWindow(hPortNoAuto, bConnectSock);
		HWND hSpecPort = GetDlgItem(hwnd, IDC_SPECPORT);
		EnableWindow(hSpecPort, bConnectSock);
		EnableWindow(GetDlgItem(hwnd, IDC_PORTRFB), bConnectSock &&
			(SendMessage(hSpecPort, BM_GETCHECK, 0, 0) == BST_CHECKED));
		EnableWindow(GetDlgItem(hwnd, IDC_PORTHTTP), bConnectSock &&
			(SendMessage(hSpecPort, BM_GETCHECK, 0, 0) == BST_CHECKED) && bConnectHttp);
	}
	return TRUE;

	// TightVNC 1.2.7 method
	case IDC_PORTNO_AUTO:
	{
		EnableWindow(GetDlgItem(hwnd, IDC_PORTRFB), FALSE);
		EnableWindow(GetDlgItem(hwnd, IDC_PORTHTTP), FALSE);
		SetDlgItemText(hwnd, IDC_PORTRFB, "");
		SetDlgItemText(hwnd, IDC_PORTHTTP, "");
	}
	return TRUE;

	case IDC_SPECPORT:
		{
			BOOL bConnectHttp =
				(SendDlgItemMessage(hwnd, IDC_CONNECT_HTTP,
					BM_GETCHECK, 0, 0) == BST_CHECKED);

			EnableWindow(GetDlgItem(hwnd, IDC_PORTRFB), TRUE);
			EnableWindow(GetDlgItem(hwnd, IDC_PORTHTTP), bConnectHttp);

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
			EnableWindow(GetDlgItem(hwnd, IDC_DRefuseOnly), queryon);
			EnableWindow(GetDlgItem(hwnd, IDC_DACCEPT), queryon);
			EnableWindow(GetDlgItem(hwnd, IDC_QNOLOGON), queryon);
		}
		return TRUE;

	case IDC_STARTREP:
		if (m_server) {
			auto newconn = std::make_unique<vncConnDialog>(m_server);
			if (newconn)
				newconn->DoDialog(true);
		}
		return TRUE;

	// sf@2002 - DSM Plugin
	case IDC_PLUGIN_CHECK:
		if (m_server)
			EnableWindow(GetDlgItem(hwnd, IDC_PLUGIN_BUTTON), m_server->AuthClientCount() == 0
				? SendMessage(GetDlgItem(hwnd, IDC_PLUGIN_CHECK), BM_GETCHECK, 0, 0) == BST_CHECKED
				: BST_UNCHECKED);
		return TRUE;

	case IDC_MSLOGON_CHECKD:
	{
		BOOL bMSLogonChecked =
			(SendDlgItemMessage(hwnd, IDC_MSLOGON_CHECKD,
				BM_GETCHECK, 0, 0) == BST_CHECKED);

		EnableWindow(GetDlgItem(hwnd, IDC_NEW_MSLOGON), bMSLogonChecked);
		EnableWindow(GetDlgItem(hwnd, IDC_MSLOGON), bMSLogonChecked);
	}
	case IDC_AUTHREQUIRED:{
		bool checked = (SendMessage(GetDlgItem(hwnd, IDC_AUTHREQUIRED), BM_GETCHECK, 0, 0) == BST_CHECKED);
		EnableWindow(GetDlgItem(hwnd, IDC_CLEARPASSWORD), checked == 0);
		EnableWindow(GetDlgItem(hwnd, IDC_CLEARPASSWORDVO), checked == 0);
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
			if (m_server) {
				m_vncauth.Init(m_server);
				m_vncauth.Show(TRUE);
			}
		}
	}
	return TRUE;
#endif // SC_20
	case IDC_CHANGEPASSWORD:
	{
		DlgChangePassword* dlgChangePassword = new DlgChangePassword();
		if (dlgChangePassword->ShowDlg(NULL, "Change/Set password", 8)) {
			char password[1024];
			strcpy_s(password, dlgChangePassword->getPassword());
			if (strlen(password) == 0) {
				settings->setPasswd(password);
				settings->savePassword();
			}
			else {
				vncPasswd::FromText crypt(password, settings->getSecure());
				settings->setPasswd(crypt);
				settings->savePassword();
			}
		}
		return true;
	}
	case IDC_CHANGEPASSWORDVO:
	{
		DlgChangePassword* dlgChangePassword = new DlgChangePassword();
		if (dlgChangePassword->ShowDlg(NULL, "Change/Set View-only password", 8)) {
			char password[1024];
			strcpy_s(password, dlgChangePassword->getPassword());
			if (strlen(password) == 0) {
				settings->setPasswdViewOnly(password);
				settings->saveViewOnlyPassword();
			}
			else {
				vncPasswd::FromText crypt2(password, settings->getSecure());
				settings->setPasswdViewOnly(crypt2);
				settings->saveViewOnlyPassword();
			}
		}
		return true;
	}
	case IDC_CHANGEPASSWORDADMIN: {
		DlgChangePassword* dlgChangePassword = new DlgChangePassword();
		if (dlgChangePassword->ShowDlg(NULL, "Change/Set Admin password", 128)) {
			char password[1024];
			strcpy_s(password, dlgChangePassword->getPassword());
			settings->setAdminPasswordHash(password);
		}
	}
		return true;

	case IDC_CLEARPASSWORD:
	{
		settings->setPasswd("");
		settings->savePassword();
		return true;
	}
	case IDC_CLEARPASSWORDVO:
	{
		settings->setPasswdViewOnly("");
		settings->saveViewOnlyPassword();
		return true;
	}
	case IDC_PLUGIN_BUTTON:
	{
		if (m_server) {
			HWND hPlugin = GetDlgItem(hwnd, IDC_PLUGIN_CHECK);
			if (SendMessage(hPlugin, BM_GETCHECK, 0, 0) == BST_CHECKED) {
				TCHAR szPlugin[MAX_PATH];
				GetDlgItemText(hwnd, IDC_PLUGINS_COMBO, szPlugin, MAX_PATH);
				PathStripPathA(szPlugin);

				if (!m_server->GetDSMPluginPointer()->IsLoaded())
					m_server->GetDSMPluginPointer()->LoadPlugin(szPlugin, false);
				else {
					// sf@2003 - We check if the loaded plugin is the same than
					// the currently selected one or not
					m_server->GetDSMPluginPointer()->DescribePlugin();
					if (_stricmp(m_server->GetDSMPluginPointer()->GetPluginFileName(), szPlugin)) {
						m_server->GetDSMPluginPointer()->UnloadPlugin();
						m_server->GetDSMPluginPointer()->LoadPlugin(szPlugin, false);
					}
				}

				if (m_server->GetDSMPluginPointer()->IsLoaded())
					Secure_Save_Plugin_Config(szPlugin);
				else
					MessageBoxSecure(NULL, sz_ID_PLUGIN_NOT_LOAD, sz_ID_PLUGIN_LOADIN, MB_OK | MB_ICONEXCLAMATION);
			}
		}
		return TRUE;
	}

	case IDC_CHECKIP: 
		{
		char oldAuthHost[1280];
		strcpy_s(oldAuthHost, settings->getAuthhosts());
		settings->setAuthhosts(rulesListView->getAuthHost());
		char tempchar[25];
		GetDlgItemText(hwnd, IDC_IP_FOR_CHECK_EDIT, tempchar, 25);
		if (m_server) {
			vncServer::AcceptQueryReject status = m_server->VerifyHost(tempchar);
			switch (status) {
			case vncServer::aqrAccept:
				SetDlgItemText(hwnd, IDC_IP_CHECK_RESULT_LABEL, "Accept");
				break;
			case vncServer::aqrQuery:
				SetDlgItemText(hwnd, IDC_IP_CHECK_RESULT_LABEL, "Query");
				break;
			case vncServer::aqrReject:
				SetDlgItemText(hwnd, IDC_IP_CHECK_RESULT_LABEL, "Reject");
				break;
			}
		settings->setAuthhosts(oldAuthHost);
		}
		break; 
		}
	default:
		break;
	}
	return TRUE;
}
void PropertiesDialog::Secure_Plugin(char* szPlugin)
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

void PropertiesDialog::onTabsAPPLY(HWND hwnd)
{
	if (GetDlgItem(hwnd, IDC_PRIM))
		settings->setPrimary(SendMessage(GetDlgItem(hwnd, IDC_PRIM), BM_GETCHECK, 0, 0) == BST_CHECKED);
	if (GetDlgItem(hwnd, IDC_SEC))
		settings->setSecondary(SendMessage(GetDlgItem(hwnd, IDC_SEC), BM_GETCHECK, 0, 0) == BST_CHECKED);

	if (GetDlgItem(hwnd, IDC_MAXCPU)) {
		int maxcpu = GetDlgItemInt(hwnd, IDC_MAXCPU, NULL, FALSE);
		settings->setMaxCpu(maxcpu);
	}

	if (GetDlgItem(hwnd, IDC_SLIDERFPS)) {
		int pos = SendMessage(GetDlgItem(hwnd, IDC_SLIDERFPS), TBM_GETPOS, 0, 0L);
		settings->setMaxFPS(pos);
	}

	if (GetDlgItem(hwnd, IDC_AUTOCAPT1)) {
		HWND hcapt = GetDlgItem(hwnd, IDC_AUTOCAPT1);
		if (SendMessage(hcapt, BM_GETCHECK, 0, 0) == BST_CHECKED)
			settings->setAutocapt(1);
		settings->setAutocapt(settings->getAutocapt());
	}

	if (GetDlgItem(hwnd, IDC_AUTOCAPT2)) {
		HWND hcapt = GetDlgItem(hwnd, IDC_AUTOCAPT2);
		if (SendMessage(hcapt, BM_GETCHECK, 0, 0) == BST_CHECKED)
			settings->setAutocapt(2);
		settings->setAutocapt(settings->getAutocapt());
	}
	if (GetDlgItem(hwnd, IDC_AUTOCAPT3)) {
		HWND hcapt = GetDlgItem(hwnd, IDC_AUTOCAPT3);
		if (SendMessage(hcapt, BM_GETCHECK, 0, 0) == BST_CHECKED)
			settings->setAutocapt(3);
		settings->setAutocapt(settings->getAutocapt());
	}


	if (GetDlgItem(hwnd, IDC_TURBOMODE)) {
		HWND hTurboMode = GetDlgItem(hwnd, IDC_TURBOMODE);
		settings->setTurboMode(SendMessage(hTurboMode, BM_GETCHECK, 0, 0) == BST_CHECKED);
	}

	if (GetDlgItem(hwnd, IDC_DRIVER)) {
		HWND hDriver = GetDlgItem(hwnd, IDC_DRIVER);
		bool result = (SendMessage(hDriver, BM_GETCHECK, 0, 0) == BST_CHECKED);
		if (result)
		{
			if (m_server)
				m_server->Driver(CheckVideoDriver(0));
			settings->setDriver(CheckVideoDriver(0));
		}
		else {
			if (m_server)
				m_server->Driver(false);
			settings->setDriver(false);
		}
	}

	if (GetDlgItem(hwnd, IDC_HOOK)) {
		HWND hHook = GetDlgItem(hwnd, IDC_HOOK);
		if (m_server)
			m_server->Hook(SendMessage(hHook, BM_GETCHECK, 0, 0) == BST_CHECKED);
		settings->setHook(SendMessage(hHook, BM_GETCHECK, 0, 0) == BST_CHECKED);
	}
	if (m_server)
		m_server->SetHookings();

	if (GetDlgItem(hwnd, IDC_POLL_FULLSCREEN)) {
		HWND hPollFullScreen = GetDlgItem(hwnd, IDC_POLL_FULLSCREEN);
		settings->setPollFullScreen(SendMessage(hPollFullScreen, BM_GETCHECK, 0, 0) == BST_CHECKED);
	}

	if (GetDlgItem(hwnd, IDC_POLL_FOREGROUND)) {
		HWND hPollForeground = GetDlgItem(hwnd, IDC_POLL_FOREGROUND);
		settings->setPollForeground(
			SendMessage(hPollForeground, BM_GETCHECK, 0, 0) == BST_CHECKED);
	}

	if (GetDlgItem(hwnd, IDC_POLL_UNDER_CURSOR)) {
		HWND hPollUnderCursor = GetDlgItem(hwnd, IDC_POLL_UNDER_CURSOR);
		settings->setPollUnderCursor(
			SendMessage(hPollUnderCursor, BM_GETCHECK, 0, 0) == BST_CHECKED);
	}

	if (GetDlgItem(hwnd, IDC_CONSOLE_ONLY)) {
		HWND hPollConsoleOnly = GetDlgItem(hwnd, IDC_CONSOLE_ONLY);
		settings->setPollConsoleOnly(
			SendMessage(hPollConsoleOnly, BM_GETCHECK, 0, 0) == BST_CHECKED);
	}

	if (GetDlgItem(hwnd, IDC_ONEVENT_ONLY)) {
		HWND hPollOnEventOnly = GetDlgItem(hwnd, IDC_ONEVENT_ONLY);
		settings->setPollOnEventOnly(
			SendMessage(hPollOnEventOnly, BM_GETCHECK, 0, 0) == BST_CHECKED);
	}

	if (GetDlgItem(hwnd, IDC_EDIT_PATH)) {
		char path[512];
		int lenpath = GetDlgItemText(hwnd, IDC_EDIT_PATH, (LPSTR)&path, 512);
		if (lenpath != 0)
			vnclog.SetPath(path);
		else {
			strcpy_s(path, "");
			vnclog.SetPath(path);
		}
		settings->setDebugPath(path);
	}

	if (GetDlgItem(hwnd, IDC_SAVEPASSWORDSECURE)) {
		bool Secure_old = settings->getSecure();
		HWND hSecure = GetDlgItem(hwnd, IDC_SAVEPASSWORDSECURE);
		settings->setSecure(SendMessage(hSecure, BM_GETCHECK, 0, 0) == BST_CHECKED);
		bool bSecure = settings->getSecure();
		if (Secure_old != bSecure) {
			//We changed the method to save the password
			//load passwords and encrypt the other method
			vncPasswd::ToText plain(settings->getPasswd(), Secure_old);
			vncPasswd::ToText plainViewOnly(settings->getPasswdViewOnly(), Secure_old);
			char passwd[MAXPWLEN + 1];
			char passwdViewOnly[MAXPWLEN + 1];
			memset(passwd, '\0', MAXPWLEN + 1); //PGM
			memset(passwdViewOnly, '\0', MAXPWLEN + 1); //PGM
			strcpy_s(passwd, plain);
			strcpy_s(passwdViewOnly, plainViewOnly);
			int lenPassword = (int)strlen(passwd);
			int lenPasswordViewOnly = (int)strlen(passwdViewOnly);
			if (lenPassword != 0) {
				vncPasswd::FromText crypt(passwd, settings->getSecure());
				settings->setPasswd(crypt);
			}
			if (lenPasswordViewOnly != 0) {
				vncPasswd::FromText crypt2(passwdViewOnly, settings->getSecure());
				settings->setPasswdViewOnly(crypt2);
			}
		}
	}

	if (GetDlgItem(hwnd, IDC_PORTNO_AUTO)) {

		// Save the new settings to the server
		int state = (int)SendDlgItemMessage(hwnd, IDC_PORTNO_AUTO, BM_GETCHECK, 0, 0);
		if (m_server)
			m_server->SetAutoPortSelect(state == BST_CHECKED);
		settings->setAutoPortSelect(state == BST_CHECKED);
	}

	if (GetDlgItem(hwnd, IDC_PORTHTTP) && GetDlgItem(hwnd, IDC_PORTRFB)) {

		// Save port numbers if we're not auto selecting
		if (!settings->getAutoPortSelect()) {
			// Assuming that port numbers were specified
			BOOL ok1{}, ok2{};
			UINT port_rfb = GetDlgItemInt(hwnd, IDC_PORTRFB, &ok1, TRUE);
			UINT port_http = GetDlgItemInt(hwnd, IDC_PORTHTTP, &ok2, TRUE);
			if (ok1 && ok2) {
				if (m_server)
					m_server->SetPorts(port_rfb, port_http);
				settings->setPortNumber(port_rfb);
				settings->setHttpPortNumber(port_http);
			}
		}
	}

	if (GetDlgItem(hwnd, IDC_CONNECT_SOCK)) {
		HWND hConnectSock = GetDlgItem(hwnd, IDC_CONNECT_SOCK);
		if (m_server)
			m_server->EnableConnections(SendMessage(hConnectSock, BM_GETCHECK, 0, 0) == BST_CHECKED);
		settings->setEnableConnections(SendMessage(hConnectSock, BM_GETCHECK, 0, 0) == BST_CHECKED);
	}

	if (m_server)
		InitPortSettings(hwnd);

	if (GetDlgItem(hwnd, IDC_PORTNO_AUTO)) {
		HWND hConnectHTTP = GetDlgItem(hwnd, IDC_CONNECT_HTTP);
		if (m_server)
			m_server->EnableHTTPConnect(SendMessage(hConnectHTTP, BM_GETCHECK, 0, 0) == BST_CHECKED);
		settings->setHTTPConnect(SendMessage(hConnectHTTP, BM_GETCHECK, 0, 0) == BST_CHECKED);
	}

	if (GetDlgItem(hwnd, IDC_DISABLE_INPUTS)) {
		HWND hEnableRemoteInputs = GetDlgItem(hwnd, IDC_DISABLE_INPUTS);
		if (m_server)
			m_server->EnableRemoteInputs(SendMessage(hEnableRemoteInputs, BM_GETCHECK, 0, 0) != BST_CHECKED);
		settings->setEnableRemoteInputs(SendMessage(hEnableRemoteInputs, BM_GETCHECK, 0, 0) != BST_CHECKED);
	}

	if (GetDlgItem(hwnd, IDC_DISABLE_LOCAL_INPUTS)) {
		HWND hDisableLocalInputs = GetDlgItem(hwnd, IDC_DISABLE_LOCAL_INPUTS);
		settings->setDisableLocalInputs(SendMessage(hDisableLocalInputs, BM_GETCHECK, 0, 0) == BST_CHECKED);
	}

	if (GetDlgItem(hwnd, IDC_JAP_INPUTS)) {
		HWND hJapInputs = GetDlgItem(hwnd, IDC_JAP_INPUTS);
		if (m_server)
			m_server->EnableJapInput(SendMessage(hJapInputs, BM_GETCHECK, 0, 0) == BST_CHECKED);
		settings->setEnableJapInput(SendMessage(hJapInputs, BM_GETCHECK, 0, 0) == BST_CHECKED);
	}

	if (GetDlgItem(hwnd, IDC_UNICODE_INPUTS)) {
		HWND hUnicodeInputs = GetDlgItem(hwnd, IDC_UNICODE_INPUTS);
		if (m_server)
			m_server->EnableUnicodeInput(SendMessage(hUnicodeInputs, BM_GETCHECK, 0, 0) == BST_CHECKED);
		settings->setEnableUnicodeInput(SendMessage(hUnicodeInputs, BM_GETCHECK, 0, 0) == BST_CHECKED);
	}

	if (GetDlgItem(hwnd, IDC_WIN8_HELPER)) {
		HWND hwinhelper = GetDlgItem(hwnd, IDC_WIN8_HELPER);
		settings->setEnableWin8Helper(SendMessage(hwinhelper, BM_GETCHECK, 0, 0) == BST_CHECKED);
	}

	if (GetDlgItem(hwnd, IDC_REMOVE_WALLPAPER)) {
		HWND hRemoveWallpaper = GetDlgItem(hwnd, IDC_REMOVE_WALLPAPER);
		settings->setRemoveWallpaper(SendMessage(hRemoveWallpaper, BM_GETCHECK, 0, 0) == BST_CHECKED);
	}

	if (GetDlgItem(hwnd, IDC_LOCKSETTING_LOCK) && GetDlgItem(hwnd, IDC_LOCKSETTING_LOGOFF)) {
		if (SendMessage(GetDlgItem(hwnd, IDC_LOCKSETTING_LOCK), BM_GETCHECK, 0, 0) == BST_CHECKED) {
			settings->setLockSettings(1);
		}
		else if (SendMessage(GetDlgItem(hwnd, IDC_LOCKSETTING_LOGOFF), BM_GETCHECK, 0, 0) == BST_CHECKED) {
			settings->setLockSettings(2);
		}
		else {
			settings->setLockSettings(0);
		}
	}

	if (GetDlgItem(hwnd, IDC_RADIONOTIFICATIONON) && GetDlgItem(hwnd, IDC_RADIONOTIFICATIONIFPROVIDED)) {
		if (SendMessage(GetDlgItem(hwnd, IDC_RADIONOTIFICATIONON), BM_GETCHECK, 0, 0) == BST_CHECKED) {
			settings->setNotificationSelection(0);
		}
		else if (SendMessage(GetDlgItem(hwnd, IDC_RADIONOTIFICATIONIFPROVIDED), BM_GETCHECK, 0, 0) == BST_CHECKED) {
			settings->setNotificationSelection(1);
		}
	}

	if (GetDlgItem(hwnd, IDC_DREFUSE) && GetDlgItem(hwnd, IDC_DACCEPT)) {
		if (SendMessage(GetDlgItem(hwnd, IDC_DREFUSE), BM_GETCHECK, 0, 0) == BST_CHECKED) {
			settings->setQueryAccept(0);
		}
		else if (SendMessage(GetDlgItem(hwnd, IDC_DACCEPT), BM_GETCHECK, 0, 0) == BST_CHECKED) {
			settings->setQueryAccept(1);
		}
		else if (SendMessage(GetDlgItem(hwnd, IDC_DRefuseOnly), BM_GETCHECK, 0, 0)
			== BST_CHECKED) {
			settings->setQueryAccept(2);
		}
	}
	if (GetDlgItem(hwnd, IDC_QNOLOGON))
		settings->setQueryIfNoLogon(SendDlgItemMessage(hwnd, IDC_QNOLOGON, BM_GETCHECK, 0, 0));

	if (GetDlgItem(hwnd, IDC_MAXREFUSE) && GetDlgItem(hwnd, IDC_MAXDISCONNECT)) {
		if (SendMessage(GetDlgItem(hwnd, IDC_MAXREFUSE), BM_GETCHECK, 0, 0) == BST_CHECKED) {
			settings->setMaxViewerSetting(0);
		}
		else if (SendMessage(GetDlgItem(hwnd, IDC_MAXDISCONNECT), BM_GETCHECK, 0, 0) == BST_CHECKED) {
			settings->setMaxViewerSetting(1);
		}
	}

	if (GetDlgItem(hwnd, IDC_IDLETIME)) {
		char timeout[256];
		GetDlgItemText(hwnd, IDC_IDLETIME, (LPSTR)&timeout, 600);
		settings->setIdleTimeout(atoi(timeout));
	}
	if (GetDlgItem(hwnd, IDC_IDLETIMEINPUT)) {
		char timeout[256];
		GetDlgItemText(hwnd, IDC_IDLETIMEINPUT, (LPSTR)&timeout, 600);
		settings->setIdleInputTimeout(atoi(timeout));
	}
	if (GetDlgItem(hwnd, IDC_KINTERVAL)) {
		char timeout[256];
		GetDlgItemText(hwnd, IDC_KINTERVAL, (LPSTR)&timeout, 600);
		settings->setkeepAliveInterval(atoi(timeout));
	}

	if (GetDlgItem(hwnd, IDC_MAXVIEWERS)) {
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
	}

	if (GetDlgItem(hwnd, IDC_COLLABO)) {
		HWND hCollabo = GetDlgItem(hwnd, IDC_COLLABO);
		settings->setCollabo(SendMessage(hCollabo, BM_GETCHECK, 0, 0) == BST_CHECKED);
	}

	if (GetDlgItem(hwnd, IDC_FRAME)) {
		HWND hwndDlg = GetDlgItem(hwnd, IDC_FRAME);
		settings->setFrame(SendMessage(hwndDlg, BM_GETCHECK, 0, 0) == BST_CHECKED);
	}

	if (GetDlgItem(hwnd, IDC_NOTIFOCATION)) {
		HWND hwndDlg = GetDlgItem(hwnd, IDC_NOTIFOCATION);
		settings->setNotification(SendMessage(hwndDlg, BM_GETCHECK, 0, 0) == BST_CHECKED);
	}

	if (GetDlgItem(hwnd, IDC_OSD)) {
		HWND hwndDlg = GetDlgItem(hwnd, IDC_OSD);
		settings->setOSD(SendMessage(hwndDlg, BM_GETCHECK, 0, 0) == BST_CHECKED);
	}

	if (GetDlgItem(hwnd, IDC_MV1)) {
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
	}

	if (GetDlgItem(hwnd, IDC_FILETRANSFER)) {
		HWND hFileTransfer = GetDlgItem(hwnd, IDC_FILETRANSFER);
		settings->setEnableFileTransfer(SendMessage(hFileTransfer, BM_GETCHECK, 0, 0) == BST_CHECKED);
	}

	if (GetDlgItem(hwnd, IDC_FTUSERIMPERSONATION_CHECK)) {
		HWND hFileTransferUserImp = GetDlgItem(hwnd, IDC_FTUSERIMPERSONATION_CHECK);
		settings->setFTUserImpersonation(SendMessage(hFileTransferUserImp, BM_GETCHECK, 0, 0) == BST_CHECKED);
	}

	if (GetDlgItem(hwnd, IDC_BLANK)) {
		HWND hBlank = GetDlgItem(hwnd, IDC_BLANK);
		settings->setEnableBlankMonitor(SendMessage(hBlank, BM_GETCHECK, 0, 0) == BST_CHECKED);
	}

	if (GetDlgItem(hwnd, IDC_BLANK2)) {
		HWND hBlank2 = GetDlgItem(hwnd, IDC_BLANK2); //PGM
		settings->setBlankInputsOnly(SendMessage(hBlank2, BM_GETCHECK, 0, 0) == BST_CHECKED);
	}

	if (GetDlgItem(hwnd, IDC_ALLOWLOOPBACK))
		settings->setAllowLoopback(IsDlgButtonChecked(hwnd, IDC_ALLOWLOOPBACK));

	if (GetDlgItem(hwnd, IDC_IPV6))
		settings->setIPV6(IsDlgButtonChecked(hwnd, IDC_IPV6));

	if (GetDlgItem(hwnd, IDC_LOOPBACKONLY)) {
		if (m_server)
			m_server->SetLoopbackOnly(IsDlgButtonChecked(hwnd, IDC_LOOPBACKONLY));
		settings->setLoopbackOnly(IsDlgButtonChecked(hwnd, IDC_LOOPBACKONLY));
	}

	if (GetDlgItem(hwnd, IDC_DISABLETRAY))
		settings->setDisableTrayIcon(IsDlgButtonChecked(hwnd, IDC_DISABLETRAY));

	if (GetDlgItem(hwnd, IDC_RDPMODE))
		settings->setRdpmode(IsDlgButtonChecked(hwnd, IDC_RDPMODE));

	if (GetDlgItem(hwnd, IDC_NOSCREENSAVER)) {
		settings->setNoScreensaver(IsDlgButtonChecked(hwnd, IDC_NOSCREENSAVER));
		if (m_server)
			m_server->SetNoScreensaver(IsDlgButtonChecked(hwnd, IDC_NOSCREENSAVER));
	}

	if (GetDlgItem(hwnd, IDC_ALLOWSHUTDOWN))
		settings->setAllowShutdown(!IsDlgButtonChecked(hwnd, IDC_ALLOWSHUTDOWN));

	if (GetDlgItem(hwnd, IDC_ALLOWEDITCLIENTS))
		settings->setAllowEditClients(!IsDlgButtonChecked(hwnd, IDC_ALLOWEDITCLIENTS));

	if (GetDlgItem(hwnd, IDC_LOG)) {
		if (IsDlgButtonChecked(hwnd, IDC_LOG)) {
			vnclog.SetMode(2);
			vnclog.SetLevel(10);
		}
		else
			vnclog.SetMode(0);
	}

	if (GetDlgItem(hwnd, IDC_VIDEO)) {
		if (IsDlgButtonChecked(hwnd, IDC_VIDEO))
			vnclog.SetVideo(true);
		else
			vnclog.SetVideo(false);
	}

	if (GetDlgItem(hwnd, IDC_MSLOGON_CHECKD)) {
		HWND hMSLogon = GetDlgItem(hwnd, IDC_MSLOGON_CHECKD);
		settings->setRequireMSLogon(SendMessage(hMSLogon, BM_GETCHECK, 0, 0) == BST_CHECKED);
	}
	if (GetDlgItem(hwnd, IDC_NEW_MSLOGON)) {
		HWND hNewMSLogon = GetDlgItem(hwnd, IDC_NEW_MSLOGON);
		settings->setNewMSLogon(SendMessage(hNewMSLogon, BM_GETCHECK, 0, 0) == BST_CHECKED);
	}

	if (GetDlgItem(hwnd, IDC_REVERSEAUTH)) {
		HWND hReverseAuth = GetDlgItem(hwnd, IDC_REVERSEAUTH);
		settings->setReverseAuthRequired(SendMessage(hReverseAuth, BM_GETCHECK, 0, 0) == BST_CHECKED);
	}

	if (GetDlgItem(hwnd, IDC_SCALE)) {
		int nDScale = GetDlgItemInt(hwnd, IDC_SCALE, NULL, FALSE);
		if (nDScale < 1 || nDScale > 9) nDScale = 1;
		settings->setDefaultScale(nDScale);
	}

	if (GetDlgItem(hwnd, IDC_PLUGIN_CHECK)) {
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
			if (m_server && m_server->GetDSMPluginPointer()->IsLoaded())
			{
				m_server->GetDSMPluginPointer()->UnloadPlugin();
				m_server->GetDSMPluginPointer()->SetEnabled(false);
			}
		}
	}

	if (GetDlgItem(hwnd, IDQUERYTIMEOUT)) {
		char timeout[256];
		strcpy_s(timeout, "5");
		if (GetDlgItemText(hwnd, IDQUERYTIMEOUT, (LPSTR)&timeout, 256) == 0)
			settings->setQueryTimeout(atoi(timeout));
		else
			settings->setQueryTimeout(atoi(timeout));
	}

	if (GetDlgItem(hwnd, IDC_QUERYDISABLETIME)) {
		char disabletime[256];
		strcpy_s(disabletime, "5");
		if (GetDlgItemText(hwnd, IDC_QUERYDISABLETIME, (LPSTR)&disabletime, 256) == 0)
			settings->setQueryDisableTime(atoi(disabletime));
		else
			settings->setQueryDisableTime(atoi(disabletime));
	}

	if (GetDlgItem(hwnd, IDC_SERVICE_COMMANDLINE) && GetDlgItem(hwnd, IDC_EDITQUERYTEXT)) {
		char temp[1024]{};
		char temp2[512]{};
		GetDlgItemText(hwnd, IDC_SERVICE_COMMANDLINE, temp, 1024);
		GetDlgItemText(hwnd, IDC_EDITQUERYTEXT, temp2, 512);
		settings->setService_commandline(temp);
		settings->setAccept_reject_mesg(temp2);
	}

	if (GetDlgItem(hwnd, IDQUERY)) {
		HWND hQuery = GetDlgItem(hwnd, IDQUERY);
		settings->setQuerySetting((SendMessage(hQuery, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 4 : 2);
	}

	if (GetDlgItem(hwnd, IDC_AUTHREQUIRED))
		settings->setAuthRequired(SendMessage(GetDlgItem(hwnd, IDC_AUTHREQUIRED), BM_GETCHECK, 0, 0) == BST_CHECKED);

	settings->setAuthhosts(rulesListView->getAuthHost());

	if (GetDlgItem(hwnd, IDC_ALLOWUSERSSETTINGS)) {
		settings->setAllowUserSettingsWithPassword(SendMessage(GetDlgItem(hwnd, IDC_ALLOWUSERSSETTINGS), BM_GETCHECK, 0, 0) == BST_CHECKED);
	}
}
void PropertiesDialog::onTabsOK(HWND hwnd)
{
	EndDialog(hwnd, IDOK);
	m_dlgvisible = FALSE;
}

void PropertiesDialog::InitPortSettings(HWND hwnd)
{
	BOOL bConnectSock = m_server ? m_server->SockConnected() : false;
	BOOL bConnectHttp = settings->getHTTPConnect();
	BOOL bAutoPort = settings->getAutoPortSelect();
	UINT port_rfb = settings->getPortNumber();
	UINT port_http = settings->getHttpPortNumber();
	int d1 = PORT_TO_DISPLAY(port_rfb);
	int d2 = HPORT_TO_DISPLAY(port_http);
	BOOL bValidDisplay = (d1 == d2 && d1 >= 0 && d1 <= 99);

	if (GetDlgItem(hwnd, IDC_PORTNO_AUTO)) {
		CheckDlgButton(hwnd, IDC_PORTNO_AUTO,
			(bAutoPort) ? BST_CHECKED : BST_UNCHECKED);
		EnableWindow(GetDlgItem(hwnd, IDC_PORTNO_AUTO), bConnectSock);
	}

	if (GetDlgItem(hwnd, IDC_PORTRFB)) {
		SetDlgItemInt(hwnd, IDC_PORTRFB, port_rfb, FALSE);
		EnableWindow(GetDlgItem(hwnd, IDC_PORTRFB), bConnectSock && !bAutoPort && !bValidDisplay);
	}

	if (GetDlgItem(hwnd, IDC_PORTHTTP)) {
		SetDlgItemInt(hwnd, IDC_PORTHTTP, port_http, FALSE);
		EnableWindow(GetDlgItem(hwnd, IDC_PORTHTTP), bConnectSock && !bAutoPort && !bValidDisplay && bConnectHttp);
	}
}

void PropertiesDialog::onApply(HWND hwnd)
{
	SendMessage(hTabAuthentication, WM_COMMAND, IDC_APPLY, 0);
	SendMessage(hTabIncoming, WM_COMMAND, IDC_APPLY, 0);
	SendMessage(hTabInput, WM_COMMAND, IDC_APPLY, 0);
	SendMessage(hTabMisc, WM_COMMAND, IDC_APPLY, 0);
	SendMessage(hTabNotifications, WM_COMMAND, IDC_APPLY, 0);
	SendMessage(hTabReverse, WM_COMMAND, IDC_APPLY, 0);
	SendMessage(hTabRules, WM_COMMAND, IDC_APPLY, 0);
	SendMessage(hTabCapture, WM_COMMAND, IDC_APPLY, 0);
	SendMessage(hTabAdministration, WM_COMMAND, IDC_APPLY, 0);
}
void PropertiesDialog::onOK(HWND hwnd)
	{
#ifndef SC_20
	settings->save();
#endif // SC_20	
	SendMessage(hTabAuthentication, WM_COMMAND, IDOK, 0);
	SendMessage(hTabIncoming, WM_COMMAND, IDOK, 0);
	SendMessage(hTabInput, WM_COMMAND, IDOK, 0);
	SendMessage(hTabMisc, WM_COMMAND, IDOK, 0);
	SendMessage(hTabNotifications, WM_COMMAND, IDOK, 0);
	SendMessage(hTabReverse, WM_COMMAND, IDOK, 0);
	SendMessage(hTabRules, WM_COMMAND, IDOK, 0);
	SendMessage(hTabCapture, WM_COMMAND, IDOK, 0);
	SendMessage(hTabAdministration, WM_COMMAND, IDOK, 0);

	DestroyWindow(hTabAuthentication);
	DestroyWindow(hTabIncoming);
	DestroyWindow(hTabInput);
	DestroyWindow(hTabMisc);
	DestroyWindow(hTabNotifications);
	DestroyWindow(hTabReverse);
	DestroyWindow(hTabRules);
	DestroyWindow(hTabCapture);
	DestroyWindow(hTabAdministration);
	EndDialog(hwnd, TRUE);
}
void PropertiesDialog::onCancel(HWND hwnd)
{
	SendMessage(hTabAuthentication, WM_COMMAND, IDCANCEL, 0);
	SendMessage(hTabIncoming, WM_COMMAND, IDCANCEL, 0);
	SendMessage(hTabInput, WM_COMMAND, IDCANCEL, 0);
	SendMessage(hTabMisc, WM_COMMAND, IDCANCEL, 0);
	SendMessage(hTabNotifications, WM_COMMAND, IDCANCEL, 0);
	SendMessage(hTabReverse, WM_COMMAND, IDCANCEL, 0);
	SendMessage(hTabRules, WM_COMMAND, IDCANCEL, 0);
	SendMessage(hTabCapture, WM_COMMAND, IDCANCEL, 0);
	SendMessage(hTabAdministration, WM_COMMAND, IDCANCEL, 0);
	DestroyWindow(hTabAuthentication);
	DestroyWindow(hTabIncoming);
	DestroyWindow(hTabInput);
	DestroyWindow(hTabMisc);
	DestroyWindow(hTabNotifications);
	DestroyWindow(hTabReverse);
	DestroyWindow(hTabRules);
	DestroyWindow(hTabCapture);
	DestroyWindow(hTabAdministration);
	EndDialog(hwnd, FALSE);
}

void PropertiesDialog::Secure_Save_Plugin_Config(char* szPlugin)
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

void PropertiesDialog::Secure_Plugin_elevated(char* szPlugin)
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