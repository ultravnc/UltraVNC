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
 

// This is the source for the low-level keyboard hook, which allows intercepting and sending
// special keys (such as ALT,CTRL, ALT+TAB, etc) to the VNCServer side.
// written by Assaf Gordon (Assaf@mazleg.com), 10/9/2003


#pragma once

//#define WINVER 0x0400
//#define _WIN32_WINNT 0x0400
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