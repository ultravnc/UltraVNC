// Cloud Connect implementation
// UDP Matchmaker + UDT rendezvous + TCP<->UDT proxy for VNC viewer

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define NOMINMAX
#include "cloud_connect.h"
#include "../udt4/src/udt.h"

#ifdef _WIN64
#pragma comment(lib, "../udt4/src/x64/udt.lib")
#else
#pragma comment(lib, "../udt4/src/udt.lib")
#endif

#include <iostream>
#include <chrono>
#include <thread>

using namespace std::chrono;

CloudProxyServer::CloudProxyServer()
    : udpSocket_(INVALID_SOCKET)
    , localUdpPort_(0)
    , connected_(false)
    , running_(false) {
    memset(&matchmakerAddr_, 0, sizeof(matchmakerAddr_));
}

CloudProxyServer::~CloudProxyServer() {
    Stop();
}

bool CloudProxyServer::Connect(const std::string& code,
                                const std::string& matchmakerHost,
                                CloudStatusCallback statusCb) {
    code_ = code;

    // Resolve matchmaker
    addrinfo hints{}, *result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    char portStr[8];
    sprintf_s(portStr, "%u", (unsigned)CLOUD_MATCHMAKER_PORT);
    if (getaddrinfo(matchmakerHost.c_str(), portStr, &hints, &result) != 0 || !result) {
        if (statusCb) statusCb(L"Failed to resolve matchmaker host");
        return false;
    }
    matchmakerAddr_ = *(sockaddr_in*)result->ai_addr;
    freeaddrinfo(result);

    // Create UDP socket
    udpSocket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket_ == INVALID_SOCKET) {
        if (statusCb) statusCb(L"Failed to create UDP socket");
        return false;
    }

    // Bind to any available port in range 5000-5100
    sockaddr_in bindAddr{};
    bindAddr.sin_family = AF_INET;
    bindAddr.sin_addr.s_addr = INADDR_ANY;
    bool bound = false;
    for (int port = 5000; port < 5100; ++port) {
        bindAddr.sin_port = htons((u_short)port);
        if (bind(udpSocket_, (sockaddr*)&bindAddr, sizeof(bindAddr)) == 0) {
            localUdpPort_ = (uint16_t)port;
            bound = true;
            break;
        }
    }
    if (!bound) {
        if (statusCb) statusCb(L"Failed to bind UDP socket");
        closesocket(udpSocket_);
        udpSocket_ = INVALID_SOCKET;
        return false;
    }

    // Announce to matchmaker
    if (statusCb) statusCb(L"Registering with matchmaker...");
    if (!Announce(code)) {
        if (statusCb) statusCb(L"Failed to send announcement");
        closesocket(udpSocket_);
        udpSocket_ = INVALID_SOCKET;
        return false;
    }

    // Wait for match
    CloudPeerInfo peer{};
    if (statusCb) statusCb(L"Waiting for VNC server peer...");
    if (!WaitForMatch(peer, 60, statusCb)) {
        closesocket(udpSocket_);
        udpSocket_ = INVALID_SOCKET;
        return false;
    }

    // UDT rendezvous
    wchar_t status[128];
    _snwprintf_s(status, 128, _TRUNCATE, L"Matched! Connecting to %S:%d...",
                 peer.externalIp, peer.externalPort);
    if (statusCb) statusCb(status);

    if (!DoRendezvous(peer, statusCb)) {
        closesocket(udpSocket_);
        udpSocket_ = INVALID_SOCKET;
        return false;
    }

    connected_ = true;
    return true;
}

