//  Copyright (C) 2002 Ultr@VNC Team Members. All Rights Reserved.
//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file is part of the VNC system.
//
//  The VNC system is free software; you can redistribute it and/or modify
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
// If the source code for the VNC system is not available from the place 
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.


// vncMenu

// Implementation of a system tray icon & menu for WinVNC

#include "stdhdrs.h"
#include "WinVNC.h"
#include "vncService.h"
//#include "vncConnDialog.h"
#include <lmcons.h>
#include <wininet.h>
#include <shlobj.h>

// Header

#include "vncMenu.h"
#include "HideDesktop.h"

// Constants
const UINT MENU_PROPERTIES_SHOW = RegisterWindowMessage("WinVNC.Properties.User.Show");
const UINT MENU_DEFAULT_PROPERTIES_SHOW = RegisterWindowMessage("WinVNC.Properties.Default.Show");
const UINT MENU_ABOUTBOX_SHOW = RegisterWindowMessage("WinVNC.AboutBox.Show");
const UINT MENU_SERVICEHELPER_MSG = RegisterWindowMessage("WinVNC.ServiceHelper.Message");
const UINT MENU_ADD_CLIENT_MSG = RegisterWindowMessage("WinVNC.AddClient.Message");
const UINT MENU_REMOVE_CLIENTS_MSG = RegisterWindowMessage("WinVNC.RemoveClients.Message"); // REalVNc 336

const UINT FileTransferSendPacketMessage = RegisterWindowMessage("UltraVNC.Viewer.FileTransferSendPacketMessage");

const char *MENU_CLASS_NAME = "WinVNC SC Tray Icon";

BOOL g_restore_ActiveDesktop = FALSE;
extern char		g_idcode[_MAX_PATH];
extern bool		g_id;
unsigned short nport = 0;
bool connected=0;
char szAdrName[64];
int connect_counter=0;
extern char		Balloon1Title[150];
extern char		Balloon2Title[150];
extern char		Balloon1A[150];
extern char		Balloon1B[150];
extern char		Balloon1C[150];
extern char		Balloon2A[150];
extern char		Balloon2B[150];
extern char		Balloon2C[150];

#define RETRY_MESSAGE WM_USER+43

static void
KillWallpaper()
{
	HideDesktop();
}



static void
RestoreWallpaper()
{
  RestoreDesktop();
}

// Implementation

