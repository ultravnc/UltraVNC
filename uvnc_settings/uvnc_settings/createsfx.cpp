#include "stdafx.h"
#include "resource.h"
#include <time.h>
#include "upnp.h"
#include "firewall.h"
#include "log.h"
#include <iphlpapi.h>
#pragma comment ( lib, "iphlpapi" )
#include <shlwapi.h>
#pragma comment ( lib, "shlwapi" )
#ifndef _WINSOCK2API_
	#include <winsock2.h>
	#pragma comment(lib, "ws2_32.lib")
#endif
int wiz=0;
extern bool UPNP_status;
HWND netproc;
int testport=5500;
extern bool FireWall_status;
extern bool Service_status;
extern bool UPNP_status;
extern bool UPNP_status_checked;
extern char strLocalIP[256];
extern char strExternIP[256];
extern HWND hTab7dialog;
extern HWND hTab6dialog;
bool manual=false;
HWND EditControl2;
char myname[256];



DWORD WINAPI upnpthread_( LPVOID lpParam )
{
	if(FAILED(CoInitializeEx(0, COINIT_APARTMENTTHREADED)))
    {
       CoUninitialize();
        return 0;
    }
	WSADATA wdata;
    WSAStartup(MAKEWORD(2, 2), &wdata);

	UPnP UPnPvar;

	PIP_ADAPTER_INFO pAdapterInfo = NULL;
	DWORD dwRetVal = 0;
	
	pAdapterInfo = (IP_ADAPTER_INFO *) malloc( sizeof(IP_ADAPTER_INFO) );
	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
	
	// Make an initial call to GetAdaptersInfo to get
	// the necessary size into the ulOutBufLen variable
	
	if (GetAdaptersInfo( pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) 
	{
		free(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *) malloc (ulOutBufLen); 
	}
	
	if ((dwRetVal = GetAdaptersInfo( pAdapterInfo, &ulOutBufLen)) == NO_ERROR)
	{
		strcpy(strLocalIP,pAdapterInfo->IpAddressList.IpAddress.String);
		debug("Using port 5999 for Ext IP");
		UPnPvar.Set_UPnP(strLocalIP,"TCP","UVNC",5999);
		UPnPvar.OpenPorts(false);
		strcpy(strExternIP,UPnPvar.GetExternalIP());
		SendMessage(netproc,WM_COMMAND,2003,0);
		
	}
	free(pAdapterInfo);
	UPnPvar.ClosePorts(false);
	debug("Close 5999");
	WSACleanup();
	CoUninitialize();
	return 0;
}

DWORD WINAPI checkthread_( LPVOID lpParam )
{
	if(FAILED(CoInitializeEx(0, COINIT_APARTMENTTHREADED)))
    {
        CoUninitialize();
        return 0;
    }
	WSADATA wdata;
    WSAStartup(MAKEWORD(2, 2), &wdata);
	FirewallCheck(netproc);
	
	WSACleanup();
	CoUninitialize();
	return 0;
}

DWORD WINAPI mapthread_( LPVOID lpParam )
{
	if(FAILED(CoInitializeEx(0, COINIT_APARTMENTTHREADED)))
    {
       CoUninitialize();
        return 0;
    }
	WSADATA wdata;
    WSAStartup(MAKEWORD(2, 2), &wdata);
	checksetport(testport);

	UPnP UPnPvar;

	PIP_ADAPTER_INFO pAdapterInfo = NULL;
	DWORD dwRetVal = 0;
	
	pAdapterInfo = (IP_ADAPTER_INFO *) malloc( sizeof(IP_ADAPTER_INFO) );
	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
	
	// Make an initial call to GetAdaptersInfo to get
	// the necessary size into the ulOutBufLen variable
	
	if (GetAdaptersInfo( pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) 
	{
		free(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *) malloc (ulOutBufLen); 
	}
	
	if ((dwRetVal = GetAdaptersInfo( pAdapterInfo, &ulOutBufLen)) == NO_ERROR)
	{
		strcpy(strLocalIP,pAdapterInfo->IpAddressList.IpAddress.String);
		UPnPvar.Set_UPnP(strLocalIP,"TCP","UVNC_TCP",testport);
		UPnPvar.OpenPorts(true);
		strcpy(strExternIP,UPnPvar.GetExternalIP());
		SendMessage(netproc,WM_COMMAND,2003,0);
		
	}
	free(pAdapterInfo);
	WSACleanup();
	CoUninitialize();
	return 0;
}

DWORD WINAPI unmapthread_( LPVOID lpParam )
{
	if(FAILED(CoInitializeEx(0, COINIT_APARTMENTTHREADED)))
    {
       CoUninitialize();
        return 0;
    }
	WSADATA wdata;
    WSAStartup(MAKEWORD(2, 2), &wdata);

	UPnP UPnPvar;

	PIP_ADAPTER_INFO pAdapterInfo = NULL;
	DWORD dwRetVal = 0;
	
	pAdapterInfo = (IP_ADAPTER_INFO *) malloc( sizeof(IP_ADAPTER_INFO) );
	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
	
	// Make an initial call to GetAdaptersInfo to get
	// the necessary size into the ulOutBufLen variable
	
	if (GetAdaptersInfo( pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) 
	{
		free(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *) malloc (ulOutBufLen); 
	}
	
	if ((dwRetVal = GetAdaptersInfo( pAdapterInfo, &ulOutBufLen)) == NO_ERROR)
	{
		strcpy(strLocalIP,pAdapterInfo->IpAddressList.IpAddress.String);
		UPnPvar.Set_UPnP(strLocalIP,"TCP","UVNC_TCP",testport);
		//UPnPvar.OpenPorts(true);
		strcpy(strExternIP,UPnPvar.GetExternalIP());
		SendMessage(netproc,WM_COMMAND,2003,0);
		
	}
	free(pAdapterInfo);
	UPnPvar.ClosePorts(true);
	WSACleanup();
	CoUninitialize();
	return 0;
}

DWORD WINAPI Fixhread_( LPVOID lpParam )
{
	if(FAILED(CoInitializeEx(0, COINIT_APARTMENTTHREADED)))
    {
        CoUninitialize();
        return 0;
    }
	WSADATA wdata;
    WSAStartup(MAKEWORD(2, 2), &wdata);
	if (!Service_status) 
	{
		if (ControlSSDPService(true)) Service_status=true;
	}
	if (!FireWall_status) 
	{
		if(IsICSConnEnabled()) debug("WARNING!  Internet Connection Sharing is active.  Opening UPnP ports could expose the computer directly to the Internet.");
		if (ControlUPnPPorts(true)) FireWall_status=true;
	}
	
	WSACleanup();
	CoUninitialize();
	SendMessage(netproc,WM_COMMAND,2001,0);
	return 0;
}

BOOL CALLBACK DlgProcSFX(HWND hwnd, UINT uMsg,WPARAM wParam, LPARAM lParam)
{	
	switch (uMsg) {
		
	case WM_INITDIALOG: 
		{	
			netproc=hwnd;
			SetDlgItemInt(hwnd,IDC_INVERSPORT,5500,true);
			EditControl2=GetDlgItem(hwnd,IDC_EDIT3);
			EnableWindow(GetDlgItem(hwnd, IDC_INVERSPORT), false);			
			EnableWindow(GetDlgItem(hwnd, IDC_UPNPON), UPNP_status);
			EnableWindow(GetDlgItem(hwnd, IDC_UPNPOFF), UPNP_status);
			EnableWindow(GetDlgItem(hwnd, IDC_TESTNET), UPNP_status);
			EnableWindow(GetDlgItem(hwnd, IDC_CHECKUPNP), false);
			EnableWindow(GetDlgItem(hwnd, IDC_FIXUPNP),!UPNP_status && UPNP_status_checked );
			EnableWindow(GetDlgItem(hwnd, IDC_NEXT), false);

			return TRUE;
		}
	
	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{
		case 2001:
			if (FireWall_status && Service_status) UPNP_status=true;
				else UPNP_status=false;
			UPNP_status_checked=true;
			EnableWindow(GetDlgItem(hwnd, IDC_UPNPON), UPNP_status);
			EnableWindow(GetDlgItem(hwnd, IDC_UPNPOFF), UPNP_status);
			EnableWindow(GetDlgItem(hwnd, IDC_TESTNET), UPNP_status);
			EnableWindow(GetDlgItem(hwnd, IDC_CHECKUPNP), true);
			EnableWindow(GetDlgItem(hwnd, IDC_FIXUPNP),!UPNP_status && UPNP_status_checked );

			break;
		case 2003:
			SetWindowText(GetDlgItem(hwnd, IDC_LOCAL),strLocalIP);
			SetWindowText(GetDlgItem(hwnd, IDC_REMOTE),strExternIP);
			if (strlen(strLocalIP)!=0)EnableWindow(GetDlgItem(hwnd, IDC_NEXT), true);
			break;
		case IDC_CHECKUPNP:
			{
			DWORD dw;
			HANDLE thread=CreateThread( NULL, 0,checkthread_, NULL, 0, &dw);
			}
			break;
		case IDC_TESTNET:
			{
			DWORD dw;
			HANDLE thread=CreateThread( NULL, 0,upnpthread_, NULL, 0, &dw);
			}
			break;
		case IDC_FIXUPNP:
			{
			DWORD dw;
			HANDLE thread=CreateThread( NULL, 0,Fixhread_, NULL, 0, &dw);
			}
			break;
		case IDC_UPNPON:
			{
			DWORD dw;
			HANDLE thread=CreateThread( NULL, 0,mapthread_, NULL, 0, &dw);
			}
			break;
		case IDC_UPNPOFF:
			{
			DWORD dw;
			HANDLE thread=CreateThread( NULL, 0,unmapthread_, NULL, 0, &dw);
			}
			break;
		case IDOK:
			break;
		case IDC_EDITNAME:
			GetWindowText(GetDlgItem(hwnd, IDC_EDITNAME),myname,256);
			if (strlen(myname)!=0)
			{
				EnableWindow(GetDlgItem(hwnd, IDC_INVERSPORT), true);			
				EnableWindow(GetDlgItem(hwnd, IDC_CHECKUPNP), true);
			}
			else
			{
				EnableWindow(GetDlgItem(hwnd, IDC_INVERSPORT), false);			
				EnableWindow(GetDlgItem(hwnd, IDC_CHECKUPNP), false);
			}
			break;
		case IDC_NEXT:
			{
			manual=SendDlgItemMessage(hwnd, IDC_MANUAL, BM_GETCHECK, 0, 0);
			BOOL ok1;
			testport=GetDlgItemInt(hwnd, IDC_INVERSPORT, &ok1, TRUE);
			ShowWindow(hTab6dialog, SW_HIDE);
			ShowWindow(hTab7dialog, SW_SHOW);
			SetFocus(hTab7dialog);
			}

			break;
		case IDC_BACK:
			ShowWindow(hTab7dialog, SW_HIDE);
			ShowWindow(hTab6dialog, SW_SHOW);
			SetFocus(hTab6dialog);
			break;
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			return TRUE;
		case IDC_MANUAL:
			manual=SendDlgItemMessage(hwnd, IDC_MANUAL, BM_GETCHECK, 0, 0);
			if (manual) EnableWindow(GetDlgItem(hwnd, IDC_NEXT), true); 
			else if (strlen(strLocalIP)!=0)EnableWindow(GetDlgItem(hwnd, IDC_NEXT), true);
			else EnableWindow(GetDlgItem(hwnd, IDC_NEXT), false);
			break;
		}
		return 0;	
	}

	return 0;
}