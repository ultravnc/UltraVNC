#include "stdhdrs.h"
#include "VirtualDisplay.h"
#include "versionhelpers.h"
#include "vncservice.h"
#include <newdev.h>
#pragma comment(lib, "Newdev.lib")
#pragma comment(lib, "swdevice.lib")

VOID WINAPI CreationCallback(_In_ HSWDEVICE hSwDevice, _In_ HRESULT hrCreateResult, _In_opt_ PVOID pContext, _In_opt_ PCWSTR pszDeviceInstanceId)

{
	HANDLE hEvent = *(HANDLE*)pContext;

	SetEvent(hEvent);
	UNREFERENCED_PARAMETER(hSwDevice);
	UNREFERENCED_PARAMETER(hrCreateResult);
	UNREFERENCED_PARAMETER(pszDeviceInstanceId);
}

BOOL GetVersion2(OSVERSIONINFOEX* os) {
	HMODULE hMod;
	RtlGetVersion_FUNC func;
#ifdef UNICODE
	OSVERSIONINFOEXW* osw = os;
#else
	OSVERSIONINFOEXW o;
	OSVERSIONINFOEXW* osw = &o;
#endif
	hMod = LoadLibrary(TEXT("ntdll.dll"));
	if (hMod) {
		func = (RtlGetVersion_FUNC)GetProcAddress(hMod, "RtlGetVersion");
		if (func == 0) {
			FreeLibrary(hMod);
			return FALSE;
		}
		ZeroMemory(osw, sizeof(*osw));
		osw->dwOSVersionInfoSize = sizeof(*osw);
		func(osw);
#ifndef UNICODE
		os->dwBuildNumber = osw->dwBuildNumber;
		os->dwMajorVersion = osw->dwMajorVersion;
		os->dwMinorVersion = osw->dwMinorVersion;
		os->dwPlatformId = osw->dwPlatformId;
		os->dwOSVersionInfoSize = sizeof(*os);
		DWORD sz = sizeof(os->szCSDVersion);
		WCHAR* src = osw->szCSDVersion;
		unsigned char* dtc = (unsigned char*)os->szCSDVersion;
		while (*src)
			*dtc++ = (unsigned char)*src++;
		*dtc = '\0';
#endif

	}
	else
		return FALSE;
	FreeLibrary(hMod);
	return TRUE;
}

VirtualDisplay::VirtualDisplay()
{
	FileView = NULL;
	hFileMap = NULL;
	pbuff = NULL;
	initialized = false;
	restoreNeeded = false;
	
	DISPLAY_DEVICE dd;
	ZeroMemory(&dd, sizeof(dd));
	dd.cb = sizeof(dd);
	DWORD dev = 0;

	while (EnumDisplayDevices(0, dev, &dd, 0))
	{
		if (!(dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) && (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) {
			DISPLAY_DEVICE ddMon;
			ZeroMemory(&ddMon, sizeof(ddMon));
			ddMon.cb = sizeof(ddMon);
			DWORD devMon = 0;

			while (EnumDisplayDevices(dd.DeviceName, devMon, &ddMon, 0)) {
				if (ddMon.StateFlags & DISPLAY_DEVICE_ACTIVE)
					break;
				devMon++;
			}

			DEVMODE dm;
			ZeroMemory(&dm, sizeof(dm));
			dm.dmSize = sizeof(dm);
			dm.dmDriverExtra = 0;
			if (EnumDisplaySettingsEx(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm, 0) == FALSE)
				EnumDisplaySettingsEx(dd.DeviceName, ENUM_REGISTRY_SETTINGS, &dm, 0);
			DISPLAYINFO di;
			di.dm = dm;
			strcpy_s(di.devicenaam, 256, dd.DeviceName);
			di.primary = dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE;
			diplayInfoList.push_back(di);
		}
		ZeroMemory(&dd, sizeof(dd));
		dd.cb = sizeof(dd);
		dev++;
	}

	hdll = LoadLibrary("cfgmgr32.dll");
	SwDeviceCreateUVNC = NULL;
	SwDeviceCloseUVNC = NULL;
	if (hdll) {
		SwDeviceCreateUVNC = (PSwDeviceCreate)GetProcAddress(hdll, "SwDeviceCreate");
		SwDeviceCloseUVNC = (PSwDeviceClose)GetProcAddress(hdll, "SwDeviceClose");
	}

	hFileMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SUPPORTEDMONITORS), g_szIPC);
	if (hFileMap) {
		SetSecurityInfo(hFileMap, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, 0, 0, (PACL)NULL, NULL);
		FileView = MapViewOfFile(hFileMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
		pbuff = (SUPPORTEDMONITORS*)FileView;
	}
	if (!pbuff) {
		if (FileView)
			UnmapViewOfFile((LPVOID)FileView);
		FileView = NULL;
		return;
	}

	initialized = true;
	pbuff->counter = 0;
}

