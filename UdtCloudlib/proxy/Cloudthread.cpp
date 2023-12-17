#include "pch.h"
#include "Cloudthread.h"
#include "../proxy/proxy.h"
#include "../CloudManager.h"


CloudThread::CloudThread(CloudManager * cloudManager) :cloudManager(cloudManager)
{   
    usock = UDT::INVALID_SOCK;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    UDT::startup();
}

CloudThread::~CloudThread()
{
    closesocket(udpSocket);
    threadRunning = false;
    if (proxy)
        delete proxy;
    if (hThread)
        WaitForSingleObject(hThread, INFINITE);
    hThread = NULL;   
    WSACleanup();
}

void convertHostnameToIp(char *server)
{
    struct sockaddr_in thataddr {};
    thataddr.sin_addr.s_addr = inet_addr(server);
    if (thataddr.sin_addr.s_addr == INADDR_NONE) {
        LPHOSTENT lphost;
        lphost = gethostbyname(server);
        char* ipaddress;
        ipaddress = inet_ntoa(*(struct in_addr*)*lphost->h_addr_list);
        strcpy_s(server, MAX_HOST_NAME_LEN, ipaddress);
    }
}

void CloudThread::startThread(int port, int vncPort, char* server, char *code, CONNECTIONTYPE ctType)
{
    if (!isThreadRunning()) {  
        convertHostnameToIp(server);
        strcpy_s(this->server, server);
        strcpy_s(this->code, 32, code);
        this->port = port;
        this->ctType = ctType;
        DWORD ThreadID;
        threadRunning = true;
        hThread = CreateThread(NULL, 0, StaticThreadStart, (void*)this, 0, &ThreadID);
        setVNcPort(vncPort);
    }
}

DWORD WINAPI CloudThread::StaticThreadStart(void* Param)
{
    CloudThread* This = (CloudThread*)Param;
    This->ThreadStart();
    This->cloudManager->isConnected();
    return 0;
}

DWORD CloudThread::ThreadStart(void)
{
    bool binded = false;
    int portoffset = 0;
    udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket == INVALID_SOCKET)
        return 0;
    while (!binded) {
        memset(&receiverAddr, 0, sizeof(SOCKADDR_IN));
        receiverAddr.sin_family = AF_INET;
        receiverAddr.sin_port = htons(port + portoffset);
        receiverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        if (bind(udpSocket, (SOCKADDR*)&receiverAddr, sizeof(receiverAddr)) != SOCKET_ERROR) {
            binded = true;
        }
        else {
            portoffset++;
            if (portoffset > 25)
                return 0;
        }
    }
    udpPort = port + portoffset;

    if (!SendInfo())
        return 0;

    int counter = 0;
    cStatus = csConnecting;
    struct timeval timeout;
    struct fd_set fds;
    while ( (cStatus == csConnecting  || cStatus == csOnline) && threadRunning) {
        
        timeout.tv_sec = 6;
        timeout.tv_usec = 0;
        FD_ZERO(&fds);
        FD_SET(udpSocket, &fds);
        int value = select(0, &fds, 0, 0, &timeout);
        if (!threadRunning) {            
            return 0;
        }

        switch (value) {
            case -1:
                cStatus = csError;
                SendDisconnectInfo();
                return 0;
            case 0:
                counter++;
                if (counter == 10) {//60s
                    if (!SendInfo())
                        return 0;
                    counter = 0;
                }
                break;

            default:
            {
                udppacket sp;
                int ReceiverAddrSize = sizeof(receiverAddr);
                int ByteReceived = recvfrom(udpSocket, (char*)&sp, sizeof(sp), 0, (SOCKADDR*)&receiverAddr, &ReceiverAddrSize);
                if (ByteReceived == sizeof(sp)) {
                    if (sp.contype == 0)
                        cStatus = csOnline;
                    else if (sp.contype == 1) {
                        cStatus = csRendezvous;
                        SendDisconnectInfo();
                        if (rendezvous(udpSocket, sp.externip, sp.externport)) {
                            if (ctSERVER == ctType) {
                                ConnectToVNCServer();                                
                                return 1;
                            }
                            else {
                                ConnectVNCViewer();
                                return 0;
                            }
                        }
                    }
                    else if (sp.contype == 2) {
                        cStatus = csRendezvous;
                        SendDisconnectInfo();
                        if (rendezvous(udpSocket, sp.localip, sp.localport)) {
                            if (ctSERVER == ctType) {
                                ConnectToVNCServer();
                                return 1;
                            }
                            else {
                                ConnectVNCViewer();
                                return 0;
                            }
                        }
                    }                   
                    strncpy_s(G_externalip, sp.externip, 16);
                    for (int i = 0; i < 10; i++) {
                        if (!threadRunning) {
                            return 0;
                        }
                        Sleep(500);
                    }
                }
            }
        }
    }
    SendDisconnectInfo();
    return 0;
}

