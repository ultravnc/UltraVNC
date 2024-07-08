/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
//  Copyright (C) 2001 HorizonLive.com, Inc. All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
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


// VSocket.cpp

// The VSocket class provides a platform-independent socket abstraction
// with the simple functionality required for an RFB server.

class VSocket;

////////////////////////////////////////////////////////
// System includes

#define _WINSOCK_DEPRECATED_NO_WARNINGS 1

#include "stdhdrs.h"
#include <iostream>

#include <stdio.h>
#ifdef __WIN32__
#include <io.h>
#include <winsock2.h>
#include <in6addr.h>
#include <mstcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#endif
#include <sys/types.h>

////////////////////////////////////////////////////////
// Custom includes

#include <assert.h>
#include "vtypes.h"
#include "SettingsManager.h"
////////////////////////////////////////////////////////
// *** Lovely hacks to make Win32 work. Hurrah!

#ifdef __WIN32__
#ifndef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif
#endif

////////////////////////////////////////////////////////
// Socket implementation

#include "vsocket.h"

// The socket timeout value (currently 5 seconds, for no reason...)
// *** THIS IS NOT CURRENTLY USED ANYWHERE

////////////////////////////
// Socket implementation initialisation
static WORD winsockVersion = 0;
bool sendall(SOCKET RemoteSocket,char *buff,unsigned int bufflen,int dummy);

VSocketSystem::VSocketSystem()
{
  // Initialise the socket subsystem
  // This is only provided for compatibility with Windows.

#ifdef __WIN32__
  // Initialise WinPoxySockets on Win32
  WORD wVersionRequested;
  WSADATA wsaData;
	
  wVersionRequested = MAKEWORD(2, 0);
  if (WSAStartup(wVersionRequested, &wsaData) != 0)
  {
    m_status = VFalse;
	return;
  }

  winsockVersion = wsaData.wVersion;
 
#else
  // Disable the nasty read/write failure signals on UNIX
  signal(SIGPIPE, SIG_IGN);
#endif

  // If successful, or if not required, then continue!
  m_status = VTrue;
}

VSocketSystem::~VSocketSystem()
{
	if (m_status)
	{
		WSACleanup();
	}
}

////////////////////////////


// adzm 2010-08
int VSocket::m_defaultSocketKeepAliveTimeout = 10000;

VSocket::VSocket()
{
	// Clear out the internal socket fields

	sock4 = INVALID_SOCKET;
	sock6 = INVALID_SOCKET;
	sock = INVALID_SOCKET;

	//vnclog.Print(LL_SOCKINFO, VNCLOG("VSocket() m_pDSMPlugin = NULL \n"));
	m_pDSMPlugin = NULL;
	//adzm 2009-06-20
	m_pPluginInterface = NULL;
	//adzm 2010-05-10
	m_pIntegratedPluginInterface = NULL;
	m_fUsePlugin = false;
	
	m_pNetRectBuf = NULL;
	m_nNetRectBufSize = 0;
	m_fWriteToNetRectBuf = false;
	m_nNetRectBufOffset = 0;
	queuebuffersize=0;
	memset( queuebuffer, 0, sizeof( queuebuffer ) );

	//adzm 2010-08-01
	m_LastSentTick = 0;

	//adzm 2010-09
	m_fPluginStreamingIn = false;
	m_fPluginStreamingOut = false;	
	G_SENDBUFFER= settings->getSENDBUFFER_EX();
}

////////////////////////////

VSocket::~VSocket()
{
  // Close the socket
  Close();
  if (m_pNetRectBuf != NULL)
 	delete [] m_pNetRectBuf;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

VBool
VSocket::CreateConnect(const VString address, const VCard port)
{
	const int one = 1;
	int RetVal;
	ADDRINFO Hints, *AddrInfo, *AI;

	memset(&Hints, 0, sizeof(Hints));
	Hints.ai_family = AF_UNSPEC;
	Hints.ai_socktype = SOCK_STREAM;
	char temp [10];
	_itoa(port, temp, 10);
	RetVal = getaddrinfo(address, temp, &Hints, &AddrInfo);
	if (RetVal != 0) return false;

	for (AI = AddrInfo; AI != NULL; AI = AI->ai_next) {
		if (AI->ai_family == AF_INET)
		{
			sock4 = socket(AI->ai_family, AI->ai_socktype, AI->ai_protocol);
			if (sock4 == INVALID_SOCKET) continue;
			setsockopt(sock4, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one));
			SetDefaultSocketOptions4();
			if (connect(sock4, AI->ai_addr, (int)AI->ai_addrlen) != SOCKET_ERROR)
				{
					SetDefaultSocketOptions4();
					break;
				}
			else
			{
				closesocket(sock4);
				sock4 = INVALID_SOCKET;
			}
		}
		if (AI->ai_family == AF_INET6)
		{
			sock6 = socket(AI->ai_family, AI->ai_socktype, AI->ai_protocol);
			if (sock6 == INVALID_SOCKET) continue;
			setsockopt(sock6, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one));
			SetDefaultSocketOptions6();
			if (connect(sock6, AI->ai_addr, (int)AI->ai_addrlen) != SOCKET_ERROR)
			{
				SetDefaultSocketOptions6();
				break;
			}
			else
			{
				closesocket(sock6);
				sock6 = INVALID_SOCKET;
			}
		}
	}
	freeaddrinfo(AddrInfo);
	if (AI == NULL) return false;
	return VTrue;
}
VBool
VSocket::CreateBindConnect(const VString address, const VCard port)
{
	const int one = 1;
	int RetVal;
	ADDRINFO Hints, *AddrInfo, *AI;

	memset(&Hints, 0, sizeof(Hints));
	Hints.ai_family = PF_UNSPEC;
	Hints.ai_socktype = SOCK_STREAM;
	char temp[10];
	_itoa(port, temp, 10);
	RetVal = getaddrinfo(address, temp, &Hints, &AddrInfo);
	if (RetVal != 0) return false;

	for (AI = AddrInfo; AI != NULL; AI = AI->ai_next) {
		if (AI->ai_family == AF_INET)
		{
			sock4 = socket(AI->ai_family, AI->ai_socktype, AI->ai_protocol);
			if (sock4 == INVALID_SOCKET) continue;
			setsockopt(sock4, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one));
			SetDefaultSocketOptions4();
			Bind4(0);
			if (connect(sock4, AI->ai_addr, (int)AI->ai_addrlen) != SOCKET_ERROR)
				{
					SetDefaultSocketOptions4();
					break;
				}
			else
			{
				closesocket(sock6);
				sock6 = INVALID_SOCKET;
			}
		}
		if (AI->ai_family == AF_INET6)
		{
			sock6 = socket(AI->ai_family, AI->ai_socktype, AI->ai_protocol);
			if (sock6 == INVALID_SOCKET) continue;
			setsockopt(sock6, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one));
			SetDefaultSocketOptions6();
			Bind6(0);
			if (connect(sock6, AI->ai_addr, (int)AI->ai_addrlen) != SOCKET_ERROR)
			{
				SetDefaultSocketOptions6();
				break;
			}
			else
			{
				closesocket(sock6);
				sock6 = INVALID_SOCKET;
			}
		}
	}
	freeaddrinfo(AddrInfo);
	if (AI == NULL) return VFalse;
	return VTrue;
}

VBool	VSocket::CreateBindListen(const VCard port, const VBool localOnly)
{
	const int one = 1;
	int RetVal,i;
	ADDRINFO Hints, *AddrInfo, *AI;
	char temp[10];
	_itoa(port, temp, 10);

	memset(&Hints, 0, sizeof(Hints));
	Hints.ai_family = PF_UNSPEC;
	Hints.ai_socktype = SOCK_STREAM;
	Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;
	RetVal = getaddrinfo(NULL, temp, &Hints, &AddrInfo);
	if (RetVal != 0) return VFalse;
	for (i = 0, AI = AddrInfo; AI != NULL; AI = AI->ai_next) {
		if (i == FD_SETSIZE) break;
		if ((AI->ai_family != PF_INET) && (AI->ai_family != PF_INET6)) continue;


		if (AI->ai_family == AF_INET)
		{
			sock4 = socket(AI->ai_family, AI->ai_socktype, AI->ai_protocol);
			if (sock4 == INVALID_SOCKET) continue;
			setsockopt(sock4, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one));
			SetDefaultSocketOptions4();
			if (!Bind4(port, localOnly))
			{
				closesocket(sock4);
				sock4 = INVALID_SOCKET;
				continue;
			}

			if (!Listen4())
			{
				closesocket(sock4);
				sock4 = INVALID_SOCKET;
				continue;
			}
		}

		if (AI->ai_family == AF_INET6)
		{
			sock6 = socket(AI->ai_family, AI->ai_socktype, AI->ai_protocol);
			if (sock6 == INVALID_SOCKET) continue;
			setsockopt(sock6, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one));
			SetDefaultSocketOptions6();
			if (!Bind6(port, localOnly))
			{
				closesocket(sock6);
				sock6 = INVALID_SOCKET;
				continue;
			}
			if (!Listen6())
			{
				closesocket(sock6);
				sock6 = INVALID_SOCKET;
				continue;
			}	
		}
		i++;
	}
	freeaddrinfo(AddrInfo);
	if (i == 0) return VFalse;
	return VTrue;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