void VirtualDisplay::disconnectDisplay(int clientId, bool lastViewer)
{
	std::list<VIRTUALDISPLAY>::iterator virtualDisplayIter;
	virtualDisplayIter = virtualDisplayList.begin();
	while (virtualDisplayIter != virtualDisplayList.end())
	{
		if ((*virtualDisplayIter).hDevice && SwDeviceCloseUVNC && (*virtualDisplayIter).clientId == clientId && (*virtualDisplayIter).singleExtendMode) {
			SwDeviceCloseUVNC((*virtualDisplayIter).hDevice);
			CloseHandle((*virtualDisplayIter).hEvent);
			virtualDisplayList.erase(virtualDisplayIter);
			return;
		}
		virtualDisplayIter++;
	}
	if (lastViewer)
		disconnectAllDisplays();
}

void VirtualDisplay::disconnectAllDisplays()
{
	if (restoreNeeded) {
		std::list<DISPLAYINFO>::iterator resIter;
		resIter = diplayInfoList.begin();
		while (resIter != diplayInfoList.end())
		{
			if ((*resIter).dm.dmPosition.x == 0 && (*resIter).dm.dmPosition.y == 0)
				ChangeDisplaySettingsEx((*resIter).devicenaam, &(*resIter).dm, NULL, (CDS_UPDATEREGISTRY | CDS_NORESET | CDS_SET_PRIMARY), NULL);
			else
				ChangeDisplaySettingsEx((*resIter).devicenaam, &(*resIter).dm, NULL, (CDS_UPDATEREGISTRY | CDS_NORESET), NULL);
			resIter++;
		}
		HDESK   hdeskInput = NULL;
		HDESK   hdeskCurrent = NULL;
		hdeskCurrent = GetThreadDesktop(GetCurrentThreadId());
		if (hdeskCurrent != NULL) {
			hdeskInput = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
			if (hdeskInput != NULL)
				SetThreadDesktop(hdeskInput);
		}
		ChangeDisplaySettingsEx(NULL, NULL, NULL, 0, NULL);
		if (hdeskCurrent)
			SetThreadDesktop(hdeskCurrent);
		if (hdeskInput)
			CloseDesktop(hdeskInput);
		restoreNeeded = false;
	}

	if (!initialized)
		return;
	pbuff->counter = 0;
	// Disable virtual displays
	std::list<VIRTUALDISPLAY>::iterator virtualDisplayIter;
	virtualDisplayIter = virtualDisplayList.begin();
	while (virtualDisplayIter != virtualDisplayList.end())
	{
		if ((*virtualDisplayIter).hDevice)
			if (SwDeviceCloseUVNC)
				SwDeviceCloseUVNC((*virtualDisplayIter).hDevice);
		if ((*virtualDisplayIter).hEvent)
			CloseHandle((*virtualDisplayIter).hEvent);
		virtualDisplayIter++;
	}
	virtualDisplayList.clear();	
}

VirtualDisplay::~VirtualDisplay()
{
	disconnectAllDisplays();
	if (FileView)
		UnmapViewOfFile((LPVOID)FileView);
	if (hFileMap)
		CloseHandle(hFileMap);
	initialized = false;
}

