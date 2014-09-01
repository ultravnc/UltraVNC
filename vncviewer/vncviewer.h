/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2013 UltraVNC Team Members. All Rights Reserved.
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
// If the source code for the program is not available from the place from
// which you received this file, check 
// http://www.uvnc.com/
//
////////////////////////////////////////////////////////////////////////////
 

#ifndef VNCVIEWER_H__
#define VNCVIEWER_H__

#pragma once

#include "res\resource.h"
#include "VNCviewerApp.h"
#include "Log.h"
#include "AccelKeys.h"

#define WM_SOCKEVENT WM_APP+1
#define WM_TRAYNOTIFY WM_SOCKEVENT+1
//adzm 2010-09
#define WM_REQUESTUPDATE WM_TRAYNOTIFY+1
	// WM_REQUESTUPDATE (wParam, lParam)
	// wParam: 
	//		0x00000000 = Full Framebuffer Update Request
	//		0x00000001 = Incremental Framebuffer Update Request
	//		0xFFFFFFFF = 'Appropriate' Framebuffer Update Request
//adzm 2010-09
#define WM_UPDATEREMOTECLIPBOARDCAPS WM_REQUESTUPDATE+1
#define WM_NOTIFYPLUGINSTREAMING WM_UPDATEREMOTECLIPBOARDCAPS+1
//adzm 2010-09
#define WM_SENDKEEPALIVE WM_NOTIFYPLUGINSTREAMING+1

// The Application
extern VNCviewerApp *pApp;

// Global logger - may be used by anything
extern Log vnclog;
extern AccelKeys TheAccelKeys;

// Display given window in centre of screen
void CentreWindow(HWND hwnd);

// Convert "host:display" into host and port
// Returns true if valid.
bool ParseDisplay(LPTSTR display, LPTSTR phost, int hostlen, int *port);

// Macro DIALOG_MAKEINTRESOURCE is used to allow both normal windows dialogs
// and the selectable aspect ratio dialogs under WinCE (PalmPC vs HPC).
#ifndef UNDER_CE
#define DIALOG_MAKEINTRESOURCE MAKEINTRESOURCE
#else
// Under CE we pick dialog resource according to the 
// screen format selected or determined.
#define DIALOG_MAKEINTRESOURCE(res) SELECT_MAKEINTRESOURCE(res ## _PALM, res)
inline LPTSTR SELECT_MAKEINTRESOURCE(WORD res_palm, WORD res_hpc)
{
	if (pApp->m_options.m_palmpc)
		return MAKEINTRESOURCE(res_palm);
	else
		return MAKEINTRESOURCE(res_hpc);
}
#endif

#endif