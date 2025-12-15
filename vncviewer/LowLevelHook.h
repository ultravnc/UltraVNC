// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//
 

// This is the source for the low-level keyboard hook, which allows intercepting and sending
// special keys (such as ALT,CTRL, ALT+TAB, etc) to the VNCServer side.
// written by Assaf Gordon (Assaf@mazleg.com), 10/9/2003


#pragma once
#include <winsock2.h>
#include <windows.h>

class LowLevelHook
{
public:
        static BOOL Initialize(HWND hwndMain);
        static BOOL Release();

		static DWORD WINAPI HookThreadProc(LPVOID lpParam);

private:
		// adzm 2009-09-25 - Different way to check the scroll lock state. Only query if we know it has changed.
        static BOOL GetCurrentScrollLockState();
		static BOOL CheckScrollLock();
        static BOOL  g_fScrollLock;
        static BOOL  g_fCheckScrollLock;

        static LRESULT CALLBACK VncLowLevelKbHookProc(INT nCode, WPARAM wParam, LPARAM lParam);

        static HWND g_hwndVNCViewer;
        static DWORD g_VncProcessID;
        static HHOOK g_HookID;

		// adzm 2009-09-25 - Hook installed from separate thread
		static HANDLE g_hThread;
		static DWORD g_nThreadID;
};