void VirtualDisplay::extendMonitors(map< pair<int, int>, pair<int, int> >resolutionMap, int clientId, bool singleExtendMode, char * displayName)
{
	map< pair<int, int>, pair<int, int> >::iterator it;
	for (it = resolutionMap.begin(); it != resolutionMap.end(); it++) {
		int w = (it->second).first;
		int h = (it->second).second;
		SetVirtualMonitorsSize(h, w);
	}

	for (it = resolutionMap.begin(); it != resolutionMap.end(); it++) {
		getSetDisplayName(NULL);
		AddVirtualMonitors(clientId, resolutionMap.size() == 1);
		char display[256] = {};
		getSetDisplayName(display);
		changeDisplaySize((it->second).first, (it->second).second, display);
		if (singleExtendMode)
			strcpy_s(displayName, 256, display);
	}
	HDESK   hdeskInput = NULL;
	HDESK   hdeskCurrent = NULL;
	hdeskCurrent = GetThreadDesktop(GetCurrentThreadId());
	if (hdeskCurrent != NULL) {
		hdeskInput = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
		if (hdeskInput != NULL)
			SetThreadDesktop(hdeskInput);
	}
	ChangeDisplaySettingsEx(NULL, NULL, NULL, 0, NULL);
	if (hdeskCurrent)
		SetThreadDesktop(hdeskCurrent);
	if (hdeskInput)
		CloseDesktop(hdeskInput);
}

void VirtualDisplay::realMonitors(map< pair<int, int>, pair<int, int> >resolutionMap)
{
	map< pair<int, int>, pair<int, int> >::iterator it;
	bool used[254]{};
	for (it = resolutionMap.begin(); it != resolutionMap.end(); it++) {
		int x = (it->first).first;
		int y = (it->first).second;
		int w = (it->second).first;
		int h = (it->second).second;
		std::list<DISPLAYINFO>::iterator resIter;
		resIter = diplayInfoList.begin();
		int counter = 0;
		while (resIter != diplayInfoList.end())
		{
			DEVMODE dm = (*resIter).dm;
			if (used[counter] == true) {
				resIter++;
				counter++;
				continue;
			}
			if (dm.dmPosition.x == 0 && dm.dmPosition.y == 0 && x == 0 && y == 0) {
				dm.dmPelsHeight = h;
				dm.dmPelsWidth = w;
				dm.dmFields = DM_PELSWIDTH |DM_PELSHEIGHT;
				ChangeDisplaySettingsEx((*resIter).devicenaam, &dm, NULL, (CDS_UPDATEREGISTRY | CDS_NORESET), NULL);
				used[counter] = true;
				restoreNeeded = true;
				break;;
			}
			else {
				dm.dmPosition.x = x;
				dm.dmPosition.y = y;
				dm.dmPelsHeight = h;
				dm.dmPelsWidth = w;
				ChangeDisplaySettingsEx((*resIter).devicenaam, &dm, NULL, (CDS_UPDATEREGISTRY | CDS_NORESET), NULL);
				used[counter] = true;
				restoreNeeded = true;
				break;

			}
			resIter++;
			counter++;
		}
	}
	HDESK   hdeskInput = NULL;
	HDESK   hdeskCurrent = NULL;
	hdeskCurrent = GetThreadDesktop(GetCurrentThreadId());
	if (hdeskCurrent != NULL) {
		hdeskInput = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
		if (hdeskInput != NULL)
			SetThreadDesktop(hdeskInput);
	}
	ChangeDisplaySettingsEx(NULL, NULL, NULL, 0, NULL);
	if (hdeskCurrent)
		SetThreadDesktop(hdeskCurrent);
	if (hdeskInput)
		CloseDesktop(hdeskInput);
}

