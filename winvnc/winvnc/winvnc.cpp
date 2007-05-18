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

// WinVNC.cpp

// 24/11/97		WEZ

// WinMain and main WndProc for the new version of WinVNC

////////////////////////////
// System headers
#include "stdhdrs.h"

////////////////////////////
// Custom headers
#include "VSocket.h"
#include "WinVNC.h"

#include "vncServer.h"
#include "vncMenu.h"
#include "vncInstHandler.h"
#include "vncService.h"
///unload driver
#include "vncOSVersion.h"
#include "videodriver.h"
//#define CRASH_ENABLED
#ifdef CRASH_ENABLED
	#ifndef _CRASH_RPT_
	#include "crashrpt.h"
	#pragma comment(lib, "crashrpt.lib")
	#endif
#endif

#define LOCALIZATION_MESSAGES   // ACT: full declaration instead on extern ones
#include "localization.h" // Act : add localization on messages



// Application instance and name
HINSTANCE	hAppInstance;
const char	*szAppName = "WinVNC";
DWORD		mainthreadId;
BOOL		AllowMulti=false;
BOOL		DisableMultiWarning=false;
BOOL		fRunningAsApplication0=false;
BOOL		fRunningAsApplication0System=false;
BOOL		fRunningAsApplication0User=false;


void WRITETOLOG(char *szText, int size, DWORD *byteswritten, void *);

bool SelectDesktop();
bool InputDesktopSelected()
{

	DWORD dummy;
	char threadname[256];
	char inputname[256];
	bool IsWinNT;

	OSVERSIONINFO WinVer;	
	WinVer.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	GetVersionEx(&WinVer);
	if ((WinVer.dwPlatformId==VER_PLATFORM_WIN32_NT)) IsWinNT=1;
	else IsWinNT=0;

	if (IsWinNT)
	{
		HDESK threaddesktop = GetThreadDesktop(GetCurrentThreadId());
		HDESK inputdesktop = OpenInputDesktop(0, FALSE,
				DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
				DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
				DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
				DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);

		if (inputdesktop == NULL) return FALSE;


		if (!GetUserObjectInformation(threaddesktop, UOI_NAME, &threadname, 256, &dummy)) {
			CloseDesktop(inputdesktop);
			return FALSE;
		}
		if (!GetUserObjectInformation(inputdesktop, UOI_NAME, &inputname, 256, &dummy)) {
			CloseDesktop(inputdesktop);
			return FALSE;
		}

		CloseDesktop(inputdesktop);
		vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - threadname %s\n"),threadname);
		vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - inputname %s\n"),inputname);

		if (strcmp(threadname, inputname) != 0)
		{
			if (strcmp(inputname, "Screen-saver") == 0)
			{
				return SelectDesktop();
			}
			return FALSE;
		}
	}
	return TRUE;
}


bool SelectDesktop()
{
		HDESK desktop;
		HDESK old_desktop;
		DWORD dummy;
		char new_name[256];

		desktop = OpenInputDesktop(0, FALSE,
				DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
				DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
				DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
				DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);

		if (desktop == NULL) return FALSE;


		old_desktop = GetThreadDesktop(GetCurrentThreadId());


		if (!GetUserObjectInformation(desktop, UOI_NAME, &new_name, 256, &dummy)) {
			CloseDesktop(desktop);
			return FALSE;
		}

		if(!SetThreadDesktop(desktop)) {
			CloseDesktop(desktop);
			return FALSE;
		}

		CloseDesktop(old_desktop);
			
		return TRUE;
}


// WinMain parses the command line and either calls the main App
// routine or, under NT, the main service routine.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	setbuf(stderr, 0);

        //ACT: Load all messages from ressource file
        Load_Localization(hInstance) ;
        //ACT: end

	// Configure the log file, in case one is required
	vnclog.SetFile("WinVNC.log", false);
	vnclog.SetMode(2);
	vnclog.SetLevel(10);

