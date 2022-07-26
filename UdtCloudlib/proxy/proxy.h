#pragma once

#include "../../udt4/src/udt.h"
#include "ringbuffer.h"

#define BUFFERSIZE 5120000
#define READSIZE 1280000

class Proxy
{
private:
	DWORD ThreadStart(void);
	static DWORD WINAPI StaticThreadStart(void* Param);
	SOCKET tcpsocket;
	UDTSOCKET usock;
	RingBuffer udtBuffer;
	RingBuffer tcpBuffer;
	bool running;
	HANDLE threatdHandle = NULL;
	int eid = 0;

public:
	Proxy(SOCKET tcpsocket, UDTSOCKET usock);
	~Proxy();
	void proxyAll();
	void proxyBuffer();

	void startProxy();
};

