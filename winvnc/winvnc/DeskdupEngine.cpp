#include "DeskdupEngine.h"
#include <stdio.h>
#include "stdhdrs.h"
//-----------------------------------------------------------
DeskDupEngine::DeskDupEngine()
{
	pSharedMemory = NULL;
	pFramebuffer = NULL;
	pChangebuf = NULL;

	hFileMap = NULL;
	fileView = 0;

	hFileMapBitmap = NULL;
	fileViewBitmap = 0;

	init = true;
	hModule = NULL;
	osVer = osVersion();
	if (osVer == OSWIN10) {
#ifdef _X64
		hModule = LoadLibrary("ddengine64.dll");
#else
		hModule = LoadLibrary("ddengine.dll");
#endif
		
		if (hModule) {
			StartW8 = (StartW8Fn)GetProcAddress(hModule, "StartW8");
			StartW8V2 = (StartW8V2Fn)GetProcAddress(hModule, "StartW8V2");
			StopW8 = (StopW8Fn)GetProcAddress(hModule, "StopW8");

			LockW8 = (LockW8Fn)GetProcAddress(hModule, "LockW8");
			UnlockW8 = (UnlockW8Fn)GetProcAddress(hModule, "UnlockW8");

			ShowCursorW8 = (ShowCursorW8Fn)GetProcAddress(hModule, "ShowCursorW8");
			HideCursorW8 = (HideCursorW8Fn)GetProcAddress(hModule, "HideCursorW8");

			if (StartW8 == NULL || StopW8 == NULL || LockW8 == NULL || UnlockW8 == NULL || ShowCursorW8 == NULL || HideCursorW8 == NULL)
				init = false;
		}
		else
			init = false;
		
	}
#ifdef _DEBUG
	char			szText[256];
	sprintf_s(szText, "DeskDupEngine\n");
	OutputDebugString(szText);
#endif
	hScreenEvent = NULL;
	hPointerEvent = NULL;
}
//-----------------------------------------------------------
DeskDupEngine::~DeskDupEngine()
{
#ifdef _DEBUG
	char			szText[256];
	sprintf_s(szText, "~DeskDupEngine\n");
	OutputDebugString(szText);
#endif
	videoDriver_Stop();
	if (osVer == OSWIN10) {
		if (hModule)
			FreeLibrary(hModule);
	}
}
//-----------------------------------------------------------
void DeskDupEngine::videoDriver_start(int x, int y, int w, int h, bool onlyVirtual, int maxFPS)
{
#ifdef _DEBUG
	char			szText[256];
	sprintf_s(szText, "DeskDupEngine Start\n");
	OutputDebugString(szText);
#endif
	oldAantal = 1;

	if (!init)
		return;
	int refw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	int refh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	bool primonly = !(refh == h && refw == w);

	if (StartW8V2) {
		if (!StartW8V2(primonly, onlyVirtual, 1000/maxFPS)) {
			vnclog.Print(LL_INTWARN, VNCLOG("DDengine V2 failed, not supported by video driver\n")); 
			return;
		}
	}
	else if(StartW8) {
		if (!StartW8(primonly)) {
			vnclog.Print(LL_INTWARN, VNCLOG("DDengine failed, not supported by video driver\n"));
			return;
		}
	}

	if (hFileMap != NULL)
		return;
	hFileMap = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, g_szIPCSharedMMF);
	if (hFileMap == NULL)
		return;

	fileView = MapViewOfFile(hFileMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
	if (fileView == NULL)
		return;

	pChangebuf = (CHANGES_BUF*)fileView;
	int size = pChangebuf->pointrect[0].rect.left;

	if (hFileMapBitmap != NULL)
		return;
	hFileMapBitmap = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, g_szIPCSharedMMFBitmap);
	if (hFileMapBitmap == NULL)
		return;

	fileViewBitmap = MapViewOfFile(hFileMapBitmap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
	if (fileViewBitmap == NULL)
		return;

	if (hScreenEvent != NULL)
		return;
	hScreenEvent = OpenEvent(EVENT_ALL_ACCESS, false, g_szIPCSharedEvent);
	if (hScreenEvent == NULL)
		return;

	if (hPointerEvent != NULL)
		return;
	hPointerEvent = OpenEvent(EVENT_ALL_ACCESS, false, g_szIPCSharedPointerEvent);
	if (hPointerEvent == NULL)
		return;

	pFramebuffer = (PCHAR)fileViewBitmap;
}
//-----------------------------------------------------------
void DeskDupEngine::videoDriver_Stop()
{
#ifdef _DEBUG
	char			szText[256];
	sprintf_s(szText, "DeskDupEngine Stop\n");
	OutputDebugString(szText);
#endif
	if (!init)
		return;	
	if (fileView)
		UnmapViewOfFile(fileView);
	fileView = NULL;
	if (hFileMap != NULL)
		CloseHandle(hFileMap);
	hFileMap = NULL;
	if (fileViewBitmap)
		UnmapViewOfFile((LPVOID)fileViewBitmap);
	fileViewBitmap = NULL;
	if (hFileMapBitmap != NULL)
		CloseHandle(hFileMapBitmap);
	hFileMapBitmap = NULL;
	if (hScreenEvent != NULL)
		CloseHandle(hScreenEvent);
	if (hPointerEvent != NULL)	
		CloseHandle(hPointerEvent);
	hScreenEvent = NULL;
	hPointerEvent = NULL;
	StopW8();
}
//-----------------------------------------------------------
void DeskDupEngine::Lock()
{
		LockW8();
}
//-----------------------------------------------------------
void DeskDupEngine::Unlock()
{
		UnlockW8();
}
//-----------------------------------------------------------
bool DeskDupEngine::hardwareCursor()
{
	ShowCursorW8();
	return true;
}
//-----------------------------------------------------------
bool DeskDupEngine::noHardwareCursor()
{
	HideCursorW8();
	return true;
}
//-----------------------------------------------------------