// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


class vncListDlg;

#if (!defined(_WINVNC_VNCLISTDLG))
#define _WINVNC_VNCLISTDLG

#include "stdhdrs.h"
#include "vncserver.h"


class vncListDlg
{
public:
	// Constructor / destructor
	vncListDlg();
	~vncListDlg();

	BOOL Init(vncServer* pServer);
	static BOOL CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void Display();

	vncServer* m_pServer;
	BOOL m_dlgvisible;	
};

#endif