bool CloudProxyServer::Announce(const std::string& code) {
    // Get local IP
    char hostname[256]{};
    gethostname(hostname, sizeof(hostname));
    hostent* host = gethostbyname(hostname);

    CloudPacket pkt;
    strncpy_s(pkt.name, code.c_str(), sizeof(pkt.name) - 1);
    strncpy_s(pkt.group, "uvnc", sizeof(pkt.group) - 1);
    strncpy_s(pkt.ident, CLOUD_PROTOCOL_IDENT, sizeof(pkt.ident) - 1);

    if (host && host->h_addr_list[0]) {
        const char* localIp = inet_ntoa(*(in_addr*)host->h_addr_list[0]);
        strncpy_s(pkt.localip, localIp, sizeof(pkt.localip) - 1);
    }

    pkt.localport = localUdpPort_;
    pkt.serverviewer = false;  // false = viewer

    int sent = sendto(udpSocket_, (char*)&pkt, sizeof(pkt), 0,
                     (sockaddr*)&matchmakerAddr_, sizeof(matchmakerAddr_));
    return sent == sizeof(pkt);
}

bool CloudProxyServer::WaitForMatch(CloudPeerInfo& peer, int timeoutSeconds, CloudStatusCallback& statusCb) {
    fd_set fds;
    timeval tv;
    auto startTime = steady_clock::now();
    auto lastAnnounce = startTime;

    while (true) {
        auto elapsed = duration_cast<seconds>(steady_clock::now() - startTime).count();
        if (elapsed >= timeoutSeconds) {
            if (statusCb) statusCb(L"Timeout waiting for peer");
            return false;
        }

        FD_ZERO(&fds);
        FD_SET(udpSocket_, &fds);
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        int r = select(0, &fds, nullptr, nullptr, &tv);
        if (r < 0) {
            if (statusCb) statusCb(L"Socket error while waiting");
            return false;
        }

        if (r > 0) {
            CloudPacket pkt;
            sockaddr_in from;
            int fromLen = sizeof(from);
            int received = recvfrom(udpSocket_, (char*)&pkt, sizeof(pkt), 0,
                                   (sockaddr*)&from, &fromLen);

            if (received == sizeof(pkt) && pkt.IsValid()) {
                if (pkt.contype == 0) {
                    // Acknowledged - still waiting
                    if (statusCb) {
                        wchar_t msg[128];
                        _snwprintf_s(msg, 128, _TRUNCATE,
                                     L"Registered (external: %S:%d), waiting for server...",
                                     pkt.externip, pkt.externport);
                        statusCb(msg);
                    }
                } else if (pkt.contype == 1 || pkt.contype == 2) {
                    // Match found
                    strncpy_s(peer.externalIp, pkt.externip, sizeof(peer.externalIp) - 1);
                    peer.externalPort = (uint16_t)pkt.externport;
                    strncpy_s(peer.localIp, pkt.localip, sizeof(peer.localIp) - 1);
                    peer.localPort = (uint16_t)pkt.localport;
                    peer.isLan = (pkt.contype == 2);
                    return true;
                }
            }
        }

        // Re-announce every 10 seconds to keep registration alive
        auto now = steady_clock::now();
        if (duration_cast<seconds>(now - lastAnnounce).count() >= 10) {
            lastAnnounce = now;
            Announce(code_);
        }
    }
}

