#pragma once

// Cloud Server Proxy: UDP Matchmaker + UDT rendezvous + TCP proxy for WinVNC server
// Announces to support1.uvnc.com:5352 as a server peer, waits for a viewer to match,
// then performs UDT rendezvous and proxies traffic to/from 127.0.0.1:<vncport>.

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <mutex>
#include <vector>
#include <cstdint>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

// Protocol constants - must match matchmaker server and cloud_connect.h
#define CLOUD_SERVER_PROTOCOL_IDENT "uvnc677"
constexpr uint16_t CLOUD_SERVER_MATCHMAKER_PORT = 5352;
constexpr const char* CLOUD_SERVER_MATCHMAKER_HOST = "support1.uvnc.com";

#pragma pack(push, 1)
struct CloudServerPacket {
    char name[32];
    char group[32];
    char ident[8];
    char localip[32];
    char externip[32];
    int32_t localport;
    int32_t externport;
    int32_t contype;
    bool serverviewer;
    bool dummy0;
    bool dummy1;
    bool dummy2;

    CloudServerPacket() { memset(this, 0, sizeof(*this)); }

    bool IsValid() const {
        return strncmp(ident, CLOUD_SERVER_PROTOCOL_IDENT, 7) == 0;
    }
};
#pragma pack(pop)

enum CloudStatus {
    csOffline = 0,
    csOnline,
    csRendezvous,
    csConnected
};

struct CloudServerPeerInfo {
    char externalIp[32];
    uint16_t externalPort;
    char localIp[32];
    uint16_t localPort;
    bool isLan;
};

class CloudServerProxy {
public:
    CloudServerProxy(const std::string& code,
                     uint16_t vncPort,
                     const std::string& matchmakerHost = CLOUD_SERVER_MATCHMAKER_HOST);
    ~CloudServerProxy();

    // Start: announces to matchmaker and enters wait+serve loop
    // Runs until Stop() is called.
    void Run();

    void Stop();

    bool IsRunning() const { return running_; }

    CloudStatus GetStatus() const { return status_; }
    std::string GetStatusText() const { return statusText_; }
    std::string GetExternalIp() const { return externalIp_; }

    using LogCallback = std::function<void(const std::string&)>;
    void SetLogCallback(LogCallback cb) { std::lock_guard<std::mutex> l(logMutex_); logCallback_ = cb; }

    // Drain pending log lines (called from UI thread)
    std::vector<std::string> DrainLogs() {
        std::lock_guard<std::mutex> l(logMutex_);
        return std::move(pendingLogs_);
    }

private:
    bool SetupUdpSocket();
    bool Announce();
    bool WaitForMatch(CloudServerPeerInfo& peer, int timeoutSeconds);
    bool DoRendezvous(const CloudServerPeerInfo& peer, int& outSock);  // int == UDTSOCKET
    void ProxyViewerSession(int udtSock);                               // int == UDTSOCKET

    std::string code_;
    uint16_t vncPort_;
    std::string matchmakerHost_;

    SOCKET udpSocket_;
    sockaddr_in matchmakerAddr_;
    uint16_t localUdpPort_;

    std::atomic<bool> running_;
    std::string externalIp_;
    std::atomic<CloudStatus> status_;
    std::string statusText_;

    std::mutex logMutex_;
    LogCallback logCallback_;
    std::vector<std::string> pendingLogs_;

    void PushLog(const std::string& line);
};
