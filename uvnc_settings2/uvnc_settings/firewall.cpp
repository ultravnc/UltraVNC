#include "stdafx.h"
#include "log.h"
#include "firewall.h"
// Windows Firewall API
#ifndef __netfw_h__
	#include <Netfw.h>
#endif
// ICS API
#ifndef __netcon_h__
	#include <netcon.h>
#endif

bool FireWall_status=false;
bool Service_status=false;
extern HWND networkproc;



int CheckFirewallPortState(long number, NET_FW_IP_PROTOCOL protocol)
{
	INetFwMgr *imgr = NULL;
	INetFwPolicy *ipol = NULL;
	INetFwProfile *iprof = NULL;
	HRESULT hr = S_OK;
	VARIANT_BOOL portenabled = 0; // false
	int result = 0; // error

	hr = CoCreateInstance(__uuidof(NetFwMgr), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwMgr), (void**)&imgr);
	if(FAILED(hr))
		return 0;

	hr = S_FALSE;

	if(imgr->get_LocalPolicy(&ipol) == S_OK)
	{
		if(ipol->get_CurrentProfile(&iprof) == S_OK)
		{
			INetFwOpenPorts *iports = NULL;
			if(iprof->get_GloballyOpenPorts(&iports) == S_OK)
			{
				INetFwOpenPort *iport = NULL;
				
				hr = iports->Item(number, protocol, &iport);
				if(SUCCEEDED(hr))
				{
					hr = iport->get_Enabled(&portenabled);
					iport->Release();
				}
				
				iports->Release();
			}
			
			iprof->Release();
		}
		
		ipol->Release();
	}
	
	imgr->Release();

	if(hr == S_OK)
	{
		if(portenabled)
			result = 1;
		else
			result = -1;
	}

	return result;
}

bool ControlUPnPPorts(bool open)
{
	INetFwMgr *imgr = NULL;
	INetFwPolicy *ipol = NULL;
	INetFwProfile *iprof = NULL;
	HRESULT hr = S_OK;
	bool port2869 = false;
	bool port1900 = false;

	hr = CoCreateInstance(__uuidof(NetFwMgr), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwMgr), (void**)&imgr);
	if(FAILED(hr))
		return false;

	if(imgr->get_LocalPolicy(&ipol) == S_OK)
	{
		if(ipol->get_CurrentProfile(&iprof) == S_OK)
		{
			INetFwOpenPorts *iports = NULL;
			if(iprof->get_GloballyOpenPorts(&iports) == S_OK)
			{
				INetFwOpenPort *iport = NULL;
				VARIANT_BOOL portenabled = open ? -1 : 0;
				
				hr = iports->Item(2869L, NET_FW_IP_PROTOCOL_TCP, &iport);
				if(FAILED(hr))
				{
					hr = CoCreateInstance(__uuidof(NetFwOpenPort), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwOpenPort), (void**)&iport);
					if(SUCCEEDED(hr))
					{
						iport->put_Name(L"UPnP TCP 2869");
						iport->put_Port(2869L);
						iport->put_Protocol(NET_FW_IP_PROTOCOL_TCP);
						iport->put_Scope(NET_FW_SCOPE_LOCAL_SUBNET);
						hr = iports->Add(iport);
					}
				}
				if(hr == S_OK && iport->put_Enabled(portenabled) == S_OK)
				{
					debug("TCP 2869 enabled");
					port2869 = true;
				}
				
				if(iport)
					iport->Release();
				hr = iports->Item(1900L, NET_FW_IP_PROTOCOL_UDP, &iport);
				if(FAILED(hr))
				{
					hr = CoCreateInstance(__uuidof(NetFwOpenPort), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwOpenPort), (void**)&iport);
					if(SUCCEEDED(hr))
					{
						iport->put_Name(L"UPnP UDP 1900");
						iport->put_Port(1900L);
						iport->put_Protocol(NET_FW_IP_PROTOCOL_UDP);
						iport->put_Scope(NET_FW_SCOPE_LOCAL_SUBNET);
						hr = iports->Add(iport);
					}
				}
				if(hr == S_OK && iport->put_Enabled(portenabled) == S_OK)
				{
					debug("UDP 1900 enabled");
					port1900 = true;
				}

				if(iport)
					iport->Release();
				
				iports->Release();
			}
			
			iprof->Release();
		}
		
		ipol->Release();
	}
	
	imgr->Release();

	return port2869 & port1900;
}

