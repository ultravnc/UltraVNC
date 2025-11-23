// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#include "stdhdrs.h"
#include "vncviewer.h"
#include "VNCviewerApp.h"
#include "Exception.h"
#include "UltraVNCHelperFunctions.h"
#include "common/win32_helpers.h"
using namespace helper;
extern char sz_A2[64];
extern char sz_B1[64];
extern char sz_B2[64];
extern char sz_B3[64];
extern HINSTANCE m_hInstResDLL;

// For WinCE Palm, you might want to use this for debugging, since it
// seems impossible to give the command some arguments.
// #define PALM_LOG 1

VNCviewerApp *pApp;

VNCviewerApp::VNCviewerApp(HINSTANCE hInstance, LPTSTR szCmdLine) {
	pApp = this;
	// m_instance points to the resource DLL (language DLL or main exe)
	// Language DLLs contain dialogs, strings, AND images
	m_instance = m_hInstResDLL;

	// Read the command line
	m_options.SetFromCommandLine(szCmdLine);
	
	// Logging info
	vnclog.SetLevel(m_options.m_logLevel);
	if (m_options.m_logToConsole) {
		vnclog.SetMode(Log::ToConsole | Log::ToDebug);
	}
	if (m_options.m_logToFile) {
		vnclog.SetFile(m_options.m_logFilename);
	}
//vnclog.SetLevel(20);
//vnclog.SetMode(1);
//vnclog.SetFile(_T(".\vncviewer.log"));
#ifdef PALM_LOG
	// Hack override for WinCE Palm developers who can't give
	// options to commands, not even via shortcuts.
	vnclog.SetLevel(20);
 	vnclog.SetFile(_T("\\Temp\\log"));
#endif
 	
	// Clear connection list
	for (int i = 0; i < MAX_CONNECTIONS; i++)
		m_clilist[i] = NULL;

	// Initialise winsock
	WORD wVersionRequested = MAKEWORD(2, 0);
	WSADATA wsaData;
	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		yesUVNCMessageBox(m_hInstResDLL, NULL, sz_B1, sz_A2, MB_ICONSTOP);
		PostQuitMessage(1);
	}
	vnclog.Print(3, _T("Started and Winsock (v %d) initialised\n"), wsaData.wVersion);
}


// The list of clients should fill up from the start and have NULLs
// afterwards. If the first entry is a NULL, the list is empty.

void VNCviewerApp::RegisterConnection(ClientConnection *pConn) {
	omni_mutex_lock l(m_clilistMutex);
	int i;
	for (i = 0; i < MAX_CONNECTIONS; i++) {
		if (m_clilist[i] == NULL) {
			m_clilist[i] = pConn;
			vnclog.Print(4,_T("Registered connection with app\n"));
			return;
		}
	}
	// If we've got here, something is wrong.
	vnclog.Print(-1, _T("Client list overflow!\n"));
	yesUVNCMessageBox(m_hInstResDLL, NULL, sz_B2, sz_B3,MB_ICONSTOP);
	PostQuitMessage(1);

}

void VNCviewerApp::DeregisterConnection(ClientConnection *pConn) {
	omni_mutex_lock l(m_clilistMutex);
	int i;
	for (i = 0; i < MAX_CONNECTIONS; i++) {
		if (m_clilist[i] == pConn) {
			// shuffle everything above downwards
			for (int j = i; m_clilist[j] &&	j < MAX_CONNECTIONS-1 ; j++)
				m_clilist[j] = m_clilist[j+1];
			m_clilist[MAX_CONNECTIONS-1] = NULL;
			vnclog.Print(4,_T("Deregistered connection from app\n"));

			// No clients left? then we should finish, unless we're in
			// listening mode.
			if ((m_clilist[0] == NULL) && (!pApp->m_options.m_listening))
				PostQuitMessage(0);

			return;
		}
	}
	// If we've got here, something is wrong.
	vnclog.Print(-1, _T("Client not found for deregistering!\n"));
	PostQuitMessage(1);
}

// ----------------------------------------------


VNCviewerApp::~VNCviewerApp() {
		
	
	// Clean up winsock
	WSACleanup();
	
	vnclog.Print(2, _T("VNC Viewer closing down\n"));

}
