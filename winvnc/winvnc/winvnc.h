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
extern char winvncFolder[MAX_PATH];

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
