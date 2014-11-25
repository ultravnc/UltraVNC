#include "stdafx.h"
#include "resource.h"
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

extern LONG SocketConnect;
extern LONG HTTPConnect;
extern LONG AutoPortSelect;
extern LONG PortNumber;
extern LONG HttpPortNumber;
extern LONG LoopbackOnly;
extern LONG AllowLoopback;

extern bool FireWall_status;
extern bool Service_status;
bool UPNP_status=false;
bool UPNP_status_checked=false;

HWND EditControl;
HWND networkproc;
char strLocalIP[256];
char strExternIP[256];

DWORD WINAPI upnpthread( LPVOID lpParam )
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
		SendMessage(networkproc,WM_COMMAND,2003,0);
		
	}
	free(pAdapterInfo);
	UPnPvar.ClosePorts(false);
	debug("Close 5999");
	WSACleanup();
	CoUninitialize();
	return 0;
}

DWORD WINAPI checkthread( LPVOID lpParam )
{
	if(FAILED(CoInitializeEx(0, COINIT_APARTMENTTHREADED)))
    {
        CoUninitialize();
        return 0;
    }
	WSADATA wdata;
    WSAStartup(MAKEWORD(2, 2), &wdata);
	FirewallCheck(networkproc);
	
	WSACleanup();
	CoUninitialize();
	return 0;
}

DWORD WINAPI mapthread( LPVOID lpParam )
{
	if(FAILED(CoInitializeEx(0, COINIT_APARTMENTTHREADED)))
    {
       CoUninitialize();
        return 0;
    }
	WSADATA wdata;
    WSAStartup(MAKEWORD(2, 2), &wdata);
	checksetport(PortNumber);

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
		UPnPvar.Set_UPnP(strLocalIP,"TCP","UVNC_TCP",PortNumber);
		UPnPvar.OpenPorts(true);
		strcpy(strExternIP,UPnPvar.GetExternalIP());
		SendMessage(networkproc,WM_COMMAND,2003,0);
		
	}
	free(pAdapterInfo);
	WSACleanup();
	CoUninitialize();
	return 0;
}

DWORD WINAPI unmapthread( LPVOID lpParam )
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
		UPnPvar.Set_UPnP(strLocalIP,"TCP","UVNC_TCP",PortNumber);
		//UPnPvar.OpenPorts(true);
		strcpy(strExternIP,UPnPvar.GetExternalIP());
		SendMessage(networkproc,WM_COMMAND,2003,0);
		
	}
	free(pAdapterInfo);
	UPnPvar.ClosePorts(true);
	WSACleanup();
	CoUninitialize();
	return 0;
}

