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


// vncTimedMsgBox.h

// Static class used to generate message boxes which allow the program's
// execution to continue after a set delay (4 seconds at present)

class vncTimedMsgBox;

#if(!defined(_WINVNC_VNCTIMEDMSGBOX))
#define _WINVNC_VNCTIMEDMSGBOX

class vncTimedMsgBox
{
public:
	// Bring up a message box, wait for two seconds, then return
	static void Do(const char *caption, const char *title, UINT type);
};

#endif

