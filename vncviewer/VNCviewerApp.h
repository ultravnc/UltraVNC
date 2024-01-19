/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2013 UltraVNC Team Members. All Rights Reserved.
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
// If the source code for the program is not available from the place from
// which you received this file, check 
// http://www.uvnc.com/
//
////////////////////////////////////////////////////////////////////////////
 

#ifndef VNCVIEWERAPP_H__
#define VNCVIEWERAPP_H__

#pragma once

// The state of the application as a whole is contained in the app object
class VNCviewerApp;

// I doubt we'll ever want more simultaneous connections than this
#define MAX_CONNECTIONS 128

#include "ClientConnection.h"

class ClientConnection;

class VNCviewerApp {
public:
	VNCviewerApp(HINSTANCE hInstance, LPTSTR szCmdLine);

	virtual void NewConnection(bool Is_Listening) = 0;
	virtual void NewConnection(bool Is_Listening,TCHAR *host, int port) = 0;
	virtual void NewConnection(bool Is_Listening,SOCKET sock) = 0;
		
	~VNCviewerApp();

	// This should be used by Connections to register and deregister 
	// themselves.  When the last connection is deregistered, the app
	// will close unless in listening mode.
	void RegisterConnection(ClientConnection *pConn);
	void DeregisterConnection(ClientConnection *pConn);
	
	VNCOptions m_options;
	HINSTANCE  m_instance;

private:
	ClientConnection *m_clilist[MAX_CONNECTIONS];
	omni_mutex m_clilistMutex;
};

#endif
