#define _WINSOCKAPI_
#define NOMINMAX
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <udt.h>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "../../CloudConnect/CloudConnectWin/udt4/src/udt.lib")

const int UDT_RENDEZVOUS_PORT = 50000;

// Simple STUN client to get public IP:port
struct StunResult {
    std::string ip;
    uint16_t port;
    bool success;
};

// RFC 5389 STUN binding request
StunResult stun_get_public_endpoint(const char* stun_server, uint16_t stun_port) {
    StunResult result = {"", 0, false};
    
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) return result;
    
    // Bind to any port
    sockaddr_in local_addr = {};
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = 0;
    if (bind(sock, (sockaddr*)&local_addr, sizeof(local_addr)) != 0) {
        closesocket(sock);
        return result;
    }
    
    // Resolve STUN server
    sockaddr_in stun_addr = {};
    stun_addr.sin_family = AF_INET;
    stun_addr.sin_port = htons(stun_port);
    inet_pton(AF_INET, stun_server, &stun_addr.sin_addr);
    
    // Build STUN binding request (RFC 5389)
    // Message Type: Binding Request (0x0001)
    // Message Length: 0 (no attributes)
    // Magic Cookie: 0x2112A442
    // Transaction ID: 12 random bytes
    unsigned char request[20];
    request[0] = 0x00; request[1] = 0x01;  // Binding Request
    request[2] = 0x00; request[3] = 0x00;  // Length = 0
    request[4] = 0x21; request[5] = 0x12;  // Magic Cookie
    request[6] = 0xA4; request[7] = 0x42;
    for (int i = 8; i < 20; i++) request[i] = rand() & 0xFF;
    
    // Send STUN request
    if (sendto(sock, (char*)request, 20, 0, (sockaddr*)&stun_addr, sizeof(stun_addr)) != 20) {
        closesocket(sock);
        return result;
    }
    
    // Receive response with timeout
    DWORD timeout = 3000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    
    unsigned char response[512];
    sockaddr_in from_addr;
    int from_len = sizeof(from_addr);
    int recv_len = recvfrom(sock, (char*)response, sizeof(response), 0, (sockaddr*)&from_addr, &from_len);
    
    if (recv_len < 20 || response[0] != 0x01 || response[1] != 0x01) {
        closesocket(sock);
        return result;
    }
    
    // Parse XOR-MAPPED-ADDRESS attribute (0x0020)
    int pos = 20;
    while (pos + 4 <= recv_len) {
        uint16_t attr_type = (response[pos] << 8) | response[pos + 1];
        uint16_t attr_len = (response[pos + 2] << 8) | response[pos + 3];
        
        if (attr_type == 0x0020 && attr_len >= 4 && pos + 4 + attr_len <= recv_len) {
            // XOR-MAPPED-ADDRESS
            uint8_t family = response[pos + 5];
            if (family == 0x01) { // IPv4
                uint16_t xored_port = (response[pos + 6] << 8) | response[pos + 7];
                result.port = xored_port ^ 0x2112;
                
                uint32_t xored_ip = ((response[pos + 8] << 24) | (response[pos + 9] << 16) |
                                   (response[pos + 10] << 8) | response[pos + 11]);
                uint32_t ip = xored_ip ^ 0x2112A442;
                
                char ip_str[16];
                sprintf_s(ip_str, sizeof(ip_str), "%d.%d.%d.%d",
                       (ip >> 24) & 0xFF, (ip >> 16) & 0xFF,
                       (ip >> 8) & 0xFF, ip & 0xFF);
                result.ip = ip_str;
                result.success = true;
            }
            break;
        }
        pos += 4 + attr_len;
        // Align to 4 bytes
        if (attr_len % 4) pos += 4 - (attr_len % 4);
    }
    
    closesocket(sock);
    return result;
}

// Generate short code from endpoint
std::string endpoint_to_code(const std::string& ip, uint16_t port) {
    // Simple hash for display (not cryptographic)
    std::string input = ip + ":" + std::to_string(port);
    uint32_t hash = 0;
    for (char c : input) {
        hash = hash * 31 + c;
    }
    
    // Format as XXX-XXXX (easy to type)
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');
    ss << std::setw(3) << (hash & 0xFFF) << "-";
    ss << std::setw(4) << ((hash >> 12) & 0xFFFF);
    return ss.str();
}

