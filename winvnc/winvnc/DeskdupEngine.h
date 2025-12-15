// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


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
