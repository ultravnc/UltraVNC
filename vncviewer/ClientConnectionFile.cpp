// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//


#include "stdhdrs.h"
#include "vncviewer.h"
#include "ClientConnection.h"
#include "Exception.h"

//AaronP
#include "SessionDialog.h"
//EndAaronP

#include "vncauth.h"
#include "UltraVNCHelperFunctions.h"
#include "common/win32_helpers.h"
using namespace helper;
extern HINSTANCE m_hInstResDLL;

extern wchar_t sz_K1[64];
extern wchar_t sz_K2[64];
extern wchar_t sz_K3[128];
extern wchar_t sz_K4[64];
extern wchar_t sz_K5[64];
extern wchar_t sz_K6[64];
extern wchar_t sz_K7[64];
extern bool config_specified;

// This file contains the code for saving and loading connection info.

static OPENFILENAME ofn;

void ofnInit()
{
	static TCHAR filter[] = _T("VNC files (*.vnc)\0*.vnc\0")
						   _T("All files (*.*)\0*.*\0");
	memset((void *) &ofn, 0, sizeof(OPENFILENAME));

	// sf@2002 v1.1.1 - OPENFILENAME is Plateforme dependent !
	// Under Windows NT4, the dialog box wouldn't appear if we don't use OPENFILENAME_SIZE_VERSION_400
	// when compiling using BCC55 under Windows 2000.
#ifdef OPENFILENAME_SIZE_VERSION_400
    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
#else
    ofn.lStructSize = sizeof(OPENFILENAME);
#endif

	ofn.lpstrFilter = filter;
	ofn.nMaxFile = _MAX_PATH;
	ofn.nMaxFileTitle = _MAX_FNAME + _MAX_EXT;
	ofn.lpstrDefExt = _T("vnc");
}

//
// SaveConnection
// Save info about current connection to a file
//

void ClientConnection::SaveConnection()
{
	vnclog.Print(2, _T("Saving connection info\n"));	
	TCHAR fname[_MAX_PATH];
	TCHAR tname[_MAX_FNAME + _MAX_EXT];
	ofnInit();
	int disp = PORT_TO_DISPLAY(m_port);
	_sntprintf_s(fname, _countof(fname), _TRUNCATE, _T("%.15s-%d.vnc"), m_host, (disp > 0 && disp < 100) ? disp : m_port);
	ofn.hwndOwner = m_hwndcn;
	ofn.lpstrFile = fname;
	ofn.lpstrFileTitle = tname;
	ofn.Flags = OFN_HIDEREADONLY;
	if (!GetSaveFileName(&ofn)) {
		DWORD err = CommDlgExtendedError();
		switch(err) {
		case 0:	// user cancelled
			break;
		case FNERR_INVALIDFILENAME:
			yesUVNCMessageBox(m_hInstResDLL, m_hwndcn, sz_K1, sz_K2, MB_ICONERROR);
			break;
		default:
			vnclog.Print(0, _T("Error %d from GetSaveFileName\n"), err);
			break;
		}
		return;
	}
	vnclog.Print(1, _T("Saving to %s\n"), fname);

	WritePrivateProfileString(_T("connection"), _T("host"), m_host, fname);
	char buf[32];
	TCHAR bufW[32];
	_stprintf_s(bufW, _T("%d"), m_port);
	WritePrivateProfileString(_T("connection"), _T("port"), bufW, fname);

	WritePrivateProfileString(_T("connection"), _T("proxyhost"), m_proxyhost, fname);
	_stprintf_s(bufW, _T("%d"), m_proxyport);
	WritePrivateProfileString(_T("connection"), _T("proxyport"), bufW, fname);
	BOOL bCheckboxChecked;
	int yes = yesnoUVNCMessageBox(m_hInstResDLL, m_hwndcn, sz_K3, sz_K4, str50287, str50288, _T(""), bCheckboxChecked);
	if (yes)
	{
		for (int i = 0; i < MAXPWLEN; i++) {
			sprintf_s(buf+i*2, 32-i*2, "%02x", (unsigned int) m_encPasswd[i]);
		}
	} else
		buf[0] = '\0';
	// Password stored as hex string, use Unicode API for correct filename handling
	wchar_t wbuf[32];
	MultiByteToWideChar(CP_UTF8, 0, buf, -1, wbuf, 32);
	WritePrivateProfileStringW(L"connection", L"password", wbuf, fname);
	m_opts->SaveOptions(fname);
	//m_opts->Register();
}

	void ClientConnection::SaveAllowUntrustedServers()
{
	TCHAR buf[10];
	_stprintf_s(buf, _T("%d"), 1);
	if (m_opts->m_UseOnlyDefaultConfigFile) {
		WritePrivateProfileString(_T("options"), _T("AllowUntrustedServers"), buf, m_opts->getDefaultOptionsFileName());
	} else {
		TCHAR fname[_MAX_PATH];
		TCHAR tname[_MAX_FNAME + _MAX_EXT];
		ofnInit();
		int disp = PORT_TO_DISPLAY(m_port);
		_sntprintf_s(fname, _countof(fname), _TRUNCATE, _T("%.15s-%d.vnc"), m_host, (disp > 0 && disp < 100) ? disp : m_port);
		ofn.hwndOwner = m_hwndcn;
		ofn.lpstrFile = fname;
		ofn.lpstrFileTitle = tname;
		ofn.Flags = OFN_HIDEREADONLY;
		if (!GetSaveFileName(&ofn)) {
			DWORD err = CommDlgExtendedError();
			switch (err) {
			case 0:	// user cancelled
				break;
			case FNERR_INVALIDFILENAME:
				yesUVNCMessageBox(m_hInstResDLL, m_hwndcn, sz_K1, sz_K2, MB_ICONERROR);
				break;
			default:
				vnclog.Print(0, _T("Error %d from GetSaveFileName\n"), err);
				break;
			}
			return;
		}
		WritePrivateProfileString(_T("options"), _T("AllowUntrustedServers"), buf, fname);
	}
}