// Parse code back to endpoint (stored locally)
struct EndpointInfo {
    std::string public_ip;
    uint16_t public_port;
    std::string local_ip;
    uint16_t local_port;
};

void run_server(const std::string& discovery_code) {
    std::cout << "🌉 UDT Bridge Server Mode" << std::endl;
    
    // Step 1: Get public endpoint via STUN
    std::cout << "🔍 Discovering public endpoint via STUN..." << std::endl;
    StunResult stun = stun_get_public_endpoint("stun.l.google.com", 19302);
    
    if (!stun.success) {
        std::cerr << "❌ STUN failed" << std::endl;
        return;
    }
    
    std::cout << "✅ Public endpoint: " << stun.ip << ":" << stun.port << std::endl;
    
    // Step 2: Create UDT socket in rendezvous mode
    UDTSOCKET udt_socket = UDT::socket(AF_INET, SOCK_STREAM, 0);
    
    bool rendezvous = true;
    UDT::setsockopt(udt_socket, 0, UDT_RENDEZVOUS, &rendezvous, sizeof(rendezvous));
    
    // Bind to local port 50000
    sockaddr_in local_addr = {};
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(UDT_RENDEZVOUS_PORT);
    local_addr.sin_addr.s_addr = INADDR_ANY;
    
    if (UDT::bind(udt_socket, (sockaddr*)&local_addr, sizeof(local_addr)) != 0) {
        std::cerr << "❌ UDT bind failed: " << UDT::getlasterror().getErrorMessage() << std::endl;
        UDT::close(udt_socket);
        return;
    }
    
    std::cout << "✅ UDT bound to port " << UDT_RENDEZVOUS_PORT << std::endl;
    
    // Step 3: Display connection info
    std::cout << "\n═══════════════════════════════════════════════════" << std::endl;
    std::cout << "📢 SHARE THIS WITH VIEWER:" << std::endl;
    std::cout << "   Public IP:   " << stun.ip << std::endl;
    std::cout << "   Public Port: " << stun.port << std::endl;
    std::cout << "═══════════════════════════════════════════════════" << std::endl;
    
    // Step 4: Wait for UDP hole punch (viewer must connect simultaneously)
    std::cout << "\n⏳ Waiting for viewer to connect..." << std::endl;
    std::cout << "   (Both sides must send UDP packets simultaneously)" << std::endl;
    
    // Start a thread to listen for UDP packets (hole punch detection)
    std::atomic<bool> hole_punched{false};
    std::string peer_ip;
    uint16_t peer_port = 0;
    
    std::thread punch_listener([&]() {
        SOCKET udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (udp_sock == INVALID_SOCKET) return;
        
        // Bind to same port as UDT for hole punching
        sockaddr_in punch_addr = {};
        punch_addr.sin_family = AF_INET;
        punch_addr.sin_port = htons(UDT_RENDEZVOUS_PORT);
        punch_addr.sin_addr.s_addr = INADDR_ANY;
        bind(udp_sock, (sockaddr*)&punch_addr, sizeof(punch_addr));
        
        char buf[1024];
        sockaddr_in from;
        int fromlen = sizeof(from);
        
        while (!hole_punched) {
            int len = recvfrom(udp_sock, buf, sizeof(buf), 0, (sockaddr*)&from, &fromlen);
            if (len > 0) {
                char ip_str[16];
                inet_ntop(AF_INET, &from.sin_addr, ip_str, sizeof(ip_str));
                peer_ip = ip_str;
                peer_port = ntohs(from.sin_port);
                hole_punched = true;
                std::cout << "\n🔨 Hole punch detected from " << peer_ip << ":" << peer_port << std::endl;
            }
        }
        closesocket(udp_sock);
    });
    
    // Keep server running
    std::cout << "\nPress Enter to exit..." << std::endl;
    std::cin.get();
    
    hole_punched = true;
    punch_listener.join();
    UDT::close(udt_socket);
}

