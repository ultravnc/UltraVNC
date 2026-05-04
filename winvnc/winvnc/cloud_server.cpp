// Cloud Server Proxy implementation
// UDP Matchmaker + UDT rendezvous + TCP<->UDT proxy for WinVNC server

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define NOMINMAX
#include "cloud_server.h"
#include "../../udt4/src/udt.h"

#ifdef _WIN64
#pragma comment(lib, "../../udt4/src/x64/udt.lib")
#else
#pragma comment(lib, "../../udt4/src/udt.lib")
#endif

#include <chrono>
#include <thread>
#include <cstdio>
#include <cstdarg>

void CloudServerProxy::PushLog(const std::string& line) {
    // Convert \n to \r\n for Windows edit control
    std::string crlf;
    crlf.reserve(line.size() + 4);
    for (char c : line) {
        if (c == '\n') crlf += "\r\n";
        else crlf += c;
    }
    std::lock_guard<std::mutex> lock(logMutex_);
    pendingLogs_.push_back(crlf);
    // Also write to file (original line)
    FILE* f = nullptr;
    if (fopen_s(&f, "C:\\ProgramData\\UltraVNC\\cloud_nat.log", "a") == 0 && f) {
        fputs(line.c_str(), f);
        fclose(f);
    }
}

using namespace std::chrono;

CloudServerProxy::CloudServerProxy(const std::string& code,
                                   uint16_t vncPort,
                                   const std::string& matchmakerHost)
    : code_(code)
    , vncPort_(vncPort)
    , matchmakerHost_(matchmakerHost)
    , udpSocket_(INVALID_SOCKET)
    , localUdpPort_(0)
    , running_(false)
    , status_(csOffline) {
    memset(&matchmakerAddr_, 0, sizeof(matchmakerAddr_));
}

CloudServerProxy::~CloudServerProxy() {
    Stop();
}

void CloudServerProxy::Stop() {
    running_ = false;
    if (udpSocket_ != INVALID_SOCKET) {
        closesocket(udpSocket_);
        udpSocket_ = INVALID_SOCKET;
    }
}

bool CloudServerProxy::SetupUdpSocket() {
    { char _buf[256]; snprintf(_buf, sizeof(_buf), "[CloudNAT] SetupUdpSocket: resolving %s\n", matchmakerHost_.c_str()); PushLog(_buf); }
    addrinfo hints{}, *result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    char portStr[8];
    sprintf_s(portStr, "%u", (unsigned)CLOUD_SERVER_MATCHMAKER_PORT);
    if (getaddrinfo(matchmakerHost_.c_str(), portStr, &hints, &result) != 0 || !result) {
        { char _buf[256]; snprintf(_buf, sizeof(_buf), "[CloudNAT] SetupUdpSocket: getaddrinfo FAILED for %s\n", matchmakerHost_.c_str()); PushLog(_buf); }
        return false;
    }
    matchmakerAddr_ = *(sockaddr_in*)result->ai_addr;
    freeaddrinfo(result);
    PushLog("[CloudNAT] SetupUdpSocket: matchmaker resolved OK\n");

    udpSocket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket_ == INVALID_SOCKET) {
        { char _buf[256]; snprintf(_buf, sizeof(_buf), "[CloudNAT] SetupUdpSocket: socket() FAILED err=%d\n", WSAGetLastError()); PushLog(_buf); }
        return false;
    }

    sockaddr_in bindAddr{};
    bindAddr.sin_family = AF_INET;
    bindAddr.sin_addr.s_addr = INADDR_ANY;
    for (int port = 5000; port < 5100; ++port) {
        bindAddr.sin_port = htons((u_short)port);
        if (bind(udpSocket_, (sockaddr*)&bindAddr, sizeof(bindAddr)) == 0) {
            localUdpPort_ = (uint16_t)port;
            { char _buf[64]; snprintf(_buf, sizeof(_buf), "[CloudNAT] bound to port %d\n", port); PushLog(_buf); }
            return true;
        }
    }

    PushLog("[CloudNAT] SetupUdpSocket: bind FAILED - no free port in 5000-5099\n");
    closesocket(udpSocket_);
    udpSocket_ = INVALID_SOCKET;
    return false;
}

