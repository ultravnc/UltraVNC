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

#include "stdhdrs.h"
#include "VideoDriver.h"
#include <strsafe.h>

LPSTR driverName = "mv video hook driver2";

#define MAP1 1030
#define UNMAP1 1031
#define CURSOREN 1060
#define CURSORDIS 1061
//----------------------------------------------------------
//----------------------------------------------------------
VideoDriver::VideoDriver()
{
	init = true;
	pSharedMemory = NULL;
	pChangebuf = NULL;
	pFramebuffer = NULL;
	shared_buffer_size = 0;
	osVer = osVersion();
}
//----------------------------------------------------------
VideoDriver::~VideoDriver()
{
	if (osVer == OSWINXP) {
		mirror_driver_detach_XP();
		if (pSharedMemory != NULL) videoMemory_ReleaseSharedMemory(pSharedMemory);
	}
	if (osVer == OSVISTA) {
		mirror_driver_Vista(0, 0, 0, 0, 0);
		if (pSharedMemory != NULL) videoMemory_ReleaseSharedMemory(pSharedMemory);
	}	
}
//----------------------------------------------------------
void VideoDriver::videoDriver_start(int x, int y, int w, int h)
{
	if (init)
		return;
	blocked = true;
	oldAantal = 1;
	pSharedMemory = NULL;
	if (osVer == OSWINXP) {
		if (mirror_driver_attach_XP(x, y, w, h)) {
			if (getDcMirror() != NULL) {
				pSharedMemory = videoMemory_GetSharedMemory();
				pChangebuf = (PCHANGES_BUF)pSharedMemory;
				pFramebuffer = pSharedMemory + sizeof(CHANGES_BUF);
			}
			else {
				pSharedMemory = NULL;
			}
		}
		else
			pSharedMemory = NULL;
	}

	if (osVer == OSVISTA) {
		if (mirror_driver_Vista(1, x, y, w, h)) {
			if (getDcMirror() != NULL) {
				pSharedMemory = videoMemory_GetSharedMemory();
				pChangebuf = (PCHANGES_BUF)pSharedMemory;
				pFramebuffer = pSharedMemory + sizeof(CHANGES_BUF);
			}
			else {
				if (mirror_driver_attach_XP(x, y, w, h)) {
					if (getDcMirror() != NULL) {
						pSharedMemory = videoMemory_GetSharedMemory();
						pChangebuf = (PCHANGES_BUF)pSharedMemory;
						pFramebuffer = pSharedMemory + sizeof(CHANGES_BUF);
					}
					else
						pSharedMemory = NULL;
				}
				else
					pSharedMemory = NULL;
			}
		}
		else
			pSharedMemory = NULL;
	}
	blocked = false;
}
//----------------------------------------------------------
void VideoDriver::videoDriver_Stop()
{
	if (init)
		return;
	if (osVer == OSWINXP) {
		mirror_driver_detach_XP();
		if (pSharedMemory != NULL) videoMemory_ReleaseSharedMemory(pSharedMemory);
	}
	if (osVer == OSVISTA) {
		mirror_driver_Vista(0, 0, 0, 0, 0);
		if (pSharedMemory != NULL) videoMemory_ReleaseSharedMemory(pSharedMemory);
	}
	pSharedMemory = NULL;
	pChangebuf = NULL;
	pFramebuffer = NULL;
}
//----------------------------------------------------------
BOOL VideoDriver::hardwareCursor()
{
	HDC gdc;
	int returnvalue;
	gdc = GetDC(NULL);
	returnvalue = ExtEscape(gdc, MAP1, 0, NULL, NULL, NULL);
	returnvalue = ExtEscape(gdc, CURSOREN, 0, NULL, NULL, NULL);
	ReleaseDC(NULL, gdc);
	return true;
}
//----------------------------------------------------------
BOOL VideoDriver::noHardwareCursor()
{
	HDC gdc;
	int returnvalue;
	gdc = GetDC(NULL);
	returnvalue = ExtEscape(gdc, CURSORDIS, 0, NULL, NULL, NULL);
	ReleaseDC(NULL, gdc);
	return true;
}
//----------------------------------------------------------
// Private 
//----------------------------------------------------------
void VideoDriver::videoMemory_ReleaseSharedMemory(PCHAR pVideoMemory)
{
	UnmapViewOfFile(pVideoMemory);
}
//----------------------------------------------------------
PCHAR VideoDriver::videoMemory_GetSharedMemory(void)
{
	PCHAR pVideoMemory = NULL;
	HANDLE hMapFile, hFile, hFile0, hFile1;
	hFile = NULL;

	hFile0 = CreateFile("c:\\video0.dat", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	hFile1 = CreateFile("c:\\video1.dat", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	if ((hFile0 && hFile0 != INVALID_HANDLE_VALUE) && !(hFile1 && hFile1 != INVALID_HANDLE_VALUE)) hFile = hFile0;
	if ((hFile1 && hFile1 != INVALID_HANDLE_VALUE) && !(hFile0 && hFile0 != INVALID_HANDLE_VALUE)) hFile = hFile1;
	if ((hFile0 && hFile0 != INVALID_HANDLE_VALUE) && (hFile1 && hFile1 != INVALID_HANDLE_VALUE)) {
		DWORD size0 = GetFileSize(hFile0, NULL);
		DWORD size1 = GetFileSize(hFile1, NULL);
		if (size0 == shared_buffer_size) hFile = hFile0;
		if (size1 == shared_buffer_size) hFile = hFile1;
		if (size1 == shared_buffer_size && size0 == shared_buffer_size) {
			//find last modification time
			FILETIME createt0, lastWriteTime0, lastAccessTime0;
			GetFileTime(hFile0, &createt0, &lastAccessTime0, &lastWriteTime0);
			FILETIME createt1, lastWriteTime1, lastAccessTime1;
			GetFileTime(hFile0, &createt1, &lastAccessTime1, &lastWriteTime1);
			LONG result = CompareFileTime(&lastWriteTime0, &lastWriteTime1);
			if (result == 0 || result == 1) hFile = hFile0;
			else hFile = hFile1;
		}
	}

	if (hFile && hFile != INVALID_HANDLE_VALUE) {
		hMapFile = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
		if (hMapFile && hMapFile != INVALID_HANDLE_VALUE) {
			pVideoMemory = (char *)MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 0);
			CloseHandle(hMapFile);
		}
		CloseHandle(hFile);
	}
	else
		vnclog.Print(LL_INTERR, VNCLOG("Error video.dat \n"));
	return pVideoMemory;
}
//----------------------------------------------------------
bool VideoDriver::mirror_driver_Vista(DWORD dwAttach, int x, int y, int w, int h)
{
	HDESK   hdeskInput = NULL;
	HDESK   hdeskCurrent = NULL;
	BOOL  bED = TRUE;
	DEVMODE devmode;

	FillMemory(&devmode, sizeof(DEVMODE), 0);
	devmode.dmSize = sizeof(DEVMODE);
	devmode.dmDriverExtra = 0;

	// Make sure we have a display on this thread.
	BOOL change = EnumDisplaySettings(NULL,
		ENUM_CURRENT_SETTINGS,
		&devmode);

	devmode.dmFields = DM_BITSPERPEL |
		DM_PELSWIDTH |
		DM_PELSHEIGHT |
		DM_POSITION;

	if (change) {
		DISPLAY_DEVICE dispDevice;
		FillMemory(&dispDevice, sizeof(DISPLAY_DEVICE), 0);
		dispDevice.cb = sizeof(DISPLAY_DEVICE);
		LPSTR deviceName = NULL;
		devmode.dmDeviceName[0] = '\0';
		INT devNum = 0;
		BOOL result;
		DWORD cxPrimary = 0xFFFFFFFF;
		DWORD cyPrimary = 0xFFFFFFFF;
		// First enumerate for Primary display device:
		while (result = EnumDisplayDevicesA(NULL, devNum, &dispDevice, 0)) {
			//          MessageBoxSecure(NULL, &dispDevice.DeviceString[0], NULL, MB_OK);
			if (dispDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) {
				// Primary device. Find out its dmPelsWidht and dmPelsHeight.
				EnumDisplaySettings(dispDevice.DeviceName, ENUM_CURRENT_SETTINGS, &devmode);
				cxPrimary = devmode.dmPelsWidth;
				cyPrimary = devmode.dmPelsHeight;
				break;
			}
			devNum++;
		}
#if defined(MIRROR_24)
		if (devmode.dmBitsPerPel == 24)
			devmode.dmBitsPerPel = 32;
#endif
		if (devmode.dmBitsPerPel != 8 && devmode.dmBitsPerPel != 16 && devmode.dmBitsPerPel != 32)
			return false;
		shared_buffer_size = devmode.dmBitsPerPel / 8 * w*h + sizeof(CHANGES_BUF);
		// error check
		if (!result)
			return false;
		if (cxPrimary == 0xffffffff || cyPrimary == 0xffffffff)
			return false;

		// Enumerate again for the mirror driver:
		devNum = 0;
		while (result = EnumDisplayDevicesA(NULL, devNum, &dispDevice, 0)) {
			if (strcmp(&dispDevice.DeviceString[0], driverName) == 0)
				break;
			devNum++;
		}

		// error check       
		if (!result)
			return false;

		CHAR deviceNum[MAX_PATH];
		LPSTR deviceSub;

		// Simply extract 'DEVICE#' from registry key.  This will depend
		// on how many mirrored devices your driver has and which ones
		// you intend to use.

		_strupr(&dispDevice.DeviceKey[0]);
		deviceSub = strstr(&dispDevice.DeviceKey[0],
			"\\DEVICE");
		if (!deviceSub)
			StringCbCopy(&deviceNum[0], sizeof(deviceNum), "DEVICE0");
		else
			StringCbCopy(&deviceNum[0], sizeof(deviceNum), ++deviceSub);

		// Reset the devmode for mirror driver use:
		int depth = devmode.dmBitsPerPel;
		FillMemory(&devmode, sizeof(DEVMODE), 0);
		devmode.dmSize = sizeof(DEVMODE);
		devmode.dmDriverExtra = 0;
		devmode.dmBitsPerPel = depth;
		devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_POSITION;

		StringCbCopy((LPSTR)&devmode.dmDeviceName[0], sizeof(devmode.dmDeviceName), "mv2");
		deviceName = (LPSTR)&dispDevice.DeviceName[0];

		if (bED) {
			// Attach and detach information is sent via the dmPelsWidth/Height
			// of the devmode.
			//
//			MessageBoxSecure(NULL,"bED", NULL, MB_OK);
			if (dwAttach == 0) {
				devmode.dmPelsWidth = 0;
				devmode.dmPelsHeight = 0;
			}
			else {
				devmode.dmPelsWidth = w;
				devmode.dmPelsHeight = h;
				devmode.dmPosition.x = x;
				devmode.dmPosition.y = y;
			}
			// Update the mirror device's registry data with the devmode. Dont
			// do a mode change.

/////////////////////////
			hdeskCurrent = GetThreadDesktop(GetCurrentThreadId());
			if (hdeskCurrent != NULL) {
				hdeskInput = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
				if (hdeskInput != NULL)
					SetThreadDesktop(hdeskInput);
			}


			INT code = ChangeDisplaySettingsEx(deviceName, &devmode, NULL, (CDS_UPDATEREGISTRY | CDS_RESET | CDS_GLOBAL), NULL);
			//            GetDispCode(code);
			if (code != 0)
				return false;
			// Now do the real mode change to take mirror driver changes into
			// effect.
			code = ChangeDisplaySettingsEx(NULL, NULL, NULL, 0, NULL);

			if (hdeskCurrent) SetThreadDesktop(hdeskCurrent);
			if (hdeskInput) CloseDesktop(hdeskInput);
			if (code != 0)
				return false;
			return true;
		}
		return false;
	}
	return false;
}
//----------------------------------------------------------
void VideoDriver::mirror_driver_detach_XP()
{
	HDESK   hdeskInput = NULL;
	HDESK   hdeskCurrent = NULL;
	DEVMODE devmode;

	FillMemory(&devmode, sizeof(DEVMODE), 0);

	devmode.dmSize = sizeof(DEVMODE);
	devmode.dmDriverExtra = 0;

	BOOL change = EnumDisplaySettings(NULL,
		ENUM_CURRENT_SETTINGS,
		&devmode);

	devmode.dmFields = DM_BITSPERPEL |
		DM_PELSWIDTH |
		DM_PELSHEIGHT;

	if (change) {
		// query all display devices in the system until we hit
		// our favourate mirrored driver, then extract the device name string
		// of the format '\\.\DISPLAY#'

		DISPLAY_DEVICE dispDevice;
		FillMemory(&dispDevice, sizeof(DISPLAY_DEVICE), 0);
		dispDevice.cb = sizeof(DISPLAY_DEVICE);
		LPSTR deviceName = NULL;
		devmode.dmDeviceName[0] = '\0';
		INT devNum = 0;
		BOOL result;
		while (result = EnumDisplayDevicesA(NULL, devNum, &dispDevice, 0)) {
			if (strcmp(&dispDevice.DeviceString[0], driverName) == 0)
				break;
			devNum++;
		}

		if (!result)
			return;

		printf("DevNum:%d\nName:%s\nString:%s\nID:%s\nKey:%s\n\n",
			devNum,
			&dispDevice.DeviceName[0],
			&dispDevice.DeviceString[0],
			&dispDevice.DeviceID[0],
			&dispDevice.DeviceKey[0]);

		CHAR deviceNum[MAX_PATH];
		LPSTR deviceSub;

		// Simply extract 'DEVICE#' from registry key.  This will depend
		// on how many mirrored devices your driver has and which ones
		// you intend to use.

		_strupr(&dispDevice.DeviceKey[0]);

		deviceSub = strstr(&dispDevice.DeviceKey[0],
			"\\DEVICE");

		if (!deviceSub)
			StringCbCopy(&deviceNum[0], sizeof(deviceNum), "DEVICE0");
		else
			StringCbCopy(&deviceNum[0], sizeof(deviceNum), ++deviceSub);


		HKEY hKeyProfileMirror = (HKEY)0;
		if (RegCreateKey(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Hardware Profiles\\Current\\System\\CurrentControlSet\\Services\\mv2"), &hKeyProfileMirror) != ERROR_SUCCESS)
			return;
		HKEY hKeyDevice = (HKEY)0;
		if (RegCreateKey(hKeyProfileMirror, _T(&deviceNum[0]), &hKeyDevice) != ERROR_SUCCESS)
			return;
		DWORD one = 0;
		if (RegSetValueEx(hKeyDevice, _T("Attach.ToDesktop"), 0, REG_DWORD, (unsigned char *)&one, 4) != ERROR_SUCCESS)
			return;
		RegCloseKey(hKeyProfileMirror);
		RegCloseKey(hKeyDevice);
		deviceSub = strstr(&dispDevice.DeviceKey[0], "SYSTEM");

		HKEY hKeyProfileMirror2 = (HKEY)0;
		if (RegCreateKey(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Hardware Profiles\\Current"), &hKeyProfileMirror2) != ERROR_SUCCESS)
			return;
		HKEY hKeyDevice2 = (HKEY)0;
		if (RegCreateKey(hKeyProfileMirror2, _T(deviceSub), &hKeyDevice2) != ERROR_SUCCESS)
			return;
		if (RegSetValueEx(hKeyDevice2, _T("Attach.ToDesktop"), 0, REG_DWORD, (unsigned char *)&one, 4) != ERROR_SUCCESS)
			return;
		RegCloseKey(hKeyProfileMirror2);
		RegCloseKey(hKeyDevice2);

		FillMemory(&devmode, sizeof(DEVMODE), 0);
		devmode.dmSize = sizeof(DEVMODE);
		devmode.dmDriverExtra = 0;
		StringCbCopy((LPSTR)&devmode.dmDeviceName[0], sizeof(devmode.dmDeviceName), "mv2");
		deviceName = (LPSTR)&dispDevice.DeviceName[0];
		devmode.dmBitsPerPel = 32;
		// add 'Default.*' settings to the registry under above hKeyProfile\mirror\device
		hdeskCurrent = GetThreadDesktop(GetCurrentThreadId());
		if (hdeskCurrent != NULL) {
			hdeskInput = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
			if (hdeskInput != NULL)
				SetThreadDesktop(hdeskInput);
		}

		INT code = ChangeDisplaySettingsEx(deviceName, &devmode, NULL, CDS_UPDATEREGISTRY, NULL);
		code = ChangeDisplaySettingsEx(deviceName, &devmode, NULL, 0, NULL);
		if (hdeskCurrent) SetThreadDesktop(hdeskCurrent);
		if (hdeskInput) CloseDesktop(hdeskInput);

	}
}
//----------------------------------------------------------
bool VideoDriver::mirror_driver_attach_XP(int x, int y, int w, int h)
{
	HDESK   hdeskInput = NULL;
	HDESK   hdeskCurrent = NULL;
	DEVMODE devmode;

	FillMemory(&devmode, sizeof(DEVMODE), 0);
	devmode.dmSize = sizeof(DEVMODE);
	devmode.dmDriverExtra = 0;
	BOOL change = EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);
	devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_POSITION | DM_PELSHEIGHT;
	if (devmode.dmBitsPerPel != 8 && devmode.dmBitsPerPel != 16 && devmode.dmBitsPerPel != 32)
		return false;
	shared_buffer_size = devmode.dmBitsPerPel / 8 * w*h + sizeof(CHANGES_BUF);
	if (change) {
		// query all display devices in the system until we hit
		// our favourate mirrored driver, then extract the device name string
		// of the format '\\.\DISPLAY#'

		DISPLAY_DEVICE dispDevice;
		FillMemory(&dispDevice, sizeof(DISPLAY_DEVICE), 0);
		dispDevice.cb = sizeof(DISPLAY_DEVICE);
		LPSTR deviceName = NULL;
		devmode.dmDeviceName[0] = '\0';
		INT devNum = 0;
		BOOL result;
		while (result = EnumDisplayDevicesA(NULL, devNum, &dispDevice, 0)) {
			if (strcmp(&dispDevice.DeviceString[0], driverName) == 0)
				break;
			devNum++;
		}

		if (!result)
			return false;

		printf("DevNum:%d\nName:%s\nString:%s\nID:%s\nKey:%s\n\n",
			devNum,
			&dispDevice.DeviceName[0],
			&dispDevice.DeviceString[0],
			&dispDevice.DeviceID[0],
			&dispDevice.DeviceKey[0]);

		CHAR deviceNum[MAX_PATH];
		LPSTR deviceSub;

		// Simply extract 'DEVICE#' from registry key.  This will depend
		// on how many mirrored devices your driver has and which ones
		// you intend to use.

		_strupr(&dispDevice.DeviceKey[0]);
		deviceSub = strstr(&dispDevice.DeviceKey[0], "\\DEVICE");
		if (!deviceSub)
			StringCbCopy(&deviceNum[0], MAX_PATH, "DEVICE0");
		else
			StringCbCopy(&deviceNum[0], MAX_PATH, ++deviceSub);

		HKEY hKeyProfileMirror = (HKEY)0;
		if (RegCreateKey(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Hardware Profiles\\Current\\System\\CurrentControlSet\\Services\\mv2"), &hKeyProfileMirror) != ERROR_SUCCESS)
			return false;

		HKEY hKeyDevice = (HKEY)0;
		if (RegCreateKey(hKeyProfileMirror, _T(&deviceNum[0]), &hKeyDevice) != ERROR_SUCCESS)
			return false;

		DWORD one = 1;
		if (RegSetValueEx(hKeyDevice, _T("Attach.ToDesktop"), 0, REG_DWORD, (unsigned char *)&one, 4) != ERROR_SUCCESS)
			return false;
		int depth = devmode.dmBitsPerPel;

		FillMemory(&devmode, sizeof(DEVMODE), 0);
		devmode.dmSize = sizeof(DEVMODE);
		devmode.dmDriverExtra = 0;
		StringCbCopy((LPSTR)&devmode.dmDeviceName[0], 32, "mv2");
		devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_POSITION | DM_PELSHEIGHT;
		deviceName = (LPSTR)&dispDevice.DeviceName[0];
		devmode.dmPelsWidth = w;
		devmode.dmPelsHeight = h;
		devmode.dmPosition.x = x;
		devmode.dmPosition.y = y;
		devmode.dmBitsPerPel = depth;

		hdeskCurrent = GetThreadDesktop(GetCurrentThreadId());
		if (hdeskCurrent != NULL) {
			hdeskInput = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
			if (hdeskInput != NULL)
				SetThreadDesktop(hdeskInput);
		}
		if (ChangeDisplaySettingsEx(deviceName, &devmode, NULL, CDS_UPDATEREGISTRY, NULL) != 0)
			return false;

		int code = ChangeDisplaySettingsEx(deviceName, &devmode, NULL, 0, NULL);
		if (hdeskCurrent)
			SetThreadDesktop(hdeskCurrent);
		if (hdeskInput)
			CloseDesktop(hdeskInput);
		RegCloseKey(hKeyProfileMirror);
		RegCloseKey(hKeyDevice);
		if (code != 0)
			return false;
		return true;
	}
	return false;
}
//---------------------------------------------------------- 
HDC VideoDriver::getDcMirror()
{
	HDC m_hrootdc = NULL;
	LPSTR driverName = "mv video hook driver2";
	BOOL DriverFound;
	DEVMODE devmode;
	FillMemory(&devmode, sizeof(DEVMODE), 0);
	devmode.dmSize = sizeof(DEVMODE);
	devmode.dmDriverExtra = 0;
	BOOL change = EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);
	devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

	LPSTR deviceName = NULL;
	DISPLAY_DEVICE dd;
	ZeroMemory(&dd, sizeof(dd));
	dd.cb = sizeof(dd);
	devmode.dmDeviceName[0] = '\0';
	INT devNum = 0;
	BOOL result;
	DriverFound = false;
	while (result = EnumDisplayDevicesA(NULL, devNum, &dd, 0)) {
		if (strcmp((const char *)&dd.DeviceString[0], driverName) == 0) {
			DriverFound = true;
			break;
		}
		devNum++;
	}
	if (DriverFound) {
		deviceName = (LPSTR)&dd.DeviceName[0];
		m_hrootdc = CreateDC("DISPLAY", deviceName, NULL, NULL);
		DWORD myerror = GetLastError();
		if (m_hrootdc)
			DeleteDC(m_hrootdc);
	}
	return m_hrootdc;
}
//----------------------------------------------------------

