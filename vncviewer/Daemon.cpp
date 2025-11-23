// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma warning(disable: 4996) // Suppress deprecated API warnings

// Daemon.cpp: implementation of the Daemon class.

#include "stdhdrs.h"
#include "vncviewer.h"
#include "Daemon.h"
#include "Exception.h"
//#include "ClientConnection.h"
#include "AboutBox.h"
#include "common/win32_helpers.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#define DAEMON_CLASS_NAME "VNCviewer Daemon"
extern char sz_I1[64];
extern char sz_I2[64];
extern char sz_I3[64];

Daemon::Daemon(int port, bool ipv6)
{
	this->ipv6 = ipv6;
	// Create a dummy window
	WNDCLASSEX wndclass;

	wndclass.cbSize			= sizeof(wndclass);
	wndclass.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wndclass.lpfnWndProc	= Daemon::WndProc;
	wndclass.cbClsExtra		= 0;
	wndclass.cbWndExtra		= 0;
	wndclass.hInstance		= m_hInstResDLL;
	wndclass.hIcon			= LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground	= (HBRUSH) GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName	= (const char *) NULL;
	wndclass.lpszClassName	= DAEMON_CLASS_NAME;
	wndclass.hIconSm		= LoadIcon(NULL, IDI_APPLICATION);

	RegisterClassEx(&wndclass);

	m_hwnd = CreateWindow(DAEMON_CLASS_NAME,
				DAEMON_CLASS_NAME,
				WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				200, 200,
				NULL,
				NULL,
				m_hInstResDLL,
				NULL);
	
	// record which client created this window
    helper::SafeSetWindowUserData(m_hwnd, (LONG_PTR)this);

	// Load a popup menu
	m_hmenu = LoadMenu(m_hInstResDLL, MAKEINTRESOURCE(IDR_TRAYMENU));

	// sf@2003 - Store Port number for systray display
	m_nPort = port;

	// Create a listening socket
	if (ipv6) {
		struct sockaddr_in6 addr6;
		memset(&addr6, 0, sizeof(addr6));
		addr6.sin6_family = AF_INET6;
		addr6.sin6_port = htons(port);
		addr6.sin6_addr = in6addr_any;
		m_deamon_sock6 = socket(AF_INET6, SOCK_STREAM, 0);

		struct sockaddr_in addr4;
		memset(&addr4, 0, sizeof(addr4));
		addr4.sin_family = AF_INET;
		addr4.sin_port = htons(port);
		addr4.sin_addr.s_addr = INADDR_ANY;
		m_deamon_sock4 = socket(AF_INET6, SOCK_STREAM, 0);
		if (!m_deamon_sock6 && !m_deamon_sock4) throw WarningException(sz_I1);


		if (m_deamon_sock4 && !m_deamon_sock6)
			try {
			int res4 = 0;
			res4 = bind(m_deamon_sock4, (struct sockaddr*)&addr4, sizeof(addr4));
			if (res4 == SOCKET_ERROR)
				throw WarningException(sz_I2);

			res4 = listen(m_deamon_sock4, 5);
			if (res4 == SOCKET_ERROR)
				throw WarningException(sz_I3);
		}
		catch (...) {
			closesocket(m_deamon_sock4);
			m_deamon_sock4 = INVALID_SOCKET;
			throw;
		}
		if (m_deamon_sock4 && m_deamon_sock6)
			try {
			int res4 = 0;
			int res6 = 0;
			res4 = bind(m_deamon_sock4, (struct sockaddr*)&addr4, sizeof(addr4));
			res6 = bind(m_deamon_sock6, (struct sockaddr*)&addr6, sizeof(addr6));
			if (res4 == SOCKET_ERROR && res6 == SOCKET_ERROR)
				throw WarningException(sz_I2);

			res4 = listen(m_deamon_sock4, 5);
			res6 = listen(m_deamon_sock6, 5);
			if (res4 == SOCKET_ERROR && res6 == SOCKET_ERROR)
				throw WarningException(sz_I3);
			if (res4 == SOCKET_ERROR)
			{
				closesocket(m_deamon_sock4);
				m_deamon_sock4 = INVALID_SOCKET;
			}
			if (res6 == SOCKET_ERROR)
			{
				closesocket(m_deamon_sock6);
				m_deamon_sock6 = INVALID_SOCKET;
			}


		}
		catch (...) {
			closesocket(m_deamon_sock4);
			m_deamon_sock4 = INVALID_SOCKET;
			closesocket(m_deamon_sock6);
			m_deamon_sock6 = INVALID_SOCKET;
			throw;
		}

		// Send a message to specified window on an incoming connection
		WSAAsyncSelect(m_deamon_sock6, m_hwnd, WM_SOCKEVENT6, FD_ACCEPT);
		// Send a message to specified window on an incoming connection
		WSAAsyncSelect(m_deamon_sock4, m_hwnd, WM_SOCKEVENT4, FD_ACCEPT);
	}
	else {
		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = INADDR_ANY;
		m_deamon_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (!m_deamon_sock) throw WarningException(sz_I1);



		try {
			int res = 0;
			res = bind(m_deamon_sock, (struct sockaddr*)&addr, sizeof(addr));
			if (res == SOCKET_ERROR)
				throw WarningException(sz_I2);

			res = listen(m_deamon_sock, 5);
			if (res == SOCKET_ERROR)
				throw WarningException(sz_I3);
		}
		catch (...) {
			closesocket(m_deamon_sock);
			m_deamon_sock = INVALID_SOCKET;
			throw;
		}

		// Send a message to specified window on an incoming connection
		WSAAsyncSelect(m_deamon_sock, m_hwnd, WM_SOCKEVENT, FD_ACCEPT);
	}
	

	// Create the Tray icon
	AddTrayIcon();
	
	// A timer checks that the Tray icon is intact
	m_timer = SetTimer( m_hwnd, IDT_TRAYTIMER,  15000, NULL);
}

