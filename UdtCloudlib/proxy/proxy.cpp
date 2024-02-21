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
            int error_code = lasterror.getErrorCode();
            closesocket(tcpsocket);            
            UDT::close(usock);
            tcpsocket = INVALID_SOCKET;
            usock = UDT::INVALID_SOCK;
            return;
        }
        else if (result > 0) {
            if (readfds.size() > 0) {
                int recvlen = UDT::recv(usock, (char*)tempbuffer, READSIZE, 0);
                if (recvlen <= 0) {
                    UDT::epoll_remove_usock(eid, usock);
                    closesocket(tcpsocket);
                    UDT::close(usock);
                    tcpsocket = INVALID_SOCKET;
                    usock = UDT::INVALID_SOCK;
                    return;
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
                    UDT::epoll_remove_ssock(eid, tcpsocket);
                    closesocket(tcpsocket);
                    UDT::close(usock);
                    tcpsocket = INVALID_SOCKET;
                    usock = UDT::INVALID_SOCK;
                    return;
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
                    if (val == UDT::ERROR)
                        return;
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
                    if (val == SOCKET_ERROR)
                        return;
                    totsend += val;
                }
            }
        }
    }
}