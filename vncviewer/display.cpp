#include "display.h"

void
tempdisplayclass::checkmonitors()
{

	DISPLAY_DEVICE dd;
	ZeroMemory(&dd, sizeof(dd));
	dd.cb = sizeof(dd);
	DWORD dev = 0; // device index
	int id = 1; // monitor number, as used by Display Properties > Settings

	while (EnumDisplayDevices(0, dev, &dd, 0))
	{
		if (!(dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER))
			if ((dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
		{
			// ignore virtual mirror displays

			// get information about the monitor attached to this display adapter. dualhead cards
			// and laptop video cards can have multiple monitors attached
			DISPLAY_DEVICE ddMon;
			ZeroMemory(&ddMon, sizeof(ddMon));
			ddMon.cb = sizeof(ddMon);
			DWORD devMon = 0;

			// please note that this enumeration may not return the correct monitor if multiple monitors
			// are attached. this is because not all display drivers return the ACTIVE flag for the monitor
			// that is actually active
			while (EnumDisplayDevices(dd.DeviceName, devMon, &ddMon, 0))
			{
				if (ddMon.StateFlags & DISPLAY_DEVICE_ACTIVE)
					break;

				devMon++;
			}

			if (!*ddMon.DeviceString)
			{
				EnumDisplayDevices(dd.DeviceName, 0, &ddMon, 0);
				if (!*ddMon.DeviceString)
					lstrcpy((char *)ddMon.DeviceString, "Default Monitor");
			}

			// get information about the display's position and the current display mode
			DEVMODE dm;
			ZeroMemory(&dm, sizeof(dm));
			dm.dmSize = sizeof(dm);
			dm.dmDriverExtra=0;
			if (EnumDisplaySettingsEx((char *)dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm, 0) == FALSE)
				EnumDisplaySettingsEx((char *)dd.DeviceName, ENUM_REGISTRY_SETTINGS, &dm, 0);
			

			// get the monitor handle and workspace
			HMONITOR hm = 0;
			MONITORINFO mi;
			ZeroMemory(&mi, sizeof(mi));
			mi.cbSize = sizeof(mi);
			if (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
			{
				// display is enabled. only enabled displays have a monitor handle
				POINT pt = { dm.dmPosition.x, dm.dmPosition.y };
				hm = MonitorFromPoint(pt, MONITOR_DEFAULTTONULL);
				if (hm)
					GetMonitorInfo(hm, &mi);
			}

			if (hm)
			{
					monarray[id].wl=mi.rcWork.left;
					monarray[id].wt=mi.rcWork.top;
					monarray[id].wr=mi.rcWork.right;
					monarray[id].wb=mi.rcWork.bottom;
			}
			sprintf(monarray[id].buttontext, "%d. %d x %d @ %d,%d - %d-bit - %d Hz", id,dm.dmPelsWidth, dm.dmPelsHeight,
				dm.dmPosition.x, dm.dmPosition.y, dm.dmBitsPerPel, dm.dmDisplayFrequency);
			monarray[id].width=dm.dmPelsWidth;
			monarray[id].height=dm.dmPelsHeight;
			monarray[id].depth=dm.dmBitsPerPel;
			monarray[id].offsetx=dm.dmPosition.x;
			monarray[id].offsety=dm.dmPosition.y;
			monarray[id].freq=dm.dmDisplayFrequency;
			strcpy(monarray[id].devicename,(char *)dd.DeviceName);
			nr_monitors=id;

			id++;
		}
		dev++;
	}
	//if (nr_monitors>1)
			{
					monarray[0].width=GetSystemMetrics (SM_CXVIRTUALSCREEN);
					monarray[0].height=GetSystemMetrics (SM_CYVIRTUALSCREEN);
					monarray[0].depth=monarray[1].depth;
					monarray[0].offsetx=GetSystemMetrics (SM_XVIRTUALSCREEN);
					monarray[0].offsety=GetSystemMetrics (SM_YVIRTUALSCREEN);
					strcpy(monarray[0].devicename,"All displays");
					sprintf(monarray[0].buttontext, "%d. %d x %d @ %d,%d - %d-bit ", 0,monarray[0].width, monarray[0].height,monarray[0].offsetx, monarray[0].offsety, monarray[0].depth);

					RECT workrect = { 0, 0, 0, 0 };
 					SystemParametersInfo(SPI_GETWORKAREA, 0, &workrect, 0);

					/* Update work rectangle to use the virtual screen size */
					if (monarray[0].offsetx < 0) 
					{
						workrect.left = monarray[0].offsetx;
					}
						if (monarray[0].offsety < 0) 
					{
						workrect.top = monarray[0].offsetx;
					}
					workrect.right = workrect.left + monarray[0].width - (monarray[1].width - (monarray[1].wr - monarray[1].wl));
					workrect.bottom = workrect.top  + monarray[0].height - (monarray[1].height - (monarray[1].wb - monarray[1].wt));

					monarray[0].wl=workrect.left;
					monarray[0].wt=workrect.top;
					monarray[0].wr=workrect.right;
					monarray[0].wb=workrect.bottom;

			}
}

tempdisplayclass::tempdisplayclass()
{
  nr_monitors=0;
  selected_monitor=1;
  Init();
}

tempdisplayclass::~tempdisplayclass()
{
	FreeLibrary(hUser32);
}

void
tempdisplayclass::Init()
{
	hUser32= LoadLibrary("user32.dll");
	checkmonitors();
}
