#pragma once
#include <iostream>
#include <Windows.h>
#include <stdio.h>
#include <vector>
#include <list>
#include <swdevice.h>
#include <aclapi.h>
#include<map>
using namespace std;

const static LPCSTR g_szIPC = ("Global\\{4A77E11C-B0B4-40F9-AA8B-D249116A76FE}");

typedef struct _SUPPORTEDMONITORS
{
	int counter;
	int w[200];
	int h[200];
}SUPPORTEDMONITORS;

typedef struct _DISPLAYINFO
{
	DEVMODE dm;
	CHAR naam[256];
	bool primary;
}DISPLAYINFO;

typedef struct _VIRTUALDISPLAY
{
	HSWDEVICE hDevice;
	HANDLE hEvent;
}VIRTUALDISPLAY;

typedef HRESULT(__cdecl* PSwDeviceCreate)(
	PCWSTR                      pszEnumeratorName,
	PCWSTR                      pszParentDeviceInstance,
	const SW_DEVICE_CREATE_INFO* pCreateInfo,
	ULONG                       cPropertyCount,
	const DEVPROPERTY* pProperties,
	SW_DEVICE_CREATE_CALLBACK   pCallback,
	PVOID                       pContext,
	PHSWDEVICE                  phSwDevice
	);

typedef void(__cdecl* PSwDeviceClose)(HSWDEVICE hSwDevice);

class VirtualDisplay
{
private:
	LPVOID FileView;
	HANDLE hFileMap;
	SUPPORTEDMONITORS* pbuff;
	std::list<DISPLAYINFO> diplayInfoList;
	std::list <VIRTUALDISPLAY> virtualDisplayList;
	bool initialized;
	bool restoreNeeded;
	HMODULE hdll;
	PSwDeviceCreate SwDeviceCreateUVNC;
	PSwDeviceClose SwDeviceCloseUVNC;
public:
	VirtualDisplay();
	~VirtualDisplay();
	void SetVirtualMonitorsSize(int height, int width);
	void AddVirtualMonitors();
	bool AddVirtualDisplay(HSWDEVICE& hSwDevice, HANDLE& hEvent, WCHAR* name);
	static bool InstallDriver();
	void changeMonitors(int flag, map< pair<int, int>, pair<int, int> >resolutionMap);
	void VirtualDisplay::getMapElement(int nummer, int& x, int& y, int& w, int& h, map< pair<int, int>, pair<int, int> >resolutionMap);	
};