bool CloudServerProxy::Announce() {
    char hostname[256]{};
    gethostname(hostname, sizeof(hostname));
    hostent* host = gethostbyname(hostname);

    CloudServerPacket pkt;
    strncpy_s(pkt.name, code_.c_str(), sizeof(pkt.name) - 1);
    strncpy_s(pkt.group, "uvnc", sizeof(pkt.group) - 1);
    strncpy_s(pkt.ident, CLOUD_SERVER_PROTOCOL_IDENT, sizeof(pkt.ident) - 1);

    if (host && host->h_addr_list[0]) {
        const char* localIp = inet_ntoa(*(in_addr*)host->h_addr_list[0]);
        strncpy_s(pkt.localip, localIp, sizeof(pkt.localip) - 1);
    }

    pkt.localport = localUdpPort_;
    pkt.serverviewer = true;  // true = server

    { char _buf[256]; snprintf(_buf, sizeof(_buf), "[CloudNAT] Announce: code=%s port=%d\n", pkt.name, localUdpPort_); PushLog(_buf); }
    int sent = sendto(udpSocket_, (char*)&pkt, sizeof(pkt), 0,
                     (sockaddr*)&matchmakerAddr_, sizeof(matchmakerAddr_));
    if (sent != (int)sizeof(pkt))
        { char _buf[128]; snprintf(_buf, sizeof(_buf), "[CloudNAT] Announce: sendto FAILED sent=%d err=%d\n", sent, WSAGetLastError()); PushLog(_buf); }
    else
        { char _buf[64]; snprintf(_buf, sizeof(_buf), "[CloudNAT] Announce: sent %d bytes OK\n", sent); PushLog(_buf); }
    return sent == sizeof(pkt);
}

bool CloudServerProxy::WaitForMatch(CloudServerPeerInfo& peer, int timeoutSeconds) {
    fd_set fds;
    timeval tv;
    auto startTime = steady_clock::now();
    int reannounceCounter = 0;

    while (running_) {
        auto elapsed = duration_cast<seconds>(steady_clock::now() - startTime).count();
        if (elapsed >= timeoutSeconds)
            return false;

        FD_ZERO(&fds);
        FD_SET(udpSocket_, &fds);
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        int r = select(0, &fds, nullptr, nullptr, &tv);
        if (r < 0)
            return false;

        if (r > 0) {
            CloudServerPacket pkt;
            sockaddr_in from{};
            int fromLen = sizeof(from);
            int received = recvfrom(udpSocket_, (char*)&pkt, sizeof(pkt), 0,
                                   (sockaddr*)&from, &fromLen);

            { char _buf[128]; snprintf(_buf, sizeof(_buf), "[CloudNAT] recvfrom bytes=%d contype=%d\n", received, (received == (int)sizeof(pkt)) ? pkt.contype : -1); PushLog(_buf); }

            if (received == sizeof(pkt) && pkt.IsValid()) {
                if (pkt.contype == 0) {
                    // Acknowledged by matchmaker - now truly online
                    externalIp_ = pkt.externip;
                    status_ = csOnline;
                    statusText_ = "Matchserver found, waiting for viewer...";
                    { char _buf[128]; snprintf(_buf, sizeof(_buf), "[CloudNAT] ACK - external=%s:%d\n", pkt.externip, pkt.externport); PushLog(_buf); }
                } else if (pkt.contype == 1 || pkt.contype == 2) {
                    // Match found
                    strncpy_s(peer.externalIp, pkt.externip, sizeof(peer.externalIp) - 1);
                    peer.externalPort = (uint16_t)pkt.externport;
                    strncpy_s(peer.localIp, pkt.localip, sizeof(peer.localIp) - 1);
                    peer.localPort = (uint16_t)pkt.localport;
                    peer.isLan = (pkt.contype == 2);
                    { char _buf[128]; snprintf(_buf, sizeof(_buf), "[CloudNAT] MATCH [%s] peer=%s:%d\n", pkt.contype == 2 ? "LAN" : "WAN", peer.externalIp, peer.externalPort); PushLog(_buf); }
                    return true;
                }
            }
        }

        // Re-announce every ~30 seconds
        reannounceCounter++;
        if (reannounceCounter >= 15) {
            reannounceCounter = 0;
            Announce();
        }
    }
    return false;
}