void Daemon::AddTrayIcon() {
	vnclog.Print(4, _T("Adding Tray icon\n"));
	SendTrayMsg(NIM_ADD);
}

void Daemon::CheckTrayIcon() {
	vnclog.Print(8, _T("Checking Tray icon\n"));
	if (!SendTrayMsg(NIM_MODIFY)) {
		vnclog.Print(4, _T("Tray icon not there - reinstalling\n"));
		AddTrayIcon();
	};
}

void Daemon::RemoveTrayIcon() {
	vnclog.Print(4, _T("Deleting Tray icon\n"));
	SendTrayMsg(NIM_DELETE);
}

bool Daemon::SendTrayMsg(DWORD msg)
{
	m_nid.hWnd = m_hwnd;
	m_nid.cbSize = sizeof(m_nid);
	m_nid.uID = IDR_TRAY;	// never changes after construction

	// Phil Money @ Advantig, LLC 7-9-2005
	// Load icons from main executable, not language DLL
	extern HINSTANCE hInstance;
if (GetListenMode()){ 
        m_nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDR_TRAY));
	}else{ 
        m_nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDR_TRAY_DISABLED)); // Phil Money @ Advantig, LLC 7-9-2005
	} 
  	

	m_nid.uFlags = NIF_ICON | NIF_MESSAGE;
	m_nid.uCallbackMessage = WM_TRAYNOTIFY;
	m_nid.szTip[0] = '\0';
	// Use resource string as tip if there is one
	if (LoadString(m_hInstResDLL, IDR_TRAY, m_nid.szTip, sizeof(m_nid.szTip))) {
		m_nid.uFlags |= NIF_TIP;
	}

	// sf@2003 - Add the port number to the tip
	char szTmp[16];
	sprintf_s(szTmp, " - Port:%ld", m_nPort);
	strcat_s(m_nid.szTip, szTmp);

	return (bool) (Shell_NotifyIcon(msg, &m_nid) != 0);
}

