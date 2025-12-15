// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#include "stdhdrs.h"
#include "vncviewer.h"
#include "AccelKeys.h"

//
// Build the Accelerators Table
// 
AccelKeys::AccelKeys()
{
	ACCEL AccelTable[16];
	int i = 0;
	m_hWnd = 0;

	/*
	AccelTable[i].fVirt = FVIRTKEY | FCONTROL | FALT | FNOINVERT;
	AccelTable[i].key = VK_F3;
	AccelTable[i++].cmd = ID_CONN_CTLESC;
	*/

	AccelTable[i].fVirt = FVIRTKEY | FCONTROL | FALT | FNOINVERT;
	AccelTable[i].key = VK_F3;
	AccelTable[i++].cmd = ID_VIEWONLYTOGGLE;

	AccelTable[i].fVirt = FVIRTKEY | FCONTROL | FALT | FNOINVERT;
	AccelTable[i].key = VK_F4;
	AccelTable[i++].cmd = ID_CONN_CTLALTDEL;

	AccelTable[i].fVirt = FVIRTKEY | FCONTROL | FALT | FNOINVERT;
	AccelTable[i].key = VK_F5;
	AccelTable[i++].cmd = ID_CONN_SAVE_AS;

	AccelTable[i].fVirt = FVIRTKEY | FCONTROL | FALT | FNOINVERT;
	AccelTable[i].key = VK_F6;
	AccelTable[i++].cmd = IDC_OPTIONBUTTON;

	AccelTable[i].fVirt = FVIRTKEY | FCONTROL | FALT | FNOINVERT;
	AccelTable[i].key = VK_F7;
	AccelTable[i++].cmd = ID_FILETRANSFER;

	AccelTable[i].fVirt = FVIRTKEY | FCONTROL | FALT | FNOINVERT;
	AccelTable[i].key = VK_F8;
	AccelTable[i++].cmd = ID_TEXTCHAT;

	AccelTable[i].fVirt = FVIRTKEY | FCONTROL | FALT | FNOINVERT;
	AccelTable[i].key = VK_F9;
	AccelTable[i++].cmd = ID_DBUTTON;  // Toolbar Toggle

	AccelTable[i].fVirt = FVIRTKEY | FCONTROL | FALT | FNOINVERT;
	AccelTable[i].key = VK_F10;
	AccelTable[i++].cmd = ID_AUTOSCALING;

	AccelTable[i].fVirt = FVIRTKEY | FCONTROL | FNOINVERT;
	AccelTable[i].key = VK_F11;
	AccelTable[i++].cmd = ID_NORMALSCREEN2;

	AccelTable[i].fVirt = FVIRTKEY | FCONTROL | FALT | FNOINVERT;
	AccelTable[i].key = VK_F11;
	AccelTable[i++].cmd = ID_HALFSCREEN;

	AccelTable[i].fVirt = FVIRTKEY | FCONTROL | FALT | FNOINVERT;
	AccelTable[i].key = VK_F12;
	AccelTable[i++].cmd = ID_FULLSCREEN;

	AccelTable[i].fVirt = FVIRTKEY | FCONTROL | FALT | FNOINVERT |FSHIFT;
	AccelTable[i].key = 0x49;
	AccelTable[i++].cmd = ID_CONN_ABOUT;

	AccelTable[i].fVirt = FVIRTKEY | FCONTROL | FALT | FNOINVERT |FSHIFT;
	AccelTable[i].key = 0x52;
	AccelTable[i++].cmd = ID_REQUEST_REFRESH;

	AccelTable[i].fVirt = FVIRTKEY | FCONTROL | FALT | FNOINVERT|FSHIFT;
	AccelTable[i].key =0x4E;
	AccelTable[i++].cmd = ID_NEWCONN;

	m_hAccelTable = CreateAcceleratorTable((LPACCEL)AccelTable, i);
}

//
//
//
bool AccelKeys::TranslateAccelKeys(MSG *pMsg)
{
	if (m_hWnd == 0)
		return false;
	else
		return (TranslateAccelerator(m_hWnd, m_hAccelTable, pMsg) != 0);
}


//
// 
//
AccelKeys::~AccelKeys()
{
	DestroyAcceleratorTable(m_hAccelTable);
}