void VirtualDisplay::virtualMonitors(map< pair<int, int>, pair<int, int> >resolutionMap, int clientId)
{
	map< pair<int, int>, pair<int, int> >::iterator it;
	for (it = resolutionMap.begin(); it != resolutionMap.end(); it++) {
		int w = (it->second).first;
		int h = (it->second).second;
		SetVirtualMonitorsSize(h, w);
	}
	char newPrimaryDisplayName[256] = {};
	for (it = resolutionMap.begin(); it != resolutionMap.end(); it++) {
		getSetDisplayName(NULL);
		AddVirtualMonitors(clientId, false);
		char displayName[256] = {};
		getSetDisplayName(displayName);
		changeDisplaySize((it->second).first, (it->second).second, displayName);
		if (it == resolutionMap.begin())
			strcpy_s(newPrimaryDisplayName, displayName);
	}
	ChangePrimaryMonitor(newPrimaryDisplayName);
	

	std::list<DISPLAYINFO>::iterator resIter;
	resIter = diplayInfoList.begin();
	while (resIter != diplayInfoList.end())
	{
		DEVMODE dm = (*resIter).dm;
		dm.dmPosition.x = 10000;
		dm.dmPosition.y = 10000;
		dm.dmPelsHeight = 0;
		dm.dmPelsWidth = 0;
		ChangeDisplaySettingsEx((*resIter).devicenaam, &dm, NULL, (CDS_UPDATEREGISTRY | CDS_NORESET), NULL);
		restoreNeeded = true;
		resIter++;
	}

	HDESK   hdeskInput = NULL;
	HDESK   hdeskCurrent = NULL;
	hdeskCurrent = GetThreadDesktop(GetCurrentThreadId());
	if (hdeskCurrent != NULL) {
		hdeskInput = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
		if (hdeskInput != NULL)
			SetThreadDesktop(hdeskInput);
	}
	ChangeDisplaySettingsEx(NULL, NULL, NULL, 0, NULL);
	if (hdeskCurrent)
		SetThreadDesktop(hdeskCurrent);
	if (hdeskInput)
		CloseDesktop(hdeskInput);

}

void VirtualDisplay::attachDisplay(DisplayMode flag , map< pair<int, int>, pair<int, int> >resolutionMap, bool singleExtendMode, int clientId, char *displayName)
{
	if (flag == dmDisplay && resolutionMap.size() > 0)
		realMonitors(resolutionMap);

	if (!initialized || restoreNeeded) //restoreNeeded, only set once for real and virtual
		return;
		
	if (flag == dmVirtual && resolutionMap.size() > 0) 
		virtualMonitors(resolutionMap, clientId);

	if ((flag == dmExtend  || flag == dmExtendOnly) && resolutionMap.size() > 0)
		extendMonitors(resolutionMap, clientId, singleExtendMode, displayName);
}


void VirtualDisplay::SetVirtualMonitorsSize(int height, int width)
{
	if (!initialized)
		return;
	//we don't want dubbels
	for (int i = 0; i < pbuff->counter; i++)
		if (pbuff->w[i] == width && pbuff->h[i] == height)
			return;

	pbuff->w[pbuff->counter] = width;
	pbuff->h[pbuff->counter] = height;
	pbuff->counter += 1;
}

void VirtualDisplay::AddVirtualMonitors(int clientId, bool singleExtendMode)
{
	if (!initialized)
		return;
	HSWDEVICE hSwDevice = NULL;
	HANDLE hEvent = NULL;
	WCHAR name[256];
	wcscpy_s(name, L"UVncVirtualDisplay");
	WCHAR nummer[256];
	swprintf_s(nummer, L"%zd", virtualDisplayList.size());
	wcscat_s(name, nummer);

	if (AddVirtualDisplay(hSwDevice, hEvent, name))
	{
		VIRTUALDISPLAY virtualDisplay;
		virtualDisplay.hDevice = hSwDevice;
		virtualDisplay.hEvent = hEvent;
		virtualDisplay.clientId = clientId;
		virtualDisplay.singleExtendMode = singleExtendMode;
		virtualDisplayList.push_back(virtualDisplay);
	}
}

bool VirtualDisplay::AddVirtualDisplay(HSWDEVICE& hSwDevice, HANDLE& hEvent, WCHAR* name)
{
	if (!initialized)
		return false;
	PCWSTR description = name;
	PCWSTR instanceId = name;
	PCWSTR hardwareIds = L"UVncVirtualDisplay\0\0";
	PCWSTR compatibleIds = L"UVncVirtualDisplay\0\0";
	SW_DEVICE_CREATE_INFO createInfo = { 0 };
	hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	createInfo.cbSize = sizeof(createInfo);
	createInfo.pszzCompatibleIds = compatibleIds;
	createInfo.pszInstanceId = instanceId;
	createInfo.pszzHardwareIds = hardwareIds;
	createInfo.pszDeviceDescription = description;
	createInfo.CapabilityFlags = SWDeviceCapabilitiesRemovable |
		SWDeviceCapabilitiesSilentInstall |
		SWDeviceCapabilitiesDriverRequired;

	HRESULT hr = S_FALSE;
	if (SwDeviceCreateUVNC != NULL) {
			hr = SwDeviceCreateUVNC(name, L"HTREE\\ROOT\\0", &createInfo, 0,
				nullptr, CreationCallback, &hEvent, &hSwDevice);
	}
	else
		return false;

	if (FAILED(hr))
		return false;
	DWORD waitResult = WaitForSingleObject(hEvent, 10 * 1000);
	if (waitResult != WAIT_OBJECT_0)
		return false;
	return true;
}