VBool
VSocket::Create()
{
  const int one = 1;
  // Check that the old socket was closed
  if (sock != INVALID_SOCKET)
    Close();

  // Create the socket
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
      return VFalse;
    }
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one)))
  {
	  return VFalse;
  }
  
  //#endif

  // adzm 2010-08
  SetDefaultSocketOptions();
  return VTrue;
}

////////////////////////////

VBool
VSocket::CloseIPV6()
{
	if (sock4 != INVALID_SOCKET) Close4();
	if (sock6 != INVALID_SOCKET) Close6();
		return true;
}
VBool
VSocket::Close4()
{
	vnclog.Print(LL_SOCKINFO, VNCLOG("closing socket\n"));
	shutdown(sock4, SD_BOTH);
	closesocket(sock4);
	sock4 = INVALID_SOCKET;

	//adzm 2009-06-20
	if (m_pPluginInterface) {
		delete m_pPluginInterface;
		m_pPluginInterface=NULL;
		//adzm 2010-05-10
		m_pIntegratedPluginInterface=NULL;
	}
	return VTrue;
}
VBool
VSocket::Close6()
{
	vnclog.Print(LL_SOCKINFO, VNCLOG("closing socket\n"));
	shutdown(sock6, SD_BOTH);
	closesocket(sock6);
	sock6 = INVALID_SOCKET;

	//adzm 2009-06-20
	if (m_pPluginInterface) {
		delete m_pPluginInterface;
		m_pPluginInterface = NULL;
		//adzm 2010-05-10
		m_pIntegratedPluginInterface = NULL;
	}
	return VTrue;
}

VBool
VSocket::CloseIPV4()
{
  if (sock != INVALID_SOCKET)
    {
	  vnclog.Print(LL_SOCKINFO, VNCLOG("closing socket\n"));
	  shutdown(sock, SD_BOTH);
#ifdef __WIN32__
	  closesocket(sock);
#else
	  close(sock);
#endif
      sock = INVALID_SOCKET;
    }

  //adzm 2009-06-20
  if (m_pPluginInterface) {
    delete m_pPluginInterface;
	m_pPluginInterface=NULL;
	//adzm 2010-05-10
	m_pIntegratedPluginInterface=NULL;
  }
  return VTrue;
}


VBool
VSocket::Close()
{
	if (settings->getIPV6())
		return CloseIPV6();
	else
		return CloseIPV4();
}


////////////////////////////

VBool
VSocket::ShutdownIPV6()
{
	//shutdown both, if not used sock=-1
	if (sock4 != INVALID_SOCKET) Shutdown4();
	if (sock6 != INVALID_SOCKET)	Shutdown6();
		return true;
}
VBool
VSocket::Shutdown4()
{
	vnclog.Print(LL_SOCKINFO, VNCLOG("shutdown socket\n"));
	shutdown(sock4, SD_BOTH);
	return VTrue;
}
VBool
VSocket::Shutdown6()
{
	vnclog.Print(LL_SOCKINFO, VNCLOG("shutdown socket\n"));
	shutdown(sock6, SD_BOTH);
	return VTrue;
}

VBool
VSocket::ShutdownIPV4()
{
  if (sock != INVALID_SOCKET)
    {
	  vnclog.Print(LL_SOCKINFO, VNCLOG("shutdown socket\n"));

	  shutdown(sock, SD_BOTH);
	  closesocket(sock);
//	  sock = INVALID_SOCKET;
    }
  return VTrue;
}

VBool
VSocket::Shutdown()
{
	if (settings->getIPV6())
		return ShutdownIPV6();
	else
		return ShutdownIPV4();
}
////////////////////////////

VBool
VSocket::Bind4(const VCard port, const VBool localOnly)
{
	struct sockaddr_in addr;

	// Check that the socket is open!
	if (sock4 == INVALID_SOCKET)
		return VFalse;

	// Set up the address to bind the socket to
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if (localOnly)
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	else
		addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// And do the binding
	if (bind(sock4, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
		return VFalse;

	return VTrue;
}
VBool
VSocket::Bind6(const VCard port, const VBool localOnly)
{
	struct sockaddr_in6 addr;
	memset(&addr, 0, sizeof(addr));
	// Check that the socket is open!
	if (sock6 == INVALID_SOCKET)
		return VFalse;

	// Set up the address to bind the socket to
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(port);
	if (localOnly)
		addr.sin6_addr = in6addr_loopback;
	else
		addr.sin6_addr = in6addr_any;

	// And do the binding
	if (bind(sock6, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		DWORD lerror = WSAGetLastError();
		return VFalse;
	}

	return VTrue;
}

VBool
VSocket::Bind(const VCard port, const VBool localOnly)
{
  struct sockaddr_in addr;

  // Check that the socket is open!
  if (sock == INVALID_SOCKET)
    return VFalse;

  // Set up the address to bind the socket to
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (localOnly)
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  else
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

  // And do the binding
  if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
      return VFalse;

  return VTrue;
}


////////////////////////////
VBool
VSocket::Connect(const VString address, const VCard port)
{
  // Check the socket
  if (sock == INVALID_SOCKET)
    return VFalse;

  // Create an address structure and clear it
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));

  // Fill in the address if possible
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(address);

  // Was the string a valid IP address?
  if ((int) addr.sin_addr.s_addr == -1)
    {
      // No, so get the actual IP address of the host name specified
      struct hostent *pHost=NULL;
      pHost = gethostbyname(address);
      if (pHost != NULL)
	  {
		  if (pHost->h_addr == NULL)
			  return VFalse;
		  addr.sin_addr.s_addr = ((struct in_addr *)pHost->h_addr)->s_addr;
	  }
	  else
	    return VFalse;
    }

  // Set the port number in the correct format
  addr.sin_port = htons(port);

  // Actually connect the socket
  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) != 0)
    return VFalse;
  // adzm 2010-08
  SetDefaultSocketOptions();

  return VTrue;
}

////////////////////////////
VBool
VSocket::Listen4()
{
	// Check socket
	if (sock4 == INVALID_SOCKET)
		return VFalse;

	// Set it to listen
	if (listen(sock4, 5) == SOCKET_ERROR)
		return VFalse;

	return VTrue;
}
VBool
VSocket::Listen6()
{
	// Check socket
	if (sock6 == INVALID_SOCKET)
		return VFalse;

	// Set it to listen
	if (listen(sock6, 5) == SOCKET_ERROR)
		return VFalse;

	return VTrue;
}

VBool
VSocket::Listen()
{
  // Check socket
  if (sock == INVALID_SOCKET)
    return VFalse;

	// Set it to listen
  if (listen(sock, 5) == SOCKET_ERROR)
    return VFalse;

  return VTrue;
}
////////////////////////////
VSocket *
VSocket::AcceptIPV6()
{
	fd_set SockSet;

	if (sock4 != INVALID_SOCKET && sock6 != INVALID_SOCKET)
	{
		FD_ZERO(&SockSet);
		while (1)
		{
			FD_SET(sock4, &SockSet);
			FD_SET(sock6, &SockSet);
			int result = select(2, &SockSet, 0, 0, 0);
			if (result == SOCKET_ERROR) return NULL;
			if (FD_ISSET(sock4, &SockSet))
			{
				FD_CLR(sock4, &SockSet);
				return Accept4();
			}
			else
				if (FD_ISSET(sock6, &SockSet))
				{
					FD_CLR(sock6, &SockSet);
					return Accept6();
				}
		}
	}

	else if (sock4 != INVALID_SOCKET) return  Accept4();
	else if (sock6 != INVALID_SOCKET) return  Accept6();
	return NULL;
}

VSocket *
VSocket::Accept4()
{
	int new_socket_id;
	VSocket * new_socket;

	// Check this socket
	if (sock4 == INVALID_SOCKET)
		return NULL;

	int optVal;
	int optLen = sizeof(int);
	getsockopt(sock4, SOL_SOCKET, SO_SNDBUF, (char *)&optVal, &optLen);
	optVal = 32 * 1024;
	setsockopt(sock4, SOL_SOCKET, SO_SNDBUF, (char *)&optVal, optLen);

	// Accept an incoming connection
	if ((new_socket_id = accept(sock4, NULL, 0)) == INVALID_SOCKET)
		return NULL;

	// Create a new VSocket and return it
	new_socket = new VSocket;
	if (new_socket != NULL)
	{
		new_socket->sock4 = new_socket_id;
	}
	else
	{
		shutdown(new_socket_id, SD_BOTH);
		closesocket(new_socket_id);
		return NULL;
	}
	// adzm 2010-08
	new_socket->SetDefaultSocketOptions4();

	// Put the socket into non-blocking mode
	return new_socket;
}
VSocket *
VSocket::Accept6()
{
	int new_socket_id;
	VSocket * new_socket;

	// Check this socket
	if (sock6 == INVALID_SOCKET)
		return NULL;

	int optVal;
	int optLen = sizeof(int);
	getsockopt(sock6, SOL_SOCKET, SO_SNDBUF, (char *)&optVal, &optLen);
	optVal = 32 * 1024;
	setsockopt(sock6, SOL_SOCKET, SO_SNDBUF, (char *)&optVal, optLen);

	// Accept an incoming connection
	if ((new_socket_id = accept(sock6, NULL, 0)) == INVALID_SOCKET)
		return NULL;

	// Create a new VSocket and return it
	new_socket = new VSocket;
	if (new_socket != NULL)
	{
		new_socket->sock6 = new_socket_id;
	}
	else
	{
		shutdown(new_socket_id, SD_BOTH);
		closesocket(new_socket_id);
		return NULL;
	}
	// adzm 2010-08
	new_socket->SetDefaultSocketOptions6();

	// Put the socket into non-blocking mode
	return new_socket;
}

