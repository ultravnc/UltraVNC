// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#include <winsock2.h>
#include <windows.h>
#include "vncOSVersion.h"
#include "stdhdrs.h"
#include "dwmapi.h"
#ifndef SUCCEEDED
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#endif
VNC_OSVersion* VNC_OSVersion::vnc_OSVersion = nullptr;


typedef LONG NTSTATUS, *PNTSTATUS;
#define STATUS_SUCCESS (0x00000000)

typedef NTSTATUS (WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

RTL_OSVERSIONINFOW GetRealOSVersion() {
    HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
    if (hMod) {
        RtlGetVersionPtr fxPtr = (RtlGetVersionPtr)::GetProcAddress(hMod, "RtlGetVersion");
        if (fxPtr != nullptr) {
            RTL_OSVERSIONINFOW rovi = { 0 };
            rovi.dwOSVersionInfoSize = sizeof(rovi);
            if ( STATUS_SUCCESS == fxPtr(&rovi) ) {
                return rovi;
            }
        }
    }
    RTL_OSVERSIONINFOW rovi = { 0 };
    return rovi;
}


VNC_OSVersion::VNC_OSVersion()
{
	AeroWasEnabled=false;
	pfnDwmIsCompositionEnabled=NULL;
	pfnDwmEnableComposition = NULL;
	DMdll = NULL; 
	OS_AERO_ON=false;
	OS_LAYER_ON=false;
	OS_WIN8=false;
	OS_WIN7=false;
	OS_VISTA=false;
	OS_XP=false;
	OS_W2K=false;
	OS_WIN10=false;
	OS_NOTSUPPORTED=false;
	OS_BEFOREVISTA = false;
	OS_WIN10_TRANS = false;
	OSVERSIONINFO OSversion;	
	OSversion.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	GetVersionEx(&OSversion);
	OS_WINPE = isWINPE();

	switch(OSversion.dwPlatformId)
	{
		case VER_PLATFORM_WIN32_NT:
								  if ((OSversion.dwMajorVersion==6 && OSversion.dwMinorVersion>=2) || OSversion.dwMajorVersion > 6 ) OS_WIN8=true;
								  else if(OSversion.dwMajorVersion==6 && OSversion.dwMinorVersion==1) OS_WIN7=true;
								  else if(OSversion.dwMajorVersion==6 && OSversion.dwMinorVersion==0) OS_VISTA=true;
								  else if (OSversion.dwMajorVersion == 5 && OSversion.dwMinorVersion == 2) { OS_XP = true; OS_BEFOREVISTA = true; }
								  else if (OSversion.dwMajorVersion == 5 && OSversion.dwMinorVersion == 1) { OS_XP = true; OS_BEFOREVISTA = true; }
								  else if (OSversion.dwMajorVersion == 5 && OSversion.dwMinorVersion == 0) { OS_W2K = true; OS_BEFOREVISTA = true; }
								  else OS_NOTSUPPORTED=true;
								  break;
		case VER_PLATFORM_WIN32_WINDOWS:
								OS_NOTSUPPORTED=true;
								break;
	}

	OS_MINIMUMVISTA = !OS_BEFOREVISTA;

	if (OS_WIN8)
	{
		RTL_OSVERSIONINFOW rTL_OSVERSIONINFOW;
		rTL_OSVERSIONINFOW = GetRealOSVersion();
		if (rTL_OSVERSIONINFOW.dwMajorVersion == 10) {
			OS_WIN8 = false;
			OS_WIN10 = true;
			if (rTL_OSVERSIONINFOW.dwBuildNumber >= 19041)
				OS_WIN10_TRANS = true;
		}
	}
	LoadDM();
}

VNC_OSVersion::~VNC_OSVersion()
{
	ResetAero();
	UnloadDM();
}

void
VNC_OSVersion::SetAeroState()
{
	//are layered windows supported
	typedef DWORD (WINAPI *PSLWA)(HWND, DWORD, BYTE, DWORD);
	PSLWA pSetLayeredWindowAttributes=NULL;
	HMODULE hDLL = LoadLibrary ("user32");
	if (hDLL) pSetLayeredWindowAttributes = (PSLWA) GetProcAddress(hDLL,"SetLayeredWindowAttributes");
	if (pSetLayeredWindowAttributes) OS_LAYER_ON=true;
	else OS_LAYER_ON=false;
	//is aero on/off
	BOOL pfnDwmEnableCompositiond = FALSE;
	if (SUCCEEDED(DwmIsCompositionEnabled(&pfnDwmEnableCompositiond)))
		OS_AERO_ON = pfnDwmEnableCompositiond;
	else
		OS_AERO_ON = false;
}

bool
VNC_OSVersion::CaptureAlphaBlending()
{
	if (OS_WINPE) {
		return true; //WINPE
	}
	if (OS_LAYER_ON == false) return false;
	if (OS_XP) return true;
	if ((OS_WIN7 || OS_VISTA) && OS_AERO_ON == false) return true;
	if (OS_LAYER_ON == true && OS_AERO_ON == false) return true; 
	return false;
}

void
VNC_OSVersion::UnloadDM(VOID) 
 {  
	pfnDwmEnableComposition = NULL; 
	pfnDwmIsCompositionEnabled = NULL; 
	if (DMdll) FreeLibrary(DMdll); 
    DMdll = NULL; 
 } 

bool
VNC_OSVersion::LoadDM(VOID) 
 { 
         if (DMdll) return TRUE;   
         DMdll = LoadLibraryA("dwmapi.dll"); 
         if (!DMdll) return FALSE;   
         pfnDwmIsCompositionEnabled = (P_DwmIsCompositionEnabled)GetProcAddress(DMdll, "DwmIsCompositionEnabled");  
		 pfnDwmEnableComposition = (P_DwmEnableComposition)GetProcAddress(DMdll, "DwmEnableComposition");
         return TRUE; 
 } 

void
VNC_OSVersion::DisableAero(VOID) 
 { 

	     BOOL pfnDwmEnableCompositiond = FALSE;   
         if (!(SUCCEEDED(DwmIsCompositionEnabled(&pfnDwmEnableCompositiond))))  
			 return; 
         if (!pfnDwmEnableCompositiond) 
			 return;   
		 if (SUCCEEDED(DwmEnableComposition(FALSE))) {			  
			AeroWasEnabled = pfnDwmEnableCompositiond;
		  }

		pfnDwmEnableCompositiond = FALSE;
		if (pfnDwmIsCompositionEnabled==NULL) OS_AERO_ON=false;
		else if (SUCCEEDED(pfnDwmIsCompositionEnabled(&pfnDwmEnableCompositiond))) OS_AERO_ON=pfnDwmEnableCompositiond;
		else OS_AERO_ON=false;	
		 
 } 
  
void
VNC_OSVersion::ResetAero(VOID) 
 { 
         if (pfnDwmEnableComposition && AeroWasEnabled) pfnDwmEnableComposition(AeroWasEnabled);
		 BOOL pfnDwmEnableCompositiond = FALSE;
	     if (pfnDwmIsCompositionEnabled==NULL) OS_AERO_ON=false;
		 else if (SUCCEEDED(pfnDwmIsCompositionEnabled(&pfnDwmEnableCompositiond))) OS_AERO_ON=pfnDwmEnableCompositiond;
		 else OS_AERO_ON=false;	
        
 } 

HWND GetConsoleHwnd(void)
{
	return FindWindow("ConsoleWindowClass", NULL);
}

BOOL CALLBACK speichereFenster(HWND hwnd, LPARAM substring) {
	if (WS_EX_LAYERED == (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_LAYERED)) {
		SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED);
		RedrawWindow(hwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
	}
	return true;
}

bool
VNC_OSVersion::isWINPE(VOID)
{
	HKEY hCurrentVersion = NULL;
	bool winpe = (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\WinPE", 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &hCurrentVersion) == ERROR_SUCCESS);	
	return winpe;
}

void VNC_OSVersion::removeAlpha()
{
	EnumWindows(speichereFenster, NULL);
}