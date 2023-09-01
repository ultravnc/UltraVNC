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
	bool SendDisconnectInfo();
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