#ifdef _DEBUG
	{
		// Get current flag
		int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );

		// Turn on leak-checking bit
		tmpFlag |= _CRTDBG_LEAK_CHECK_DF;

		// Set flag to the new value
		_CrtSetDbgFlag( tmpFlag );
	}
#endif

	// Save the application instance and main thread id
	hAppInstance = hInstance;
	mainthreadId = GetCurrentThreadId();

	// Initialise the VSocket system
	VSocketSystem socksys;
	if (!socksys.Initialised())
	{
		MessageBox(NULL, sz_ID_FAILED_INIT, szAppName, MB_OK);
		return 0;
	}
	vnclog.Print(LL_STATE, VNCLOG("sockets initialised\n"));


	// Make the command-line lowercase and parse it
	int i;
	for (i = 0; i < strlen(szCmdLine); i++)
	{
		szCmdLine[i] = tolower(szCmdLine[i]);
	}

	//MessageBox(NULL, sz_ID_FAILED_INIT, szCmdLine, MB_OK);
	//char szMsg[255];
	// wsprintf(szMsg, "***** DBG - WinMain CommandLine  %s\n", szCmdLine);
	//vnclog.Print(LL_INTINFO, szMsg); 

	BOOL argfound = FALSE;
	for (i = 0; i < strlen(szCmdLine); i++)
	{
		if (szCmdLine[i] <= ' ')
			continue;
		argfound = TRUE;

		// Now check for command-line arguments
		if (strncmp(&szCmdLine[i], winvncRunServiceHelper, strlen(winvncRunServiceHelper)) == 0)
		{
			// NB : This flag MUST be parsed BEFORE "-service", otherwise it will match
			// the wrong option!  (This code should really be replaced with a simple
			// parser machine and parse-table...)

			// Run the WinVNC Service Helper app
			vncService::PostUserHelperMessage();
			return 0;
		}
		if (strncmp(&szCmdLine[i], winvncRunService, strlen(winvncRunService)) == 0)
		{
			// Run WinVNC as a service
			return vncService::WinVNCServiceMain();
		}

		if (strncmp(&szCmdLine[i], winvncRunAsUserApp, strlen(winvncRunAsUserApp)) == 0)
		{
			// WinVNC is being run as a user-level program
			return WinVNCAppMain();
		}

		if (strncmp(&szCmdLine[i], "-0run",  strlen("-0run")) == 0)
		{
			fRunningAsApplication0 = true;
			if (!InputDesktopSelected()) return 0;
			return WinVNCAppMain();
		}

		if (strncmp(&szCmdLine[i], "windesk",  strlen("-windesk")) == 0)
		{
			fRunningAsApplication0System = true;
			if (!InputDesktopSelected()) return 0;
			return WinVNCAppMain();
		}

		if (strncmp(&szCmdLine[i], "defaultdesk",  strlen("-defaultdesk")) == 0)
		{
			fRunningAsApplication0User = true;
			if (!InputDesktopSelected()) return 0;
			return WinVNCAppMain();
		}

		if (strncmp(&szCmdLine[i], winvncAllowMulti, strlen(winvncAllowMulti)) == 0)
		{
			// WinVNC is being run as a user-level program
			AllowMulti=true;
			i+=strlen(winvncAllowMulti);
			continue;
		}

		if (strncmp(&szCmdLine[i], winvncDisableMultiWarning, strlen(winvncDisableMultiWarning)) == 0)
		{
			// WinVNC is being run as a user-level program
			DisableMultiWarning=true;
			i+=strlen(winvncDisableMultiWarning);
			vnclog.SetFile("WinVNCfus.log", false);

			return WinVNCAppMain();

			continue;
		}

		if (strncmp(&szCmdLine[i], winvncInstallService, strlen(winvncInstallService)) == 0)
		{
			// Install WinVNC as a service

			vncService::InstallService();
			i+=strlen(winvncInstallService);
			continue;
		}

		if (strncmp(&szCmdLine[i], winvncInstallServices, strlen(winvncInstallServices)) == 0)
		{
			// Install WinVNC as a service

			vncService::InstallService(1);
			i+=strlen(winvncInstallServices);
			continue;
		}

		if (strncmp(&szCmdLine[i], winvncReinstallService, strlen(winvncReinstallService)) == 0)
		{
			// Silently remove WinVNC, then re-install it
			vncService::ReinstallService();
			i+=strlen(winvncReinstallService);
			continue;
		}
		if (strncmp(&szCmdLine[i], winvncRemoveService, strlen(winvncRemoveService)) == 0)
		{
			// Remove the WinVNC service
			vncService::RemoveService();
			i+=strlen(winvncRemoveService);
			continue;
		}
		if (strncmp(&szCmdLine[i], winvncShowProperties, strlen(winvncShowProperties)) == 0)
		{
			// Show the Properties dialog of an existing instance of WinVNC
			vncService::ShowProperties();
			i+=strlen(winvncShowProperties);
			continue;
		}
		if (strncmp(&szCmdLine[i], winvncShowDefaultProperties, strlen(winvncShowDefaultProperties)) == 0)
		{
			// Show the Properties dialog of an existing instance of WinVNC
			vncService::ShowDefaultProperties();
			i+=strlen(winvncShowDefaultProperties);
			continue;
		}
		if (strncmp(&szCmdLine[i], winvncShowAbout, strlen(winvncShowAbout)) == 0)
		{
			// Show the About dialog of an existing instance of WinVNC
			vncService::ShowAboutBox();
			i+=strlen(winvncShowAbout);
			continue;
		}
		if (strncmp(&szCmdLine[i], winvncKillRunningCopy, strlen(winvncKillRunningCopy)) == 0)
		{
			// Kill any already running copy of WinVNC
			vncService::KillRunningCopy();
			i+=strlen(winvncKillRunningCopy);
			continue;
		}
		if (strncmp(&szCmdLine[i], winvncAutoReconnect, strlen(winvncAutoReconnect)) == 0)
		{
			// Note that this "autoreconnect" param MUST be BEFORE the "connect" one
			// on the command line !
			// wa@2005 -- added support for the AutoReconnectId
			i+=strlen(winvncAutoReconnect);

			int start, end;
			char* pszId = NULL;
			start = i;
			// skip any spaces and grab the parameter
			while (szCmdLine[start] <= ' ' && szCmdLine[start] != 0) start++;
			
			if ( strncmp( &szCmdLine[start], winvncAutoReconnectId, strlen(winvncAutoReconnectId) ) == 0 )
			{
				end = start;
				while (szCmdLine[end] > ' ') end++;

				pszId = new char[ end - start + 1 ];
				if (pszId != 0) 
				{
					strncpy( pszId, &(szCmdLine[start]), end - start );
					pszId[ end - start ] = 0;
					pszId = strupr( pszId );
				}

				i += strlen( pszId );
			}// end of condition we found the ID: parameter
			
			// NOTE:  id must be NULL or the ID:???? (pointer will get deleted when message is processed)
			vncService::PostAddAutoConnectClient( pszId );

			continue;
		}
		/*
		if (strncmp(&szCmdLine[i], winvncFTNoUserImpersonation, strlen(winvncFTNoUserImpersonation)) == 0)
		{
			// We disable User Impersonnation for FT thread
			// (so even a non loged user can access the whole FileSystem)
			vncService::PostAddNewClient(998, 998); // sf@2005 - I still hate to do that ;)
			i+=strlen(winvncFTNoUserImpersonation);
			continue;
		}
		*/
		if (strncmp(&szCmdLine[i], winvncAddNewClient, strlen(winvncAddNewClient)) == 0)
		{
			// Add a new client to an existing copy of winvnc
			i+=strlen(winvncAddNewClient);

			// First, we have to parse the command line to get the filename to use
			int start, end;
			start=i;
			while (szCmdLine[start] <= ' ' && szCmdLine[start] != 0) start++;
			end = start;
			while (szCmdLine[end] > ' ') end++;

			// Was there a hostname (and optionally a port number) given?
			if (end-start > 0)
			{
				char *name = new char[end-start+1];
				if (name != 0) {
					strncpy(name, &(szCmdLine[start]), end-start);
					name[end-start] = 0;

					int port = INCOMING_PORT_OFFSET;
					char *portp = strchr(name, ':');
					if (portp) {
						*portp++ = '\0';
						if (*portp == ':') {
							port = atoi(++portp);	// Port number after "::"
						} else {
							port = atoi(portp);	// Display number after ":"
						}
					}
					vnclog.Print(LL_STATE, VNCLOG("test... %s %d\n"),name,port);
					VCard32 address = VSocket::Resolve(name);
					delete [] name;
					if (address != 0) {
						// Post the IP address to the server

						vncService::PostAddNewClient(address, port);
					}
				}
				i=end;
				continue;
			}
			else 
			{
				// Tell the server to show the Add New Client dialog
				vncService::PostAddNewClient(0, 0);
			}
			continue;
		}

		// Either the user gave the -help option or there is something odd on the cmd-line!

		// Show the usage dialog
		MessageBox(NULL, winvncUsageText, sz_ID_WINVNC_USAGE, MB_OK | MB_ICONINFORMATION);
		break;
	};

	// If no arguments were given then just run
	if (!argfound)
	{
		return WinVNCAppMain();
	}

	return 0;
}

