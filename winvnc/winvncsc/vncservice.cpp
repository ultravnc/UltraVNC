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


// vncService

// Implementation of service-oriented functionality of WinVNC

#include "stdhdrs.h"

// Header

#include "vncService.h"

#include <lmcons.h>
#include "omnithread.h"
#include "WinVNC.h"
#include "vncMenu.h"

#include "localization.h" // Act : add localization on messages

// Error message logging
void LogErrorMsg(char *message);
BOOL serviceshutdown;

// OS-SPECIFIC ROUTINES

// Create an instance of the vncService class to cause the static fields to be
// initialised properly

vncService init;

DWORD	g_platform_id;
BOOL	g_impersonating_user = 0;
DWORD	g_version_major;
DWORD	g_version_minor;
DWORD   g_Session_Status;
BOOL   g_Fus;



vncService::vncService()
{
    OSVERSIONINFO osversioninfo;
    osversioninfo.dwOSVersionInfoSize = sizeof(osversioninfo);

    // Get the current OS version
    if (!GetVersionEx(&osversioninfo))
	    g_platform_id = 0;
    g_platform_id = osversioninfo.dwPlatformId;
	g_version_major = osversioninfo.dwMajorVersion;
	g_version_minor = osversioninfo.dwMinorVersion;
	serviceshutdown=false;
	g_Session_Status=0;
	g_Fus=0;
}

// CurrentUser - fills a buffer with the name of the current user!
BOOL
GetCurrentUser(char *buffer, UINT size) // RealVNC 336 change
{	// How to obtain the name of the current user depends upon the OS being used
	if ((g_platform_id == VER_PLATFORM_WIN32_NT) && vncService::RunningAsService())
	{
		// Windows NT, service-mode

		// -=- FIRSTLY - verify that a user is logged on

		// Get the current Window station
		HWINSTA station = GetProcessWindowStation();
		if (station == NULL)
			return FALSE;

		// Get the current user SID size
		DWORD usersize;
		GetUserObjectInformation(station,
			UOI_USER_SID, NULL, 0, &usersize);
		DWORD  dwErrorCode = GetLastError();
		SetLastError(0);

		// Check the required buffer size isn't zero
		if (usersize == 0)
		{
			// No user is logged in - ensure we're not impersonating anyone
			RevertToSelf();
			g_impersonating_user = FALSE;

			// Return "" as the name...
			if (strlen("") >= size)
				return FALSE;
			strcpy(buffer, "");

			return TRUE;
		}

		// -=- SECONDLY - a user is logged on but if we're not impersonating
		//     them then we can't continue!
		if (!g_impersonating_user) {
			// Return "" as the name...
			if (strlen("") >= size)
				return FALSE;
			strcpy(buffer, "");
			return TRUE;
		}
	}
		
	// -=- When we reach here, we're either running under Win9x, or we're running
	//     under NT as an application or as a service impersonating a user
	// Either way, we should find a suitable user name.

	switch (g_platform_id)
	{

	case VER_PLATFORM_WIN32_WINDOWS:
	case VER_PLATFORM_WIN32_NT:
		{
			// Just call GetCurrentUser
			DWORD length = size;

			if (GetUserName(buffer, &length) == 0)
			{
				UINT error = GetLastError();

				if (error == ERROR_NOT_LOGGED_ON)
				{
					// No user logged on
					if (strlen("") >= size)
						return FALSE;
					strcpy(buffer, "");
					return TRUE;
				}
				else
				{
					// Genuine error...
					vnclog.Print(LL_INTERR, VNCLOG("getusername error %d\n"), GetLastError());
					return FALSE;
				}
			}
		}
		return TRUE;
	};

	// OS was not recognised!
	return FALSE;
}

// RealVNC 336 change
BOOL
vncService::CurrentUser(char *buffer, UINT size)
{
  BOOL result = GetCurrentUser(buffer, size);
  if (result && (strcmp(buffer, "") == 0) && !vncService::RunningAsService()) {
    strncpy(buffer, "Default", size);
  }
  return result;
}