void CloudThread::ConnectToVNCServer()
{
    cStatus = csConnected;
    cloudManager->isConnected();
    //first package instruct to connect to uvnc server
    char pk[32];
    int len = UDT::recv(usock, pk, 32, 0);
    if (len <= 0) {
        return;
    }
    
    struct in_addr addr;
    if (inet_pton(AF_INET, ADDR_LOCALHOST, &addr) <= 0)
        return;
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET)
        return;

    sockaddr_in service{};
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = inet_addr("127.0.0.1");
    service.sin_port = htons(m_VncPort);
    int result = connect(serverSocket, (LPSOCKADDR)&service, sizeof(service));
    if (result == SOCKET_ERROR)
        return;
    proxy = new Proxy(serverSocket, usock);
}

void CloudThread::ConnectVNCViewer()
{
    cStatus = csConnected;    
    struct in_addr addr;
    if (inet_pton(AF_INET, ADDR_LOCALHOST, &addr) <= 0)
        return;

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET)
        return;
    int one = 1;
    u_long iMode = 1;
    int rc;
    sockaddr_in service{};
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = inet_addr("127.0.0.1");
    service.sin_port = htons(m_VncPort);

    if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&one, sizeof(one)) < 0)
        return;

    if (ioctlsocket(listenSocket, FIONBIO, &iMode) < 0)
        return;

    if (bind(listenSocket, (SOCKADDR*)&service, sizeof(service)) == INVALID_SOCKET)
        return;

    rc = listen(listenSocket, 32);
    if (rc < 0)
        return;

    struct fd_set readfds, writefds;
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_SET(listenSocket, &readfds);
    SOCKADDR_STORAGE client;
    int clientlen, retval;
    retval = select(0, &readfds, &writefds, NULL, NULL);
    // Accept the connection
    clientlen = sizeof(client);
    viewerSocket = accept(listenSocket, (SOCKADDR*)&client, &clientlen);
    closesocket(listenSocket);
    if (viewerSocket == INVALID_SOCKET)
        return;
    char pk[32]{};
    int len = UDT::send(usock, pk, 32, 0);
    cloudManager->isConnected();
    proxy = new Proxy(viewerSocket, usock);
}

void CloudThread::stopThread()
{
    threadRunning = false;
    SendDisconnectInfo();
    if (udpSocket != INVALID_SOCKET) {
        closesocket(udpSocket);
    }

    if (listenSocket != INVALID_SOCKET) {
        shutdown(listenSocket, 2);
        closesocket(listenSocket);
    }

    if (serverSocket != INVALID_SOCKET) {
        shutdown(serverSocket, 2);
        closesocket(serverSocket);
    }
    if (viewerSocket != INVALID_SOCKET) {
        shutdown(viewerSocket, 2);
        closesocket(viewerSocket);
    }
    if (usock != UDT::INVALID_SOCK) {
        UDT::close(usock);
    }

    udpSocket = INVALID_SOCKET;
    serverSocket = INVALID_SOCKET;
    viewerSocket = INVALID_SOCKET;
    usock = UDT::INVALID_SOCK;

    cStatus = csUnknown;
    memset(G_externalip, 0, 16);
}