vncMenu::vncMenu(vncServer *server)
{
	ports_set=false;
    CoInitialize(0);
	

	// Save the server pointer
	m_server = server;

	// Set the initial user name to something sensible...
	vncService::CurrentUser((char *)&m_username, sizeof(m_username));

	// Create a dummy window to handle tray icon messages
	WNDCLASSEX wndclass;

	wndclass.cbSize			= sizeof(wndclass);
	wndclass.style			= 0;
	wndclass.lpfnWndProc	= vncMenu::WndProc;
	wndclass.cbClsExtra		= 0;
	wndclass.cbWndExtra		= 0;
	wndclass.hInstance		= hAppInstance;
	wndclass.hIcon			= LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground	= (HBRUSH) GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName	= (const char *) NULL;
	wndclass.lpszClassName	= MENU_CLASS_NAME;
	wndclass.hIconSm		= LoadIcon(NULL, IDI_APPLICATION);

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
	if (m_hwnd == NULL)
	{
		PostQuitMessage(0);
		return;
	}

	// record which client created this window
	SetWindowLong(m_hwnd, GWL_USERDATA, (LONG) this);

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

	// Only enable the timer if the tray icon will be displayed.
	if ( ! server->GetDisableTrayIcon())
	{
		// Timer to trigger icon updating
		SetTimer(m_hwnd, 1, 5000, NULL);
	}

	// Load the icons for the tray
//	m_winvnc_icon = LoadIcon(hAppInstance, MAKEINTRESOURCE(IDI_WINVNC));
//	m_flash_icon = LoadIcon(hAppInstance, MAKEINTRESOURCE(IDI_FLASH));
	{
		OSVERSIONINFO	osvi;
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	GetVersionEx(&osvi);
if (osvi.dwPlatformId==VER_PLATFORM_WIN32_NT)
		{
		  if(osvi.dwMajorVersion==5 && osvi.dwMinorVersion>=1)
		  {
			m_winvnc_icon=(HICON)LoadImage(NULL, "icon1.ico", IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                        GetSystemMetrics(SM_CYSMICON), LR_LOADFROMFILE|LR_DEFAULTCOLOR);
			m_flash_icon=(HICON)LoadImage(NULL, "icon2.ico", IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                        GetSystemMetrics(SM_CYSMICON), LR_LOADFROMFILE|LR_DEFAULTCOLOR);

			if (!m_winvnc_icon) m_winvnc_icon=(HICON)LoadImage(hAppInstance, MAKEINTRESOURCE(IDI_WINVNC), IMAGE_ICON,
                        GetSystemMetrics(SM_CXSMICON),
                        GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
			if (!m_flash_icon) m_flash_icon=(HICON)LoadImage(hAppInstance, MAKEINTRESOURCE(IDI_FLASH), IMAGE_ICON,
                        GetSystemMetrics(SM_CXSMICON),
                        GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
		  }
		  else
		 {
			  m_winvnc_icon=(HICON)LoadImage(NULL, "icon1.ico", IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                        GetSystemMetrics(SM_CYSMICON), LR_LOADFROMFILE|LR_VGACOLOR);
				m_flash_icon=(HICON)LoadImage(NULL, "icon2.ico", IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                        GetSystemMetrics(SM_CYSMICON), LR_LOADFROMFILE|LR_VGACOLOR);
			  if (!m_winvnc_icon)m_winvnc_icon=(HICON)LoadImage(hAppInstance, MAKEINTRESOURCE(IDI_WINVNC), IMAGE_ICON,
                        GetSystemMetrics(SM_CXSMICON),
                        GetSystemMetrics(SM_CYSMICON), LR_VGACOLOR);
			 if (!m_flash_icon)m_flash_icon=(HICON)LoadImage(hAppInstance, MAKEINTRESOURCE(IDI_FLASH), IMAGE_ICON,
                        GetSystemMetrics(SM_CXSMICON),
                        GetSystemMetrics(SM_CYSMICON), LR_VGACOLOR);
		  }
		 }
	else
		 {
				m_winvnc_icon=(HICON)LoadImage(NULL, "icon1.ico", IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                        GetSystemMetrics(SM_CYSMICON), LR_LOADFROMFILE|LR_VGACOLOR);
				m_flash_icon=(HICON)LoadImage(NULL, "icon2.ico", IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                        GetSystemMetrics(SM_CYSMICON), LR_LOADFROMFILE|LR_VGACOLOR);
				if (!m_winvnc_icon)m_winvnc_icon=(HICON)LoadImage(hAppInstance, MAKEINTRESOURCE(IDI_WINVNC), IMAGE_ICON,
                        GetSystemMetrics(SM_CXSMICON),
                        GetSystemMetrics(SM_CYSMICON), LR_VGACOLOR);
				if (!m_flash_icon)m_flash_icon=(HICON)LoadImage(hAppInstance, MAKEINTRESOURCE(IDI_FLASH), IMAGE_ICON,
                        GetSystemMetrics(SM_CXSMICON),
                        GetSystemMetrics(SM_CYSMICON), LR_VGACOLOR);
		  }
	}

	// Load the popup menu
	m_hmenu = LoadMenu(hAppInstance, MAKEINTRESOURCE(IDR_TRAYMENU));

	// Install the tray icon!
	AddTrayIcon();
}

vncMenu::~vncMenu()
{
	// Remove the tray icon
	DelTrayIcon();
	
	// Destroy the loaded menu
	if (m_hmenu != NULL)
		DestroyMenu(m_hmenu);

	// Tell the server to stop notifying us!
	if (m_server != NULL)
		m_server->RemNotify(m_hwnd);

	if (m_server->RemoveWallpaperEnabled())
		RestoreWallpaper();
}

void
vncMenu::AddTrayIcon()
{
	// If the user name is non-null then we have a user!
	if (strcmp(m_username, "") != 0)
	{
		// Make sure the server has not been configured to
		// suppress the tray icon.
		if ( ! m_server->GetDisableTrayIcon())
		{
			SendTrayMsg(NIM_ADD, FALSE);
		}
	}
}

void
vncMenu::DelTrayIcon()
{
	SendTrayMsg(NIM_DELETE, FALSE);
}

void
vncMenu::FlashTrayIcon(BOOL flash)
{
	SendTrayMsg(NIM_MODIFY, flash);
}

// Get the local ip addresses as a human-readable string.
// If more than one, then with \n between them.
// If not available, then gets a message to that effect.
void GetIPAddrString(char *buffer, int buflen) {
    char namebuf[256];

    if (gethostname(namebuf, 256) != 0) {
		strncpy(buffer, "Host name unavailable", buflen);
		return;
    };

    HOSTENT *ph = gethostbyname(namebuf);
    if (!ph) {
		strncpy(buffer, "IP address unavailable", buflen);
		return;
    };

    *buffer = '\0';
    char digtxt[5];
    for (int i = 0; ph->h_addr_list[i]; i++) {
    	for (int j = 0; j < ph->h_length; j++) {
			sprintf(digtxt, "%d.", (unsigned char) ph->h_addr_list[i][j]);
			strncat(buffer, digtxt, (buflen-1)-strlen(buffer));
		}	
		buffer[strlen(buffer)-1] = '\0';
		if (ph->h_addr_list[i+1] != 0)
			strncat(buffer, ", ", (buflen-1)-strlen(buffer));
    }
}

void
vncMenu::SendTrayMsg(DWORD msg, BOOL flash)
{
	// Create the tray icon message
	m_nid.hWnd = m_hwnd;
	m_nid.cbSize = sizeof(m_nid);
	m_nid.uID = ID_WINVNC;			// never changes after construction
	m_nid.hIcon = flash ? m_flash_icon : m_winvnc_icon;
	m_nid.uFlags = NIF_ICON | NIF_MESSAGE;
	m_nid.uCallbackMessage = WM_TRAYNOTIFY;
	//vnclog.Print(LL_INTINFO, VNCLOG("SendTRaymesg\n"));

	// Use resource string as tip if there is one
	if (LoadString(hAppInstance, ID_WINVNC, m_nid.szTip, sizeof(m_nid.szTip)))
	{
	    m_nid.uFlags |= NIF_TIP;
	}
	
	// Try to add the server's IP addresses to the tip string, if possible
	if (m_nid.uFlags & NIF_TIP)
	{
	    strncat(m_nid.szTip, " - ", (sizeof(m_nid.szTip)-1)-strlen(m_nid.szTip));

	    if (m_server->SockConnected())
	    {
		unsigned long tiplen = strlen(m_nid.szTip);
		char *tipptr = ((char *)&m_nid.szTip) + tiplen;

		GetIPAddrString(tipptr, sizeof(m_nid.szTip) - tiplen);
	    }
	    else
	    {
		strncat(m_nid.szTip, " listening", (sizeof(m_nid.szTip)-1)-strlen(m_nid.szTip));
	    }
	}
	if (strcmp(Balloon1Title,"")!=NULL)if (nport!=0 && (connect_counter%5==0 || connect_counter==1))
	{
	char all[450];
	strcpy(all,Balloon1A);
	strcat(all,"\n");
	strcat(all,Balloon1B);
	strcat(all,"\n");
	strcat(all,Balloon1C);
	m_nid.uFlags |= NIF_INFO;
	strncpy(m_nid.szInfo,all, sizeof(all));
	strncpy(m_nid.szInfoTitle,Balloon1Title,sizeof(Balloon1Title));
	m_nid.uTimeout=10000;
	m_nid.dwInfoFlags=NIIF_INFO;
	}
	if (strcmp(Balloon2Title,"")!=NULL)if (connected==1 && connect_counter%10==0)
	{
	connect_counter++;
	char all[450];
	strcpy(all,Balloon2A);
	strcat(all,"\n");
	strcat(all,Balloon2B);
	strcat(all,"\n");
	strcat(all,Balloon2C);
	m_nid.uFlags |= NIF_INFO;
	strncpy(m_nid.szInfo,all, sizeof(all));
	strncpy(m_nid.szInfoTitle,Balloon2Title,sizeof(Balloon2Title));
	m_nid.uTimeout=10000;
	m_nid.dwInfoFlags=NIIF_INFO;
	}

	// Send the message
	if (Shell_NotifyIcon(msg, &m_nid))
	{
		// Set the enabled/disabled state of the menu items
//		vnclog.Print(LL_INTINFO, VNCLOG("tray icon added ok\n"));

			EnableMenuItem(m_hmenu, ID_CLOSE,
			m_properties.AllowShutdown() ? MF_ENABLED : MF_GRAYED);

	} else {
		if (!vncService::RunningAsService())
		{
			if (msg == NIM_ADD)
			{
				// The tray icon couldn't be created, so use the Properties dialog
				// as the main program window
/*				vnclog.Print(LL_INTINFO, VNCLOG("opening dialog box\n"));
				m_properties.ShowAdmin(TRUE, TRUE);
				PostQuitMessage(0);*/
			}
		}
	}
}

// Process window messages
LRESULT CALLBACK vncMenu::WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	// This is a static method, so we don't know which instantiation we're 
	// dealing with. We use Allen Hadden's (ahadden@taratec.com) suggestion 
	// from a newsgroup to get the pseudo-this.
	vncMenu *_this = (vncMenu *) GetWindowLong(hwnd, GWL_USERDATA);
//	Beep(100,10);
//	vnclog.Print(LL_INTINFO, VNCLOG("iMsg 0x%x \n"),iMsg);

	switch (iMsg)
	{

	
		// Every five seconds, a timer message causes the icon to update
	case WM_TIMER:
	    // *** HACK for running servicified
		if (vncService::RunningAsService()) {
		    // Attempt to add the icon if it's not already there
		    _this->AddTrayIcon();
		    // Trigger a check of the current user
		    PostMessage(hwnd, WM_USERCHANGED, 0, 0);
		}
		if (nport!=0)
		{
			PostMessage(hwnd, RETRY_MESSAGE, 0, 0);
		}

		// Update the icon
		_this->FlashTrayIcon(_this->m_server->AuthClientCount() != 0);
		break;

		// DEAL WITH NOTIFICATIONS FROM THE SERVER:
	case WM_SRV_CLIENT_AUTHENTICATED:
	case WM_SRV_CLIENT_DISCONNECT:
		// Adjust the icon accordingly
		_this->FlashTrayIcon(_this->m_server->AuthClientCount() != 0);

		if (_this->m_server->AuthClientCount() != 0) {
			if (_this->m_server->RemoveWallpaperEnabled())
				KillWallpaper();
		} else {
			if (_this->m_server->RemoveWallpaperEnabled())
				RestoreWallpaper();
		}
		return 0;

		// STANDARD MESSAGE HANDLING
	case WM_CREATE:
		return 0;

	case WM_COMMAND:
		// User has clicked an item on the tray menu
		switch (LOWORD(wParam))
		{

		case ID_DEFAULT_PROPERTIES:
/*			// Show the default properties dialog, unless it is already displayed
			vnclog.Print(LL_INTINFO, VNCLOG("show default properties requested\n"));
			_this->m_properties.ShowAdmin(TRUE, FALSE);
			_this->FlashTrayIcon(_this->m_server->AuthClientCount() != 0);*/
			break;
		
		case ID_PROPERTIES:
			// Show the properties dialog, unless it is already displayed
			vnclog.Print(LL_INTINFO, VNCLOG("show user properties requested\n"));
			_this->m_propertiesPoll.Show(TRUE, TRUE);
			_this->FlashTrayIcon(_this->m_server->AuthClientCount() != 0);
			break;

		case ID_ABOUT:
			// Show the About box
			_this->m_about.Show(TRUE);
			break;

		case ID_CLOSE:
			// User selected Close from the tray menu
			_this->m_server->KillAuthClients();
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			break;

		}
		return 0;

	case WM_TRAYNOTIFY:
		// User has clicked on the tray icon or the menu
		{
			// Get the submenu to use as a pop-up menu
			HMENU submenu = GetSubMenu(_this->m_hmenu, 0);

			// What event are we responding to, RMB click?
			if (lParam==WM_RBUTTONUP)
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

				return 0;
			}
			
			// Or was there a LMB double click?
			if (lParam==WM_LBUTTONDBLCLK)
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
		
		// Only accept WM_CLOSE if the logged on user has AllowShutdown set
		if (!_this->m_properties.AllowShutdown())
		{
			return 0;
		}
		// tnatsni Wallpaper fix
		RestoreWallpaper();
		break;
		
	case WM_DESTROY:
		// The user wants WinVNC to quit cleanly...
		vnclog.Print(LL_INTINFO, VNCLOG("quitting from WM_DESTROY\n"));
		PostQuitMessage(0);
		return 0;
		
	case WM_QUERYENDSESSION:
		vnclog.Print(LL_INTERR, VNCLOG("WM_QUERYENDSESSION\n"));
		break;
		
	case WM_ENDSESSION:
		vnclog.Print(LL_INTERR, VNCLOG("WM_ENDSESSION\n"));
		break;

	case WM_USERCHANGED:
		// The current user may have changed.
		{
			char newuser[UNLEN+1];

			if (vncService::CurrentUser((char *) &newuser, sizeof(newuser)))
			{
				vnclog.Print(LL_INTINFO,
//					VNCLOG("usernames : old=\"%s\", new=\"%s\"\n"),
					_this->m_username, newuser);

				// Check whether the user name has changed!
				if (strcmp(newuser, _this->m_username) != 0)
				{
					vnclog.Print(LL_INTINFO,
						VNCLOG("user name has changed\n"));

					// User has changed!
					strcpy(_this->m_username, newuser);

					// Redraw the tray icon and set it's state
					_this->DelTrayIcon();
					_this->AddTrayIcon();
					_this->FlashTrayIcon(_this->m_server->AuthClientCount() != 0);
				}
			}
		}
		return 0;
	
	default:
		if (iMsg == MENU_DEFAULT_PROPERTIES_SHOW)
		{
			// External request to show our Properties dialog
			PostMessage(hwnd, WM_COMMAND, MAKELONG(ID_DEFAULT_PROPERTIES, 0), 0);
			return 0;
		}
		if (iMsg == MENU_ABOUTBOX_SHOW)
		{
			// External request to show our About dialog
			PostMessage(hwnd, WM_COMMAND, MAKELONG(ID_ABOUT, 0), 0);
			return 0;
		}
		if (iMsg == MENU_SERVICEHELPER_MSG)
		{
			// External ServiceHelper message.
			// This message holds a process id which we can use to
			// impersonate a specific user.  In doing so, we can load their
			// preferences correctly

			// vncService::ProcessUserHelperMessage(wParam, lParam);
			// Modif Jeremy C.
			vncService::ProcessUserHelperMessage((WPARAM)&_this->m_server->m_impersonationtoken, lParam); 
			// - Trigger a check of the current user
			PostMessage(hwnd, WM_USERCHANGED, 0, 0);
			return 0;
		}
		if (iMsg == RETRY_MESSAGE)
		{
			connect_counter++;
			if (connect_counter%5==0)
			{
			// Attempt to create a new socket
//			Beep(1000,100);
			VSocket *tmpsock;
			tmpsock = new VSocket;
			if (tmpsock) {

				// Connect out to the specified host on the VNCviewer listen port
				tmpsock->Create();
				if (tmpsock->Connect(szAdrName, nport)) {
					// Add the new client to this server
					
					if (strcmp(g_idcode,"")==NULL) g_id=false;
					else g_id=true;
					char sendID[_MAX_PATH];
					strcpy(sendID,"ID:");
					strcat(sendID,g_idcode);
					if (g_id) 
						tmpsock->Send(sendID,250);
					_this->m_server->AddClient(tmpsock, TRUE, TRUE);
					nport=0;
					connected=1;
					_this->FlashTrayIcon(_this->m_server->AuthClientCount() != 0);
					return 0;
				} else {
					delete tmpsock;
					connect_counter++;
					if (connect_counter>25)
//					Sleep(5000);
					PostMessage(hwnd, WM_CLOSE, 0, 0);
				}
			}
			}
			return 0;
		}
		if (iMsg == MENU_ADD_CLIENT_MSG)
		{
			// sf@2003 - Autoreconnect
			// Dirty trick to avoid to add a new MSG... no time
			if (lParam == 999)
			{
				_this->m_server->AutoReconnect(true);
				return 0;
			}

			/*
			// sf@2005 - FTNoUserImpersonation
			// Dirty trick to avoid to add a new MSG... no time
			if (lParam == 998)
			{
				_this->m_server->FTUserImpersonation(false);
				return 0;
			}
			*/

			// Add Client message.  This message includes an IP address
			// of a listening client, to which we should connect.

			char *nameDup = 0;
			// sf@2003 - Values are already converted
			if (_this->m_server->AutoReconnect() && strlen(_this->m_server->AutoReconnectAdr()) > 0)
			{
				nport = _this->m_server->AutoReconnectPort();
				strcpy(szAdrName, _this->m_server->AutoReconnectAdr());
			}
			else
			{
				// Get the IP address stringified
				struct in_addr address;
				address.S_un.S_addr = lParam;
				char *name = inet_ntoa(address);
				if (name == 0)
					return 0;
				nameDup = strdup(name);
				if (nameDup == 0)
					return 0;
				strcpy(szAdrName, nameDup);
				// Free the duplicate name
				if (nameDup != 0) free(nameDup);

				// Get the port number
				nport = (unsigned short)wParam;
				if (nport == 0)
					nport = INCOMING_PORT_OFFSET;
			}

			// sf@2003
			// Stores the client adr/ports the first time we try to connect
			// This way we can call this message again later to reconnect with the same values
			if (_this->m_server->AutoReconnect() && strlen(_this->m_server->AutoReconnectAdr()) == 0)
			{
				_this->m_server->AutoReconnectAdr(szAdrName);
				_this->m_server->AutoReconnectPort(nport);
			}

			// Attempt to create a new socket
			_this->FlashTrayIcon(_this->m_server->AuthClientCount() != 0);
			VSocket *tmpsock;
			tmpsock = new VSocket;
			if (tmpsock) {

				// Connect out to the specified host on the VNCviewer listen port
				tmpsock->Create();
				if (tmpsock->Connect(szAdrName, nport)) {
					// Add the new client to this server
					
					if (strcmp(g_idcode,"")==NULL) g_id=false;
					else g_id=true;
					char sendID[_MAX_PATH];
					strcpy(sendID,"ID:");
					strcat(sendID,g_idcode);
					if (g_id) 
						tmpsock->Send(sendID,250);
					_this->m_server->AddClient(tmpsock, TRUE, TRUE);
					nport=0;
					connected=1;
					_this->FlashTrayIcon(_this->m_server->AuthClientCount() != 0);
					return 0;
				} else {
					delete tmpsock;
//					Sleep(5000);
//					PostMessage(hwnd, WM_CLOSE, 0, 0);
				}
			}
		
			return 0;
		}

		// Process FileTransfer asynchronous Send Packet Message
		if (iMsg == FileTransferSendPacketMessage) 
		{
		  vncClient* pClient = (vncClient*) wParam;
		  if (_this->m_server->IsClient(pClient)) pClient->SendFileChunk();
		}

	}

	// Message not recognised
	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}
