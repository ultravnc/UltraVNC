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


class vncConnDialog;

#if (!defined(_WINVNC_VNCCONNDIALOG))
#define _WINVNC_VNCCONNDIALOG

#pragma once

#include "vncserver.h"

// Outgoing connection dialog. This allows people running UltraVNC Servers on
// Win32 platforms to _push_ their displays out to other people's screens
// rather than having to _pull_ them across.

class vncConnDialog  
{
public:

	// Create an outgoing-connection dialog
	vncConnDialog(vncServer *server);

	// Destructor
	virtual ~vncConnDialog();

	// Once a dialog object is created, either delete it again, or
	// call DoDialog. DoDialog will run the object and delete it when done
	//adzm 2009-06-20 - Return the result
	INT_PTR DoDialog(bool rep = false);

	// Internal stuffs
private:

	// Routine to call when a dialog event occurs
	static BOOL CALLBACK vncConnDlgProc(HWND hwndDlg,
										UINT uMsg, 
										WPARAM wParam,
										LPARAM lParam);

	// Pointer back to the server object
	vncServer *m_server;

	//adzm 2009-06-20
	HICON m_hicon;
	HFONT m_hfont;
	bool m_repeater;
};

#endif
