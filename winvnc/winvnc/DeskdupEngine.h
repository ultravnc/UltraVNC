#pragma once
#include "ScreenCapture.h"
#include <tchar.h>

typedef BOOL(*StartW8Fn)(bool);
typedef BOOL(*StopW8Fn)();
typedef BOOL(*CaptureW8Fn)();
typedef void (*AutoCaptureW8Fn)(int);
typedef void (*StopAutoCaptureW8Fn)();

const static LPCTSTR g_szIPCSharedMMF = _T("{3DA76AC7-62E7-44AF-A8D1-45022044BB3E}");
const static LPCTSTR g_szIPCSharedMMFBitmap = _T("{0E3D996F-B070-4503-9090-198A9DA092D5}");

class DeskDupEngine : public ScreenCapture
{
public:
	DeskDupEngine();
	~DeskDupEngine();
	virtual void videoDriver_start(int x, int y, int w, int h);
	virtual void videoDriver_Stop();
	virtual BOOL hardwareCursor() { return false; }
	virtual BOOL noHardwareCursor() { return false; }
private:
	HMODULE hModule;
	StartW8Fn StartW8;
	StopW8Fn StopW8;
	CaptureW8Fn CaptureW8;
	AutoCaptureW8Fn AutoCaptureW8;
	StopAutoCaptureW8Fn StopAutoCaptureW8;

	HANDLE hFileMap;
	LPVOID fileView;

	HANDLE hFileMapBitmap;
	LPVOID fileViewBitmap;
};

