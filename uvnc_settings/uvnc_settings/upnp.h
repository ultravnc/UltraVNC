/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
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
//  If the source code for the program is not available from the place from
//  which you received this file, check
//  https://uvnc.com/
//
////////////////////////////////////////////////////////////////////////////


#if !defined(UPNP_H)
#define UPNP_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#include <stdio.h>
#include <tchar.h>
#include <windows.h>

#include <natupnp.h>
#include <comutil.h>

class UPnP
{
public:
	UPnP();
	~UPnP();
	void Set_UPnP(char * , char *, char *, const short );
	HRESULT OpenPorts(bool log);
	HRESULT ClosePorts(bool log);
	_bstr_t GetExternalIP();
private:
	bool PortsAreOpen;
	int PortNumber;				// The Port number required to be opened
	BSTR bstrInternalClient;	// Local IP Address
	BSTR bstrDescription;		// name shown in UPnP interface details
	BSTR bstrProtocol;			// protocol (TCP or UDP)
	BSTR bstrExternalIP;		// external IP address
	IUPnPNAT* pUN;				// pointer to the UPnPNAT interface
	IStaticPortMappingCollection* pSPMC; // pointer to the collection
	IStaticPortMapping * pSPM;	// pointer to the port map
};

#endif // UPNP_H
