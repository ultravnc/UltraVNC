// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1997 AT&T Laboratories Cambridge. All Rights Reserved.
//


/////////////////////////////////////////////////////////////////////////////
// VNC Hooks library
//
// WinVNC uses this DLL to hook into the system message pipeline, allowing it
// to intercept messages which may be relevant to screen update strategy
//
// This version created:
// 24/11/97

#if !defined(_VNCHOOKS_DLL_)
#define _VNCHOOKS_DLL_

#include <windows.h>

/////////////////////////////////////////////////////////////////////////////
// Define the import/export tags

#define DllImport __declspec(dllimport)
#define DllExport __declspec(dllexport)

/////////////////////////////////////////////////////////////////////////////
//
// Functions used by WinVNC

#define VNC_HOOKS_CATCHES_ALL 0x1					// Doesn't miss updates
#define VNC_HOOKS_CATCHES_MIN 0x2					// Reports minimal updates

extern "C"
{
	// DLL functions:
	DllExport DWORD HooksType();                    // Find out whether hooks are reliable/hints
	DllExport BOOL SetHooks(
		DWORD thread_id,
		UINT UpdateMsg,
		UINT CopyMsg,
		UINT MouseMsg,
		BOOL ddihook
		);											// Set the hook
	DllExport BOOL UnSetHooks(DWORD thread_id);		// Remove it

	DllExport BOOL SetKeyboardFilterHook(BOOL activate);
													// Control keyboard filtering
	DllExport BOOL SetMouseFilterHook(BOOL activate);
													// Control mouse filtering
}

#endif // !defined(_VNCHOOKS_DLL_)