bool CloudServerProxy::DoRendezvous(const CloudServerPeerInfo& peer, int& outSock) {
    // Choose address: LAN match uses local IP, WAN uses external
    const char* ip   = (peer.isLan && peer.localPort > 0 && strlen(peer.localIp) > 0)
                       ? peer.localIp : peer.externalIp;
    int          port = (peer.isLan && peer.localPort > 0 && strlen(peer.localIp) > 0)
                       ? peer.localPort : peer.externalPort;

    { char _buf[128]; snprintf(_buf, sizeof(_buf), "[CloudNAT] DoRendezvous %s:%d\n", ip, port); PushLog(_buf); }

    int udtSock = UDT::socket(AF_INET, SOCK_STREAM, 0);
    if (udtSock == UDT::INVALID_SOCK) {
        PushLog("[CloudNAT] UDT socket() failed\n");
        return false;
    }

    bool rendezvous = true;
    UDT::setsockopt(udtSock, 0, UDT_RENDEZVOUS, &rendezvous, sizeof(bool));
    std::this_thread::sleep_for(milliseconds(1000));

    int mss = 1052;
    UDT::setsockopt(udtSock, 0, UDT_MSS, &mss, sizeof(int));
    std::this_thread::sleep_for(milliseconds(1000));

    if (UDT::ERROR == UDT::bind2(udtSock, udpSocket_)) {
        { char _buf[128]; snprintf(_buf, sizeof(_buf), "[CloudNAT] UDT bind2 FAILED err=%d\n", UDT::getlasterror().getErrorCode()); PushLog(_buf); }
        UDT::close(udtSock);
        return false;
    }
    std::this_thread::sleep_for(milliseconds(1000));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    if (UDT::ERROR == UDT::connect(udtSock, (sockaddr*)&addr, sizeof(addr))) {
        { char _buf[128]; snprintf(_buf, sizeof(_buf), "[CloudNAT] UDT connect FAILED err=%d\n", UDT::getlasterror().getErrorCode()); PushLog(_buf); }
        UDT::close(udtSock);
        return false;
    }
    std::this_thread::sleep_for(milliseconds(1000));

    // 32-byte handshake to confirm both sides connected
    char pk[32];
    memset(pk, 5, 32);
    int s = UDT::send(udtSock, pk, 32, 0);
    int r = UDT::recv(udtSock, pk, 32, 0);
    if (s != 32 || r != 32) {
        { char _buf[128]; snprintf(_buf, sizeof(_buf), "[CloudNAT] handshake FAILED s=%d r=%d\n", s, r); PushLog(_buf); }
        UDT::close(udtSock);
        return false;
    }

    PushLog("[CloudNAT] DoRendezvous OK\n");
    outSock = udtSock;
    return true;
}

void CloudServerProxy::ProxyViewerSession(int udtSock) {
    // Connect TCP to local VNC server
    SOCKET tcpSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tcpSock == INVALID_SOCKET) {
        UDT::close(udtSock);
        return;
    }

    sockaddr_in vncAddr{};
    vncAddr.sin_family = AF_INET;
    vncAddr.sin_port = htons(vncPort_);
    vncAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (connect(tcpSock, (sockaddr*)&vncAddr, sizeof(vncAddr)) == SOCKET_ERROR) {
        closesocket(tcpSock);
        UDT::close(udtSock);
        return;
    }

    // Set UDT recv timeout so we can check active flag
    int udtTimeout = 500;
    UDT::setsockopt(udtSock, 0, UDT_RCVTIMEO, &udtTimeout, sizeof(udtTimeout));

    // Proxy bidirectionally: UDT (viewer) <-> TCP (local VNC server)
    const int BUFSIZE = 65536;
    std::atomic<bool> active{ true };

    // Thread: TCP -> UDT (VNC server responses to viewer)
    std::thread tcpToUdt([&udtSock, tcpSock, &active]() {
        char* buf = new char[65536];
        fd_set fds;
        timeval tv;
        while (active) {
            FD_ZERO(&fds); FD_SET(tcpSock, &fds);
            tv.tv_sec = 0; tv.tv_usec = 500000;
            int sel = select(0, &fds, NULL, NULL, &tv);
            if (sel <= 0) continue;
            int n = recv(tcpSock, buf, 65536, 0);
            if (n <= 0) { active = false; break; }
            int sent = 0;
            while (sent < n && active) {
                int s = UDT::send(udtSock, buf + sent, n - sent, 0);
                if (s == UDT::ERROR) { active = false; break; }
                sent += s;
            }
        }
        active = false;
        delete[] buf;
    });

    // Main: UDT -> TCP (viewer input to VNC server)
    char* buf = new char[BUFSIZE];
    while (active) {
        int n = UDT::recv(udtSock, buf, BUFSIZE, 0);
        if (n == UDT::ERROR) {
            if (UDT::getlasterror().getErrorCode() == CUDTException::ETIMEOUT) continue;
            active = false; break;
        }
        if (n == 0) { active = false; break; }
        int sent = 0;
        while (sent < n && active) {
            int s = send(tcpSock, buf + sent, n - sent, 0);
            if (s == SOCKET_ERROR) { active = false; break; }
            sent += s;
        }
    }
    active = false;

    if (tcpToUdt.joinable()) tcpToUdt.join();

    delete[] buf;
    closesocket(tcpSock);
    UDT::close(udtSock);
}