bool CloudThread::SendDisconnectInfo()
{
    udppacket up{};
    up.serverviewer = ctType;
    strcpy_s(up.name, code);
    strcpy_s(up.group, "ikke");
    strcpy_s(up.ident, "uvnc677");
    char szHostName[MAX_HOST_NAME_LEN];
    gethostname(szHostName, MAX_HOST_NAME_LEN);
    struct hostent* host_entry;
    host_entry = gethostbyname(szHostName);
    char* szLocalIP;
    szLocalIP = inet_ntoa(*(struct in_addr*)*host_entry->h_addr_list);
    strncpy_s(up.localip, szLocalIP, 32);
    up.localport = udpPort;
    memset(&receiverAddr, 0, sizeof(SOCKADDR_IN));
    receiverAddr.sin_family = AF_INET;
    receiverAddr.sin_port = htons(NATREMOTE);
    inet_pton(AF_INET, server, &receiverAddr.sin_addr.s_addr);
    up.contype = 10;
    int result = sendto(udpSocket, (char*)&up, sizeof(up), 0, (SOCKADDR*)&receiverAddr, sizeof(receiverAddr));   
    if (result == -1) {
        cStatus = csError;
        return false;
    }
    return true;
}

bool CloudThread::SendInfo()
{
    udppacket up{};
    up.serverviewer = ctType;
    strcpy_s(up.name, code);
    strcpy_s(up.group, "ikke");
    strcpy_s(up.ident, "uvnc677");
    char szHostName[MAX_HOST_NAME_LEN];
    gethostname(szHostName, MAX_HOST_NAME_LEN);
    struct hostent* host_entry;
    host_entry = gethostbyname(szHostName);
    char* szLocalIP;
    szLocalIP = inet_ntoa(*(struct in_addr*)*host_entry->h_addr_list);
    strncpy_s(up.localip, szLocalIP, 32);
    up.localport = udpPort;
    memset(&receiverAddr, 0, sizeof(SOCKADDR_IN));
    receiverAddr.sin_family = AF_INET;
    receiverAddr.sin_port = htons(NATREMOTE);
    inet_pton(AF_INET, server, &receiverAddr.sin_addr.s_addr);
    up.contype = 11;
    int result = sendto(udpSocket, (char*)&up, sizeof(up), 0, (SOCKADDR*)&receiverAddr, sizeof(receiverAddr));
    if (result == -1) {
        cStatus = csError;
        return false;
    }
    return true;
}

bool CloudThread::isThreadRunning()
{
    if (hThread) {
        DWORD result = WaitForSingleObject(hThread, 100);
        if (result == WAIT_TIMEOUT)
            return true;
    }
    threadRunning = false;
    hThread = NULL;
    return false;
}


bool CloudThread::rendezvous(UDPSOCKET udpsock, char* ip, int port)
{
    bool rendezvous = true;
    SOCKADDR_IN          Addr;
    Addr.sin_family = AF_INET;
    Addr.sin_port = htons(port);
    Addr.sin_addr.s_addr = inet_addr(ip);
    usock = UDT::socket(AF_INET, SOCK_STREAM, 0);
    if (usock == UDT::INVALID_SOCK)
        return 0;
    UDT::setsockopt(usock, 0, UDT_RENDEZVOUS, &rendezvous, sizeof(bool));
    //int value = 1052;
    //UDT::setsockopt(usock, 0, UDT_MSS, &value, sizeof(int));

    if (UDT::ERROR != UDT::bind2(usock, udpsock) && (UDT::ERROR != UDT::connect(usock, (sockaddr*)&Addr, sizeof(Addr))) ) {
            char pk[32];
            memset(pk, 5, 32);
            int len = UDT::send(usock, pk, 32, 0);
            int len2 = UDT::recv(usock, pk, 32, 0);
            if (len == 32 && len2 == 32)
                return 1;
        }
    UDT::close(usock);
    usock = UDT::INVALID_SOCK;
    return 0;
}

char* CloudThread::getExternalIpAddress()
{
    return G_externalip;
}

CONNECTIONSTATUS CloudThread::getStatus()
{
    return cStatus;
}

void CloudThread::setVNcPort(int port)
{
    m_VncPort = port;
}