// IsWin95 - returns a BOOL indicating whether the current OS is Win95
BOOL
vncService::IsWin95()
{
	return (g_platform_id == VER_PLATFORM_WIN32_WINDOWS);
}

// IsWinNT - returns a bool indicating whether the current OS is WinNT
BOOL
vncService::IsWinNT()
{
	return (g_platform_id == VER_PLATFORM_WIN32_NT);
}

// Version info
DWORD
vncService::VersionMajor()
{
	return g_version_major;
}

DWORD
vncService::VersionMinor()
{
	return g_version_minor;
}

// Internal routine to find the WinVNC menu class window and
// post a message to it!

BOOL
PostToWinVNC(UINT message, WPARAM wParam, LPARAM lParam)
{
	// Locate the hidden WinVNC menu window
	HWND hservwnd = FindWindow(MENU_CLASS_NAME, NULL);
	if (hservwnd == NULL)
		return FALSE;

	// Post the message to WinVNC
	PostMessage(hservwnd, message, wParam, lParam);
	return TRUE;
}

// Static routines only used on Windows NT to ensure we're in the right desktop
// These routines are generally available to any thread at any time.

// - SelectDesktop(HDESK)
// Switches the current thread into a different desktop by deskto handle
// This call takes care of all the evil memory management involved

BOOL
vncService::SelectHDESK(HDESK new_desktop)
{
	// Are we running on NT?
	if (IsWinNT())
	{
		HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());

		DWORD dummy;
		char new_name[256];

		if (!GetUserObjectInformation(new_desktop, UOI_NAME, &new_name, 256, &dummy)) {
			return FALSE;
		}

		vnclog.Print(LL_INTERR, VNCLOG("SelectHDESK to %s (%x) from %x\n"), new_name, new_desktop, old_desktop);

		// Switch the desktop
		if(!SetThreadDesktop(new_desktop)) {
			return FALSE;
		}

		// Switched successfully - destroy the old desktop
		if (!CloseDesktop(old_desktop))
			vnclog.Print(LL_INTERR, VNCLOG("SelectHDESK failed to close old desktop %x (Err=%d)\n"), old_desktop, GetLastError());

		return TRUE;
	}

	return TRUE;
}

