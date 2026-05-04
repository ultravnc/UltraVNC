// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#include "CloudDialog.h"
#include "vncserver.h"
#include "SettingsManager.h"
#include "resource.h"

extern HINSTANCE	hInstResDLL;

CloudDialog::CloudDialog()
{
	m_dlgvisible = FALSE;
}

CloudDialog::~CloudDialog()
{
}

void CloudDialog::LoadFromIniFile()
{
#ifndef SC_20
	settings->load();
#endif // SC_20
}
void CloudDialog::SaveToIniFile()
{
#ifndef SC_20
	settings->save();
#endif // SC_20
}

// Initialisation
void CloudDialog::Init(vncServer* server)
{
	// Save the server pointer
	m_server = server;
	LoadFromIniFile();
}

// Dialog box handling functions
void CloudDialog::Show(bool show, bool SC)
{
	this->SC = SC;
	if (show)
	{
		if (!m_dlgvisible)
		{
			//	[v1.0.2-jp1 fix]
			//DialogBoxParam(hAppInstance,
			DialogBoxParam(hInstResDLL, MAKEINTRESOURCE(IDD_DIALOGCLOUD), NULL, (DLGPROC)DialogProc, (LONG_PTR)this);
		}
	}
}

BOOL CALLBACK
CloudDialog::DialogProc(HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	// We use the dialog-box's USERDATA to store a _this pointer
	// This is set only once WM_INITDIALOG has been recieved, though!
#ifndef _X64
	CloudDialog* _this = (CloudDialog*)GetWindowLong(hwnd, GWL_USERDATA);
#else
	CloudDialog* _this = (CloudDialog*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
#endif // _X64
	switch (uMsg)
	{

	case WM_INITDIALOG:
	{
		HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_WINVNC));
		SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		// Retrieve the Dialog box parameter and use it as a pointer
		// to the calling vncProperties object
#ifndef _X64
		SetWindowLong(hwnd, GWL_USERDATA, lParam);
#else
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
#endif // _X64
		_this = (CloudDialog*)lParam;
		if (_this->SC) {
			ShowWindow(GetDlgItem(hwnd, IDC_CHECKCLOUD), false);
			ShowWindow(GetDlgItem(hwnd, IDOK), false);
		}
		SetDlgItemText(hwnd, IDC_CLOUDSERVER, settings->getCloudServer());
		SetDlgItemText(hwnd, IDC_CLOUDCODE, _this->m_server->code);
		SendMessage(GetDlgItem(hwnd, IDC_CHECKCLOUD), BM_SETCHECK, settings->getUseBridge(), 0);
		if (_this->m_server->isBridgeStarted()) {
			SetDlgItemText(hwnd, IDC_STARTCLOUD, "Stop");
			ShowWindow(GetDlgItem(hwnd, IDC_CLOUDCODE), SW_SHOW);
		}
		else {
			SetDlgItemText(hwnd, IDC_STARTCLOUD, "Start");
			ShowWindow(GetDlgItem(hwnd, IDC_CLOUDCODE), SW_HIDE);
		}

		HFONT  hfont = CreateFont(
			24,                        // nHeight
			0,                         // nWidth
			0,                         // nEscapement
			0,                         // nOrientation
			FW_BOLD,                 // nWeight
			FALSE,                     // bItalic
			FALSE,                     // bUnderline
			0,                         // cStrikeOut
			ANSI_CHARSET,              // nCharSet
			OUT_DEFAULT_PRECIS,        // nOutPrecision
			CLIP_DEFAULT_PRECIS,       // nClipPrecision
			DEFAULT_QUALITY,           // nQuality
			DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
			_T("Arial"));                 // lpszFacename

		SendMessage(GetDlgItem(hwnd, IDC_CLOUDCODE), WM_SETFONT, WPARAM(hfont), TRUE);
		SetForegroundWindow(hwnd);
		SetTimer(hwnd, 120, 1000, NULL);
		_this->m_dlgvisible = TRUE;
		// Hide all LEDs first, then show only the active one
		ShowWindow(GetDlgItem(hwnd, IDC_GREEN),  SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_YELLOW), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_RED),    SW_HIDE);
		switch (_this->m_server->getStatus()) {
		case csConnected:
			ShowWindow(GetDlgItem(hwnd, IDC_GREEN), SW_SHOW);
			break;
		case csOnline:
		case csRendezvous:
			ShowWindow(GetDlgItem(hwnd, IDC_YELLOW), SW_SHOW);
			break;
		default: // csOffline
			if (_this->m_server->isBridgeStarted())
				ShowWindow(GetDlgItem(hwnd, IDC_RED), SW_SHOW);
			break;
		}
		{
			const char* extIp = _this->m_server->getExternalIpAddress();
			SetDlgItemTextA(hwnd, IDC_EXTERNALIPADDRESS, (extIp && extIp[0] != '\0') ? extIp : "");
			char title[256];
			snprintf(title, sizeof(title), "UltraVNC Server - Cloud connect [%s]", _this->m_server->getCloudStatusText());
			SetWindowTextA(hwnd, title);
		}
		// Drain log lines from the proxy and append to IDC_CLOUDSTATUS
		{
			CloudServerProxy* proxy = _this->m_server->getCloudProxy();
			if (proxy) {
				auto lines = proxy->DrainLogs();
				if (!lines.empty()) {
					HWND hLog = GetDlgItem(hwnd, IDC_CLOUDSTATUS);
					for (const auto& line : lines) {
						int len = GetWindowTextLengthA(hLog);
						SendMessageA(hLog, EM_SETSEL, len, len);
						SendMessageA(hLog, EM_REPLACESEL, FALSE, (LPARAM)line.c_str());
					}
					// Scroll to bottom
					SendMessageA(GetDlgItem(hwnd, IDC_CLOUDSTATUS), WM_VSCROLL, SB_BOTTOM, 0);
				}
			}
		}
		if (_this->SC) {
			SendMessage(GetDlgItem(hwnd, IDC_STARTCLOUD), BM_CLICK, 0, 0);
		}
		return TRUE;
	}
	case WM_TIMER:
		if (wParam == 120) {
			if (_this->m_server->isCloudThreadRunning()) {
				SetDlgItemText(hwnd, IDC_STARTCLOUD, "Stop");
				ShowWindow(GetDlgItem(hwnd, IDC_CLOUDCODE), true);
			}
			else {
				SetDlgItemText(hwnd, IDC_STARTCLOUD, "Start");
				ShowWindow(GetDlgItem(hwnd, IDC_CLOUDCODE), false);
			}
			{
				const char* extIp = _this->m_server->getExternalIpAddress();
				SetDlgItemTextA(hwnd, IDC_EXTERNALIPADDRESS, (extIp && extIp[0] != '\0') ? extIp : "");
				char title[256];
				snprintf(title, sizeof(title), "UltraVNC Server - Cloud connect [%s]", _this->m_server->getCloudStatusText());
				SetWindowTextA(hwnd, title);
			}
			// Hide all LEDs first, then show only the active one
			ShowWindow(GetDlgItem(hwnd, IDC_GREEN),  SW_HIDE);
			ShowWindow(GetDlgItem(hwnd, IDC_YELLOW), SW_HIDE);
			ShowWindow(GetDlgItem(hwnd, IDC_RED),    SW_HIDE);
			switch (_this->m_server->getStatus()) {
				case csConnected:
					ShowWindow(GetDlgItem(hwnd, IDC_GREEN), SW_SHOW);
					break;
				case csOnline:
				case csRendezvous:
					ShowWindow(GetDlgItem(hwnd, IDC_YELLOW), SW_SHOW);
					break;
				default:
					if (_this->m_server->isBridgeStarted())
						ShowWindow(GetDlgItem(hwnd, IDC_RED), SW_SHOW);
					break;
			}
			// Drain log lines from proxy into IDC_CLOUDSTATUS
			{
				CloudServerProxy* proxy = _this->m_server->getCloudProxy();
				if (proxy) {
					auto lines = proxy->DrainLogs();
					HWND hLog = GetDlgItem(hwnd, IDC_CLOUDSTATUS);
					for (const auto& line : lines) {
						int len = GetWindowTextLengthA(hLog);
						SendMessageA(hLog, EM_SETSEL, len, len);
						SendMessageA(hLog, EM_REPLACESEL, FALSE, (LPARAM)line.c_str());
					}
					if (!lines.empty())
						SendMessageA(hLog, WM_VSCROLL, SB_BOTTOM, 0);
				}
			}
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
	{

		case IDCANCEL:
			EndDialog(hwnd, TRUE);
			_this->m_dlgvisible = FALSE;
			return TRUE;
		case IDOK:
		{
			TCHAR cloudServer[MAX_HOST_NAME_LEN];
			GetDlgItemText(hwnd, IDC_CLOUDSERVER, cloudServer, MAX_HOST_NAME_LEN);
			settings->setCloudServer(cloudServer);
			GetDlgItemText(hwnd, IDC_CLOUDCODE, _this->m_server->code, 18);
			BOOL autoStart = SendMessage(GetDlgItem(hwnd, IDC_CHECKCLOUD), BM_GETCHECK, 0, 0) == BST_CHECKED;
			settings->setCloudEnabled(autoStart);
			settings->setUseBridge(autoStart);
			_this->SaveToIniFile();
			EndDialog(hwnd, TRUE);
			_this->m_dlgvisible = FALSE;
			return TRUE;
		}

		case IDC_STARTCLOUD:
			ShowWindow(GetDlgItem(hwnd, IDC_CLOUDCODE), true);
			GetDlgItemText(hwnd, IDC_CLOUDSERVER, settings->getCloudServer(), MAX_HOST_NAME_LEN);
			if (!_this->m_server->isCloudThreadRunning()) {
				_this->m_server->cloudConnect(true, settings->getCloudServer());
			}
			else {				
				_this->m_server->cloudConnect(false, settings->getCloudServer());
			}
			return TRUE;
		}

		break;

	case WM_DESTROY:
		KillTimer(hwnd, 120);
		EndDialog(hwnd, FALSE);
		_this->m_dlgvisible = FALSE;
		return TRUE;
	}
	return 0;
}