VSocket *
VSocket::AcceptIPV4()
{
	SOCKET new_socket_id;
	VSocket * new_socket;

	// Check this socket
	if (sock == INVALID_SOCKET)
		return NULL;

	int optVal;
	int optLen = sizeof(int);
	getsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&optVal, &optLen);
	optVal = 32 * 1024;
	setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&optVal, optLen);

	// Accept an incoming connection
	if ((new_socket_id = accept(sock, NULL, 0)) == INVALID_SOCKET)
		return NULL;

  // Create a new VSocket and return it
  new_socket = new VSocket;
  if (new_socket != NULL)
  {
      new_socket->sock = new_socket_id;
  }
  else
  {
	  shutdown(new_socket_id, SD_BOTH);
	  closesocket(new_socket_id);
	  return NULL;
  }
  // adzm 2010-08
  new_socket->SetDefaultSocketOptions();

  // Put the socket into non-blocking mode
  return new_socket;
}

VSocket*
VSocket::Accept()
{
	if (settings->getIPV6())
		return AcceptIPV6();
	else
		return AcceptIPV4();
}

////////////////////////////
////////////////////////////

VString
VSocket::GetPeerNameIPV6(bool naam)
{
	if (sock4 != INVALID_SOCKET) return GetPeerName4(naam);
	if (sock6 != INVALID_SOCKET) return GetPeerName6(naam);
	return "<unavailable>";
}
VString
VSocket::GetPeerName4(bool naam)
{
	struct sockaddr_in	sockinfo;
	struct in_addr		address;
	int					sockinfosize = sizeof(sockinfo);
	VString				name;

	// Get the peer address for the client socket
	getpeername(sock4, (struct sockaddr *)&sockinfo, &sockinfosize);
	memcpy(&address, &sockinfo.sin_addr, sizeof(address));

	name = inet_ntoa(address);
	if (name == NULL)
		return "<unavailable>";
	else
		return name;
}

char straddr[INET6_ADDRSTRLEN];
VString
VSocket::GetPeerName6(bool naam)
{
	struct sockaddr_in6	sockinfo;
	struct in6_addr		address;
	int					sockinfosize = sizeof(sockinfo);	
	memset(straddr, 0, INET6_ADDRSTRLEN);
	memset(&sockinfo, 0, sizeof(sockinfo));
	memset(&address, 0, sizeof(address));
	// Get the peer address for the client socket
	getpeername(sock6, (struct sockaddr *)&sockinfo, &sockinfosize);
	memcpy(&address, &sockinfo.sin6_addr, sizeof(address));
	inet_ntop(AF_INET6, &address, straddr, sizeof(straddr));
	if (strlen(straddr) == 0) return "<unavailable>";
	else
		return straddr;
}

VString
VSocket::GetPeerNameIPV4(bool naam)
{
	// the lookup is to slow
	naam = false;
	struct sockaddr_in	sockinfo{};
	struct in_addr		address{};
	int					sockinfosize = sizeof(sockinfo);
	VString				name{};

	// Get the peer address for the client socket
	getpeername(sock, (struct sockaddr *)&sockinfo, &sockinfosize);
	memcpy(&address, &sockinfo.sin_addr, sizeof(address));

	

	if (naam) {
		struct hostent* remoteHost = gethostbyaddr((char*)&address, sizeof(address), AF_INET);
		if (remoteHost != NULL) {
			name = remoteHost->h_name;
			return name;
		}
	}

	name = inet_ntoa(address);;
	if (name == NULL)
		return "<unavailable>";
	else
		return name;
}


VString
VSocket::GetPeerName(bool naam)
{
	if (settings->getIPV6())
		return GetPeerNameIPV6(naam);
	else
		return GetPeerNameIPV4(naam);
}
////////////////////////////
VString
VSocket::GetSockNameIPV6()
{
	if (sock4 != INVALID_SOCKET ) return GetSockName4();
	if (sock6 != INVALID_SOCKET ) return GetSockName6();
	return "<unavailable>";
}

VString
VSocket::GetSockName4()
{
	struct sockaddr_in	sockinfo;
	struct in_addr		address;
	int					sockinfosize = sizeof(sockinfo);
	VString				name;

	// Get the peer address for the client socket
	getsockname(sock4, (struct sockaddr *)&sockinfo, &sockinfosize);
	memcpy(&address, &sockinfo.sin_addr, sizeof(address));

	name = inet_ntoa(address);
	if (name == NULL)
		return "<unavailable>";
	else
		return name;
}
char straddr2[INET6_ADDRSTRLEN];
VString
VSocket::GetSockName6()
{
	struct sockaddr_in6	sockinfo;
	struct in6_addr		address;
	int					sockinfosize = sizeof(sockinfo);
	
	memset(straddr2, 0, INET6_ADDRSTRLEN);
	memset(&sockinfo, 0, sizeof(sockinfo));
	memset(&address, 0, sizeof(address));
	// Get the peer address for the client socket
	getsockname(sock6, (struct sockaddr *)&sockinfo, &sockinfosize);
	memcpy(&address, &sockinfo.sin6_addr, sizeof(address));
	inet_ntop(AF_INET6, &address, straddr2, sizeof(straddr2));

	if (strlen(straddr2) == 0) return "<unavailable>";
	else
		return straddr2;
}

VString
VSocket::GetSockNameIPV4()
{
	struct sockaddr_in	sockinfo;
	struct in_addr		address;
	int					sockinfosize = sizeof(sockinfo);
	VString				name;

	// Get the peer address for the client socket
	getsockname(sock, (struct sockaddr *)&sockinfo, &sockinfosize);
	memcpy(&address, &sockinfo.sin_addr, sizeof(address));

	name = inet_ntoa(address);
	if (name == NULL)
		return "<unavailable>";
	else
		return name;
}

VString
VSocket::GetSockName()
{
	if (settings->getIPV6())
		return GetSockNameIPV6();
	else
		return GetSockNameIPV4();
}
// 25 January 2008 jdp

bool VSocket::GetPeerAddress4(char *address, int size)
{
	struct sockaddr_in addr;
	int addrsize = sizeof(sockaddr_in);

	if (sock4 == INVALID_SOCKET)
		return false;

	if (getpeername(sock4, (struct sockaddr *) &addr, &addrsize) != 0)
		return false;

	_snprintf_s(address, size, size, "%s:%d",inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));

	return true;
}
bool VSocket::GetPeerAddress6(char *address, int size)
{
	struct sockaddr_in6 addr;
	memset(&addr, 0, sizeof(addr));
	int addrsize = sizeof(sockaddr_in6);
	char straddr[INET6_ADDRSTRLEN];
	memset(straddr, 0, INET6_ADDRSTRLEN);
	if (sock6 == INVALID_SOCKET)
		return false;
	if (getpeername(sock6, (struct sockaddr *) &addr, &addrsize) != 0)
		return false;
	inet_ntop(AF_INET6, &addr.sin6_addr, straddr, sizeof(straddr));
	_snprintf_s(address, size, size, "%s:%d", straddr, ntohs(addr.sin6_port));

	return true;
}

bool VSocket::GetPeerAddress(char *address, int size)
{
    struct sockaddr_in addr;
    int addrsize = sizeof(sockaddr_in);

    if (sock == INVALID_SOCKET)
        return false;

    if (getpeername(sock, (struct sockaddr *) &addr, &addrsize) != 0)
        return false;

    _snprintf_s(address, size, size, "%s:%d",
              inet_ntoa(addr.sin_addr),
              ntohs(addr.sin_port));

    return true;
}

////////////////////////////

VCard32
VSocket::Resolve4(const VString address)
{
	VCard32 addr;

	// Try converting the address as IP
	addr = inet_addr(address);

	// Was it a valid IP address?
	if (addr == INADDR_NONE)
	{
		// No, so get the actual IP address of the host name specified
		struct hostent *pHost=NULL;
		pHost = gethostbyname(address);
		if (pHost != NULL)
		{
			if (pHost->h_addr == NULL)
				return 0;
			addr = ((struct in_addr *)pHost->h_addr)->s_addr;
		}
		else
			return 0;
	}

	// Return the resolved IP address as an integer
	return addr;
}
bool
VSocket::Resolve6(const VString address, in6_addr *addr)
{
	struct addrinfo hint, *info = 0;
	memset(&hint, 0, sizeof(hint));

	hint.ai_family = AF_UNSPEC;
	hint.ai_flags = AI_NUMERICHOST;
	bool IsIpv6 = false;
	if (getaddrinfo(address, 0, &hint, &info) == 0)
	{
		if (info->ai_family == AF_INET6)
		{
			IsIpv6 = true;
			inet_pton(AF_INET6, address, addr);
		}
	}
	freeaddrinfo(info);	

	if (!IsIpv6)
	{
		struct addrinfo *serverinfo = 0;
		memset(&hint, 0, sizeof(hint));
		hint.ai_family = AF_UNSPEC;
		hint.ai_socktype = SOCK_STREAM;
		hint.ai_protocol = IPPROTO_TCP;
		struct sockaddr_in6 *pIpv6Addr;
		if (getaddrinfo(address, 0, &hint, &serverinfo) == 0)
		{
			struct addrinfo *p;
			for (p = serverinfo; p != NULL; p = p->ai_next) {
				switch (p->ai_family) {
				case AF_INET6:
					IsIpv6 = true;
					pIpv6Addr = (struct sockaddr_in6 *) p->ai_addr;
					memcpy(addr, &(pIpv6Addr->sin6_addr), sizeof(in6_addr));
					break;
				default:
					break;
				}


			}

		}
		freeaddrinfo(serverinfo);
	}

	return IsIpv6;
}