// - SelectDesktop(char *)
// Switches the current thread into a different desktop, by name
// Calling with a valid desktop name will place the thread in that desktop.
// Calling with a NULL name will place the thread in the current input desktop.
void
vncService::Start_new_instance()
{
		vnclog.Print(LL_INTERR, VNCLOG("New Instance \n"));
//		mytest();
}
BOOL
vncService::SelectDesktop(char *name)
{
	// Are we running on NT?
	if (IsWinNT())
	{
		HDESK desktop;
		vnclog.Print(LL_INTERR, VNCLOG("SelectDesktop \n"));
		if (name != NULL)
		{
			vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop2 named\n"));
			// Attempt to open the named desktop
			desktop = OpenDesktop(name, 0, FALSE,
				DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
				DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
				DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
				DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
		}
		else
		{
			vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop2 NULL\n"));
			// No, so open the input desktop
			desktop = OpenInputDesktop(0, FALSE,
				DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
				DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
				DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
				DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
		}
		if (g_Session_Status==1) {
				vnclog.Print(LL_INTERR, VNCLOG("g_Session_Status 1 \n"));
//				mytest();
				return TRUE;
		}

		// Did we succeed?
		if (desktop == NULL && g_Session_Status!=1) {
				vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop2 \n"));
				return FALSE;
		}
		else vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop2 OK\n"));

		// Switch to the new desktop
		if (!SelectHDESK(desktop)) {
			// Failed to enter the new desktop, so free it!
			if (!CloseDesktop(desktop))
				vnclog.Print(LL_INTERR, VNCLOG("SelectDesktop failed to close desktop\n"));
			return FALSE;
		}

		// We successfully switched desktops!
		return TRUE;
	}

	return (name == NULL);
}

// Find the visible window station and switch to it
// This would allow the service to be started non-interactive
// Needs more supporting code & a redesign of the server core to
// work, with better partitioning between server & UI components.

static HWINSTA home_window_station = GetProcessWindowStation();

BOOL CALLBACK WinStationEnumProc(LPTSTR name, LPARAM param) {
	HWINSTA station = OpenWindowStation(name, FALSE, GENERIC_ALL);
	HWINSTA oldstation = GetProcessWindowStation();
	USEROBJECTFLAGS flags;
	if (!GetUserObjectInformation(station, UOI_FLAGS, &flags, sizeof(flags), NULL)) {
		return TRUE;
	}
	BOOL visible = flags.dwFlags & WSF_VISIBLE;
	if (visible) {
		if (SetProcessWindowStation(station)) {
			if (oldstation != home_window_station) {
				CloseWindowStation(oldstation);
			}
		} else {
			CloseWindowStation(station);
		}
		return FALSE;
	}
	return TRUE;
}

BOOL
vncService::SelectInputWinStation()
{
	home_window_station = GetProcessWindowStation();
	return EnumWindowStations(&WinStationEnumProc, NULL);
}

void
vncService::SelectHomeWinStation()
{
	HWINSTA station=GetProcessWindowStation();
	SetProcessWindowStation(home_window_station);
	CloseWindowStation(station);
}

// NT only function to establish whether we're on the current input desktop

BOOL
vncService::InputDesktopSelected()
{
	// Are we running on NT?
//	if (serviceshutdown==true) return TRUE;
	if (IsWinNT())
	{
		// Get the input and thread desktops
		HDESK threaddesktop = GetThreadDesktop(GetCurrentThreadId());
		HDESK inputdesktop = OpenInputDesktop(0, FALSE,
				DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
				DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
				DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
				DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);


		// Get the desktop names:
		// *** I think this is horribly inefficient but I'm not sure.
		if (inputdesktop == NULL)
			{
				DWORD lasterror;
				lasterror=GetLastError();
				vnclog.Print(LL_INTERR, VNCLOG("OpenInputDesktop I\n"));
			    if (lasterror==170) return TRUE;
				vnclog.Print(LL_INTERR, VNCLOG("OpenInputDesktop II\n"));
				return FALSE;
			}

		DWORD dummy;
		char threadname[256];
		char inputname[256];

		if (!GetUserObjectInformation(threaddesktop, UOI_NAME, &threadname, 256, &dummy)) {
			if (!CloseDesktop(inputdesktop))
				vnclog.Print(LL_INTWARN, VNCLOG("failed to close input desktop\n"));
			return FALSE;
		}
		_ASSERT(dummy <= 256);
		if (!GetUserObjectInformation(inputdesktop, UOI_NAME, &inputname, 256, &dummy)) {
			if (!CloseDesktop(inputdesktop))
				vnclog.Print(LL_INTWARN, VNCLOG("failed to close input desktop\n"));
			return FALSE;
		}
		_ASSERT(dummy <= 256);

		if (!CloseDesktop(inputdesktop))
			vnclog.Print(LL_INTWARN, VNCLOG("failed to close input desktop\n"));

		if (strcmp(threadname, inputname) != 0)
			return FALSE;
	}

	return TRUE;
}

// Static routine used to fool Winlogon into thinking CtrlAltDel was pressed

void *
SimulateCtrlAltDelThreadFn(void *context)
{
	HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());

	// Switch into the Winlogon desktop
	if (!vncService::SelectDesktop("Winlogon"))
	{
		vnclog.Print(LL_INTERR, VNCLOG("failed to select logon desktop\n"));
		return FALSE;
	}

	vnclog.Print(LL_ALL, VNCLOG("generating ctrl-alt-del\n"));

	// Fake a hotkey event to any windows we find there.... :(
	// Winlogon uses hotkeys to trap Ctrl-Alt-Del...
	PostMessage(HWND_BROADCAST, WM_HOTKEY, 0, MAKELONG(MOD_ALT | MOD_CONTROL, VK_DELETE));

	// Switch back to our original desktop
	if (old_desktop != NULL)
		vncService::SelectHDESK(old_desktop);

	return NULL;
}

// Static routine to simulate Ctrl-Alt-Del locally

BOOL
vncService::SimulateCtrlAltDel()
{
	vnclog.Print(LL_ALL, VNCLOG("preparing to generate ctrl-alt-del\n"));

	// Are we running on NT?
	if (IsWinNT())
	{
		vnclog.Print(LL_ALL, VNCLOG("spawn ctrl-alt-del thread...\n"));

		// *** This is an unpleasant hack.  Oh dear.

		// I simulate CtrAltDel by posting a WM_HOTKEY message to all
		// the windows on the Winlogon desktop.
		// This requires that the current thread is part of the Winlogon desktop.
		// But the current thread has hooks set & a window open, so it can't
		// switch desktops, so I instead spawn a new thread & let that do the work...

		omni_thread *thread = omni_thread::create(SimulateCtrlAltDelThreadFn);
		if (thread == NULL)
			return FALSE;
		thread->join(NULL);

		return TRUE;
	}

	return TRUE;
}

// Static routine to lock a 2K or above workstation

BOOL
vncService::LockWorkstation()
{
	if (!IsWinNT()) {
		vnclog.Print(LL_INTERR, VNCLOG("unable to lock workstation - not NT\n"));
		return FALSE;
	}

	vnclog.Print(LL_ALL, VNCLOG("locking workstation\n"));

	// Load the user32 library
	HMODULE user32 = LoadLibrary("user32.dll");
	if (!user32) {
		vnclog.Print(LL_INTERR, VNCLOG("unable to load User32 DLL (%u)\n"), GetLastError());
		return FALSE;
	}

	// Get the LockWorkstation function
	typedef BOOL (*LWProc) ();
	LWProc lockworkstation = (LWProc)GetProcAddress(user32, "LockWorkStation");
	if (!lockworkstation) {
		vnclog.Print(LL_INTERR, VNCLOG("unable to locate LockWorkStation - requires Windows 2000 or above (%u)\n"), GetLastError());
		FreeLibrary(user32);
		return FALSE;
	}
	
	// Attempt to lock the workstation
	BOOL result = (lockworkstation)();

	if (!result) {
		vnclog.Print(LL_INTERR, VNCLOG("call to LockWorkstation failed\n"));
		FreeLibrary(user32);
		return FALSE;
	}

	FreeLibrary(user32);
	return result;
}

// Static routine to show the Properties dialog for a currently-running
// copy of WinVNC, (usually a servicified version.)

BOOL
vncService::ShowProperties()
{
	// Post to the WinVNC menu window
	if (!PostToWinVNC(MENU_PROPERTIES_SHOW, 0, 0))
	{

#ifndef SINGLECLICKULTRA

		MessageBox(NULL, sz_ID_NO_EXIST_INST, szAppName, MB_ICONEXCLAMATION | MB_OK);

#else
		//MessageBox(NULL, sz_ID_NO_EXIST_INST, szAppName, MB_ICONEXCLAMATION | MB_OK);
#endif

		return FALSE;
	}

	return TRUE;
}

// Static routine to show the Default Properties dialog for a currently-running
// copy of WinVNC, (usually a servicified version.)

BOOL
vncService::ShowDefaultProperties()
{
	// Post to the WinVNC menu window
	if (!PostToWinVNC(MENU_DEFAULT_PROPERTIES_SHOW, 0, 0))
	{
		MessageBox(NULL, sz_ID_NO_EXIST_INST, szAppName, MB_ICONEXCLAMATION | MB_OK);
		return FALSE;
	}

	return TRUE;
}

// Static routine to show the About dialog for a currently-running
// copy of WinVNC, (usually a servicified version.)

BOOL
vncService::ShowAboutBox()
{
	// Post to the WinVNC menu window
	if (!PostToWinVNC(MENU_ABOUTBOX_SHOW, 0, 0))
	{
		MessageBox(NULL, sz_ID_NO_EXIST_INST, szAppName, MB_ICONEXCLAMATION | MB_OK);
		return FALSE;
	}

	return TRUE;
}

// Static routine to tell a locally-running instance of the server
// to connect out to a new client

BOOL
vncService::PostAddNewClient(unsigned long ipaddress, unsigned short port)
{
	// Post to the WinVNC menu window
	if (!PostToWinVNC(MENU_ADD_CLIENT_MSG, (WPARAM)port, (LPARAM)ipaddress))
	{

#ifndef SINGLECLICKULTRA
		MessageBox(NULL, sz_ID_NO_EXIST_INST, szAppName, MB_ICONEXCLAMATION | MB_OK);
#else
		//MessageBox(NULL, sz_ID_NO_EXIST_INST, szAppName, MB_ICONEXCLAMATION | MB_OK);
#endif
		return FALSE;
	}

	return TRUE;
}

// SERVICE-MODE ROUTINES

// Service-mode defines:

// Executable name
#define VNCAPPNAME            "winvnc"

// Internal service name
#define VNCSERVICENAME        "winvnc"

// Displayed service name
#define VNCSERVICEDISPLAYNAME "VNC Server"

// List of other required services ("dependency 1\0dependency 2\0\0")
// *** These need filling in properly
#define VNCDEPENDENCIES       ""

// Internal service state
SERVICE_STATUS          g_srvstatus;       // current status of the service
SERVICE_STATUS_HANDLE   g_hstatus;
DWORD                   g_error = 0;
DWORD					g_servicethread = NULL;
char*                   g_errortext[256];

// Forward defines of internal service functions
//void WINAPI ServiceMain(DWORD argc, char **argv);

//void ServiceWorkThread(void *arg);
void ServiceStop();
void WINAPI ServiceCtrl(DWORD ctrlcode);

bool WINAPI CtrlHandler (DWORD ctrltype);

BOOL ReportStatus(DWORD state, DWORD exitcode, DWORD waithint);

// ROUTINE TO QUERY WHETHER THIS PROCESS IS RUNNING AS A SERVICE OR NOT

BOOL	g_servicemode = FALSE;

BOOL
vncService::RunningAsService()
{
	return g_servicemode;
}

BOOL
vncService::KillRunningCopy()
{
	// Locate the hidden WinVNC menu window
	HWND hservwnd;

	while ((hservwnd = FindWindow(MENU_CLASS_NAME, NULL)) != NULL)
	{
		// Post the message to WinVNC
		PostMessage(hservwnd, WM_CLOSE, 0, 0);

		omni_thread::sleep(1);
	}

	return TRUE;
}


// ROUTINE TO POST THE HANDLE OF THE CURRENT USER TO THE RUNNING WINVNC, IN ORDER
// THAT IT CAN LOAD THE APPROPRIATE SETTINGS.  THIS IS USED ONLY BY THE SVCHELPER
// OPTION, WHEN RUNNING UNDER NT
BOOL
vncService::PostUserHelperMessage()
{
	// - Check the platform type
	if (!IsWinNT())
		return TRUE;

	// - Get the current process ID
	DWORD processId = GetCurrentProcessId();

	// - Post it to the existing WinVNC
	// RealVNC 336 change
	// if (!PostToWinVNC(MENU_SERVICEHELPER_MSG, 0, (LPARAM)processId))
	//	return FALSE;
	int retries = 6;
	while (!PostToWinVNC(MENU_SERVICEHELPER_MSG, 0, (LPARAM)processId) && retries--)
    omni_thread::sleep(10);

	// - Wait until it's been used
	omni_thread::sleep(5);
	return retries;
}

// ROUTINE TO PROCESS AN INCOMING INSTANCE OF THE ABOVE MESSAGE
BOOL
vncService::ProcessUserHelperMessage(WPARAM wParam, LPARAM lParam) {
	// - Check the platform type
	if (!IsWinNT() || !vncService::RunningAsService())
		return TRUE;

	// - Close the HKEY_CURRENT_USER key, to force NT to reload it for the new user
	// NB: Note that this is _really_ dodgy if ANY other thread is accessing the key!
	if (RegCloseKey(HKEY_CURRENT_USER) != ERROR_SUCCESS) {
		vnclog.Print(LL_INTERR, VNCLOG("failed to close current registry hive\n"));
		return FALSE;
	}

	// - Revert to our own identity
	RevertToSelf();
	g_impersonating_user = FALSE;

	// Modif Jeremy C.
	if (lParam!=0) // Normal case
	{ // if lParam is zero then we are just re-setting the impersonation
      *((PHANDLE)wParam)=NULL;

      // - Open the specified process
      HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, (DWORD)lParam);
      if (processHandle == NULL) {
         vnclog.Print(LL_INTERR, VNCLOG("failed to open specified process(%d)\n"), GetLastError());
         return FALSE;
      }

      // - Get the token for the given process
      if (!OpenProcessToken(processHandle, TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE, (PHANDLE)wParam)) {
         vnclog.Print(LL_INTERR, VNCLOG("failed to get user token(%d)\n"), GetLastError());
         CloseHandle(processHandle);
         return FALSE;
      }
      CloseHandle(processHandle);

   } 
		
	// Modif Jeremy C. - Duplicate and store the security token to use later
	HANDLE userToken = NULL;
	if (!DuplicateToken(*((PHANDLE)wParam), SecurityImpersonation, &userToken))
	{
      vnclog.Print(LL_INTERR, VNCLOG("failed to duplicate user token(%d)\n"), GetLastError());
      return FALSE;
	}
	
	// - Set this thread to impersonate them
	if (!ImpersonateLoggedOnUser(userToken)) {
		vnclog.Print(LL_INTERR, VNCLOG("failed to impersonate user(%d)\n"), GetLastError());
		CloseHandle(userToken);
		return FALSE;
	}
	CloseHandle(userToken);

	g_impersonating_user = TRUE;
	vnclog.Print(LL_INTINFO, VNCLOG("impersonating logged on user\n"));
	return TRUE;
}






