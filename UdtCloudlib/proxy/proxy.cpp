#include "pch.h"
#include "proxy.h"
#define KEEPALIVE_TIME (time_t)5 

using namespace std;

Proxy::Proxy(SOCKET tcpsocket, UDTSOCKET usock)
{
    tcpBuffer.RingBufferSet(BUFFERSIZE);
    udtBuffer.RingBufferSet(BUFFERSIZE);
    this->tcpsocket = tcpsocket;
    this->usock = usock;
    eid = UDT::epoll_create();
    UDT::epoll_add_usock(eid, usock);
    UDT::epoll_add_ssock(eid, tcpsocket);
    running = true;
    startProxy();
    proxyAll();
    //proxyToTcp();
}

Proxy::~Proxy()
{    
    running = false;
    UDT::epoll_release(eid);

    if (threatdHandle)
        WaitForSingleObject(threatdHandle, INFINITE);    
}

DWORD WINAPI Proxy::StaticThreadStart(void* Param)
{
    Proxy* This = (Proxy*)Param;
    return This->ThreadStart();
}

void Proxy::startProxy()
{
    DWORD ThreadID;
    threatdHandle = CreateThread(NULL, 0, StaticThreadStart, (void*)this, 0, &ThreadID);
}

DWORD Proxy::ThreadStart(void)
{
    proxyBuffer();
    return true;
}


void Proxy::proxyAll()
{
    char* tempbuffer = new char[READSIZE];
    while (running) {
        set<SYSSOCKET> lrfds;
        set<UDTSOCKET> readfds;
        int result = UDT::epoll_wait(eid, &readfds, NULL, -1, &lrfds);
        if (result < 0) {
            CUDTException& lasterror = UDT::getlasterror();
            goto error;
        }
        else if (result > 0) {
            if (readfds.size() > 0) {
                int recvlen = UDT::recv(usock, (char*)tempbuffer, READSIZE, 0);
                if (recvlen <= 0) {
                    goto error;
                }
                while (udtBuffer.GetWriteAvail() < recvlen)
                {
                    Sleep(5);
                    if (!running)
                        return;
                }
                udtBuffer.Write(tempbuffer, recvlen);
            }
            if (lrfds.size() > 0)
            {
                int recvlen = recv(tcpsocket, tempbuffer, READSIZE, 0);
                if (recvlen <= 0) {
                    goto error;
                }
                while (tcpBuffer.GetWriteAvail() < recvlen)
                {
                    Sleep(5);
                    if (!running)
                        return;
                }
                tcpBuffer.Write(tempbuffer, recvlen);
            }
        }
        
    }
    error:
        UDT::epoll_remove_usock(eid, usock);
        shutdown(tcpsocket, SD_BOTH);
        closesocket(tcpsocket);
        UDT::close(usock);
        tcpsocket = INVALID_SOCKET;
        usock = UDT::INVALID_SOCK;
        running = false;

}
void Proxy::proxyBuffer()
{
    char* tempbuffer = new char[READSIZE];
    HANDLE testevent[2];
    testevent[0] = tcpBuffer.inputevent;
    testevent[1] = udtBuffer.inputevent;

    while (running) {
        WaitForMultipleObjects(2, testevent, false, 1000);
        if (tcpBuffer.GetReadAvail() > 0) {
            int totsend = 0;
            while (tcpBuffer.GetReadAvail() > 0) {
                int val;
                int totsend = 0;
                int recvlen = tcpBuffer.Read(tempbuffer, READSIZE);
                while (totsend < recvlen) {
                    val = UDT::send(usock, tempbuffer + totsend, recvlen - totsend, 0);
                    if (val == UDT::ERROR) {
                        running = false;
                        return;
                    }
                    totsend += val;
                }
            }
        }
        if (udtBuffer.GetReadAvail() > 0) {
            int totsend = 0;
            while (udtBuffer.GetReadAvail() > 0) {
                int val;
                int totsend = 0;
                int recvlen = udtBuffer.Read(tempbuffer, READSIZE);
                while (totsend < recvlen) {
                    val = send(tcpsocket, tempbuffer + totsend, recvlen - totsend, 0);
                    if (val == SOCKET_ERROR) {
                        running = false;
                        return;
                    }
                    totsend += val;
                }
            }
        }
    }
}