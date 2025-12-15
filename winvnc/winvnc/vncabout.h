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


// vncAbout

// Object implementing the About dialog for UltraVNC Server.

class vncAbout;

#if (!defined(_WINVNC_VNCABOUT))
#define _WINVNC_VNCABOUT

// Includes
#include "stdhdrs.h"

// The vncAbout class itself
class vncAbout
{
public:
	// Constructor/destructor
	vncAbout();
	~vncAbout();

	// Initialisation
	BOOL Init();

	// The dialog box window proc
	static BOOL CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// General
	void Show(BOOL show);

	// Implementation
	BOOL m_dlgvisible;
};

#endif // _WINVNC_VNCABOUT