void run_client(const std::string& server_ip, uint16_t server_port) {
    std::cout << "🌉 UDT Bridge Client Mode" << std::endl;
    
    // Step 1: Get our public endpoint via STUN
    std::cout << "🔍 Discovering our public endpoint via STUN..." << std::endl;
    StunResult stun = stun_get_public_endpoint("stun.l.google.com", 19302);
    
    if (!stun.success) {
        std::cerr << "❌ STUN failed" << std::endl;
        return;
    }
    
    std::cout << "✅ Our public endpoint: " << stun.ip << ":" << stun.port << std::endl;
    
    // Step 2: Create UDT socket in rendezvous mode
    UDTSOCKET udt_socket = UDT::socket(AF_INET, SOCK_STREAM, 0);
    
    bool rendezvous = true;
    UDT::setsockopt(udt_socket, 0, UDT_RENDEZVOUS, &rendezvous, sizeof(rendezvous));
    
    // Bind to local port 50000 (same as server for hole punching)
    sockaddr_in local_addr = {};
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(UDT_RENDEZVOUS_PORT);
    local_addr.sin_addr.s_addr = INADDR_ANY;
    
    if (UDT::bind(udt_socket, (sockaddr*)&local_addr, sizeof(local_addr)) != 0) {
        std::cerr << "❌ UDT bind failed: " << UDT::getlasterror().getErrorMessage() << std::endl;
        UDT::close(udt_socket);
        return;
    }
    
    std::cout << "✅ UDT bound to port " << UDT_RENDEZVOUS_PORT << std::endl;
    
    // Step 3: UDP hole punch
    std::cout << "\n🔨 Performing UDP hole punch to " << server_ip << ":" << server_port << "..." << std::endl;
    
    SOCKET udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_sock == INVALID_SOCKET) {
        std::cerr << "❌ Failed to create UDP socket" << std::endl;
        return;
    }
    
    // Send multiple UDP packets to punch hole
    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);
    
    const char* punch_msg = "PUNCH";
    for (int i = 0; i < 10; i++) {
        sendto(udp_sock, punch_msg, strlen(punch_msg), 0, (sockaddr*)&server_addr, sizeof(server_addr));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    closesocket(udp_sock);
    std::cout << "✅ Hole punch packets sent" << std::endl;
    
    // Step 4: Wait a moment for hole to establish, then UDT connect
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::cout << "🔗 Attempting UDT rendezvous connection..." << std::endl;
    
    // In rendezvous mode, we call connect() which will use the punched hole
    int result = UDT::connect(udt_socket, (sockaddr*)&server_addr, sizeof(server_addr));
    
    if (result == UDT::ERROR) {
        std::cerr << "❌ UDT connect failed: " << UDT::getlasterror().getErrorMessage() << std::endl;
    } else {
        std::cout << "✅ Connected!" << std::endl;
        
        // Send test data
        const char* test = "Hello from client!";
        UDT::send(udt_socket, test, strlen(test), 0);
        
        char buf[1024];
        int len = UDT::recv(udt_socket, buf, sizeof(buf), 0);
        if (len > 0) {
            buf[len] = '\0';
            std::cout << "📨 Received: " << buf << std::endl;
        }
    }
    
    UDT::close(udt_socket);
}

int main(int argc, char* argv[]) {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    UDT::startup();
    
    if (argc < 2) {
        std::cout << "🌉 UDT STUN Bridge - UDP Hole Punching\n\n";
        std::cout << "Usage:\n";
        std::cout << "  Server: udt_stun_bridge server [discovery_code]\n";
        std::cout << "  Client: udt_stun_bridge client <server_ip> <server_port>\n\n";
        std::cout << "Example:\n";
        std::cout << "  Server: udt_stun_bridge server\n";
        std::cout << "          (displays public IP:port to share)\n";
        std::cout << "  Client: udt_stun_bridge client 81.243.193.59 56918\n";
        WSACleanup();
        return 1;
    }
    
    std::string mode = argv[1];
    
    try {
        if (mode == "server") {
            std::string code = (argc >= 3) ? argv[2] : "";
            run_server(code);
        } else if (mode == "client") {
            if (argc < 4) {
                std::cerr << "❌ Client requires server IP and port" << std::endl;
                WSACleanup();
                return 1;
            }
            run_client(argv[2], (uint16_t)atoi(argv[3]));
        } else {
            std::cerr << "❌ Unknown mode: " << mode << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
    }
    
    UDT::cleanup();
    WSACleanup();
    return 0;
}
