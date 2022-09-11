#pragma once
#include <iostream>
#include <windows.h>
#include <stdio.h>
#include <vector>
#include <list>
#include <swdevice.h>
#include <aclapi.h>
#include<map>
using namespace std;

const static LPCSTR g_szIPC = ("Global\\{4A77E11C-B0B4-40F9-AA8B-D249116A76FE}");

enum DisplayMode {dmDisplay, dmVirtual, dmExtend, dmExtendOnly};

typedef struct _SUPPORTEDMONITORS
{
	int counter;
	int w[200];
	int h[200];
}SUPPORTEDMONITORS;

typedef struct _DISPLAYINFO
{
	DEVMODE dm;
	CHAR devicenaam[256];
	bool primary;
}DISPLAYINFO;

typedef struct _VIRTUALDISPLAY
{
	int clientId;
	CHAR devicenaam[256];
	HSWDEVICE hDevice;
	HANDLE hEvent;
	bool singleExtendMode;
}VIRTUALDISPLAY;

typedef struct _NAMES
{
	CHAR naam[256];
}NAMES;

typedef HRESULT(__stdcall* PSwDeviceCreate)(
	PCWSTR                      pszEnumeratorName,
	PCWSTR                      pszParentDeviceInstance,
	const SW_DEVICE_CREATE_INFO* pCreateInfo,
	ULONG                       cPropertyCount,
	const DEVPROPERTY* pProperties,
	SW_DEVICE_CREATE_CALLBACK   pCallback,
	PVOID                       pContext,
	PHSWDEVICE                  phSwDevice
	);

typedef void(__stdcall* PSwDeviceClose)(HSWDEVICE hSwDevice);
typedef BOOL(WINAPI* DiInstallDriverAFn) (HWND hwndParent OPTIONAL, LPCSTR InfPath, DWORD Flags, PBOOL NeedReboot OPTIONAL);
typedef void (WINAPI* RtlGetVersion_FUNC)(OSVERSIONINFOEXW*);

class VirtualDisplay
{
private:
	LPVOID FileView;
	HANDLE hFileMap;
	SUPPORTEDMONITORS* pbuff;
	std::list<DISPLAYINFO> diplayInfoList;
	std::list <VIRTUALDISPLAY> virtualDisplayList;
	std::list<NAMES> displayList;
	bool initialized;
	bool restoreNeeded;
	HMODULE hdll;
	PSwDeviceCreate SwDeviceCreateUVNC;
	PSwDeviceClose SwDeviceCloseUVNC;

	void realMonitors(map< pair<int, int>, pair<int, int> >resolutionMap);
	void extendMonitors(map< pair<int, int>, pair<int, int> >resolutionMap, int clientId, bool singleExtendMode, char *displayName);
	void virtualMonitors(map< pair<int, int>, pair<int, int> >resolutionMap, int clientId);

	bool ContainDisplayName(char naam[256]);
	void getSetDisplayName(char* displayName);
	void recordDisplayNames();
	void changeDisplaySize(int w, int h, char naam[256]);
	void disconnectAllDisplays();
	void SetVirtualMonitorsSize(int height, int width);
	void AddVirtualMonitors(int clientId, bool singleExtendMode);
	bool AddVirtualDisplay(HSWDEVICE& hSwDevice, HANDLE& hEvent, WCHAR* name);
	HRESULT ChangePrimaryMonitor(char gdiDeviceName[256]);

public:
	VirtualDisplay();	
	~VirtualDisplay();
	static bool InstallDriver(bool fromCommandline);
	void attachDisplay(DisplayMode flag, map< pair<int, int>, pair<int, int> >resolutionMap, bool singleExtendMode, int clientId, char* displayName);
	void disconnectDisplay(int clientId, bool lastViewer);
};