// Error reporting
void LogErrorMsg(char *message)
{
    char	msgbuff[256];
    HANDLE	heventsrc;
    char *	strings[2];

	// Save the error code
	g_error = GetLastError();

	// Use event logging to log the error
    heventsrc = RegisterEventSource(NULL, VNCSERVICENAME);

	_snprintf(msgbuff, 256, "%s error: %d", VNCSERVICENAME, g_error);
    strings[0] = msgbuff;
    strings[1] = message;

	if (heventsrc != NULL)
	{
		MessageBeep(MB_OK);

		ReportEvent(
			heventsrc,				// handle of event source
			EVENTLOG_ERROR_TYPE,	// event type
			0,						// event category
			0,						// event ID
			NULL,					// current user's SID
			2,						// strings in 'strings'
			0,						// no bytes of raw data
			(const char **)strings,	// array of error strings
			NULL);					// no raw data

		DeregisterEventSource(heventsrc);
	}
}


BOOL
vncService::Get_Fus()
{
	return g_Fus;
}

// FAST USER SWITCH TEST
void
vncService::Set_SessionStatus(DWORD status)
{
	g_Session_Status=status;
}
DWORD
vncService::Get_SessionStatus()
{
	return g_Session_Status;
}
void
vncService::Set_Fus(BOOL status)
{
	g_Fus=status;
}

