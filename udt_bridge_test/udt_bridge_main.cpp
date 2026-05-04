// UDT Bridge Test - NAT Traversal with UDT Rendezvous + DHT Discovery
// Replaces external rendezvous server with librats DHT

// Must define NOMINMAX before Windows headers to prevent min/max macro conflicts
#ifndef NOMINMAX
#define NOMINMAX
#endif

// Prevent windows.h from including winsock.h (we use winsock2.h)
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

// Undefine Windows min/max macros to prevent conflicts with std::min/std::max
#undef min
#undef max

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <algorithm>

// UDT library for UDP hole punching
#include "../udt4/src/udt.h"

// librats for DHT discovery
#include "../librats/src/librats.h"
#include "../librats/src/logger.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "../udt4/src/udt.lib")

const int BRIDGE_SERVER_PORT = 50000;
const int BRIDGE_CLIENT_PORT = 50001;

// Simple STUN client - queries public STUN server to get our external IP:port
// Returns the UDP socket (kept open for UDT::bind2 - preserves NAT mapping)
struct StunEndpoint { std::string ip; uint16_t port; bool ok; SOCKET udp_sock; };

StunEndpoint stun_get_external(uint16_t local_port) {
    StunEndpoint result = {"", 0, false, INVALID_SOCKET};
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) return result;

    sockaddr_in local = {};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons(local_port);
    bind(sock, (sockaddr*)&local, sizeof(local));

    // Resolve stun.l.google.com
    addrinfo hints = {}, *res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo("stun.l.google.com", "19302", &hints, &res) != 0) {
        closesocket(sock); return result;
    }
    sockaddr_in stun_addr = *(sockaddr_in*)res->ai_addr;
    freeaddrinfo(res);

    // RFC 5389 Binding Request
    unsigned char req[20] = {};
    req[0]=0x00; req[1]=0x01;  // Binding Request
    req[4]=0x21; req[5]=0x12; req[6]=0xA4; req[7]=0x42;  // Magic Cookie
    for (int i = 8; i < 20; i++) req[i] = rand() & 0xFF;

    sendto(sock, (char*)req, 20, 0, (sockaddr*)&stun_addr, sizeof(stun_addr));

    DWORD timeout = 3000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

    unsigned char resp[512];
    int rlen = recv(sock, (char*)resp, sizeof(resp), 0);

    if (rlen < 20 || resp[0] != 0x01 || resp[1] != 0x01) {
        closesocket(sock); return result;
    }

    // Parse XOR-MAPPED-ADDRESS (0x0020)
    int pos = 20;
    while (pos + 4 <= rlen) {
        uint16_t atype = (resp[pos]<<8)|resp[pos+1];
        uint16_t alen  = (resp[pos+2]<<8)|resp[pos+3];
        if (atype == 0x0020 && alen >= 8 && pos+4+alen <= rlen) {
            result.port = ((resp[pos+6]<<8)|resp[pos+7]) ^ 0x2112;
            uint32_t xip = ((resp[pos+8]<<24)|(resp[pos+9]<<16)|(resp[pos+10]<<8)|resp[pos+11]);
            uint32_t ip  = xip ^ 0x2112A442;
            char buf[16];
            sprintf_s(buf, sizeof(buf), "%d.%d.%d.%d",
                (ip>>24)&0xFF,(ip>>16)&0xFF,(ip>>8)&0xFF,ip&0xFF);
            result.ip = buf;
            result.ok = true;
            result.udp_sock = sock;  // Keep socket open - preserves NAT hole
            break;
        }
        pos += 4 + alen;
        if (alen % 4) pos += 4 - (alen % 4);
    }
    if (!result.ok) closesocket(sock);
    return result;
}

// Get local IP of the outbound interface by connecting a UDP socket to 8.8.8.8
std::string get_local_ip() {
    SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == INVALID_SOCKET) return "127.0.0.1";
    sockaddr_in dest = {};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(53);
    inet_pton(AF_INET, "8.8.8.8", &dest.sin_addr);
    if (connect(s, (sockaddr*)&dest, sizeof(dest)) != 0) {
        closesocket(s); return "127.0.0.1";
    }
    sockaddr_in local = {};
    int len = sizeof(local);
    getsockname(s, (sockaddr*)&local, &len);
    closesocket(s);
    char buf[16];
    inet_ntop(AF_INET, &local.sin_addr, buf, sizeof(buf));
    return buf;
}