bool CloudProxyServer::DoRendezvous(const CloudPeerInfo& peer, CloudStatusCallback& statusCb) {
    UDT::startup();

    UDTSOCKET udtSock = UDT::socket(AF_INET, SOCK_STREAM, 0);
    if (udtSock == UDT::INVALID_SOCK) {
        if (statusCb) statusCb(L"Failed to create UDT socket");
        UDT::cleanup();
        return false;
    }

    // Enable rendezvous (both sides connect simultaneously)
    bool rendezvous = true;
    UDT::setsockopt(udtSock, 0, UDT_RENDEZVOUS, &rendezvous, sizeof(bool));
    std::this_thread::sleep_for(milliseconds(1000));

    int mss = 1052;
    UDT::setsockopt(udtSock, 0, UDT_MSS, &mss, sizeof(int));
    std::this_thread::sleep_for(milliseconds(1000));

    // Bind to same UDP socket (reuse port for NAT hole punching)
    if (UDT::ERROR == UDT::bind2(udtSock, udpSocket_)) {
        if (statusCb) statusCb(L"UDT bind failed");
        UDT::close(udtSock);
        UDT::cleanup();
        return false;
    }
    std::this_thread::sleep_for(milliseconds(1000));

    // LAN match: use local IP, WAN: use external IP
    const char* ip   = (peer.isLan && peer.localPort > 0 && strlen(peer.localIp) > 0)
                       ? peer.localIp : peer.externalIp;
    int          port = (peer.isLan && peer.localPort > 0 && strlen(peer.localIp) > 0)
                       ? peer.localPort : peer.externalPort;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    if (UDT::ERROR == UDT::connect(udtSock, (sockaddr*)&addr, sizeof(addr))) {
        if (statusCb) statusCb(L"UDT rendezvous failed");
        UDT::close(udtSock);
        UDT::cleanup();
        return false;
    }
    std::this_thread::sleep_for(milliseconds(1000));

    // 32-byte handshake to confirm both sides connected
    char pk[32];
    memset(pk, 5, 32);
    int hs = UDT::send(udtSock, pk, 32, 0);
    int hr = UDT::recv(udtSock, pk, 32, 0);
    if (hs != 32 || hr != 32) {
        if (statusCb) statusCb(L"UDT handshake failed");
        UDT::close(udtSock);
        UDT::cleanup();
        return false;
    }

    if (statusCb) statusCb(L"UDT tunnel established! Starting TCP proxy...");

    // Create the TCP listen socket here (before the thread) so we can
    // signal readiness before returning - avoids race with the caller connecting.
    SOCKET tcpListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tcpListen == INVALID_SOCKET) {
        UDT::close(udtSock);
        UDT::cleanup();
        return false;
    }
    int reuseAddr = 1;
    setsockopt(tcpListen, SOL_SOCKET, SO_REUSEADDR, (char*)&reuseAddr, sizeof(reuseAddr));
    sockaddr_in listenAddr{};
    listenAddr.sin_family = AF_INET;
    listenAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    listenAddr.sin_port = htons(CLOUD_LOCAL_TCP_PORT);
    if (bind(tcpListen, (sockaddr*)&listenAddr, sizeof(listenAddr)) == SOCKET_ERROR ||
        listen(tcpListen, 1) == SOCKET_ERROR) {
        closesocket(tcpListen);
        UDT::close(udtSock);
        UDT::cleanup();
        return false;
    }

    // Proxy is now listening - safe for caller to connect to localhost:5901
    running_ = true;
    proxyThread_ = std::thread([this, udtSock, tcpListen]() mutable {
        // Set accept timeout so we can check running_
        DWORD timeout = 2000;
        setsockopt(tcpListen, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

        SOCKET tcpClient = INVALID_SOCKET;
        while (running_ && tcpClient == INVALID_SOCKET) {
            sockaddr_in clientAddr{};
            int clientLen = sizeof(clientAddr);
            tcpClient = accept(tcpListen, (sockaddr*)&clientAddr, &clientLen);
        }
        closesocket(tcpListen);

        if (tcpClient == INVALID_SOCKET || !running_) {
            UDT::close(udtSock);
            UDT::cleanup();
            return;
        }

        // Set UDT recv timeout so we can check running_ flag
        int udtTimeout = 500;
        UDT::setsockopt(udtSock, 0, UDT_RCVTIMEO, &udtTimeout, sizeof(udtTimeout));

        // Proxy bidirectionally with separate buffers for each direction
        const int BUFSIZE = 65536;

        // Thread: TCP -> UDT (viewer input to server)
        std::thread tcpToUdt([&udtSock, tcpClient, this]() {
            char* buf = new char[65536];
            fd_set fds;
            timeval tv;
            while (running_) {
                FD_ZERO(&fds); FD_SET(tcpClient, &fds);
                tv.tv_sec = 0; tv.tv_usec = 500000;
                int sel = select(0, &fds, NULL, NULL, &tv);
                if (sel <= 0) continue;
                int n = recv(tcpClient, buf, 65536, 0);
                if (n <= 0) { running_ = false; break; }
                int sent = 0;
                while (sent < n && running_) {
                    int s = UDT::send(udtSock, buf + sent, n - sent, 0);
                    if (s == UDT::ERROR) { running_ = false; break; }
                    sent += s;
                }
            }
            running_ = false;
            delete[] buf;
        });

        // Main: UDT -> TCP (server responses to viewer)
        char* buf = new char[BUFSIZE];
        while (running_) {
            int n = UDT::recv(udtSock, buf, BUFSIZE, 0);
            if (n == UDT::ERROR) {
                if (UDT::getlasterror().getErrorCode() == CUDTException::ETIMEOUT) continue;
                running_ = false; break;
            }
            if (n == 0) { running_ = false; break; }
            int sent = 0;
            while (sent < n && running_) {
                int s = send(tcpClient, buf + sent, n - sent, 0);
                if (s == SOCKET_ERROR) { running_ = false; break; }
                sent += s;
            }
        }
        running_ = false;

        if (tcpToUdt.joinable()) tcpToUdt.join();

        delete[] buf;
        closesocket(tcpClient);
        UDT::close(udtSock);
        UDT::cleanup();
    });

    return true;
}