DWORD WINAPI Fixhread( LPVOID lpParam )
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
	SendMessage(networkproc,WM_COMMAND,2001,0);
	return 0;
}
bool initdone=false;
BOOL CALLBACK DlgProcOptions1(HWND hwnd, UINT uMsg,
											   WPARAM wParam, LPARAM lParam)
{	
	switch (uMsg) {
		
	case WM_INITDIALOG: 
		{	
			initdone=false;
			SendMessage(GetDlgItem(hwnd, IDC_CONNECT_SOCK), BM_SETCHECK, SocketConnect, 0);
			SendMessage(GetDlgItem(hwnd, IDC_CONNECT_HTTP), BM_SETCHECK, HTTPConnect, 0);
			SendMessage(GetDlgItem(hwnd, IDC_ALLOWLOOPBACK), BM_SETCHECK, AllowLoopback, 0);
			SendMessage(GetDlgItem(hwnd, IDC_LOOPBACKONLY), BM_SETCHECK, LoopbackOnly, 0);
			SetDlgItemInt(hwnd, IDC_PORTRFB, PortNumber, FALSE);
			SetDlgItemInt(hwnd, IDC_PORTHTTP, HttpPortNumber, FALSE);
			CheckDlgButton(hwnd, IDC_PORTNO_AUTO,(AutoPortSelect) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwnd, IDC_SPECPORT,(!AutoPortSelect) ? BST_CHECKED : BST_UNCHECKED);
			EditControl=GetDlgItem(hwnd,IDC_EDIT3);
			networkproc=hwnd;
			initdone=true;
			SendMessage(hwnd,WM_COMMAND,2005,0);
			return TRUE;
		}
	
	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{	
		case 2001:
			if (FireWall_status && Service_status) UPNP_status=true;
				else UPNP_status=false;
			UPNP_status_checked=true;
			SendMessage(hwnd,WM_COMMAND,2002,0);
			break;
		case 2003:
			SetWindowText(GetDlgItem(hwnd, IDC_LOCAL),strLocalIP);
			SetWindowText(GetDlgItem(hwnd, IDC_REMOTE),strExternIP);
			break;
		case IDC_CHECKUPNP:
			{
			DWORD dw;
			HANDLE thread=CreateThread( NULL, 0,checkthread, NULL, 0, &dw);
			}
			break;
		case IDC_TESTNET:
			{
			DWORD dw;
			HANDLE thread=CreateThread( NULL, 0,upnpthread, NULL, 0, &dw);
			}
			break;
		case IDC_FIXUPNP:
			{
			DWORD dw;
			HANDLE thread=CreateThread( NULL, 0,Fixhread, NULL, 0, &dw);
			}
			break;
		case IDC_UPNPON:
			{
			DWORD dw;
			HANDLE thread=CreateThread( NULL, 0,mapthread, NULL, 0, &dw);
			}
			break;
		case IDC_UPNPOFF:
			{
			DWORD dw;
			HANDLE thread=CreateThread( NULL, 0,unmapthread, NULL, 0, &dw);
			}
			break;
		case IDOK:
			SocketConnect=SendDlgItemMessage(hwnd, IDC_CONNECT_SOCK, BM_GETCHECK, 0, 0);
			HTTPConnect=SendDlgItemMessage(hwnd, IDC_CONNECT_HTTP, BM_GETCHECK, 0, 0);
			AllowLoopback=SendDlgItemMessage(hwnd, IDC_ALLOWLOOPBACK, BM_GETCHECK, 0, 0);
			LoopbackOnly=SendDlgItemMessage(hwnd, IDC_LOOPBACKONLY, BM_GETCHECK, 0, 0);
			AutoPortSelect=SendDlgItemMessage(hwnd, IDC_PORTNO_AUTO, BM_GETCHECK, 0, 0);
			BOOL ok1, ok2;
			PortNumber=GetDlgItemInt(hwnd, IDC_PORTRFB, &ok1, TRUE);
			HttpPortNumber=GetDlgItemInt(hwnd, IDC_PORTHTTP, &ok2, TRUE);
			break;
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			return TRUE;
		default:
			if (initdone){
				SocketConnect=SendDlgItemMessage(hwnd, IDC_CONNECT_SOCK, BM_GETCHECK, 0, 0);
				HTTPConnect=SendDlgItemMessage(hwnd, IDC_CONNECT_HTTP, BM_GETCHECK, 0, 0);
				AllowLoopback=SendDlgItemMessage(hwnd, IDC_ALLOWLOOPBACK, BM_GETCHECK, 0, 0);
				LoopbackOnly=SendDlgItemMessage(hwnd, IDC_LOOPBACKONLY, BM_GETCHECK, 0, 0);
				AutoPortSelect=SendDlgItemMessage(hwnd, IDC_PORTNO_AUTO, BM_GETCHECK, 0, 0);


				EnableWindow(GetDlgItem(hwnd, IDC_CONNECT_HTTP), SocketConnect);
				EnableWindow(GetDlgItem(hwnd, IDC_ALLOWLOOPBACK), SocketConnect);
				EnableWindow(GetDlgItem(hwnd, IDC_LOOPBACKONLY), SocketConnect);
				EnableWindow(GetDlgItem(hwnd, IDC_PORTRFB), SocketConnect && !AutoPortSelect);
				EnableWindow(GetDlgItem(hwnd, IDC_PORTHTTP), SocketConnect && HTTPConnect && !AutoPortSelect);
				EnableWindow(GetDlgItem(hwnd, IDC_PORTNO_AUTO), SocketConnect);
				EnableWindow(GetDlgItem(hwnd, IDC_SPECPORT), SocketConnect);
				EnableWindow(GetDlgItem(hwnd, IDC_UPNPON), SocketConnect && !AutoPortSelect && UPNP_status);
				EnableWindow(GetDlgItem(hwnd, IDC_UPNPOFF), SocketConnect && !AutoPortSelect && UPNP_status);
				EnableWindow(GetDlgItem(hwnd, IDC_TESTNET), SocketConnect && !AutoPortSelect && UPNP_status);
				EnableWindow(GetDlgItem(hwnd, IDC_CHECKUPNP), SocketConnect && !AutoPortSelect);
				EnableWindow(GetDlgItem(hwnd, IDC_FIXUPNP),!UPNP_status && UPNP_status_checked );
				BOOL ok1, ok2;
				PortNumber=GetDlgItemInt(hwnd, IDC_PORTRFB, &ok1, TRUE);
				HttpPortNumber=GetDlgItemInt(hwnd, IDC_PORTHTTP, &ok2, TRUE);
			}
			break;

		}
		return 0;	
	}

	return 0;
}