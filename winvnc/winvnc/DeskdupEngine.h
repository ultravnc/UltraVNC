#pragma once
#include "ScreenCapture.h"
#include <tchar.h>

typedef bool(*StartW8Fn)(bool);
typedef bool (*StopW8Fn)();

typedef void (*LockW8Fn)();
typedef void (*UnlockW8Fn)();

typedef void (*ShowCursorW8Fn)();
typedef void (*HideCursorW8Fn)();

const static LPCTSTR g_szIPCSharedMMF = _T("{3DA76AC7-62E7-44AF-A8D1-45022044BB3E}");
const static LPCTSTR g_szIPCSharedMMFBitmap = _T("{0E3D996F-B070-4503-9090-198A9DA092D5}");

class DeskDupEngine : public ScreenCapture
{
public:
	DeskDupEngine();
	~DeskDupEngine();
	virtual void videoDriver_start(int x, int y, int w, int h);
	virtual void videoDriver_Stop();
	virtual bool hardwareCursor();
	virtual bool noHardwareCursor();
	virtual void Lock();
	virtual void Unlock();
private:
	HMODULE hModule;
	StartW8Fn StartW8;
	StopW8Fn StopW8;

	LockW8Fn LockW8;
	UnlockW8Fn UnlockW8;

	ShowCursorW8Fn ShowCursorW8;
	HideCursorW8Fn HideCursorW8;

	HANDLE hFileMap;
	LPVOID fileView;

	HANDLE hFileMapBitmap;
	LPVOID fileViewBitmap;
};