void CloudProxyServer::Stop() {
    running_ = false;
    connected_ = false;

    if (proxyThread_.joinable()) {
        proxyThread_.join();
    }

    if (udpSocket_ != INVALID_SOCKET) {
        closesocket(udpSocket_);
        udpSocket_ = INVALID_SOCKET;
    }
}

bool CloudProxyServer::Probe(const std::string& code,
                              const std::string& matchmakerHost,
                              int timeoutMs) {
    // Resolve matchmaker address
    addrinfo hints{}, *result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    char portStr[8];
    sprintf_s(portStr, "%u", (unsigned)CLOUD_MATCHMAKER_PORT);
    if (getaddrinfo(matchmakerHost.c_str(), portStr, &hints, &result) != 0 || !result)
        return false;

    sockaddr_in mmAddr{};
    memcpy(&mmAddr, result->ai_addr, sizeof(mmAddr));
    freeaddrinfo(result);

    // Temporary UDP socket
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET)
        return false;

    sockaddr_in bindAddr{};
    bindAddr.sin_family = AF_INET;
    bindAddr.sin_addr.s_addr = INADDR_ANY;
    bindAddr.sin_port = 0;
    bind(sock, (sockaddr*)&bindAddr, sizeof(bindAddr));

    // Send PROBE packet (contype=3)
    CloudPacket pkt;
    strncpy_s(pkt.name, code.c_str(), sizeof(pkt.name) - 1);
    strncpy_s(pkt.ident, CLOUD_PROTOCOL_IDENT, sizeof(pkt.ident) - 1);
    pkt.serverviewer = false;
    pkt.contype = 3; // ConnType::PROBE
    sendto(sock, (char*)&pkt, sizeof(pkt), 0, (sockaddr*)&mmAddr, sizeof(mmAddr));

    // Wait for PROBE_ONLINE(4) or PROBE_OFFLINE(5) response
    fd_set fds;
    timeval tv;
    tv.tv_sec  = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);

    bool online = false;
    if (select(0, &fds, NULL, NULL, &tv) > 0) {
        CloudPacket resp;
        sockaddr_in from{};
        int fromLen = sizeof(from);
        int r = recvfrom(sock, (char*)&resp, sizeof(resp), 0, (sockaddr*)&from, &fromLen);
        if (r == sizeof(resp) && resp.IsValid())
            online = (resp.contype == 4); // ConnType::PROBE_ONLINE
    }

    closesocket(sock);
    return online;
}