// Forward declarations
std::string sha1_hex(const std::string& input);

class UdtBridge {
public:
    UdtBridge(const std::string& mode, const std::string& discovery_code, 
              const std::string& vnc_host = "127.0.0.1", int vnc_port = 5900)
        : mode_(mode), discovery_code_(discovery_code), 
          vnc_host_(vnc_host), vnc_port_(vnc_port), running_(true), connected_(false) {
        
        // Generate content hash from discovery code
        content_hash_ = generate_hash(discovery_code);
        
        // Initialize UDT
        UDT::startup();
        
        // Silence logger before construction so not a single line is printed
        librats::Logger::getInstance().set_log_level(static_cast<librats::LogLevel>(99));

        // Initialize librats (DHT port 0 = auto-assign)
        client_ = std::make_unique<librats::RatsClient>(0);
        client_->set_log_level(static_cast<librats::LogLevel>(99));

        // Start the DHT client
        if (!client_->start()) {
            std::cerr << "❌ Failed to start librats DHT client" << std::endl;
            UDT::cleanup();
            return;
        }
        std::cout << "✅ librats DHT client started" << std::endl;
        
        // Start DHT discovery (required for peer finding) - port 0 = auto-assign
        if (!client_->start_dht_discovery(0)) {
            std::cerr << "❌ Failed to start DHT discovery" << std::endl;
        } else {
            std::cout << "✅ DHT discovery started" << std::endl;
        }
        
        // Wait for DHT routing table to populate (needed for successful announce)
        std::cout << "⏳ Waiting for DHT routing table to populate..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));

    }
    
    ~UdtBridge();  // Defined after class where RatsClient is complete
    