bool IsICSConnEnabled()
{
	HRESULT hr = S_OK;

	hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_PKT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
	if(hr != S_OK && hr != RPC_E_TOO_LATE)
		return false;

	long pubconns = 0;
	long prvconns = 0;
	bool pubactive = false;
	bool prvactive = false;

	INetSharingManager *csmgr = NULL;
	hr = CoCreateInstance(CLSID_NetSharingManager, NULL, CLSCTX_INPROC_SERVER, IID_INetSharingManager, (LPVOID*)&csmgr);
	if(SUCCEEDED(hr))
	{
		VARIANT_BOOL isinstalled = FALSE;
		csmgr->get_SharingInstalled(&isinstalled);
		if(isinstalled)
		{
			ULONG lfetched = 0;
			IUnknown *ienum = NULL;
			VARIANT vconn;
			INetConnection *iconn = NULL;

			INetSharingPublicConnectionCollection *ipubcol = NULL;
			hr = csmgr->get_EnumPublicConnections(ICSSC_DEFAULT, &ipubcol);
			if(SUCCEEDED(hr))
			{
				if(ipubcol->get_Count(&pubconns) == S_OK && pubconns > 0)
				{
					if(ipubcol->get__NewEnum(&ienum) == S_OK)
					{
						IEnumNetSharingPublicConnection *ipubs = NULL;
						hr = ienum->QueryInterface(IID_IEnumNetSharingPublicConnection, (void**)&ipubs);
						if(SUCCEEDED(hr))
						{
							VariantInit(&vconn);
							ipubs->Reset();
							while(ipubs->Next(1, &vconn, &lfetched) == S_OK)
							{
								pubactive = true;
								iconn = NULL;
								iconn = (INetConnection*)vconn.punkVal;
								VariantClear(&vconn);
							}
							
							ipubs->Release();
						}

						ienum->Release();
						ienum = NULL;
					}
				}
				
				ipubcol->Release();
			}

			INetSharingPrivateConnectionCollection *iprvcol = NULL;
			hr = csmgr->get_EnumPrivateConnections(ICSSC_DEFAULT, &iprvcol);
			if(SUCCEEDED(hr))
			{
				if(iprvcol->get_Count(&prvconns) == S_OK && prvconns > 0)
				{
					if(iprvcol->get__NewEnum(&ienum) == S_OK)
					{
						IEnumNetSharingPrivateConnection *iprvs = NULL;
						hr = ienum->QueryInterface(IID_IEnumNetSharingPrivateConnection, (void**)&iprvs);
						if(SUCCEEDED(hr))
						{
							VariantInit(&vconn);
							iprvs->Reset();
							while(iprvs->Next(1, &vconn, &lfetched) == S_OK)
							{
								prvactive = true;
								iconn = NULL;
								iconn = (INetConnection*)vconn.punkVal;
								VariantClear(&vconn);
							}
							
							iprvs->Release();
						}
						
						ienum->Release();
						ienum = NULL;
					}
				}
				
				iprvcol->Release();
			}
		}
		
		csmgr->Release();
	}

	return (pubconns & prvconns) && (pubactive & prvactive);
}

void checksetport(int port)
{
	int result;
	if(result = CheckFirewallPortState(port, NET_FW_IP_PROTOCOL_TCP))
	{
		if(result > 0)
			debug("FireWall TCP %i ok",port);
		else
		{
			debug("Check FireWall TCP %i blocked",port);
		}
	}
}