VCard32
VSocket::Resolve(const VString address)
{
  VCard32 addr;

  // Try converting the address as IP
  addr = inet_addr(address);

  // Was it a valid IP address?
  if (addr == INADDR_NONE)
    {
      // No, so get the actual IP address of the host name specified
      struct hostent *pHost=NULL;
      pHost = gethostbyname(address);
      if (pHost != NULL)
	  {
		  if (pHost->h_addr == NULL)
			  return 0;
		  addr = ((struct in_addr *)pHost->h_addr)->s_addr;
	  }
	  else
		  return 0;
    }

  // Return the resolved IP address as an integer
  return addr;
}

////////////////////////////
// adzm 2010-08
VBool
VSocket::SetDefaultSocketOptions4()
{
	VBool result = VTrue;

	const int one = 1;

	if (setsockopt(sock4, IPPROTO_TCP, TCP_NODELAY, (const char *)&one, sizeof(one)))
	{
		result = VFalse;
	}

	int defaultSocketKeepAliveTimeout = m_defaultSocketKeepAliveTimeout;
	if (defaultSocketKeepAliveTimeout > 0) {
		if (setsockopt(sock4, SOL_SOCKET, SO_KEEPALIVE, (const char *)&one, sizeof(one))) {
			result = VFalse;
		}
		else {
			DWORD bytes_returned = 0;
			tcp_keepalive keepalive_requested;
			tcp_keepalive keepalive_returned;
			ZeroMemory(&keepalive_requested, sizeof(keepalive_requested));
			ZeroMemory(&keepalive_returned, sizeof(keepalive_returned));

			keepalive_requested.onoff = 1;
			keepalive_requested.keepalivetime = defaultSocketKeepAliveTimeout;
			keepalive_requested.keepaliveinterval = 1000;
			// 10 probes always used by default in Windows Vista+; not changeable. 

			if (0 != WSAIoctl(sock4, SIO_KEEPALIVE_VALS,
				&keepalive_requested, sizeof(keepalive_requested),
				&keepalive_returned, sizeof(keepalive_returned),
				&bytes_returned, NULL, NULL))
			{
				result = VFalse;
			}
		}
	}

	assert(result);	
	return result;
}
VBool
VSocket::SetDefaultSocketOptions6()
{
	VBool result = VTrue;

	const int one = 1;

	if (setsockopt(sock6, IPPROTO_TCP, TCP_NODELAY, (const char *)&one, sizeof(one)))
	{
		result = VFalse;
	}

	int defaultSocketKeepAliveTimeout = m_defaultSocketKeepAliveTimeout;
	if (defaultSocketKeepAliveTimeout > 0) {
		if (setsockopt(sock6, SOL_SOCKET, SO_KEEPALIVE, (const char *)&one, sizeof(one))) {
			result = VFalse;
		}
		else {
			DWORD bytes_returned = 0;
			tcp_keepalive keepalive_requested;
			tcp_keepalive keepalive_returned;
			ZeroMemory(&keepalive_requested, sizeof(keepalive_requested));
			ZeroMemory(&keepalive_returned, sizeof(keepalive_returned));

			keepalive_requested.onoff = 1;
			keepalive_requested.keepalivetime = defaultSocketKeepAliveTimeout;
			keepalive_requested.keepaliveinterval = 1000;
			// 10 probes always used by default in Windows Vista+; not changeable. 

			if (0 != WSAIoctl(sock6, SIO_KEEPALIVE_VALS,
				&keepalive_requested, sizeof(keepalive_requested),
				&keepalive_returned, sizeof(keepalive_returned),
				&bytes_returned, NULL, NULL))
			{
				result = VFalse;
			}
		}
	}

	assert(result);
	return result;
}

VBool
VSocket::SetDefaultSocketOptions()
{
	VBool result = VTrue;

	const int one = 1;

	if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&one, sizeof(one)))
	{
		result = VFalse;
	}

	int defaultSocketKeepAliveTimeout = m_defaultSocketKeepAliveTimeout;
	if (defaultSocketKeepAliveTimeout > 0) {
		if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (const char *)&one, sizeof(one))) {
			result = VFalse;
		} else {
			DWORD bytes_returned = 0;
			tcp_keepalive keepalive_requested;
			tcp_keepalive keepalive_returned;
			ZeroMemory(&keepalive_requested, sizeof(keepalive_requested));
			ZeroMemory(&keepalive_returned, sizeof(keepalive_returned));

			keepalive_requested.onoff = 1;
			keepalive_requested.keepalivetime = defaultSocketKeepAliveTimeout;
			keepalive_requested.keepaliveinterval = 1000;
			// 10 probes always used by default in Windows Vista+; not changeable. 

			if (0 != WSAIoctl(sock, SIO_KEEPALIVE_VALS, 
					&keepalive_requested, sizeof(keepalive_requested), 
					&keepalive_returned, sizeof(keepalive_returned), 
					&bytes_returned, NULL, NULL))
			{
				result = VFalse;
			}
		}
	}
	return result;
} 

////////////////////////////
VBool
VSocket::SetTimeoutIPV6(VCard32 msecs)
{
	if (sock4 != INVALID_SOCKET) SetTimeout4(msecs);
	if (sock6 != INVALID_SOCKET) SetTimeout6(msecs);
	return true;
}

