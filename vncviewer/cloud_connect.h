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
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")

// Protocol constants - must match matchmaker server
#define CLOUD_PROTOCOL_IDENT "uvnc677"
constexpr uint16_t CLOUD_MATCHMAKER_PORT = 5352;
constexpr const char* CLOUD_MATCHMAKER_HOST = "support1.uvnc.com";
constexpr uint16_t CLOUD_LOCAL_TCP_PORT = 5901;  // VNC viewer connects here

#pragma pack(push, 1)
struct CloudPacket {
    char name[32];
    char group[32];       // token (from portal) used as HMAC key derivation input
    char ident[8];
    char localip[32];
    char externip[32];
    int32_t localport;
    int32_t externport;
    int32_t contype;
    bool serverviewer;
    uint8_t _pad0;
    uint8_t _pad1;
    uint8_t _pad2;
    uint32_t timestamp;   // unix time (UTC) when packet was created
    uint8_t hmac[32];     // HMAC-SHA256 over packet body (hmac field zeroed)

    CloudPacket() { memset(this, 0, sizeof(*this)); }

    bool IsValid() const {
        return strncmp(ident, CLOUD_PROTOCOL_IDENT, 7) == 0;
    }
};
#pragma pack(pop)

// Sign a CloudPacket using HMAC-SHA256 (token as group key derivation input).
// Token goes into group[]; timestamp is set to now; hmac computed over full packet.
// If token is empty, packet is sent unsigned (group = "uvnc").
inline bool SignCloudPacket(CloudPacket& pkt, const std::string& token) {
    if (token.empty()) {
        strncpy_s(pkt.group, "uvnc", sizeof(pkt.group) - 1);
        pkt.timestamp = 0;
        return true;
    }
    strncpy_s(pkt.group, token.c_str(), sizeof(pkt.group) - 1);
    pkt.timestamp = (uint32_t)time(nullptr);

    // Derive group key: HMAC-SHA256(token, group)
    uint8_t groupKey[32] = {};
    {
        BCRYPT_ALG_HANDLE hAlg = nullptr;
        BCRYPT_HASH_HANDLE hHash = nullptr;
        DWORD hashObjSize = 0, cbData = 0;
        uint8_t* hashObj = nullptr;
        bool ok = false;
        if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG) == 0 &&
            BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&hashObjSize, sizeof(DWORD), &cbData, 0) == 0) {
            hashObj = new uint8_t[hashObjSize];
            size_t groupLen = strnlen(pkt.group, 32);
            if (BCryptCreateHash(hAlg, &hHash, hashObj, hashObjSize,
                                 (PBYTE)token.data(), (ULONG)token.size(), 0) == 0 &&
                BCryptHashData(hHash, (PBYTE)pkt.group, (ULONG)groupLen, 0) == 0 &&
                BCryptFinishHash(hHash, groupKey, 32, 0) == 0) ok = true;
        }
        if (hHash) BCryptDestroyHash(hHash);
        if (hAlg)  BCryptCloseAlgorithmProvider(hAlg, 0);
        delete[] hashObj;
        if (!ok) return false;
    }

    // Zero fields filled by server before signing
    uint8_t buf[sizeof(CloudPacket)];
    memcpy(buf, &pkt, sizeof(CloudPacket));
    CloudPacket* tmp = (CloudPacket*)buf;
    memset(tmp->externip, 0, sizeof(tmp->externip));
    tmp->externport = 0;
    memset(tmp->hmac, 0, sizeof(tmp->hmac));

    // HMAC-SHA256(groupKey, buf) -> pkt.hmac
    {
        BCRYPT_ALG_HANDLE hAlg = nullptr;
        BCRYPT_HASH_HANDLE hHash = nullptr;
        DWORD hashObjSize = 0, cbData = 0;
        uint8_t* hashObj = nullptr;
        bool ok = false;
        if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG) == 0 &&
            BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&hashObjSize, sizeof(DWORD), &cbData, 0) == 0) {
            hashObj = new uint8_t[hashObjSize];
            if (BCryptCreateHash(hAlg, &hHash, hashObj, hashObjSize,
                                 groupKey, 32, 0) == 0 &&
                BCryptHashData(hHash, buf, sizeof(CloudPacket), 0) == 0 &&
                BCryptFinishHash(hHash, pkt.hmac, 32, 0) == 0) ok = true;
        }
        if (hHash) BCryptDestroyHash(hHash);
        if (hAlg)  BCryptCloseAlgorithmProvider(hAlg, 0);
        delete[] hashObj;
        if (!ok) return false;
    }
    return true;
}

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
                 CloudStatusCallback statusCb = nullptr,
                 const std::string& token = "");

    // Probe: check if server with this code is online without making a connection.
    // Returns true if online, false if offline or no response within timeoutMs.
    static bool Probe(const std::string& code,
                      const std::string& matchmakerHost = CLOUD_MATCHMAKER_HOST,
                      int timeoutMs = 3000,
                      const std::string& token = "");

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
    std::string token_;
    SOCKET udpSocket_;
    sockaddr_in matchmakerAddr_;
    uint16_t localUdpPort_;

    std::atomic<bool> connected_;
    std::atomic<bool> running_;

    std::thread proxyThread_;
};
