#pragma once

// Cloud Connect: UDP Matchmaker + UDT rendezvous + TCP proxy for VNC viewer
// Contacts support1.uvnc.com:5352, matches with server peer, then
// exposes a local TCP port that the VNC viewer connects to.

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <cstdint>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

// Protocol constants - must match matchmaker server
#define CLOUD_PROTOCOL_IDENT "uvnc677"
constexpr uint16_t CLOUD_MATCHMAKER_PORT = 5352;
constexpr const char* CLOUD_MATCHMAKER_HOST = "support1.uvnc.com";
constexpr uint16_t CLOUD_LOCAL_TCP_PORT = 5901;  // VNC viewer connects here

#pragma pack(push, 1)
struct CloudPacket {
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

    CloudPacket() { memset(this, 0, sizeof(*this)); }

    bool IsValid() const {
        return strncmp(ident, CLOUD_PROTOCOL_IDENT, 7) == 0;
    }
};
#pragma pack(pop)

struct CloudPeerInfo {
    char externalIp[32];
    uint16_t externalPort;
    char localIp[32];
    uint16_t localPort;
    bool isLan;
};

// Status callback: called on connection state changes
using CloudStatusCallback = std::function<void(const wchar_t* status)>;

class CloudProxyServer {
public:
    CloudProxyServer();
    ~CloudProxyServer();

    // Connect to matchmaker and wait for peer.
    // matchmakerHost: defaults to support1.uvnc.com
    // Returns true if match found and UDT socket opened.
    bool Connect(const std::string& code,
                 const std::string& matchmakerHost = CLOUD_MATCHMAKER_HOST,
                 CloudStatusCallback statusCb = nullptr);

    // Probe: check if server with this code is online without making a connection.
    // Returns true if online, false if offline or no response within timeoutMs.
    static bool Probe(const std::string& code,
                      const std::string& matchmakerHost = CLOUD_MATCHMAKER_HOST,
                      int timeoutMs = 3000);

    // Stop everything
    void Stop();

    // After Connect() returns true, viewer should connect TCP to this port
    uint16_t GetLocalTcpPort() const { return CLOUD_LOCAL_TCP_PORT; }

    bool IsConnected() const { return connected_; }

private:
    bool Announce(const std::string& code);
    bool WaitForMatch(CloudPeerInfo& peer, int timeoutSeconds, CloudStatusCallback& cb);
    bool DoRendezvous(const CloudPeerInfo& peer, CloudStatusCallback& cb);
    void RunProxy(SOCKET tcpClient);

    std::string code_;
    SOCKET udpSocket_;
    sockaddr_in matchmakerAddr_;
    uint16_t localUdpPort_;

    std::atomic<bool> connected_;
    std::atomic<bool> running_;

    std::thread proxyThread_;
};