// Process window messages
LRESULT CALLBACK Daemon::WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
	// This is a static method, so we don't know which instantiation we're 
	// dealing with. We have stored a pseudo-this in the window user data, 
	// though.
    Daemon *_this = helper::SafeGetWindowUserData<Daemon>(hwnd);

	switch (iMsg) {

	case WM_CREATE:
		{
			return 0;
		}
	case WM_SOCKEVENT4:
	{
		assert(HIWORD(lParam) == 0);
		// A new socket created by accept might send messages to
		// this procedure. We can ignore them.
		if(wParam != _this->m_deamon_sock4) {
			return 0;
		}

		switch(lParam) {
		case FD_ACCEPT:
		{
			struct sockaddr_in incoming;
			int size_incoming = sizeof(incoming);
			memset(&incoming, 0, sizeof(incoming));

			SOCKET hNewSock;
			hNewSock = accept(_this->m_deamon_sock4, (struct sockaddr *)&incoming,&size_incoming);
			WSAAsyncSelect(hNewSock, hwnd, 0, 0);
			unsigned long nbarg = 0;
			ioctlsocket(hNewSock, FIONBIO, &nbarg);
			// Phil Money @ Advantig, LLC 7-9-2005
			if (ListenMode){ 

				pApp->NewConnection(true,hNewSock);

			}else{ 
				closesocket(hNewSock); 
				hNewSock = INVALID_SOCKET;
			} 

			break;
		}
		case FD_READ:
		{
			unsigned long numbytes=0;
			ioctlsocket(_this->m_deamon_sock4, FIONREAD, &numbytes);
			recv(_this->m_deamon_sock4, _this->netbuf, numbytes, 0);
			break;
		}
		case FD_CLOSE:
			vnclog.Print(5, _T("Daemon connection closed\n"));
			DestroyWindow(hwnd);
			break;
		}

		return 0;
	}

	case WM_SOCKEVENT6:
	{
		assert(HIWORD(lParam) == 0);
		// A new socket created by accept might send messages to
		// this procedure. We can ignore them.
		if (wParam != _this->m_deamon_sock6) {
			return 0;
		}

		switch (lParam) {
		case FD_ACCEPT:
		{
			struct sockaddr_in6 incoming;
			int size_incoming = sizeof(incoming);
			memset(&incoming, 0, sizeof(incoming));


			SOCKET hNewSock;
			hNewSock = accept(_this->m_deamon_sock6, (struct sockaddr *)&incoming, &size_incoming);
			WSAAsyncSelect(hNewSock, hwnd, 0, 0);
			unsigned long nbarg = 0;
			ioctlsocket(hNewSock, FIONBIO, &nbarg);
			// Phil Money @ Advantig, LLC 7-9-2005
			if (ListenMode){

				pApp->NewConnection(true, hNewSock);

			}
			else{
				closesocket(hNewSock);
				hNewSock = INVALID_SOCKET;
			}

			break;
		}
		case FD_READ:
		{
			unsigned long numbytes = 0;
			ioctlsocket(_this->m_deamon_sock6, FIONREAD, &numbytes);
			recv(_this->m_deamon_sock6, _this->netbuf, numbytes, 0);
			break;
		}
		case FD_CLOSE:
			vnclog.Print(5, _T("Daemon connection closed\n"));
			DestroyWindow(hwnd);
			break;
		}

		return 0;
	}

	case WM_SOCKEVENT:
		{
			assert(HIWORD(lParam) == 0);
			// A new socket created by accept might send messages to
			// this procedure. We can ignore them.
			if(wParam != _this->m_deamon_sock) {
				return 0;
			}

			switch(lParam) {
			case FD_ACCEPT:
				{
					struct sockaddr_in incoming;
					int size_incoming = sizeof(incoming);
					memset(&incoming, 0, sizeof(incoming));


					SOCKET hNewSock;
					hNewSock = accept(_this->m_deamon_sock, (struct sockaddr *)&incoming,&size_incoming);
					WSAAsyncSelect(hNewSock, hwnd, 0, 0);
					unsigned long nbarg = 0;
					ioctlsocket(hNewSock, FIONBIO, &nbarg);
					// Phil Money @ Advantig, LLC 7-9-2005
					if (ListenMode){ 

						pApp->NewConnection(true,hNewSock);

					}else{ 
						closesocket(hNewSock); 
						hNewSock = INVALID_SOCKET;
					} 
					
					break;
				}
			case FD_READ:
				{
					unsigned long numbytes=0;
					ioctlsocket(_this->m_deamon_sock, FIONREAD, &numbytes);
					recv(_this->m_deamon_sock, _this->netbuf, numbytes, 0);
					break;
				}
			case FD_CLOSE:
				vnclog.Print(5, _T("Daemon connection closed\n"));
				DestroyWindow(hwnd);
				break;
			}
			
			return 0;
		}

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case ID_NEWCONN:
			pApp->NewConnection(false);
			break;
		case IDC_OPTIONBUTTON:
			pApp->m_options.DoDialog();
			break;
		// Phil Money @ Advantig, LLC 7-9-2005
		case ID_LISTEN_MODE:
			if (GetListenMode()){ 
				SetListenMode(false); 
			}else{ 
				SetListenMode(true); 
			} 
			_this->CheckTrayIcon(); 
			break;
		case ID_CLOSEAPP:
			PostQuitMessage(0);
			//DestroyWindow(hwnd);
			break;
		case ID_CLOSEDAEMON:
			//if (_this->m_sock!=NULL) shutdown(_this->m_sock, SD_BOTH);
			//if (_this->m_sock!=NULL) closesocket(_this->m_sock);
			//_this->m_sock=NULL;
			break;
		case IDD_APP_ABOUT:
			ShowAboutBox();
			break;
		}
		return 0;
	case WM_TRAYNOTIFY:
		{
			HMENU hSubMenu = GetSubMenu(_this->m_hmenu, 0);
			if (lParam==WM_LBUTTONDBLCLK) {
				// double click: execute first menu item
				::SendMessage(_this->m_nid.hWnd, WM_COMMAND, 
					GetMenuItemID(hSubMenu, 0), 0);
			} else if (lParam==WM_RBUTTONUP) {
				if (hSubMenu == NULL) { 
					vnclog.Print(2, _T("No systray submenu\n"));
					return 0;
				}
				// Make first menu item the default (bold font)
				::SetMenuDefaultItem(hSubMenu, 0, TRUE);
				/*if (_this->m_sock==NULL){
					MENUITEMINFO pItem;
					ZeroMemory( &pItem, sizeof(pItem) );
					pItem.cbSize		= sizeof(pItem);
					pItem.fMask			= MIIM_TYPE;
					pItem.fType			= MFT_STRING;
					pItem.dwTypeData	= (LPTSTR)"Deamon closed !!!";

					SetMenuItemInfo( hSubMenu, 5, TRUE, &pItem );
					::SetMenuDefaultItem(hSubMenu, 5, TRUE);
				}*/
				
				// Display the menu at the current mouse location. There's a "bug"
				// (Microsoft calls it a feature) in Windows 95 that requires calling
				// SetForegroundWindow. To find out more, search for Q135788 in MSDN.
				//
				POINT mouse;
				GetCursorPos(&mouse);
				::SetForegroundWindow(_this->m_nid.hWnd);
				::TrackPopupMenu(hSubMenu, 0, mouse.x, mouse.y, 0,
					_this->m_nid.hWnd, NULL);
				
			} 
			return 0;
		}
	case WM_TIMER:
		_this->CheckTrayIcon();
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	
	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

Daemon::~Daemon()
{
	KillTimer(m_hwnd, m_timer);
	RemoveTrayIcon();
	DestroyMenu(m_hmenu);

	if (m_deamon_sock6!=INVALID_SOCKET) 
		shutdown(m_deamon_sock6, SD_BOTH);
	if (m_deamon_sock6!=INVALID_SOCKET) 
		closesocket(m_deamon_sock6);
	if (m_deamon_sock4!=INVALID_SOCKET) 
		shutdown(m_deamon_sock4, SD_BOTH);
	if (m_deamon_sock4 != INVALID_SOCKET) 
		closesocket(m_deamon_sock4);

	if (m_deamon_sock != INVALID_SOCKET) 
		shutdown(m_deamon_sock, SD_BOTH);
	if (m_deamon_sock != INVALID_SOCKET) 
closesocket(m_deamon_sock);

}


// Phil Money @ Advantig, LLC 7-9-2005
void 
SetListenMode(bool listenmode) 
{ 
	ListenMode = listenmode; 
	return; 
} 

bool 
GetListenMode() 
{ 
	return(ListenMode); 
} 
