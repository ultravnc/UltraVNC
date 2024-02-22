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


#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include "../../udt4/src/udt.h"
#pragma comment(lib, "Ws2_32.lib")
#define MAX_HOST_NAME_LEN 250
class Proxy;


enum CONNECTIONTYPE { ctVIEWER, ctSERVER, ctInfo };
enum CONNECTIONSTATUS { csUnknown, csConnecting, csOnline, csRendezvous, csConnected, csError };
#define NATREMOTE 5352
#define ADDR_LOCALHOST "127.0.0.1"

struct udppacket
{
	char name[32];
	char group[32];
	char ident[8];
	char localip[32];
	char externip[32];
	int localport;
	int externport;
	int contype;
	bool serverviewer;
	bool dummy0;
	bool dummy1;
	bool dummy2;
	udppacket() {
	memset(ident, 0, 8);
	memset(localip, 0, 32);
	memset(externip, 0, 32);
	memset(name, 0, 32);
	memset(group, 0, 32);
	localport = 0;
	externport = 0;
	contype = 0;
	};
};

class CloudThread
{

private:
	WSADATA			wsaData = {};
	SOCKET			udpSocket = INVALID_SOCKET;
	SOCKET			serverSocket = INVALID_SOCKET;
	SOCKET			viewerSocket = INVALID_SOCKET;
	SOCKET			listenSocket = INVALID_SOCKET;
	SOCKADDR_IN		receiverAddr;
	int				udpPort = 0;
	char			server[MAX_HOST_NAME_LEN]{};
	char			code[32]{};
	CONNECTIONTYPE		ctType = ctInfo;
	CONNECTIONSTATUS	cStatus = csUnknown;
	HANDLE			hThread = NULL;
	bool			threadRunning = false;
	int				port = 0;
	char			G_externalip[16]{};

	static DWORD WINAPI StaticThreadStart(void* Param);
	DWORD ThreadStart(void);
	UDTSOCKET		usock;
	void ConnectToVNCServer();
	void ConnectVNCViewer();
	bool rendezvous(UDPSOCKET udpsock, char* ip, int port);
	bool SendInfo();
	int m_VncPort = 0;
	Proxy *proxy = NULL;

public:
	CloudThread();
	~CloudThread();
	
	void startThread(int port, char* server, char* code, CONNECTIONTYPE type);
	void stopThread();
	bool isThreadRunning();
	char* getExternalIpAddress();
	CONNECTIONSTATUS getStatus();
	void setVNcPort(int port);
};

