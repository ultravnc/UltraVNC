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

class vncAcceptDialog;

#if (!defined(_WINVNC_VNCACCEPTDIALOG))
#define _WINVNC_VNCACCEPTDIALOG

#pragma once

// Incoming connection-rejection dialog.  vncClient creates an AcceptDialog
// if it needs to query whether or not to accept a connection.

class vncAcceptDialog  
{
public:

	vncAcceptDialog(UINT timeoutSecs,BOOL acceptOnTimeout, const char *ipAddress, char* infoMsg, bool notification);
	virtual ~vncAcceptDialog();
	BOOL DoDialog();
	BOOL m_acceptOnTimeout;
	static BOOL CALLBACK vncAcceptDlgProc(HWND hwndDlg,UINT uMsg, WPARAM wParam,LPARAM lParam);
private:
	// Storage for the timeout value
	UINT m_timeoutSecs;
	UINT m_timeoutCount;

	// Flashing hack
	BOOL m_foreground_hack;
	BOOL m_flash_state;

	// Address of the offending machine
	char *m_ipAddress;	
	HANDLE ThreadHandle;
	char *infoMsg;
	bool notification;
};

#endif