// This is the main routine for WinVNC when running as an application
// (under Windows 95 or Windows NT)
// Under NT, WinVNC can also run as a service.  The WinVNCServerMain routine,
// defined in the vncService header, is used instead when running as a service.


int WinVNCAppMain()
{
	vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - WinVNCAPPMain\n"));
#ifdef CRASH_ENABLED
	LPVOID lpvState = Install(NULL,  "rudi.de.vos@skynet.be", "UltraVnc v100 RC12G");
#endif

	// Set this process to be the last application to be shut down.
	// Check for previous instances of WinVNC!
	vncInstHandler *instancehan=new vncInstHandler;
	
	if (!instancehan->Init())
	{
		if (!AllowMulti)
		{
			// We don't allow multiple instances!
			if (!DisableMultiWarning) MessageBox(NULL, sz_ID_ANOTHER_INST, szAppName, MB_OK);
			return 0;
		}
	}

	//vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - Previous instance checked - Trying to create server\n"));
	// CREATE SERVER
	vncServer server;

	// Set the name and port number
	server.SetName(szAppName);
	vnclog.Print(LL_STATE, VNCLOG("server created ok\n"));
	///uninstall driver before cont
	
	// sf@2007 - Set Application0 special mode
	server.RunningAsApplication0(fRunningAsApplication0);
	server.RunningAsApplication0System(fRunningAsApplication0System);
	server.RunningAsApplication0User(fRunningAsApplication0User);

	// Create tray icon & menu if we're running as an app
	vncMenu *menu = new vncMenu(&server);
	if (menu == NULL)
	{
		vnclog.Print(LL_INTERR, VNCLOG("failed to create tray menu\n"));
		PostQuitMessage(0);
	}

	// Now enter the message handling loop until told to quit!
	MSG msg;
	while (GetMessage(&msg, NULL, 0,0) ) {
		//vnclog.Print(LL_INTINFO, VNCLOG("Message %d received\n"), msg.message);

		TranslateMessage(&msg);  // convert key ups and downs to chars
		DispatchMessage(&msg);
	}
	vnclog.Print(LL_STATE, VNCLOG("shutting down server\n"));

	if (menu != NULL)
		delete menu;
	if (instancehan!=NULL)
		delete instancehan;

	return msg.wParam;
};