bool VirtualDisplay::InstallDriver(bool fromCommandline)
{
	if (fromCommandline)
		AllocConsole();
	OSVERSIONINFOEX os;
	if (GetVersion2(&os) == TRUE && os.dwMajorVersion == 10 && os.dwBuildNumber >= 18362) {
		if (!fromCommandline)
			return 1;
		CHAR szdriverPath[MAX_PATH];
		if (GetModuleFileName(NULL, szdriverPath, MAX_PATH)) {
			char* p = strrchr(szdriverPath, '\\');
			if (p == NULL)
				return 0;
			*p = '\0';
#ifdef _X64
			strcat_s(szdriverPath, "\\UVncVirtualDisplay64\\UVncVirtualDisplay.inf");
#else
			strcat_s(szdriverPath, "\\UVncVirtualDisplay\\UVncVirtualDisplay.inf");
#endif
		}
		std::unique_ptr<BOOL> restart(new BOOL());

		HMODULE hModule = LoadLibrary("Newdev.dll");
		DiInstallDriverAFn diInstallDriverA = NULL;
		if (hModule) {
			diInstallDriverA = (DiInstallDriverAFn)GetProcAddress(hModule, "DiInstallDriverA");
			if (diInstallDriverA) {
				BOOL status = diInstallDriverA(NULL, szdriverPath, NULL/*DIIRFLAG_FORCE_INF*/, restart.get());
				DWORD errorMessageID = GetLastError();
				LPSTR messageBuffer = nullptr;
				if (status == 0) {
					size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
					vnclog.Print(LL_INTERR, VNCLOG("InstallDriver failed %s \n"), messageBuffer);
					if (fromCommandline) {
						DWORD byteswritten;
						WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), messageBuffer, strlen(messageBuffer), &byteswritten, NULL);
						Sleep(3000);
					}
				}
				return status;
			}
			FreeLibrary(hModule);
		}
	}
	else if (fromCommandline) {
		DWORD byteswritten;
		WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), "OS not supported", strlen("OS not supported"), &byteswritten, NULL);
		Sleep(3000);
	}

	if (fromCommandline)
		FreeConsole();
	return 0;
}

bool VirtualDisplay::ContainDisplayName(char naam[256])
{
	std::list<NAMES>::iterator displayInfoIter;
	displayInfoIter = displayList.begin();
	while (displayInfoIter != displayList.end()) {
		if (strcmp((*displayInfoIter).naam, naam) == NULL)
			return true;
		displayInfoIter++;
	}
	return false;
}