void ClientConnection::Save_Latest_Connection()
{
	if (config_specified) 
		return;
	vnclog.Print(2, _T("Saving connection info\n"));
	m_opts->SaveOptions(m_opts->getDefaultOptionsFileName());
}

// returns zero if successful
int ClientConnection::LoadConnection(const wchar_t *fname, bool fFromDialog, bool defaultOption)
{
	// The Connection Profile ".vnc" has been required from Connection Session Dialog Box
	if (fFromDialog && ! defaultOption) {
		TCHAR tfname[_MAX_PATH];
		TCHAR tname[_MAX_FNAME + _MAX_EXT];
		_tcscpy_s(tfname, fname);
		ofnInit();
		ofn.hwndOwner = m_hSessionDialog;
		ofn.lpstrFile = tfname;
		ofn.lpstrFileTitle = tname;
		ofn.Flags = OFN_HIDEREADONLY;
		if (GetOpenFileName(&ofn) == 0)
			return -1;
		// Update fname pointer to point to tfname for subsequent use
		fname = tfname;
	}

	if (!defaultOption) {
		GetPrivateProfileString(_T("connection"), _T("host"), _T(""), m_host, MAX_HOST_NAME_LEN, fname);
		if ( (m_port = GetPrivateProfileInt(_T("connection"), _T("port"), 0, fname)) == 0)
			return -1;
	}
	else {
		m_host[0] = _T('\0');
		m_port = -1;
	}
	GetPrivateProfileString(_T("connection"), _T("proxyhost"), _T(""), m_proxyhost, MAX_HOST_NAME_LEN, fname);
	m_proxyport = GetPrivateProfileInt(_T("connection"), _T("proxyport"), 0, fname);
    m_connectionType = (ConnectionType)GetPrivateProfileInt(_T("options"), _T("UseProxy"), DIRECT_TCP, fname);

	// Password stored as hex string, use Unicode API for correct filename handling
	wchar_t bufW[32];
	m_encPasswd[0] = '\0';
	if (GetPrivateProfileStringW(L"connection", L"password", L"", bufW, 32, fname) > 0) {
		char buf[32];
		WideCharToMultiByte(CP_UTF8, 0, bufW, -1, buf, 32, NULL, NULL);
		for (int i = 0; i < MAXPWLEN; i++)	{
			int x = 0;
			sscanf_s(buf+i*2, "%2x", &x);
			m_encPasswd[i] = (unsigned char) x;
		}
	}
	
	if (fFromDialog)
		m_opts->LoadOptions(fname);
	else {
		if (_tcslen(m_host) == 0 || _tcscmp(fname, m_opts->getDefaultOptionsFileName()) == 0) {
			// Load the rest of params
			_tcscpy_s(m_opts->m_proxyhost, _countof(m_opts->m_proxyhost), m_proxyhost);
			m_opts->m_proxyport=m_proxyport;
			m_opts->m_connectionType=m_connectionType;
			m_opts->LoadOptions(fname);
			//m_opts->Register();
			// Then display the session dialog to get missing params again
			SessionDialog sessdlg(m_opts, this, m_pDSMPlugin); //sf@2002
			if (!sessdlg.DoDialog())
			QuietException_helper(L"");
			_tcsncpy_s(m_host, MAX_HOST_NAME_LEN, sessdlg.m_host_dialog, _TRUNCATE);
			m_port = sessdlg.m_port;	
			_tcsncpy_s(m_proxyhost, MAX_HOST_NAME_LEN, sessdlg.m_proxyhost, _TRUNCATE);
			m_proxyport = sessdlg.m_proxyport;
			m_connectionType = sessdlg.m_connectionType;
		}
		else if (config_specified) {
			_tcscpy_s(m_opts->m_proxyhost, _countof(m_opts->m_proxyhost), m_proxyhost);
			m_opts->m_proxyport=m_proxyport;
			m_opts->m_connectionType=m_connectionType;
			m_opts->LoadOptions(fname);
		}
	}
	return 0;
}
