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
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")

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
    uint8_t _pad0;
    uint8_t _pad1;
    uint8_t _pad2;
    uint32_t timestamp;   // unix time (UTC) when packet was created
    uint8_t hmac[32];     // HMAC-SHA256 over packet body (hmac field zeroed)

    CloudServerPacket() { memset(this, 0, sizeof(*this)); }

    bool IsValid() const {
        return strncmp(ident, CLOUD_SERVER_PROTOCOL_IDENT, 7) == 0;
    }
};
#pragma pack(pop)

// Sign a CloudServerPacket using HMAC-SHA256 (token as group key derivation input).
// If token is empty, group = "uvnc" and packet is sent unsigned.
inline bool SignCloudServerPacket(CloudServerPacket& pkt, const std::string& token) {
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
    uint8_t buf[sizeof(CloudServerPacket)];
    memcpy(buf, &pkt, sizeof(CloudServerPacket));
    CloudServerPacket* tmp = (CloudServerPacket*)buf;
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
                BCryptHashData(hHash, buf, sizeof(CloudServerPacket), 0) == 0 &&
                BCryptFinishHash(hHash, pkt.hmac, 32, 0) == 0) ok = true;
        }
        if (hHash) BCryptDestroyHash(hHash);
        if (hAlg)  BCryptCloseAlgorithmProvider(hAlg, 0);
        delete[] hashObj;
        if (!ok) return false;
    }
    return true;
}

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
                     const std::string& matchmakerHost = CLOUD_SERVER_MATCHMAKER_HOST,
                     const std::string& token = "");
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
    std::string token_;
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
