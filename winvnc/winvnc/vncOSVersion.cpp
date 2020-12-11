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

#include <winsock2.h>
#include <windows.h>
#include "vncOSVersion.h"
#ifndef SUCCEEDED
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#endif


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
	OSVERSIONINFO OSversion;	
	OSversion.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	GetVersionEx(&OSversion);

	switch(OSversion.dwPlatformId)
	{
		case VER_PLATFORM_WIN32_NT:
								  if (OSversion.dwMajorVersion==6 && OSversion.dwMinorVersion>=2) OS_WIN8=true;
								  else if(OSversion.dwMajorVersion==6 && OSversion.dwMinorVersion==1) OS_WIN7=true;
								  else if(OSversion.dwMajorVersion==6 && OSversion.dwMinorVersion==0) OS_VISTA=true;
								  else if(OSversion.dwMajorVersion==5 && OSversion.dwMinorVersion==2) OS_XP=true;
								  else if(OSversion.dwMajorVersion==5 && OSversion.dwMinorVersion==1) OS_XP=true;								  
								  else if(OSversion.dwMajorVersion==5 && OSversion.dwMinorVersion==0) OS_W2K=true;
								  else if (OSversion.dwMajorVersion==10) OS_WIN10=true;
								  else OS_NOTSUPPORTED=true;
								  break;
		case VER_PLATFORM_WIN32_WINDOWS:
								OS_NOTSUPPORTED=true;
								break;
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
	if (pfnDwmIsCompositionEnabled==NULL) OS_AERO_ON=false;
	else if (SUCCEEDED(pfnDwmIsCompositionEnabled(&pfnDwmEnableCompositiond))) OS_AERO_ON=pfnDwmEnableCompositiond;
	else OS_AERO_ON=false;	
}

bool
VNC_OSVersion::CaptureAlphaBlending()
{
	if (OS_LAYER_ON == false) return false;
	if (OS_XP) return true;
	if ((OS_WIN7 || OS_VISTA) && OS_AERO_ON == false) return true;
	if (OS_LAYER_ON == true && OS_AERO_ON == false) return true; //WINPE
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
         if (!(pfnDwmIsCompositionEnabled && SUCCEEDED(pfnDwmIsCompositionEnabled(&pfnDwmEnableCompositiond))))  return; 
         if (!pfnDwmEnableCompositiond) return;   
		 if (pfnDwmEnableComposition && SUCCEEDED(pfnDwmEnableComposition(FALSE))) {			  
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