void FirewallCheck(HWND hwnd)
{
	Service_status=false;
	FireWall_status=true;
	CheckServiceState();
	int result = 0;

	if(result = CheckFirewallPortState(2869L, NET_FW_IP_PROTOCOL_TCP))
	{
		if(result > 0)
			debug("FireWall TCP 2869 enabled");
		else
		{
			FireWall_status=false;
			debug("FireWall TCP 2869 disabled");
		}
	}
	else
		debug("FireWall TCP 2869 error checking");

	if(result = CheckFirewallPortState(1900L, NET_FW_IP_PROTOCOL_UDP))
	{
		if(result > 0)
			debug("FireWall UDP 1900 enabled");
		else
		{
			FireWall_status=false;
			debug("FireWall UDP 1900 disabled");
		}
	}
	else
		debug("FireWall UDP 1900 error checking");
	SendMessage(hwnd,WM_COMMAND,2001,0);
}

void CheckServiceState()
{
	SC_HANDLE hscm = NULL;
	hscm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if(hscm == NULL)
		return;

	SC_HANDLE hsrv = NULL;
	hsrv = OpenService(hscm, "ssdpsrv", SERVICE_ALL_ACCESS);
	if(hsrv)
	{
		SERVICE_STATUS srvstatus;

		if(QueryServiceStatus(hsrv, &srvstatus))
		{
			switch(srvstatus.dwCurrentState)
			{
			case SERVICE_START_PENDING :
				debug("UPnP service: Start pending");
				break;
			case SERVICE_STOP_PENDING :
				debug("UPnP service:Stop pending");
				break;
			case SERVICE_PAUSE_PENDING :
				debug("UPnP service:Pause pending");
				break;
			case SERVICE_CONTINUE_PENDING :
				debug("UPnP service:Continue pending");
				break;
			case SERVICE_RUNNING :
				Service_status=true;
				debug("UPnP service:Running");
				break;
			case SERVICE_STOPPED :
				debug("UPnP service:Stopped");
				break;
			case SERVICE_PAUSED :
				debug("UPnP service:Paused");
				break;
			default:
				debug("UPnP service: Fatal Error");
				break;
			}
		}
		
		CloseServiceHandle(hsrv);
	}

	CloseServiceHandle(hscm);
}

bool ControlSSDPService(bool start)
{
	bool result = false;
	
	SC_HANDLE hscm = NULL;
	hscm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if(hscm == NULL)
		return result;

	SC_HANDLE hsrv = NULL;
	hsrv = OpenService(hscm, _T("ssdpsrv"), SERVICE_ALL_ACCESS);
	if(hsrv)
	{
		SERVICE_STATUS srvstatus;

		if(QueryServiceStatus(hsrv, &srvstatus))
		{
			HANDLE hevent = CreateEvent(0, true, false, _T("dummyevent"));
			
			// if service_*_pending then wait for complete operation
			switch(srvstatus.dwCurrentState)
			{
			case SERVICE_START_PENDING :
			case SERVICE_STOP_PENDING :
				for(int i = 0; i < 10; ++i)
				{
					if(!QueryServiceStatus(hsrv, &srvstatus))
						break;
					if(srvstatus.dwCurrentState == SERVICE_RUNNING || srvstatus.dwCurrentState == SERVICE_STOPPED)
						break;
					WaitForSingleObject(hevent, 1000);
				}
				break;
			}

			// operation completed, so change current state
			if(QueryServiceStatus(hsrv, &srvstatus))
			{
				switch(srvstatus.dwCurrentState)
				{
				case SERVICE_RUNNING : // stop it
					if(!start && ControlService(hsrv, SERVICE_CONTROL_STOP, &srvstatus))
						result = true;
					break;
				case SERVICE_STOPPED : // start it
					if(start && StartService(hsrv, 0, NULL))
						result = true;
					break;
				}

				// wait for complete operation
				if(result)
				{
					for(int i = 0; i < 10; ++i)
					{
						if(!QueryServiceStatus(hsrv, &srvstatus))
							break;
						if(srvstatus.dwCurrentState == SERVICE_RUNNING || srvstatus.dwCurrentState == SERVICE_STOPPED)
						{
							// notify scm
							//ControlService(hsrv, SERVICE_CONTROL_INTERROGATE, &srvstatus);
							break;
						}
						WaitForSingleObject(hevent, 1000);
					}
				}
			}
		}
		
		CloseServiceHandle(hsrv);
	}

	CloseServiceHandle(hscm);
	
	return result;
}


