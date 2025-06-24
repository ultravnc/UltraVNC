/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
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

extern char sz_K1[64];
extern char sz_K2[64];
extern char sz_K3[128];
extern char sz_K4[64];
extern char sz_K5[64];
extern char sz_K6[64];
extern char sz_K7[64];
extern bool config_specified;

// This file contains the code for saving and loading connection info.

static OPENFILENAME ofn;

void ofnInit()
{
	static char filter[] = "VNC files (*.vnc)\0*.vnc\0" \
						   "All files (*.*)\0*.*\0";
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
	ofn.lpstrDefExt = "vnc";
}

//
// SaveConnection
// Save info about current connection to a file
//

void ClientConnection::SaveConnection()
{
	vnclog.Print(2, _T("Saving connection info\n"));	
	char fname[_MAX_PATH];
	char tname[_MAX_FNAME + _MAX_EXT];
	ofnInit();
	int disp = PORT_TO_DISPLAY(m_port);
	sprintf_s(fname, "%.15s-%d.vnc", m_host, (disp > 0 && disp < 100) ? disp : m_port);
	ofn.hwndOwner = m_hwndcn;
	ofn.lpstrFile = fname;
	ofn.lpstrFileTitle = tname;
	ofn.Flags = OFN_HIDEREADONLY;
	if (!GetSaveFileName(&ofn)) {
		DWORD err = CommDlgExtendedError();
		char msg[1024]; 
		switch(err) {
		case 0:	// user cancelled
			break;
		case FNERR_INVALIDFILENAME:
			strcpy_s(msg, sz_K1);
			yesUVNCMessageBox(m_hInstResDLL, m_hwndcn, msg, sz_K2, MB_ICONERROR);
			break;
		default:
			vnclog.Print(0, "Error %d from GetSaveFileName\n", err);
			break;
		}
		return;
	}
	vnclog.Print(1, "Saving to %s\n", fname);	

	int ret = WritePrivateProfileString("connection", "host", m_host, fname);
	char buf[32];
	sprintf_s(buf, "%d", m_port);
	WritePrivateProfileString("connection", "port", buf, fname);

	ret = WritePrivateProfileString("connection", "proxyhost", m_proxyhost, fname);
	sprintf_s(buf, "%d", m_proxyport);
	WritePrivateProfileString("connection", "proxyport", buf, fname);
	BOOL bCheckboxChecked;
	int yes = yesnoUVNCMessageBox(m_hInstResDLL, m_hwndcn, sz_K3, sz_K4, str50287, str50288, "", bCheckboxChecked);
	if (yes)
	{
		for (int i = 0; i < MAXPWLEN; i++) {
			sprintf_s(buf+i*2, 32-i*2, "%02x", (unsigned int) m_encPasswd[i]);
		}
	} else
		buf[0] = '\0';
	WritePrivateProfileString("connection", "password", buf, fname);
	m_opts->SaveOptions(fname);
	//m_opts->Register();
}

void ClientConnection::SaveAllowUntrustedServers()
{
	char buf[10];
	sprintf_s(buf, "%d", 1);
	if (m_opts->m_UseOnlyDefaultConfigFile)
		WritePrivateProfileString("options", "AllowUntrustedServers", buf, m_opts->getDefaultOptionsFileName());
	else {
		char fname[_MAX_PATH];
		char tname[_MAX_FNAME + _MAX_EXT];
		ofnInit();
		int disp = PORT_TO_DISPLAY(m_port);
		sprintf_s(fname, "%.15s-%d.vnc", m_host, (disp > 0 && disp < 100) ? disp : m_port);
		ofn.hwndOwner = m_hwndcn;
		ofn.lpstrFile = fname;
		ofn.lpstrFileTitle = tname;
		ofn.Flags = OFN_HIDEREADONLY;
		if (!GetSaveFileName(&ofn)) {
			DWORD err = CommDlgExtendedError();
			char msg[1024];
			switch (err) {
			case 0:	// user cancelled
				break;
			case FNERR_INVALIDFILENAME:
				strcpy_s(msg, sz_K1);
				yesUVNCMessageBox(m_hInstResDLL, m_hwndcn, msg, sz_K2, MB_ICONERROR);
				break;
			default:
				vnclog.Print(0, "Error %d from GetSaveFileName\n", err);
				break;
			}
			return;
		}
		WritePrivateProfileString("options", "AllowUntrustedServers", buf, fname);
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
int ClientConnection::LoadConnection(char *fname, bool fFromDialog, bool defaultOption)
{
	// The Connection Profile ".vnc" has been required from Connection Session Dialog Box
	if (fFromDialog && ! defaultOption) {
		char tname[_MAX_FNAME + _MAX_EXT];
		ofnInit();
		ofn.hwndOwner = m_hSessionDialog;
		ofn.lpstrFile = fname;
		ofn.lpstrFileTitle = tname;
		ofn.Flags = OFN_HIDEREADONLY;
		if (GetOpenFileName(&ofn) == 0)
			return -1;
	}

	if (!defaultOption) {
		GetPrivateProfileString("connection", "host", "", m_host, MAX_HOST_NAME_LEN, fname);
		if ( (m_port = GetPrivateProfileInt("connection", "port", 0, fname)) == 0)
			return -1;
	}
	else {
		strcpy_s(m_host,"");
		m_port = -1;
	}
	GetPrivateProfileString("connection", "proxyhost", "", m_proxyhost, MAX_HOST_NAME_LEN, fname);
	m_proxyport = GetPrivateProfileInt("connection", "proxyport", 0, fname);
    m_fUseProxy = GetPrivateProfileInt("options", "UseProxy", 0, fname) ? true : false;

	char buf[32];
	m_encPasswd[0] = '\0';
	if (GetPrivateProfileString("connection", "password", "", buf, 32, fname) > 0) {
		for (int i = 0; i < MAXPWLEN; i++)	{
			int x = 0;
			sscanf_s(buf+i*2, "%2x", &x);
			m_encPasswd[i] = (unsigned char) x;
		}
	}
	
	if (fFromDialog)
		m_opts->LoadOptions(fname);
	else if (strcmp(m_host, "") == 0 || strcmp(fname, m_opts->getDefaultOptionsFileName())==0 ) {
		// Load the rest of params 
		strcpy_s(m_opts->m_proxyhost,m_proxyhost);
		m_opts->m_proxyport=m_proxyport;
		m_opts->m_fUseProxy=m_fUseProxy;
		m_opts->LoadOptions(fname);
		//m_opts->Register();
		// Then display the session dialog to get missing params again
		SessionDialog sessdlg(m_opts, this, m_pDSMPlugin); //sf@2002
		if (!sessdlg.DoDialog())
			throw QuietException("");
		_tcsncpy_s(m_host, sessdlg.m_host_dialog, MAX_HOST_NAME_LEN);
		m_port = sessdlg.m_port;	
		_tcsncpy_s(m_proxyhost, sessdlg.m_proxyhost, MAX_HOST_NAME_LEN);
		m_proxyport = sessdlg.m_proxyport;
		m_fUseProxy = sessdlg.m_fUseProxy;
	}
	else if (config_specified) {
		strcpy_s(m_opts->m_proxyhost,m_proxyhost);
		m_opts->m_proxyport=m_proxyport;
		m_opts->m_fUseProxy=m_fUseProxy;
		m_opts->LoadOptions(fname);
	}
	return 0;
}
