#include "ScreenCapture.h"

//just make it run with SDK 8.1
#ifndef _WIN32_WINNT_WINTHRESHOLD
#define _WIN32_WINNT_WINTHRESHOLD           0x0A00 
VERSIONHELPERAPI
IsWindows10OrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WINTHRESHOLD), LOBYTE(_WIN32_WINNT_WINTHRESHOLD), 0);
}
#endif

//----------------------------------------------------------
ScreenCapture::ScreenCapture()
{
	int a = 0;
}
//----------------------------------------------------------
int ScreenCapture::osVersion()
{
	int version = OSOLD;
	if (IsWindowsXPSP3OrGreater())
		version = OSWINXP;
	if (IsWindowsVistaOrGreater())
		version = OSVISTA;
	if (IsWindows8OrGreater())
		version = OSWIN10;
	return version;
}