void CloudServerProxy::Run() {
    { char _buf[256]; snprintf(_buf, sizeof(_buf), "[CloudNAT] Started code=%s port=%u host=%s\n", code_.c_str(), vncPort_, matchmakerHost_.c_str()); PushLog(_buf); }
    status_ = csOffline;
    statusText_ = "Starting...";
    UDT::startup();
    running_ = true;

    while (running_) {
        // (Re)create UDP socket each connection cycle
        if (udpSocket_ == INVALID_SOCKET) {
            statusText_ = "Connecting to matchmaker...";
            PushLog("[CloudNAT] Setting up UDP socket...\n");
            if (!SetupUdpSocket()) {
                statusText_ = "Matchmaker unreachable, retrying...";
                PushLog("[CloudNAT] SetupUdpSocket FAILED, retry in 5s\n");
                std::this_thread::sleep_for(seconds(5));
                continue;
            }
        }

        if (!Announce()) {
            statusText_ = "Announce failed, retrying...";
            PushLog("[CloudNAT] Announce FAILED, retry in 5s\n");
            closesocket(udpSocket_);
            udpSocket_ = INVALID_SOCKET;
            std::this_thread::sleep_for(seconds(5));
            continue;
        }

        statusText_ = "Waiting for matchmaker ACK...";
        status_ = csOffline;
        PushLog("[CloudNAT] Waiting for viewer match...\n");
        CloudServerPeerInfo peer{};
        if (!WaitForMatch(peer, 300)) {
            statusText_ = "Timeout waiting for viewer, re-announcing...";
            status_ = csOffline;
            PushLog("[CloudNAT] WaitForMatch timed out, re-announcing\n");
            closesocket(udpSocket_);
            udpSocket_ = INVALID_SOCKET;
            continue;
        }

        if (!running_) break;

        statusText_ = "Viewer matched, establishing connection...";
        status_ = csRendezvous;
        { char _buf[128]; snprintf(_buf, sizeof(_buf), "[CloudNAT] Matched! rendezvous with %s:%d\n", peer.externalIp, peer.externalPort); PushLog(_buf); }
        int udtSock = UDT::INVALID_SOCK;
        if (!DoRendezvous(peer, udtSock)) {
            statusText_ = "Rendezvous failed, re-announcing...";
            status_ = csOffline;
            PushLog("[CloudNAT] DoRendezvous FAILED, re-announcing\n");
            closesocket(udpSocket_);
            udpSocket_ = INVALID_SOCKET;
            continue;
        }

        statusText_ = "Connected (proxying viewer session)";
        status_ = csConnected;
        PushLog("[CloudNAT] Rendezvous OK, starting proxy thread\n");

        // Hand udpSocket_ ownership to the proxy thread - UDT needs it open during session
        SOCKET sessionUdpSock = udpSocket_;
        udpSocket_ = INVALID_SOCKET;

        std::thread clientProxyThread([this, udtSock, sessionUdpSock]() {
            ProxyViewerSession(udtSock);
            closesocket(sessionUdpSock);
        });
        clientProxyThread.detach();

        std::this_thread::sleep_for(milliseconds(500));
    }

    status_ = csOffline;
    statusText_ = "Stopped";
    PushLog("[CloudNAT] Run() exiting\n");
    UDT::cleanup();
}
