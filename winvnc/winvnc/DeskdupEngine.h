/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
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
//  If the source code for the program is not available from the place from
//  which you received this file, check
//  https://uvnc.com/
//
////////////////////////////////////////////////////////////////////////////


#pragma once
#include "ScreenCapture.h"
#include <tchar.h>
#include <memory>

typedef bool(*StartW8Fn)(bool);
typedef bool(*StartW8V2Fn)(bool, bool, UINT);
typedef bool (*StopW8Fn)();

typedef void (*LockW8Fn)();
typedef void (*UnlockW8Fn)();

typedef void (*ShowCursorW8Fn)();
typedef void (*HideCursorW8Fn)();

const static LPCTSTR g_szIPCSharedMMF = _T("{3DA76AC7-62E7-44AF-A8D1-45022044BB3E}");
const static LPCTSTR g_szIPCSharedMMFBitmap = _T("{0E3D996F-B070-4503-9090-198A9DA092D5}");
const static LPCTSTR g_szIPCSharedEvent = _T("{3BFBA3A0-2133-48B5-B5BD-E58C72853FFB}");
const static LPCTSTR g_szIPCSharedPointerEvent = _T("{3A77E11C-B0B4-40F9-BC8B-D249116A76FE}");

#ifdef SC_20
	class LoadDllFromMemory;
#endif // SC_20

class DeskDupEngine : public ScreenCapture
{
public:
	DeskDupEngine();
	~DeskDupEngine();
	virtual void videoDriver_start(int x, int y, int w, int h, bool onlyVirtual, int maxFPS);
	virtual void videoDriver_Stop();
	virtual bool hardwareCursor();
	virtual bool noHardwareCursor();
	virtual void Lock();
	virtual void Unlock();
	virtual HANDLE getHScreenEvent(){return hScreenEvent;}
	virtual HANDLE getHPointerEvent(){return hPointerEvent;}
private:
	HMODULE hModule;
	StartW8Fn StartW8;
	StartW8V2Fn StartW8V2;
	StopW8Fn StopW8;

	LockW8Fn LockW8;
	UnlockW8Fn UnlockW8;

	ShowCursorW8Fn ShowCursorW8;
	HideCursorW8Fn HideCursorW8;

	HANDLE hFileMap;
	LPVOID fileView;

	HANDLE hFileMapBitmap;
	LPVOID fileViewBitmap;
	HANDLE hScreenEvent;
	HANDLE hPointerEvent;
#ifdef SC_20
	std::unique_ptr<LoadDllFromMemory> loadDllFromMemory;
#endif // SC_20
};