VBool
VSocket::SetTimeout4(VCard32 msecs)
{
	if (LOBYTE(winsockVersion) < 2)
		return VFalse;
	int timeout=msecs;
	if (setsockopt(sock4, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR)
	{
		return VFalse;
	}
	if (setsockopt(sock4, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR)
	{
		return VFalse;
	}
	return VTrue;
}

VBool VSocket::SetSendTimeoutIPV6(VCard32 msecs)
{
	if (sock4 != INVALID_SOCKET) SetTimeout4(msecs);
	if (sock6 != INVALID_SOCKET) SetTimeout6(msecs);
	return true;

}

VBool VSocket::SetRecvTimeoutIPV6(VCard32 msecs)
{

	if (sock4 != INVALID_SOCKET) SetTimeout4(msecs);
	if (sock6 != INVALID_SOCKET) SetTimeout6(msecs);
	return true;
}

VBool VSocket::SetSendTimeout4(VCard32 msecs)
{
	return SetTimeout4 (msecs);
}

VBool VSocket::SetRecvTimeout4(VCard32 msecs)
{
	return SetTimeout4 (msecs);
}
VBool
VSocket::SetTimeout6(VCard32 msecs)
{
	if (LOBYTE(winsockVersion) < 2)
		return VFalse;
	int timeout = msecs;
	if (setsockopt(sock6, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR)
	{
		return VFalse;
	}
	if (setsockopt(sock6, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR)
	{
		return VFalse;
	}
	return VTrue;
}

VBool VSocket::SetSendTimeout6(VCard32 msecs)
{
	return SetTimeout6(msecs);
}

VBool VSocket::SetRecvTimeout6(VCard32 msecs)
{
	return SetTimeout6(msecs);
}

VBool
VSocket::SetTimeoutIPV4(VCard32 msecs)
{
	if (LOBYTE(winsockVersion) < 2)
		return VFalse;
	DWORD timeout=msecs;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR)
	{
		return VFalse;
	}
	if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR)
	{
		return VFalse;
	}
	return VTrue;
}

VBool VSocket::SetSendTimeoutIPV4(VCard32 msecs)
{
    return SetTimeout (msecs);
}

VBool VSocket::SetRecvTimeoutIPV4(VCard32 msecs)
{
    return SetTimeout (msecs);
}


VBool
VSocket::SetTimeout(VCard32 msecs)
{
	if (settings->getIPV6())
		return SetTimeoutIPV6(msecs);
	else
		return SetTimeoutIPV4(msecs);
}

VBool VSocket::SetSendTimeout(VCard32 msecs)
{
	if (settings->getIPV6())
		return SetSendTimeoutIPV6(msecs);
	else
		return SetSendTimeoutIPV4(msecs);
}

VBool VSocket::SetRecvTimeout(VCard32 msecs)
{
	if (settings->getIPV6())
		return SetRecvTimeoutIPV6(msecs);
	else
		return SetRecvTimeoutIPV4(msecs);
}
////////////////////////////
VInt
VSocket::SendIPV6(const char *buff, const VCard bufflen)
{

	if (sock4 != INVALID_SOCKET) return  SendSock(buff, bufflen, sock4);
	if (sock6 != INVALID_SOCKET) return SendSock(buff, bufflen, sock6);
	return false;
}
VInt
VSocket::SendSock(const char *buff, const VCard bufflen, SOCKET allsock)
{
	//adzm 2010-08-01
	m_LastSentTick = GetTickCount();

	unsigned int newsize=queuebuffersize+bufflen;
	char *buff2;
	buff2=(char*)buff;
	unsigned int bufflen2=bufflen;

	// adzm 2010-09 - flush as soon as we have a full buffer, not if we have exceeded it.
	if (newsize >= G_SENDBUFFER)
	{
		memcpy(queuebuffer+queuebuffersize,buff2,G_SENDBUFFER-queuebuffersize);
		if (!sendall(allsock,queuebuffer,G_SENDBUFFER,0)) return FALSE;
		//			vnclog.Print(LL_SOCKERR, VNCLOG("SEND  %i\n") ,G_SENDBUFFER);
		buff2+=(G_SENDBUFFER-queuebuffersize);
		bufflen2-=(G_SENDBUFFER-queuebuffersize);
		queuebuffersize=0;
		// adzm 2010-09 - flush as soon as we have a full buffer, not if we have exceeded it.
		while (bufflen2 >= G_SENDBUFFER)
		{
			if (!sendall(allsock,buff2,G_SENDBUFFER,0)) return false;
			//				vnclog.Print(LL_SOCKERR, VNCLOG("SEND 1 %i\n") ,G_SENDBUFFER);
			buff2+=G_SENDBUFFER;
			bufflen2-=G_SENDBUFFER;
		}
	}
	memcpy(queuebuffer+queuebuffersize,buff2,bufflen2);
	queuebuffersize+=bufflen2;
	if (queuebuffersize > 0) {
		if (!sendall(allsock,queuebuffer,queuebuffersize,0)) 
			return false;
	}
	//	vnclog.Print(LL_SOCKERR, VNCLOG("SEND 2 %i\n") ,queuebuffersize);
	queuebuffersize=0;
	return bufflen;
}

VInt
VSocket::SendIPV4(const char *buff, const VCard bufflen)
{
	//adzm 2010-08-01
	m_LastSentTick = GetTickCount();

	unsigned int newsize=queuebuffersize+bufflen;
	char *buff2;
	buff2=(char*)buff;
	unsigned int bufflen2=bufflen;

	// adzm 2010-09 - flush as soon as we have a full buffer, not if we have exceeded it.
	if (newsize >= G_SENDBUFFER)
	{
		    memcpy(queuebuffer+queuebuffersize,buff2,G_SENDBUFFER-queuebuffersize);
			if (!sendall(sock,queuebuffer,G_SENDBUFFER,0)) return FALSE;
//			vnclog.Print(LL_SOCKERR, VNCLOG("SEND  %i\n") ,G_SENDBUFFER);
			buff2+=(G_SENDBUFFER-queuebuffersize);
			bufflen2-=(G_SENDBUFFER-queuebuffersize);
			queuebuffersize=0;
			// adzm 2010-09 - flush as soon as we have a full buffer, not if we have exceeded it.
			while (bufflen2 >= G_SENDBUFFER)
			{
				if (!sendall(sock,buff2,G_SENDBUFFER,0)) return false;
//				vnclog.Print(LL_SOCKERR, VNCLOG("SEND 1 %i\n") ,G_SENDBUFFER);
				buff2+=G_SENDBUFFER;
				bufflen2-=G_SENDBUFFER;
			}
	}
	memcpy(queuebuffer+queuebuffersize,buff2,bufflen2);
	queuebuffersize+=bufflen2;
	if (queuebuffersize > 0) {
		if (!sendall(sock,queuebuffer,queuebuffersize,0)) 
			return false;
	}
//	vnclog.Print(LL_SOCKERR, VNCLOG("SEND 2 %i\n") ,queuebuffersize);
	queuebuffersize=0;
	return bufflen;
}


VInt
VSocket::Send(const char* buff, const VCard bufflen)
{
	if (settings->getIPV6())
		return SendIPV6(buff, bufflen);
	else
		return SendIPV4(buff, bufflen);
}

////////////////////////////
VInt
VSocket::SendQueuedIPV6(const char *buff, const VCard bufflen)
{
	if (sock4 != INVALID_SOCKET) return  SendQueuedSock(buff, bufflen, sock4);
	if (sock6 != INVALID_SOCKET) return SendQueuedSock(buff, bufflen, sock6);
	return false;
}

VInt
VSocket::SendQueuedSock(const char *buff, const VCard bufflen, SOCKET allsock)
{
	unsigned int newsize=queuebuffersize+bufflen;
	char *buff2;
	buff2=(char*)buff;
	unsigned int bufflen2=bufflen;

	// adzm 2010-09 - flush as soon as we have a full buffer, not if we have exceeded it.
	if (newsize >= G_SENDBUFFER)
	{	
		//adzm 2010-08-01
		m_LastSentTick = GetTickCount();

		memcpy(queuebuffer+queuebuffersize,buff2,G_SENDBUFFER-queuebuffersize);
		if (!sendall(allsock,queuebuffer,G_SENDBUFFER,0)) return FALSE;
		//	vnclog.Print(LL_SOCKERR, VNCLOG("SEND Q  %i\n") ,G_SENDBUFFER);
		buff2+=(G_SENDBUFFER-queuebuffersize);
		bufflen2-=(G_SENDBUFFER-queuebuffersize);
		queuebuffersize=0;			

		// adzm 2010-09 - flush as soon as we have a full buffer, not if we have exceeded it.
		while (bufflen2 >= G_SENDBUFFER)
		{
			if (!sendall(allsock,buff2,G_SENDBUFFER,0)) return false;				
			//adzm 2010-08-01
			m_LastSentTick = GetTickCount();
			//	vnclog.Print(LL_SOCKERR, VNCLOG("SEND Q  %i\n") ,G_SENDBUFFER);
			buff2+=G_SENDBUFFER;
			bufflen2-=G_SENDBUFFER;
		}
	}
	memcpy(queuebuffer+queuebuffersize,buff2,bufflen2);
	queuebuffersize+=bufflen2;
	return bufflen;
}

VInt
VSocket::SendQueuedIPV4(const char *buff, const VCard bufflen)
{
	unsigned int newsize=queuebuffersize+bufflen;
	char *buff2;
	buff2=(char*)buff;
	unsigned int bufflen2=bufflen;
	
	// adzm 2010-09 - flush as soon as we have a full buffer, not if we have exceeded it.
	if (newsize >= G_SENDBUFFER)
	{	
			//adzm 2010-08-01
			m_LastSentTick = GetTickCount();

		    memcpy(queuebuffer+queuebuffersize,buff2,G_SENDBUFFER-queuebuffersize);
			if (!sendall(sock,queuebuffer,G_SENDBUFFER,0)) return FALSE;
		//	vnclog.Print(LL_SOCKERR, VNCLOG("SEND Q  %i\n") ,G_SENDBUFFER);
			buff2+=(G_SENDBUFFER-queuebuffersize);
			bufflen2-=(G_SENDBUFFER-queuebuffersize);
			queuebuffersize=0;			

			// adzm 2010-09 - flush as soon as we have a full buffer, not if we have exceeded it.
			while (bufflen2 >= G_SENDBUFFER)
			{
				if (!sendall(sock,buff2,G_SENDBUFFER,0)) return false;				
				//adzm 2010-08-01
				m_LastSentTick = GetTickCount();
			//	vnclog.Print(LL_SOCKERR, VNCLOG("SEND Q  %i\n") ,G_SENDBUFFER);
				buff2+=G_SENDBUFFER;
				bufflen2-=G_SENDBUFFER;
			}
	}
	memcpy(queuebuffer+queuebuffersize,buff2,bufflen2);
	queuebuffersize+=bufflen2;
	return bufflen;
}


VInt
VSocket::SendQueued(const char* buff, const VCard bufflen)
{
	if (settings->getIPV6())
		return SendQueuedIPV6(buff, bufflen);
	else
		return SendQueuedIPV4(buff, bufflen);
}
/////////////////////////////

// sf@2002 - DSMPlugin
VBool
VSocket::SendExactIPV6(const char *buff, const VCard bufflen, unsigned char msgType)
{
	if (sock4 != INVALID_SOCKET) return SendExactSock(buff, bufflen, msgType, sock4);
	if (sock6 != INVALID_SOCKET) return SendExactSock(buff, bufflen, msgType, sock6);
	return false;
}
VBool
VSocket::SendExactSock(const char *buff, const VCard bufflen, unsigned char msgType, SOCKET allsock)
{
	if (allsock == -1) return VFalse;
	//vnclog.Print(LL_SOCKERR, VNCLOG("SendExactMsg %i\n") ,bufflen);
	// adzm 2010-09
	if (!IsPluginStreamingOut() && m_fUsePlugin && m_pDSMPlugin->IsEnabled())
	{
		// Send the transformed message type first
		char t = (char)msgType;
		SendExact(&t, 1);
		// Then we send the transformed rfb message content
		SendExact(buff, bufflen);
	}
	else
		SendExact(buff, bufflen);

	return VTrue;
}

VBool
VSocket::SendExactIPV4(const char *buff, const VCard bufflen, unsigned char msgType)
{
	if (sock==-1) return VFalse;
	//vnclog.Print(LL_SOCKERR, VNCLOG("SendExactMsg %i\n") ,bufflen);
	// adzm 2010-09
	if (!IsPluginStreamingOut() && m_fUsePlugin && m_pDSMPlugin->IsEnabled())
	{
		// Send the transformed message type first
		char t = (char)msgType;
		SendExact(&t, 1);
		// Then we send the transformed rfb message content
		SendExact(buff, bufflen);
	}
	else
		SendExact(buff, bufflen);

	return VTrue;
}

VInt
VSocket::SendExact(const char* buff, const VCard bufflen, unsigned char msgType)
{
	if (settings->getIPV6())
		return SendExactIPV6(buff, bufflen, msgType);
	else
		return SendExactIPV4(buff, bufflen, msgType);
}


VBool 
VSocket::SendExactQueueIPV6(const char *buff, const VCard bufflen, unsigned char msgType)
{
	if (sock4 != INVALID_SOCKET) return SendExactQueueSock(buff, bufflen, msgType, sock4);
	if (sock6 != INVALID_SOCKET) return SendExactQueueSock(buff, bufflen, msgType, sock6);
	return false;
}
VBool 
VSocket::SendExactQueueSock(const char *buff, const VCard bufflen, unsigned char msgType, SOCKET allsock)
{
	if (allsock == -1) return VFalse;
	//vnclog.Print(LL_SOCKERR, VNCLOG("SendExactMsg %i\n") ,bufflen);
	// adzm 2010-09
	if (!IsPluginStreamingOut() && m_fUsePlugin && m_pDSMPlugin->IsEnabled())
	{
		// Send the transformed message type first
		char t = (char)msgType;
		SendExactQueue(&t, 1);
		// Then we send the transformed rfb message content
		SendExactQueue(buff, bufflen);
	}
	else
		SendExactQueue(buff, bufflen);

	return VTrue;
}

//adzm 2010-09 - minimize packets. SendExact flushes the queue.
VBool 
VSocket::SendExactQueueIPV4(const char *buff, const VCard bufflen, unsigned char msgType)
{
	if (sock==-1) return VFalse;
	//vnclog.Print(LL_SOCKERR, VNCLOG("SendExactMsg %i\n") ,bufflen);
	// adzm 2010-09
	if (!IsPluginStreamingOut() && m_fUsePlugin && m_pDSMPlugin->IsEnabled())
	{
		// Send the transformed message type first
		char t = (char)msgType;
		SendExactQueue(&t, 1);
		// Then we send the transformed rfb message content
		SendExactQueue(buff, bufflen);
	}
	else
		SendExactQueue(buff, bufflen);

	return VTrue;
}


VInt
VSocket::SendExactQueue(const char* buff, const VCard bufflen, unsigned char msgType)
{
	if (settings->getIPV6())
		return SendExactQueueIPV6(buff, bufflen, msgType);
	else
		return SendExactQueueIPV4(buff, bufflen, msgType);
}
//////////////////////////////////////////
VBool
VSocket::SendExactIPV6(const char *buff, const VCard bufflen)
{
	if (sock4 != INVALID_SOCKET) return SendExactSock(buff, bufflen, sock4);
	if (sock6 != INVALID_SOCKET) return SendExactSock(buff, bufflen, sock6);
	return false;
}

VBool
VSocket::SendExactSock(const char *buff, const VCard bufflen, SOCKET allsock)
{	
	if (allsock==-1) return VFalse;
	//adzm 2010-09
	if (bufflen <=0) {
		return VTrue;
	}
//	vnclog.Print(LL_SOCKERR, VNCLOG("SendExact %i\n") ,bufflen);
	// sf@2002 - DSMPlugin
	VCard nBufflen = bufflen;
	char* pBuffer = NULL;
	if (m_fUsePlugin && m_pDSMPlugin->IsEnabled())
	{
		// omni_mutex_lock l(m_TransMutex);

		// If required to store data into memory
		if (m_fWriteToNetRectBuf)
		{
			memcpy((char*)(m_pNetRectBuf + m_nNetRectBufOffset), buff, bufflen);
			m_nNetRectBufOffset += bufflen;
			return VTrue;
		}
		else // Tell the plugin to transform data
		{
			int nTransDataLen = 0;
			//adzm 2009-06-20
			pBuffer = (char*)(TransformBuffer((BYTE*)buff, bufflen, &nTransDataLen));
			if (pBuffer == NULL || (bufflen > 0 && nTransDataLen == 0))
			{
				// throw WarningException("SendExact: DSMPlugin-TransformBuffer Error.");
			}
			nBufflen = nTransDataLen;
		}
		
	}
	else
		pBuffer = (char*) buff;
	
	VInt result=Send(pBuffer, nBufflen);
  return result == (VInt)nBufflen;
}

VBool
VSocket::SendExactIPV4(const char *buff, const VCard bufflen)
{	
	if (sock==-1) return VFalse;
	//adzm 2010-09
	if (bufflen <=0) {
		return VTrue;
	}
//	vnclog.Print(LL_SOCKERR, VNCLOG("SendExact %i\n") ,bufflen);
	// sf@2002 - DSMPlugin
	VCard nBufflen = bufflen;
	char* pBuffer = NULL;
	if (m_fUsePlugin && m_pDSMPlugin->IsEnabled())
	{
		// omni_mutex_lock l(m_TransMutex);

		// If required to store data into memory
		if (m_fWriteToNetRectBuf)
		{
			memcpy((char*)(m_pNetRectBuf + m_nNetRectBufOffset), buff, bufflen);
			m_nNetRectBufOffset += bufflen;
			return VTrue;
		}
		else // Tell the plugin to transform data
		{
			int nTransDataLen = 0;
			//adzm 2009-06-20
			pBuffer = (char*)(TransformBuffer((BYTE*)buff, bufflen, &nTransDataLen));
			if (pBuffer == NULL || (bufflen > 0 && nTransDataLen == 0))
			{
				// throw WarningException("SendExact: DSMPlugin-TransformBuffer Error.");
			}
			nBufflen = nTransDataLen;
		}
		
	}
	else
		pBuffer = (char*) buff;
	
	VInt result=Send(pBuffer, nBufflen);
  return result == (VInt)nBufflen;
}


VInt
VSocket::SendExact(const char* buff, const VCard bufflen)
{
	if (settings->getIPV6())
		return SendExactIPV6(buff, bufflen);
	else
		return SendExactIPV4(buff, bufflen);
}
///////////////////////////////////////

VBool
VSocket::SendExactQueueIPV6(const char *buff, const VCard bufflen)
{
	if (sock4 != INVALID_SOCKET) return SendExactQueueSock(buff, bufflen, sock4);
	if (sock6 != INVALID_SOCKET) return SendExactQueueSock(buff, bufflen, sock6);
	return false;
}
VBool
VSocket::SendExactQueueSock(const char *buff, const VCard bufflen, SOCKET allsock)
{
	if (allsock == -1) return VFalse;
	//adzm 2010-09
	if (bufflen <= 0) {
		return VTrue;
	}
	//	vnclog.Print(LL_SOCKERR, VNCLOG("SendExactQueue %i %i\n") ,bufflen,queuebuffersize);
	//	vnclog.Print(LL_SOCKERR, VNCLOG("socket size %i\n") ,bufflen);
	// sf@2002 - DSMPlugin
	VCard nBufflen = bufflen;
	char* pBuffer = NULL;
	if (m_fUsePlugin && m_pDSMPlugin->IsEnabled())
	{
		// omni_mutex_lock l(m_TransMutex);

		// If required to store data into memory
		if (m_fWriteToNetRectBuf)
		{
			memcpy((char*)(m_pNetRectBuf + m_nNetRectBufOffset), buff, bufflen);
			m_nNetRectBufOffset += bufflen;
			return VTrue;
		}
		else // Tell the plugin to transform data
		{
			int nTransDataLen = 0;
			//adzm 2009-06-20
			pBuffer = (char*)(TransformBuffer((BYTE*)buff, bufflen, &nTransDataLen));
			if (pBuffer == NULL || (bufflen > 0 && nTransDataLen == 0))
			{
				// throw WarningException("SendExact: DSMPlugin-TransformBuffer Error.");
			}
			nBufflen = nTransDataLen;
		}

	}
	else
		pBuffer = (char*)buff;

	VInt result = SendQueued(pBuffer, nBufflen);
	return result == (VInt)nBufflen;
}

VBool
VSocket::SendExactQueueIPV4(const char *buff, const VCard bufflen)
{
	if (sock==-1) return VFalse;
	//adzm 2010-09
	if (bufflen <=0) {
		return VTrue;
	}
//	vnclog.Print(LL_SOCKERR, VNCLOG("SendExactQueue %i %i\n") ,bufflen,queuebuffersize);
//	vnclog.Print(LL_SOCKERR, VNCLOG("socket size %i\n") ,bufflen);
	// sf@2002 - DSMPlugin
	VCard nBufflen = bufflen;
	char* pBuffer = NULL;
	if (m_fUsePlugin && m_pDSMPlugin->IsEnabled())
	{
		// omni_mutex_lock l(m_TransMutex);

		// If required to store data into memory
		if (m_fWriteToNetRectBuf)
		{
			memcpy((char*)(m_pNetRectBuf + m_nNetRectBufOffset), buff, bufflen);
			m_nNetRectBufOffset += bufflen;
			return VTrue;
		}
		else // Tell the plugin to transform data
		{
			int nTransDataLen = 0;
			//adzm 2009-06-20
			pBuffer = (char*)(TransformBuffer((BYTE*)buff, bufflen, &nTransDataLen));
			if (pBuffer == NULL || (bufflen > 0 && nTransDataLen == 0))
			{
				// throw WarningException("SendExact: DSMPlugin-TransformBuffer Error.");
			}
			nBufflen = nTransDataLen;
		}
		
	}
	else
		pBuffer = (char*) buff;
	
	VInt result=SendQueued(pBuffer, nBufflen);
  return result == (VInt)nBufflen;
}

VInt
VSocket::SendExactQueue(const char* buff, const VCard bufflen)
{
	if (settings->getIPV6())
		return SendExactQueueIPV6(buff, bufflen);
	else
		return SendExactQueueIPV4(buff, bufflen);
}
///////////////////////////////
VBool
VSocket::ClearQueueIPV6()
{
	if (sock4 != INVALID_SOCKET) return ClearQueueSock(sock4);
	if (sock6 != INVALID_SOCKET) return ClearQueueSock(sock6);
	return false;
}

VBool
VSocket::ClearQueueSock(SOCKET allsock)
{
	if (allsock==-1) return VFalse;
	if (queuebuffersize!=0)
	{
		//adzm 2010-08-01
		m_LastSentTick = GetTickCount();
		//adzm 2010-09 - return a bool in ClearQueue
		if (!sendall(allsock,queuebuffer,queuebuffersize,0)) 
			return VFalse;
		queuebuffersize=0;
	}
	GetOptimalSndBuf();
	return VTrue;
}
VBool
VSocket::ClearQueueIPV4()
{
	if (sock==-1) return VFalse;
	if (queuebuffersize!=0)
  {
	//adzm 2010-08-01
	m_LastSentTick = GetTickCount();
	//adzm 2010-09 - return a bool in ClearQueue
	if (!sendall(sock,queuebuffer,queuebuffersize,0)) 
		return VFalse;
	queuebuffersize=0;
  }
  GetOptimalSndBuf();
  return VTrue;
}

VInt
VSocket::ClearQueue()
{
	if (settings->getIPV6())
		return ClearQueueIPV6();
	else
		return ClearQueueIPV4();
}
////////////////////////////
VInt
VSocket::ReadIPV6(char *buff, const VCard bufflen)
{
	if (sock4 != INVALID_SOCKET) return ReadSock(buff, bufflen, sock4);
	if (sock6 != INVALID_SOCKET) return ReadSock(buff, bufflen, sock6);
	return false;
}
VInt
VSocket::ReadSock(char *buff, const VCard bufflen, SOCKET allsock)
{
	if (allsock == -1) return allsock;
	int counter = 0;
	int s = 0;
	s = recv(allsock, buff, bufflen, 0);
	return s;
}

VInt
VSocket::ReadIPV4(char *buff, const VCard bufflen)
{
	if (sock==-1) return -1;
	int s = 0;
	s = recv(sock, buff, bufflen, 0);
	return s;
}


VInt
VSocket::Read(char* buff, const VCard bufflen)
{
	if (settings->getIPV6())
		return ReadIPV6(buff, bufflen);
	else
		return ReadIPV4(buff, bufflen);
}
////////////////////////////

VBool
VSocket::ReadExactIPV6(char *buff, const VCard bufflen)
{
	if (sock4 != INVALID_SOCKET) return ReadExactSock(buff, bufflen, sock4);
	if (sock6 != INVALID_SOCKET) return ReadExactSock(buff, bufflen, sock6);
	return false;
}
VBool
VSocket::ReadExactSock(char *buff, const VCard bufflen, SOCKET allsock)
{
	if (allsock == -1) return VFalse;
	//adzm 2010-09
	if (bufflen <= 0) {
		return VTrue;
	}
	int n;
	VCard currlen = bufflen;

	// sf@2002 - DSM Plugin
	if (m_fUsePlugin && m_pDSMPlugin->IsEnabled())
	{
		//omni_mutex_lock l(m_pDSMPlugin->m_RestMutex); 
		//adzm 2009-06-20 - don't lock if we are using the new interface
		omni_mutex_conditional_lock l(m_pDSMPlugin->m_RestMutex, m_pPluginInterface ? false : true);

		// Get the DSMPlugin destination buffer where to put transformed incoming data
		// The number of bytes to read calculated from bufflen is given back in nTransDataLen
		int nTransDataLen = 0;
		//adzm 2009-06-20
		BYTE* pTransBuffer = RestoreBufferStep1(NULL, bufflen, &nTransDataLen);
		if (pTransBuffer == NULL)
		{
			// m_pDSMPlugin->RestoreBufferUnlock();
			// throw WarningException("WriteExact: DSMPlugin-RestoreBuffer Alloc Error.");
			vnclog.Print(LL_SOCKERR, VNCLOG("WriteExact: DSMPlugin-RestoreBuffer Alloc Error\n"));
			return VFalse;
		}

		// Read bytes directly into Plugin Dest Rest. buffer
		int nTransDataLenSave = nTransDataLen;
		while (nTransDataLen > 0)
		{
			// Try to read some data in
			n = Read((char*)pTransBuffer, nTransDataLen);
			if (n > 0)
			{
				// Adjust the buffer position and size
				pTransBuffer += n;
				nTransDataLen -= n;
			}
			else if (n == 0) {
				//m_pDSMPlugin->RestoreBufferUnlock();
				vnclog.Print(LL_SOCKERR, VNCLOG("zero bytes read1\n"));
				return VFalse;
			}
			else {
				if (WSAGetLastError() != WSAEWOULDBLOCK)
				{
					//m_pDSMPlugin->RestoreBufferUnlock();
					vnclog.Print(LL_SOCKERR, VNCLOG("socket error 1: %d\n"), WSAGetLastError());
					return VFalse;
				}
			}
		}

		// Ask plugin to restore data from rest. buffer into inbuf
		int nRestDataLen = 0;
		nTransDataLen = nTransDataLenSave;
		//adzm 2009-06-20
		BYTE* pPipo = RestoreBufferStep2((BYTE*)buff, nTransDataLen, &nRestDataLen);

		// Check if we actually get the real original data length
		if ((VCard)nRestDataLen != bufflen)
		{
			// throw WarningException("WriteExact: DSMPlugin-RestoreBuffer Error.");
		}
	}
	else // Non-Transformed
	{
		while (currlen > 0)
		{
			// Try to read some data in
			n = Read(buff, currlen);

			if (n > 0)
			{
				// Adjust the buffer position and size
				buff += n;
				currlen -= n;
			}
			else if (n == 0) {
				vnclog.Print(LL_SOCKERR, VNCLOG("zero bytes read2\n"));

				return VFalse;
			}
			else {
				if (WSAGetLastError() != WSAEWOULDBLOCK)
				{
					//int aa=WSAGetLastError();
					//vnclog.Print(LL_SOCKERR, VNCLOG("socket error 2: %d\n"), aa);
					return VFalse;
				}
			}
		}
	}

	return VTrue;
}

VBool
VSocket::ReadExactIPV4(char *buff, const VCard bufflen)
{	
	if (sock==-1) 
		return VFalse;
	//adzm 2010-09
	if (bufflen <=0) {
		return VTrue;
	}
	int n;
	VCard currlen = bufflen;
    
	// sf@2002 - DSM Plugin
	if (m_fUsePlugin && m_pDSMPlugin->IsEnabled())
	{
		//omni_mutex_lock l(m_pDSMPlugin->m_RestMutex); 
		//adzm 2009-06-20 - don't lock if we are using the new interface
		omni_mutex_conditional_lock l(m_pDSMPlugin->m_RestMutex, m_pPluginInterface ? false : true);

		// Get the DSMPlugin destination buffer where to put transformed incoming data
		// The number of bytes to read calculated from bufflen is given back in nTransDataLen
		int nTransDataLen = 0;
		//adzm 2009-06-20
		BYTE* pTransBuffer = RestoreBufferStep1(NULL, bufflen, &nTransDataLen);
		if (pTransBuffer == NULL)
		{
			// m_pDSMPlugin->RestoreBufferUnlock();
			// throw WarningException("WriteExact: DSMPlugin-RestoreBuffer Alloc Error.");
			vnclog.Print(LL_SOCKERR, VNCLOG("WriteExact: DSMPlugin-RestoreBuffer Alloc Error\n"));
			return VFalse;
		}
		
		// Read bytes directly into Plugin Dest Rest. buffer
		int nTransDataLenSave = nTransDataLen;
		while (nTransDataLen > 0)
		{
			// Try to read some data in
			n = Read((char*)pTransBuffer, nTransDataLen);
			if (n > 0)
			{
				// Adjust the buffer position and size
				pTransBuffer += n;
				nTransDataLen -= n;
			} else if (n == 0) {
				//m_pDSMPlugin->RestoreBufferUnlock();
				vnclog.Print(LL_SOCKERR, VNCLOG("zero bytes read1\n"));
				return VFalse;
			} else {
				if (WSAGetLastError() != WSAEWOULDBLOCK)
				{
					//m_pDSMPlugin->RestoreBufferUnlock();
					vnclog.Print(LL_SOCKERR, VNCLOG("socket error 1: %d\n"), WSAGetLastError());
					return VFalse;
				}
			}
		}
		
		// Ask plugin to restore data from rest. buffer into inbuf
		int nRestDataLen = 0;
		nTransDataLen = nTransDataLenSave;
		//adzm 2009-06-20
		RestoreBufferStep2((BYTE*)buff, nTransDataLen, &nRestDataLen);

		// Check if we actually get the real original data length
		if ((VCard)nRestDataLen != bufflen)
		{
			// throw WarningException("WriteExact: DSMPlugin-RestoreBuffer Error.");
		}
	}
	else // Non-Transformed
	{
		while (currlen > 0)
		{
			// Try to read some data in
			n = Read(buff, currlen);
			
			if (n > 0)
			{
				// Adjust the buffer position and size
				buff += n;
				currlen -= n;
			} else if (n == 0) {
				vnclog.Print(LL_SOCKERR, VNCLOG("zero bytes read2\n"));
				
				return VFalse;
			} else {
				if (WSAGetLastError() != WSAEWOULDBLOCK)
				{
					return VFalse;
				}
			}
		}
	}

return VTrue;
}



VBool
VSocket::ReadExact(char* buff, const VCard bufflen)
{
	if (settings->getIPV6())
		return ReadExactIPV6(buff, bufflen);
	else
		return ReadExactIPV4(buff, bufflen);
}

//
// sf@2002 - DSMPlugin
// 

//adzm 2009-06-20
void VSocket::SetDSMPluginPointer(CDSMPlugin* pDSMPlugin)
{
	m_pDSMPlugin = pDSMPlugin;

	if (m_pPluginInterface) {
		delete m_pPluginInterface;
		m_pPluginInterface = NULL;
		//adzm 2010-05-10
		m_pIntegratedPluginInterface = NULL;
	}

	if (m_pDSMPlugin && m_pDSMPlugin->SupportsMultithreaded()) {
		//adzm 2010-05-10
		if (m_pDSMPlugin->SupportsIntegrated()) {
			m_pIntegratedPluginInterface = m_pDSMPlugin->CreateIntegratedPluginInterface();
			m_pPluginInterface = m_pIntegratedPluginInterface;
		} else {
			m_pIntegratedPluginInterface = NULL;
			m_pPluginInterface = m_pDSMPlugin->CreatePluginInterface();
		}
	} else {
		m_pPluginInterface = NULL;
		m_pIntegratedPluginInterface = NULL;
	}
}

//adzm 2010-05-12 - dsmplugin config
void VSocket::SetDSMPluginConfig(char* szDSMPluginConfig)
{
	if (m_pIntegratedPluginInterface) {
		m_pIntegratedPluginInterface->SetServerOptions(szDSMPluginConfig);		
	}
}

//
// Tell the plugin to do its transformation on the source data buffer
// Return: pointer on the new transformed buffer (allocated by the plugin)
// nTransformedDataLen is the number of bytes contained in the transformed buffer
//adzm 2009-06-20
BYTE* VSocket::TransformBuffer(BYTE* pDataBuffer, int nDataLen, int* pnTransformedDataLen)
{
	if (m_pPluginInterface) {
		return m_pPluginInterface->TransformBuffer(pDataBuffer, nDataLen, pnTransformedDataLen);
	} else {
		return m_pDSMPlugin->TransformBuffer(pDataBuffer, nDataLen, pnTransformedDataLen);
	}
}


// - If pRestoredDataBuffer = NULL, the plugin check its local buffer and return the pointer
// - Otherwise, restore data contained in its rest. buffer and put the result in pRestoredDataBuffer
//   pnRestoredDataLen is the number bytes put in pRestoredDataBuffers
//adzm 2009-06-20
BYTE* VSocket::RestoreBufferStep1(BYTE* pRestoredDataBuffer, int nDataLen, int* pnRestoredDataLen)
{	
	if (m_pPluginInterface) {
		return m_pPluginInterface->RestoreBuffer(pRestoredDataBuffer, nDataLen, pnRestoredDataLen);
	} else {
		return m_pDSMPlugin->RestoreBufferStep1(pRestoredDataBuffer, nDataLen, pnRestoredDataLen);
	}
}

//adzm 2009-06-20
BYTE* VSocket::RestoreBufferStep2(BYTE* pRestoredDataBuffer, int nDataLen, int* pnRestoredDataLen)
{	
	if (m_pPluginInterface) {
		return m_pPluginInterface->RestoreBuffer(pRestoredDataBuffer, nDataLen, pnRestoredDataLen);
	} else {
		return m_pDSMPlugin->RestoreBufferStep2(pRestoredDataBuffer, nDataLen, pnRestoredDataLen);
	}
}

//
// Ensures that the temporary "alignement" buffer in large enough 
//
void VSocket::CheckNetRectBufferSize(int nBufSize)
{
    omni_mutex_lock l(m_CheckMutex,3); 

	if (m_nNetRectBufSize > nBufSize) return;

	BYTE *newbuf = new BYTE[nBufSize + 256];
	if (newbuf == NULL) 
	{
		// throw ErrorException("Insufficient memory to allocate network crypt buffer.");
	}

	if (m_pNetRectBuf != NULL)
	{
		// memcpy(newbuf, m_pNetRectBuf, m_nNetRectBufSize);
		delete [] m_pNetRectBuf;
	}

	m_pNetRectBuf = newbuf;
	m_nNetRectBufSize = nBufSize + 256;
	// vnclog.Print(4, _T("crypt bufsize expanded to %d\n"), m_netbufsize);
}


// sf@2002 - DSMPlugin
// Necessary for HTTP server (we'll see later if we can do something more intelligent...

VBool
VSocket::SendExactHTTP(const char *buff, const VCard bufflen)
{
  return Send(buff, bufflen) == (VInt)bufflen;
}


////////////////////////////

VBool
VSocket::ReadExactHTTP(char *buff, const VCard bufflen)
{
	int n;
	VCard currlen = bufflen;
    
	while (currlen > 0)
	{
		// Try to read some data in
		n = Read(buff, currlen);

		if (n > 0)
		{
			// Adjust the buffer position and size
			buff += n;
			currlen -= n;
		} else if (n == 0) {
			vnclog.Print(LL_SOCKERR, VNCLOG("zero bytes read3\n"));

			return VFalse;
		} else {
			if (WSAGetLastError() != WSAEWOULDBLOCK)
			{
				vnclog.Print(LL_SOCKERR, VNCLOG("HTTP socket error: %d\n"), WSAGetLastError());
				return VFalse;
			}
		}
    }

	return VTrue;
}

VBool
VSocket::ReadSelectIPV6(VCard to)
{
	if (sock4 != INVALID_SOCKET) return ReadSelectSock(to, sock4);
	if (sock6 != INVALID_SOCKET) return ReadSelectSock(to, sock6);
	return false;
}

VBool
VSocket::ReadSelectSock(VCard to, SOCKET allsock)
{
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET((unsigned long)allsock, &fds);
	struct timeval tv;
	tv.tv_sec = to / 1000;
	tv.tv_usec = (to % 1000) * 1000;
	int rc = select(allsock + 1, &fds, 0, 0, &tv);
	if (rc>0) return true;
	return false;
}

VBool
VSocket::ReadSelectIPV4(VCard to)
 {
 	fd_set fds;
 	FD_ZERO(&fds);
 	FD_SET((unsigned long)sock,&fds);
	struct timeval tv;
 	tv.tv_sec = to/1000;
 	tv.tv_usec = (to % 1000)*1000;
    int rc = select((int)(sock+1),&fds,0,0,&tv);
 	if (rc>0) return true;
 	return false;
 }

VBool
VSocket::ReadSelect(VCard to)
{
	if (settings->getIPV6())
		return ReadSelectIPV6(to);
	else
		return ReadSelectIPV4(to);
}


extern bool			fShutdownOrdered;
bool
sendall(SOCKET RemoteSocket,char *buff,unsigned int bufflen,int dummy)
{
int val =0;
	unsigned int totsend=0;
	while (totsend <bufflen)
	  {
		struct fd_set write_fds;
		struct timeval tm;
		tm.tv_sec = 1;
		tm.tv_usec = 0;

		int count;
		int aa=0;
		do {
			FD_ZERO(&write_fds);
			FD_SET(RemoteSocket, &write_fds);			
			count = select((int)(RemoteSocket+ 1), NULL, &write_fds, NULL, &tm);
			aa++;
		} while (count == 0&& !fShutdownOrdered && aa<600);
		if (aa>=600) return 0;
		if (fShutdownOrdered) return 0;
		if (count < 0 || count > 1) return 0;
		if (FD_ISSET(RemoteSocket, &write_fds)) 
		{
			val=send(RemoteSocket, buff+totsend, bufflen-totsend, 0);
		}
		if (val==0) 
			return false;
		if (val==SOCKET_ERROR) 
			return false;
		totsend+=val;
	  }
	return 1;
}

//method to get congestion window
bool VSocket::GetOptimalSndBuf()
{
	G_SENDBUFFER= settings->getSENDBUFFER_EX();
	 return TRUE;

}

