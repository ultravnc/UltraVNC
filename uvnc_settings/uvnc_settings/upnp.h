// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


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