    void run_server() {
        std::cout << "\n🌉 UDT Bridge - SERVER mode" << std::endl;
        std::cout << "   Discovery code : " << discovery_code_ << std::endl;

        // ── Step 1: STUN ────────────────────────────────────────────────────
        std::cout << "\n[1/4] STUN discovery..." << std::endl;
        StunEndpoint stun = stun_get_external(BRIDGE_SERVER_PORT);
        std::string my_local_ip  = get_local_ip();
        std::string my_ext_ip    = stun.ok ? stun.ip   : "0.0.0.0";
        uint16_t    my_ext_port  = stun.ok ? stun.port : BRIDGE_SERVER_PORT;
        announce_port_ = my_ext_port;

        std::cout << "   Local  : " << my_local_ip << ":" << BRIDGE_SERVER_PORT << std::endl;
        std::cout << "   External: " << my_ext_ip  << ":" << my_ext_port << std::endl;

        // ── Step 2: DHT announce (runs for the lifetime of the server) ────────
        std::cout << "\n[2/4] Starting DHT announce..." << std::endl;
        std::string client_hash = sha1_hex(discovery_code_ + "_client");

        // Wait for DHT routing table once at startup
        for (int i = 0; i < 30 && running_ && client_->get_dht_routing_table_size() < 5; i++) {
            std::cout << "   DHT nodes: " << client_->get_dht_routing_table_size() << "/5..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }

        client_->announce_for_hash(content_hash_, my_ext_port, [](const std::vector<std::string>&) {});
        std::cout << "   Announced: " << my_ext_ip << ":" << my_ext_port << std::endl;

        // Continuous re-announce thread (runs until program exits)
        std::thread announce_retry([this, my_ext_port]() {
            while (running_ && !dht_stop_) {
                for (int i = 0; i < 50 && running_ && !dht_stop_; i++)
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (running_ && !dht_stop_)
                    client_->announce_for_hash(content_hash_, my_ext_port,
                        [](const std::vector<std::string>&) {});
            }
        });

        // ── Outer loop: accept one client, then wait for the next ──────────
        bool first_session = true;
        while (running_) {
            connected_ = false;
            std::vector<std::string> blacklist;

            // ── Bind a fresh UDT socket for this session ────────────────────
            std::cout << "\n[3/4] Binding UDT socket..." << std::endl;
            UDTSOCKET udt_socket = UDT::socket(AF_INET, SOCK_STREAM, 0);
            if (udt_socket == UDT::INVALID_SOCK) { std::cerr << "❌ UDT socket failed" << std::endl; break; }
            bool rv = true;
            UDT::setsockopt(udt_socket, 0, UDT_RENDEZVOUS, &rv, sizeof(rv));

            // bind2 reuses the STUN UDP socket - only valid for the first session
            bool bound = false;
            if (first_session && stun.ok && stun.udp_sock != INVALID_SOCKET) {
                if (UDT::bind2(udt_socket, stun.udp_sock) != UDT::ERROR) {
                    std::cout << "   bind2 OK (NAT hole preserved)" << std::endl;
                    bound = true;
                }
            }
            if (!bound) {
                sockaddr_in la = {}; la.sin_family = AF_INET;
                la.sin_port = htons(BRIDGE_SERVER_PORT); la.sin_addr.s_addr = INADDR_ANY;
                UDT::bind(udt_socket, (sockaddr*)&la, sizeof(la));
                std::cout << "   bind OK (port " << BRIDGE_SERVER_PORT << ")" << std::endl;
            }
            first_session = false;

            // ── Find next client ────────────────────────────────────────────
            std::cout << "\n[4/4] Searching for client..." << std::endl;
            std::string peer_ext_ip;
            uint16_t    peer_ext_port = 0;

            while (running_ && peer_ext_ip.empty()) {
                client_->find_peers_by_hash(client_hash,
                    [&](const std::vector<std::string>& peers) {
                        for (const auto& p : peers) {
                            size_t c = p.find(':'); if (c == std::string::npos) continue;
                            std::string pip = p.substr(0, c);
                            uint16_t    pp  = (uint16_t)std::stoi(p.substr(c + 1));
                            if (pp < 1024) continue;
                            if (pip == my_ext_ip && (pp < 40000 || pp > 60000)) continue;
                            if (pip == my_ext_ip && pp == my_ext_port) continue;
                            std::string candidate = pip + ":" + std::to_string(pp);
                            bool bad = false;
                            for (const auto& bl : blacklist) if (bl == candidate) { bad = true; break; }
                            if (bad) { std::cout << "   Skipping blacklisted: " << candidate << std::endl; continue; }
                            if (!peer_ext_ip.empty()) continue;
                            peer_ext_ip  = pip;
                            peer_ext_port = pp;
                        }
                    });
                if (peer_ext_ip.empty()) {
                    std::cout << "   Waiting for client..." << std::endl;
                    for (int i = 0; i < 50 && running_ && peer_ext_ip.empty(); i++)
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
            if (!running_) { UDT::close(udt_socket); break; }

            // ── Rendezvous connect (retry on stale entries) ─────────────────
            while (running_ && !connected_) {
                std::string connect_ip   = peer_ext_ip;
                uint16_t    connect_port = peer_ext_port;
                bool        is_lan       = (peer_ext_ip == my_ext_ip);
                if (is_lan) { connect_ip = get_local_ip(); connect_port = BRIDGE_CLIENT_PORT; }

                std::cout << "\n┌─────────────────────────────────────────────┐" << std::endl;
                std::cout << "│  MY  (server) ext : " << my_ext_ip   << ":" << my_ext_port   << std::endl;
                std::cout << "│  MY  (server) loc : " << my_local_ip << ":" << BRIDGE_SERVER_PORT << std::endl;
                std::cout << "│  PEER(client) ext : " << peer_ext_ip << ":" << peer_ext_port << std::endl;
                std::cout << "│  Connecting to    : " << connect_ip  << ":" << connect_port  << (is_lan ? "  [LAN]" : "  [WAN]") << std::endl;
                std::cout << "└─────────────────────────────────────────────┘" << std::endl;

                sockaddr_in peer_addr = {};
                peer_addr.sin_family = AF_INET;
                peer_addr.sin_port   = htons(connect_port);
                inet_pton(AF_INET, connect_ip.c_str(), &peer_addr.sin_addr);

                std::cout << "🔗 Calling UDT::connect (rendezvous)..." << std::endl;
                if (UDT::connect(udt_socket, (sockaddr*)&peer_addr, sizeof(peer_addr)) == 0) {
                    connected_ = true;
                    std::cout << "✅ UDT rendezvous connected!" << std::endl;
                    proxy_data(udt_socket);
                    break;
                }

                std::cerr << "❌ UDT connect failed: " << UDT::getlasterror().getErrorMessage() << std::endl;
                blacklist.push_back(peer_ext_ip + ":" + std::to_string(peer_ext_port));
                std::cout << "   Blacklisted stale peer, searching DHT again..." << std::endl;

                UDT::close(udt_socket);
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                udt_socket = UDT::socket(AF_INET, SOCK_STREAM, 0);
                if (udt_socket == UDT::INVALID_SOCK) { std::cerr << "❌ Socket recreate failed" << std::endl; goto next_session; }
                bool rv2 = true;
                UDT::setsockopt(udt_socket, 0, UDT_RENDEZVOUS, &rv2, sizeof(rv2));
                sockaddr_in la2 = {}; la2.sin_family = AF_INET;
                la2.sin_port = htons(BRIDGE_SERVER_PORT); la2.sin_addr.s_addr = INADDR_ANY;
                UDT::bind(udt_socket, (sockaddr*)&la2, sizeof(la2));

                peer_ext_ip.clear(); peer_ext_port = 0;
                while (running_ && peer_ext_ip.empty()) {
                    client_->find_peers_by_hash(client_hash,
                        [&](const std::vector<std::string>& peers) {
                            for (const auto& p : peers) {
                                size_t c = p.find(':'); if (c == std::string::npos) continue;
                                std::string pip = p.substr(0, c);
                                uint16_t    pp  = (uint16_t)std::stoi(p.substr(c + 1));
                                if (pp < 1024) continue;
                                if (pip == my_ext_ip && (pp < 40000 || pp > 60000)) continue;
                                if (pip == my_ext_ip && pp == my_ext_port) continue;
                                std::string candidate = pip + ":" + std::to_string(pp);
                                bool bad = false;
                                for (const auto& bl : blacklist) if (bl == candidate) { bad = true; break; }
                                if (bad) { std::cout << "   Skipping blacklisted: " << candidate << std::endl; continue; }
                                if (!peer_ext_ip.empty()) continue;
                                peer_ext_ip  = pip;
                                peer_ext_port = pp;
                            }
                        });
                    if (peer_ext_ip.empty()) {
                        std::cout << "   Waiting for client (retry)..." << std::endl;
                        for (int i = 0; i < 50 && running_ && peer_ext_ip.empty(); i++)
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                }
            }
            next_session:
            UDT::close(udt_socket);
            std::cout << "\n🔄 Session ended, waiting for next client..." << std::endl;
        }

        dht_stop_ = true;
        if (announce_retry.joinable()) announce_retry.join();
    }
    
    void run_client() {
        std::cout << "\n🌉 UDT Bridge - CLIENT mode" << std::endl;
        std::cout << "   Discovery code : " << discovery_code_ << std::endl;

        // ── Step 1: STUN ────────────────────────────────────────────────────
        std::cout << "\n[1/4] STUN discovery..." << std::endl;
        StunEndpoint my_stun   = stun_get_external(BRIDGE_CLIENT_PORT);
        std::string my_local_ip = get_local_ip();
        std::string my_ext_ip   = my_stun.ok ? my_stun.ip   : "0.0.0.0";
        uint16_t    my_ext_port = my_stun.ok ? my_stun.port : BRIDGE_CLIENT_PORT;
        client_announce_port_   = my_ext_port;

        std::cout << "   Local  : " << my_local_ip << ":" << BRIDGE_CLIENT_PORT << std::endl;
        std::cout << "   External: " << my_ext_ip << ":" << my_ext_port << std::endl;

        // ── Step 2: Bind UDT (reuse STUN socket to keep NAT hole) ──────────
        std::cout << "\n[2/4] Binding UDT socket..." << std::endl;
        UDTSOCKET udt_socket = UDT::socket(AF_INET, SOCK_STREAM, 0);
        if (udt_socket == UDT::INVALID_SOCK) { std::cerr << "❌ UDT socket failed" << std::endl; return; }
        bool rv = true;
        UDT::setsockopt(udt_socket, 0, UDT_RENDEZVOUS, &rv, sizeof(rv));
        int mss = 1052;
        UDT::setsockopt(udt_socket, 0, UDT_MSS, &mss, sizeof(mss));

        if (my_stun.ok && my_stun.udp_sock != INVALID_SOCKET) {
            if (UDT::ERROR == UDT::bind2(udt_socket, my_stun.udp_sock))
                std::cerr << "⚠️  bind2 failed: " << UDT::getlasterror().getErrorMessage() << std::endl;
            else
                std::cout << "   bind2 OK (NAT hole preserved)" << std::endl;
        } else {
            sockaddr_in la = {}; la.sin_family = AF_INET;
            la.sin_port = htons(BRIDGE_CLIENT_PORT); la.sin_addr.s_addr = INADDR_ANY;
            UDT::bind(udt_socket, (sockaddr*)&la, sizeof(la));
        }

        // ── Step 3: DHT announce + find server ──────────────────────────────
        std::cout << "\n[3/4] DHT: announcing and searching for server..." << std::endl;
        std::string client_hash = sha1_hex(discovery_code_ + "_client");

        // Wait for DHT routing table
        for (int i = 0; i < 30 && running_ && client_->get_dht_routing_table_size() < 5; i++) {
            std::cout << "   DHT nodes: " << client_->get_dht_routing_table_size() << "/5..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }

        // Announce our external endpoint so server can find us
        client_->announce_for_hash(client_hash, my_ext_port, [](const std::vector<std::string>&) {});
        std::cout << "   Announced: " << my_ext_ip << ":" << my_ext_port << std::endl;

        // Re-announce every 15s so server can find us while we search
        std::thread reannounce_thread([this, client_hash, my_ext_port]() {
            while (running_ && !connected_ && !dht_stop_) {
                for (int i = 0; i < 150 && running_ && !connected_ && !dht_stop_; i++)
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (running_ && !connected_ && !dht_stop_)
                    client_->announce_for_hash(client_hash, my_ext_port,
                        [](const std::vector<std::string>&) {});
            }
        });

        // Search for server - retry until found
        std::string peer_ext_ip;
        uint16_t    peer_ext_port = 0;
        std::string my_endpoint   = my_ext_ip + ":" + std::to_string(my_ext_port);

        while (running_ && peer_ext_ip.empty()) {
            client_->find_peers_by_hash(content_hash_,
                [&](const std::vector<std::string>& peers) {
                    for (const auto& p : peers) {
                        size_t c = p.find(':'); if (c == std::string::npos) continue;
                        std::string pip = p.substr(0, c);
                        uint16_t    pp  = (uint16_t)std::stoi(p.substr(c + 1));
                        if (pp < 1024) continue;  // system port
                        if (p == my_endpoint) continue;  // exact self
                        // skip librats service ports on our own IP
                        if (pip == my_ext_ip && (pp < 40000 || pp > 60000)) continue;
                        if (!peer_ext_ip.empty()) continue;  // take first valid
                        peer_ext_ip  = pip;
                        peer_ext_port = pp;
                    }
                });
            if (peer_ext_ip.empty()) {
                std::cout << "   Waiting for server..." << std::endl;
                for (int i = 0; i < 50 && running_ && peer_ext_ip.empty(); i++)
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        if (!running_) { UDT::close(udt_socket); if (reannounce_thread.joinable()) reannounce_thread.join(); return; }

        // ── Step 4: Rendezvous connect loop (retry on stale DHT entries) ───
        std::vector<std::string> blacklist;

        while (running_ && !connected_) {
            std::string connect_ip   = peer_ext_ip;
            uint16_t    connect_port = peer_ext_port;
            bool        is_lan       = (peer_ext_ip == my_ext_ip);
            if (is_lan) {
                connect_ip   = get_local_ip();
                connect_port = BRIDGE_SERVER_PORT;
            }

            std::cout << "\n[4/4] Rendezvous ready!" << std::endl;
            std::cout << "┌─────────────────────────────────────────────┐" << std::endl;
            std::cout << "│  MY  (client) ext : " << my_ext_ip    << ":" << my_ext_port    << std::endl;
            std::cout << "│  MY  (client) loc : " << my_local_ip  << ":" << BRIDGE_CLIENT_PORT << std::endl;
            std::cout << "│  PEER(server) ext : " << peer_ext_ip  << ":" << peer_ext_port  << std::endl;
            std::cout << "│  Connecting to    : " << connect_ip   << ":" << connect_port   << (is_lan ? "  [LAN]" : "  [WAN]") << std::endl;
            std::cout << "└─────────────────────────────────────────────┘" << std::endl;

            sockaddr_in peer_addr = {};
            peer_addr.sin_family  = AF_INET;
            peer_addr.sin_port    = htons(connect_port);
            inet_pton(AF_INET, connect_ip.c_str(), &peer_addr.sin_addr);

            std::cout << "🔗 Calling UDT::connect (rendezvous)..." << std::endl;
            int result = UDT::connect(udt_socket, (sockaddr*)&peer_addr, sizeof(peer_addr));
            if (result == 0) {
                connected_ = true;
                std::cout << "✅ UDT rendezvous connected!" << std::endl;
                proxy_data(udt_socket);
                break;
            }

            std::cerr << "❌ UDT connect failed: " << UDT::getlasterror().getErrorMessage() << std::endl;

            blacklist.push_back(peer_ext_ip + ":" + std::to_string(peer_ext_port));
            std::cout << "   Blacklisted stale peer, searching DHT again..." << std::endl;

            UDT::close(udt_socket);
            std::this_thread::sleep_for(std::chrono::milliseconds(200)); // let UDT worker thread drain
            udt_socket = UDT::socket(AF_INET, SOCK_STREAM, 0);
            if (udt_socket == UDT::INVALID_SOCK) { std::cerr << "❌ Socket recreate failed" << std::endl; break; }
            bool rv2 = true;
            UDT::setsockopt(udt_socket, 0, UDT_RENDEZVOUS, &rv2, sizeof(rv2));
            int mss2 = 1052;
            UDT::setsockopt(udt_socket, 0, UDT_MSS, &mss2, sizeof(mss2));
            sockaddr_in la2 = {}; la2.sin_family = AF_INET;
            la2.sin_port = htons(BRIDGE_CLIENT_PORT); la2.sin_addr.s_addr = INADDR_ANY;
            UDT::bind(udt_socket, (sockaddr*)&la2, sizeof(la2));

            peer_ext_ip.clear(); peer_ext_port = 0;
            while (running_ && peer_ext_ip.empty()) {
                client_->find_peers_by_hash(content_hash_,
                    [&](const std::vector<std::string>& peers) {
                        for (const auto& p : peers) {
                            size_t c = p.find(':'); if (c == std::string::npos) continue;
                            std::string pip = p.substr(0, c);
                            uint16_t    pp  = (uint16_t)std::stoi(p.substr(c + 1));
                            if (pp < 1024) continue;
                            if (p == my_endpoint) continue;
                            if (pip == my_ext_ip && (pp < 40000 || pp > 60000)) continue;
                            std::string candidate = pip + ":" + std::to_string(pp);
                            bool bad = false;
                            for (const auto& bl : blacklist) if (bl == candidate) { bad = true; break; }
                            if (bad) { std::cout << "   Skipping blacklisted: " << candidate << std::endl; continue; }
                            if (!peer_ext_ip.empty()) continue;
                            peer_ext_ip  = pip;
                            peer_ext_port = pp;
                        }
                    });
                if (peer_ext_ip.empty()) {
                    std::cout << "   Waiting for server (retry)..." << std::endl;
                    for (int i = 0; i < 50 && running_ && peer_ext_ip.empty(); i++)
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
        }

        dht_stop_ = true;
        if (reannounce_thread.joinable()) reannounce_thread.join();
        UDT::close(udt_socket);
    }

private:
    void proxy_data(UDTSOCKET udt_socket) {
        std::cout << "\n--- UDT tunnel active ---" << std::endl;

        // 500ms recv timeout so we can check running_ flag
        int recv_timeout = 500;
        UDT::setsockopt(udt_socket, 0, UDT_RCVTIMEO, &recv_timeout, sizeof(recv_timeout));

        bool is_server = (mode_ == "server");

        // Client speaks first
        if (!is_server) {
            const char* msg = "hallo server i'm the client";
            int sent = UDT::send(udt_socket, msg, (int)strlen(msg), 0);
            if (sent > 0)
                std::cout << ">>> Sent    : " << msg << std::endl;
            else
                std::cerr << "Send failed: " << UDT::getlasterror().getErrorMessage() << std::endl;
        }

        char buffer[65536];
        while (running_) {
            int received = UDT::recv(udt_socket, buffer, sizeof(buffer), 0);
            if (received > 0) {
                buffer[received] = '\0';
                std::cout << "<<< Received: " << buffer << std::endl;

                if (is_server) {
                    const char* reply = "i'm the server";
                    UDT::send(udt_socket, reply, (int)strlen(reply), 0);
                    std::cout << ">>> Sent    : " << reply << std::endl;
                }
                break;  // one exchange done - exit
            } else if (received < 0) {
                int err = UDT::getlasterror().getErrorCode();
                if (err == CUDTException::ETIMEOUT) continue;
                std::cerr << "UDT recv error: " << UDT::getlasterror().getErrorMessage() << std::endl;
                break;
            }
        }
        std::cout << "--- UDT tunnel closed ---" << std::endl;
        connected_ = false;
    }
    
    std::string generate_hash(const std::string& code) {
        if (code.empty()) {
            // Auto-generate from machine ID + timestamp
            std::string input = std::to_string(GetTickCount()) + std::to_string(std::hash<std::string>{}("salt"));
            return sha1_hex(input);
        }
        // Hash the discovery code to 40-char SHA-1
        return sha1_hex(code);
    }
    
    // Simple SHA-1 hash to hex string (40 chars for DHT compatibility)
    std::string sha1_hex(const std::string& input) {
        HCRYPTPROV hProv = 0;
        HCRYPTHASH hHash = 0;
        BYTE hash[20];
        DWORD hashLen = 20;
        std::string result;
        
        if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
            if (CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)) {
                if (CryptHashData(hHash, (BYTE*)input.c_str(), input.length(), 0)) {
                    if (CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0)) {
                        // Convert to hex string
                        const char hex[] = "0123456789abcdef";
                        for (int i = 0; i < 20; i++) {
                            result += hex[hash[i] >> 4];
                            result += hex[hash[i] & 0x0F];
                        }
                    }
                }
                CryptDestroyHash(hHash);
            }
            CryptReleaseContext(hProv, 0);
        }
        
        // Fallback if crypto fails
        if (result.empty()) {
            // Simple hash to 40 chars (not real SHA-1 but compatible format)
            std::hash<std::string> hasher;
            size_t h = hasher(input);
            char buf[41];
            snprintf(buf, sizeof(buf), "%016zx%016zx%08zx", h, h ^ 0xABCDEF, h >> 32);
            result = buf;
        }
        
        return result;
    }
    
    std::string mode_;
    std::string discovery_code_;
    std::string content_hash_;
    std::string vnc_host_;
    int vnc_port_;
    std::atomic<bool> running_;
    std::atomic<bool> connected_;
    std::unique_ptr<librats::RatsClient> client_;
    std::atomic<bool> dht_stop_{false};
    uint16_t announce_port_ = 0;        // Our own announced external port (server)
    uint16_t client_announce_port_ = 0; // Our own announced external port (client)
};

// Destructor defined here where RatsClient is complete
UdtBridge::~UdtBridge() {
    running_ = false;
    dht_stop_ = true;
    if (client_) {
        client_->stop();
        // Intentionally leak the RatsClient: librats spawns detached connection threads that
        // hold raw pointers into the object (mutex, stats map). Destroying it while those
        // threads are alive causes a use-after-free crash. The process is exiting anyway so
        // the OS will reclaim all resources cleanly.
        (void)client_.release();
    }
    UDT::cleanup();
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    if (argc < 3) {
        std::cout << "🌉 UDT Bridge - NAT Traversal Test" << std::endl;
        std::cout << "Usage:" << std::endl;
        std::cout << "  Server: udt_bridge server <discovery_code> [vnc_ip] [vnc_port]" << std::endl;
        std::cout << "  Client: udt_bridge client <discovery_code>" << std::endl;
        std::cout << std::endl;
        std::cout << "Uses UDT rendezvous mode + DHT discovery (no external server)" << std::endl;
        return 1;
    }
    
    std::string mode = argv[1];
    std::string discovery_code = argv[2];
    
    std::string vnc_host = "127.0.0.1";
    int vnc_port = 5900;
    
    if (argc >= 4) vnc_host = argv[3];
    if (argc >= 5) vnc_port = std::stoi(argv[4]);
    
    try {
        UdtBridge bridge(mode, discovery_code, vnc_host, vnc_port);
        
        if (mode == "server") {
            bridge.run_server();
        } else if (mode == "client") {
            bridge.run_client();
        } else {
            std::cerr << "❌ Invalid mode. Use 'server' or 'client'" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