void VirtualDisplay::recordDisplayNames()
{
	DISPLAY_DEVICE dd;
	ZeroMemory(&dd, sizeof(dd));
	dd.cb = sizeof(dd);
	displayList.clear();
	int times = 0;
	bool found = false;

	DWORD dev = 0;
	while (EnumDisplayDevices(0, dev, &dd, 0))
	{
		if (!(dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) && (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) {
			DISPLAY_DEVICE ddMon;
			ZeroMemory(&ddMon, sizeof(ddMon));
			ddMon.cb = sizeof(ddMon);
			DWORD devMon = 0;
			while (EnumDisplayDevices(dd.DeviceName, devMon, &ddMon, 0)) {
				if (ddMon.StateFlags & DISPLAY_DEVICE_ACTIVE)
					break;
				devMon++;
			}
			DEVMODE dm;
			ZeroMemory(&dm, sizeof(dm));
			dm.dmSize = sizeof(dm);
			dm.dmDriverExtra = 0;
			if (!EnumDisplaySettingsEx(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm, 0))
				return;

			NAMES naam;
			strcpy_s(naam.naam, 256, dd.DeviceName);
			if (strcmp(dd.DeviceString, "UVncVirtualDisplay Device") == NULL)
				displayList.push_back(naam);
		}
		ZeroMemory(&dd, sizeof(dd));
		dd.cb = sizeof(dd);
		dev++;
	}

	return;
}

void VirtualDisplay::getSetDisplayName(char* gdiDeviceName)
{
	if (gdiDeviceName == NULL)
		recordDisplayNames();

	DISPLAY_DEVICE dd;
	ZeroMemory(&dd, sizeof(dd));
	dd.cb = sizeof(dd);
	int times = 0;
	bool found = false;
	while (!found && times < 10) {
		DWORD dev = 0;
		while (EnumDisplayDevices(0, dev, &dd, 0))
		{
			if (!(dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) && (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) {
				DISPLAY_DEVICE ddMon;
				ZeroMemory(&ddMon, sizeof(ddMon));
				ddMon.cb = sizeof(ddMon);
				DWORD devMon = 0;
				while (EnumDisplayDevices(dd.DeviceName, devMon, &ddMon, 0)) {
					if (ddMon.StateFlags & DISPLAY_DEVICE_ACTIVE)
						break;
					devMon++;
				}
				DEVMODE dm;
				ZeroMemory(&dm, sizeof(dm));
				dm.dmSize = sizeof(dm);
				dm.dmDriverExtra = 0;
				if (!EnumDisplaySettingsEx(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm, 0))
					return;

				NAMES naam;
				strcpy_s(naam.naam, 256, dd.DeviceName);
				if (strcmp(dd.DeviceString, "UVncVirtualDisplay Device") == NULL) {
					if (!ContainDisplayName(naam.naam)) {
						strcpy_s(gdiDeviceName, 256, naam.naam);
						found = true;
						return;
					}
				}
			}
			ZeroMemory(&dd, sizeof(dd));
			dd.cb = sizeof(dd);
			dev++;
		}
		times++;
		Sleep(100);
	}
	return;
}

void VirtualDisplay::changeDisplaySize(int w, int h, char gdiDeviceName[256])
{
	DEVMODE dm;
	ZeroMemory(&dm, sizeof(dm));
	dm.dmSize = sizeof(dm);
	dm.dmDriverExtra = 0;
	if (!EnumDisplaySettingsEx(gdiDeviceName, ENUM_CURRENT_SETTINGS, &dm, 0))
		return;
	dm.dmPelsHeight = h;
	dm.dmPelsWidth = w;
	ChangeDisplaySettingsEx(gdiDeviceName, &dm, NULL, (CDS_UPDATEREGISTRY | CDS_NORESET), NULL);
}

HRESULT VirtualDisplay::ChangePrimaryMonitor(char gdiDeviceName[256])
{
	HRESULT hr;
	char lastPrimaryDisplay[256] = "";
	bool shouldRefresh = false;

	DEVMODE newPrimaryDeviceMode;
	newPrimaryDeviceMode.dmSize = sizeof(newPrimaryDeviceMode);
	if (!EnumDisplaySettings(gdiDeviceName, ENUM_CURRENT_SETTINGS, &newPrimaryDeviceMode))
	{
		hr = E_FAIL;
		return hr;
	}

	for (int i = 0;; ++i)
	{
		ULONG flags = CDS_UPDATEREGISTRY | CDS_NORESET;
		DISPLAY_DEVICE device;
		device.cb = sizeof(device);
		if (!EnumDisplayDevices(NULL, i, &device, EDD_GET_DEVICE_INTERFACE_NAME))
			break;

		if ((device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) == 0)
			continue;

		if (!strcmp(device.DeviceName, gdiDeviceName))
			flags |= CDS_SET_PRIMARY;

		DEVMODE deviceMode;
		newPrimaryDeviceMode.dmSize = sizeof(deviceMode);
		if (!EnumDisplaySettings(device.DeviceName, ENUM_CURRENT_SETTINGS, &deviceMode))
		{
			hr = E_FAIL;
			return hr;
		}

		deviceMode.dmPosition.x -= newPrimaryDeviceMode.dmPosition.x;
		deviceMode.dmPosition.y -= newPrimaryDeviceMode.dmPosition.y;
		deviceMode.dmFields |= DM_POSITION;

		LONG rc = ChangeDisplaySettingsEx(device.DeviceName, &deviceMode, NULL,
			flags, NULL);

		if (rc != DISP_CHANGE_SUCCESSFUL) {
			hr = E_FAIL;
			return hr;
		}

		shouldRefresh = true;
	}

	hr = S_OK;
	return hr;
}