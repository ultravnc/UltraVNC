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


// vncSetAuth

// Object implementing the About dialog for UltraVNC Server.
#include "common/inifile.h"
class vncSetAuth;

#if (!defined(_WINVNC_VNCSETAUTH))
#define _WINVNC_VNCSETAUTH

// Includes
#include "stdhdrs.h"
#include "vncserver.h"

// The vncSetAuth class itself
class vncSetAuth
{
public:
	// Constructor/destructor
	vncSetAuth();

	// Initialisation
	BOOL Init();

	// The dialog box window proc
	static BOOL CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// General
	void Show(BOOL show);

	// Implementation
	BOOL m_dlgvisible;
};

#endif // _WINVNC_VNCPROPERTIES
