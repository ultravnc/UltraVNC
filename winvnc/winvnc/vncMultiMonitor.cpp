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

// System headers
#include <assert.h>
#include "stdhdrs.h"

// Custom headers
//#include <WinAble.h>
#include <omnithread.h>
#include "winvnc.h"
#include "vnchooks/VNCHooks.h"
#include "vncserver.h"
#include "vnckeymap.h"
#include "rfbRegion.h"
#include "rfbRect.h"
#include "vncdesktop.h"
#include "vncservice.h"
// Modif rdv@2002 - v1.1.x - videodriver
#include "vncOSVersion.h"

#include "mmsystem.h" // sf@2002
#include "TextChat.h" // sf@2002
#include "vncdesktopthread.h"
#include "common/win32_helpers.h"

void 
vncDesktop::Checkmonitors()
{
	nr_monitors = 0;
	devicenaamToPosMap.clear();
	int nr = 1;
	DISPLAY_DEVICE dd;
	ZeroMemory(&dd, sizeof(dd));
	dd.cb = sizeof(dd);
	for (int i = 0; EnumDisplayDevicesA(NULL, i, &dd, 0); i++) {
		if ((dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) && !(dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)) {
			DEVMODE devMode;
			devMode.dmSize = sizeof(DEVMODE);
			EnumDisplaySettings(dd.DeviceName, ENUM_CURRENT_SETTINGS, &devMode);
			nr_monitors++;
			if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) {
				mymonitor[MULTI_MON_PRIMARY].offsetx = devMode.dmPosition.x;
				mymonitor[MULTI_MON_PRIMARY].offsety = devMode.dmPosition.y;
				mymonitor[MULTI_MON_PRIMARY].Width = devMode.dmPelsWidth;
				mymonitor[MULTI_MON_PRIMARY].Height = devMode.dmPelsHeight;
				mymonitor[MULTI_MON_PRIMARY].Depth = devMode.dmBitsPerPel;
				devicenaamToPosMap.insert(std::pair< std::string, monitor >(dd.DeviceName, mymonitor[MULTI_MON_PRIMARY]));
			}
			else {
				mymonitor[nr].offsetx = devMode.dmPosition.x;
				mymonitor[nr].offsety = devMode.dmPosition.y;
				mymonitor[nr].Width = devMode.dmPelsWidth;
				mymonitor[nr].Height = devMode.dmPelsHeight;
				mymonitor[nr].Depth = devMode.dmBitsPerPel;
				devicenaamToPosMap.insert(std::pair< std::string, monitor >(dd.DeviceName, mymonitor[nr]));
				nr++;
			}
		}
	}
	mymonitor[MULTI_MON_ALL].offsetx = GetSystemMetrics(SM_XVIRTUALSCREEN);
	mymonitor[MULTI_MON_ALL].offsety = GetSystemMetrics(SM_YVIRTUALSCREEN);
	mymonitor[MULTI_MON_ALL].Width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	mymonitor[MULTI_MON_ALL].Height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	mymonitor[MULTI_MON_ALL].Depth = mymonitor[MULTI_MON_PRIMARY].Depth;//depth primary monitor is used
	devicenaamToPosMap.insert(std::pair< std::string, monitor >("MULTI_MON_ALL", mymonitor[MULTI_MON_ALL]));
}