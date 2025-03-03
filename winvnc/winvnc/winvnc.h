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


// WinVNC header file

#include "stdhdrs.h"
#include "resource.h"

// Application specific messages

// Message used for system tray notifications
#define WM_TRAYNOTIFY				WM_APP+1

// Messages used for the server object to notify windows of things
#define WM_SRV_CLIENT_CONNECT		WM_APP+2
#define WM_SRV_CLIENT_AUTHENTICATED	WM_APP+3
#define WM_SRV_CLIENT_DISCONNECT	WM_APP+4

#define WM_MOUSESHAPE				WM_APP+6
#define WM_HOOKCHANGE				WM_APP+7
#define WM_SHUTDOWN					WM_APP+8
#define WM_REQUESTMOUSESHAPE		WM_APP+9
#define WM_UPDATEVIEWERS			WM_APP+13


// Export the application details
extern HINSTANCE	hAppInstance;
extern const char	*szAppName;
extern DWORD		mainthreadId;

// Main UltraVNC Server routine

extern int WinVNCAppMain();

// Standard command-line flag definitions
const char winvncConfig[] = "-config";
const char winvncRunService[]		= "-service_run";
const char winvncRunServiceRdp[]		= "-service_rdp_run";
const char winvncPreConnect[]		="-preconnect";
const char winvncStartService[]		= "-service";
const char winvncRunAsUserApp[]		= "-run";
const char winvncConnect[]		= "-connect";
const char winvncAutoReconnect[]	= "-autoreconnect";
const char winvncStopReconnect[]	= "-stopreconnect";
const char winvncAutoReconnectId[]	= "id:";
const char winvncReconnectId[]	= "-id:";
const char winvncSCexit[]	= "-sc_exit";
const char winvncSCprompt[]	= "-sc_prompt";
const char winvncmulti[]	= "-multi";
const char winvnchttp[]	= "-httpproxy";
const char winvncsettings[] = "-settings";

//adzm 2009-06-20
// for use with -sc
const char winvncRepeater[]	= "-repeater"; // set default repeater host
extern char* g_szRepeaterHost;

const char winvncStopserviceHelper[]		= "-stopservicehelper";
const char winvncStopservice[]				= "-stopservice";
const char winvncStartserviceHelper[]		= "-startservicehelper";
const char winvncStartservice[]				= "-startservice";

const char winvncInstallService[]			= "-install";
const char winvncInstallDriver[]			= "-installdriver";
const char winvncUnInstallService[]			= "-uninstall";
const char winvncInstallServiceHelper[]		= "-installhelper";
const char winvncUnInstallServiceHelper[]	= "-uninstallhelper";

const char winvncSoftwarecad[]				= "-softwarecad";
const char winvncSoftwarecadHelper[]		= "-softwarecadhelper";

const char winvncRebootSafe[]				= "-rebootsafemode";
const char winvncRebootSafeHelper[]			= "-rebootsafemodehelper";

const char winvncRebootForce[]				= "-rebootforce";
const char winvncRebootForceHelper[]		= "-rebootforcehelper";

const char winvncdelSoftwarecad[]			= "-delsoftwarecad";
const char winvncdelSoftwarecadHelper[]		= "-delsoftwarecadhelper";

const char winvncSecurityEditorHelper[]		= "-securityeditorhelper";
const char winvncSecurityEditor[]			= "-securityeditor";
const char winvncKill[]						= "-kill";
const char winvncopenhomepage[]				= "-openhomepage";
const char winvncopenforum[]				= "-openforum";
const char winvncopengithub[]				= "-opengithub";
const char winvncopenmastodon[]				= "-openmastodon";
const char winvncopenbluesky[]				= "-openbluesky";
const char winvncopenfacebook[]				= "-openfacebook";
const char winvncopenxtwitter[]				= "-openxtwitter";
const char winvncopenreddit[]				= "-openreddit";
const char winvncopenopenhub[]				= "-openopenhub";

// Usage string
const char winvncUsageText[]		= "winvnc [-sc_prompt] [-sc_exit] [-id:????] [-stopreconnect][-autoreconnect[ ID:????]] [-connect host[:display]] [-connect host[::port]] [-repeater host[:port]][-run]\n";
