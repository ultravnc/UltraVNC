// upnp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "log.h"

#include "UPnP.h"
#include <comdef.h>

#include <iphlpapi.h>
#pragma comment ( lib, "iphlpapi" )
#include <shlwapi.h>
#pragma comment ( lib, "shlwapi" )




UPnP::UPnP()
{
}
void UPnP::Set_UPnP(char *theIPAddress, char *theProtocol, char *theDescription, const short thePort) {
	// need some messy string conversions in here
	// to convert STL::string to BSTR type strings
	// required for the UPnP code.
	// if anyone knows a better way, please tell....

	PortsAreOpen		= false;
	PortNumber		    = thePort;

	_bstr_t bstr1;
	_bstr_t bstr2;
	_bstr_t bstr3;
	_bstr_t bstr4;
	bstr1=theIPAddress;
	bstr2=theProtocol;
	bstr3=theDescription;
	bstr4="";

	bstrInternalClient=bstr1.copy();
	bstrProtocol=bstr2.copy();
	bstrDescription=bstr3.copy();
	bstrExternalIP=bstr4.copy();
	pUN = NULL;
}


// Opens the UPnP ports defined when the object was created
HRESULT UPnP::OpenPorts(bool log) {
	CoInitializeEx ( NULL ,COINIT_MULTITHREADED);
	HRESULT hr = CoCreateInstance (__uuidof(UPnPNAT),
		NULL,
		CLSCTX_INPROC_SERVER,
		__uuidof(IUPnPNAT),
		(void**)&pUN);

	if(SUCCEEDED(hr)) {
		IStaticPortMappingCollection * pSPMC = NULL;
		hr = pUN->get_StaticPortMappingCollection (&pSPMC);
		int counter=0;
		while(SUCCEEDED(hr) && !pSPMC && counter<20)
		{
			if(log)debug("StaticPortMappingCollection failed");
			hr = pUN->get_StaticPortMappingCollection (&pSPMC);
			counter++;
		}
		if(SUCCEEDED(hr) && pSPMC) {
			// see comment in "else"
			if(bstrProtocol && bstrInternalClient && bstrDescription) {
				IStaticPortMapping * pSPM = NULL;
				hr = pSPMC->Add(PortNumber,
					bstrProtocol,
					PortNumber,
					bstrInternalClient,
					VARIANT_TRUE,
					bstrDescription,
					&pSPM
				);

				if(SUCCEEDED(hr)) {
					if(log)debug("UPNP: open port %i",PortNumber);
					PortsAreOpen = true;
				}
				else
				{
					if(log)debug("UpNP: fail %s, Ignore external IP",_com_error(hr).ErrorMessage());
				}
			} else {
				hr = E_OUTOFMEMORY;
				debug("UPNP: Failed ");
			}
		} else {
			debug("UPNP: Failed ");
			hr = E_FAIL;    // work around a known bug here:  in some error
			// conditions, get_SPMC NULLs out the pointer, but incorrectly returns a success code.
		}
		pUN->Release();
		pUN=NULL;
	}
	CoUninitialize();
	return hr;
}

// Closes the UPnP ports defined when the object was created
HRESULT UPnP::ClosePorts(bool log) {
//	if(PortsAreOpen == false) {
//		return S_OK;
//	}

	HRESULT hr = E_FAIL;
	HRESULT hr2 = E_FAIL;

	CoInitializeEx ( NULL ,COINIT_MULTITHREADED);
	hr = CoCreateInstance (__uuidof(UPnPNAT),
		NULL,
		CLSCTX_INPROC_SERVER,
		__uuidof(IUPnPNAT),
		(void**)&pUN);

	if(SUCCEEDED(hr)) {

			if(bstrProtocol && bstrInternalClient && bstrDescription) {
				IStaticPortMappingCollection * pSPMC = NULL;
				hr2 = pUN->get_StaticPortMappingCollection (&pSPMC);

				if(SUCCEEDED(hr2) && pSPMC) {
					hr = pSPMC->Remove(PortNumber, bstrProtocol);
					if(SUCCEEDED(hr)) if(log)debug("UPNP: closed port %i",PortNumber);
					pSPMC->Release();
				}

				SysFreeString(bstrProtocol);
				SysFreeString(bstrInternalClient);
				SysFreeString(bstrDescription);
				SysFreeString(bstrExternalIP);
			}
			pUN->Release();
			pUN=NULL;
	}
	CoUninitialize();
	return hr;
}

// Returns the current external IP address
_bstr_t UPnP::GetExternalIP() {
	HRESULT hr;

	// Check if we opened the desired port, 'cause we use it for getting the IP
	// This shouldn't be a problem because we only try to get the external IP when
	// we opened the mapping
	// This function is not used somewhere else, hence it is "save" to do it like this
	if(!PortsAreOpen) {
  		return "";
  	}
	BSTR bstrExternal = NULL;
	_bstr_t bstrWrapper;
	CoInitializeEx ( NULL ,COINIT_MULTITHREADED);
	hr = CoCreateInstance (__uuidof(UPnPNAT),
		NULL,
		CLSCTX_INPROC_SERVER,
		__uuidof(IUPnPNAT),
		(void**)&pUN);

	if(SUCCEEDED(hr))
	{
			// Get the Collection
			IStaticPortMappingCollection *pIMaps = NULL;
			hr = pUN->get_StaticPortMappingCollection(&pIMaps);

			// Check it
			// We also check against that bug mentioned in OpenPorts()
			if(!SUCCEEDED(hr) || !pIMaps ) {
				 // Only release when OK
				if(pIMaps != NULL) {
					pIMaps->Release();
				}
				pUN->Release();
				pUN=NULL;
				CoUninitialize();
				return "";
			}

			// Lets Query our mapping
			IStaticPortMapping *pISM;
			hr = pIMaps->get_Item(
				PortNumber,
				bstrProtocol,
				&pISM
			);

			// Query failed!
			if(!SUCCEEDED(hr)) {
  				pIMaps->Release();
				pUN->Release();
				pUN=NULL;
				CoUninitialize();
  				return "";
  			}

			// Get the External IP from our mapping
			hr = pISM->get_ExternalIPAddress(&bstrExternal);

			// D'OH. Failed
			if(!SUCCEEDED(hr)) {
  				pIMaps->Release();
				pISM->Release();
				pUN->Release();
				pUN=NULL;
				CoUninitialize();
  				return "";
  			}

			// Check and convert the result
			if(bstrExternal != NULL) {
				bstrWrapper.Assign(bstrExternal);
  			} else {
				bstrWrapper = "";
  			}

			// no longer needed
			SysFreeString(bstrExternal);

			// no longer needed
  			pIMaps->Release();
			pISM->Release();
			pUN->Release();
			pUN=NULL;
			CoUninitialize();
	}

  	return bstrWrapper;
}

UPnP::~UPnP() {
	if (pUN) {
		pUN->Release();
	}
}


bool mapport(short port)
{
	UPnP UPnPvar;

	PIP_ADAPTER_INFO pAdapterInfo = NULL;
	DWORD dwRetVal = 0;
	char strLocalIP[256];
	
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
	}
	else 
	{
		strcpy(strLocalIP,"");
	}
	
	free(pAdapterInfo);


	UPnPvar.Set_UPnP(strLocalIP,"TCP","UVNC",port);
	UPnPvar.OpenPorts(true);
	char testchar[256];
	strcpy(testchar,UPnPvar.GetExternalIP());
	UPnPvar.ClosePorts(true);
	return 0;
}


