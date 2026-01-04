#include "vnc_bridge.h"
#include <random>
#include <ctime>
#include <thread>
#include <chrono>
#include <iostream>
#include <sstream>
#include <vector>
#include <cstdint>
#include <mutex>
#include "../librats/src/json.hpp"
#include "../librats/src/socket.h"

// Server-side logging via vnclog (not for viewer or bridge app)
#ifndef _VIEWER
#ifndef _VNC_BRIDGE_APP
#include "../winvnc/winvnc/vnclog.h"
extern VNCLog vnclog;
#define VNCLOG(s) (__FUNCTION__ " : " s)
#endif
#endif

// Static member initialization - console logging disabled by default
bool VncBridge::console_logging_enabled_ = false;

// Logging macro - outputs to console (bridge app) or vnclog (server) or silent (viewer)
#ifdef _VNC_BRIDGE_APP
// Bridge app: use std::cout when enabled
#define BRIDGE_LOG(x) do { if (VncBridge::is_console_logging_enabled()) { std::cout << x; } } while(0)
#define BRIDGE_LOG_LN(x) do { if (VncBridge::is_console_logging_enabled()) { std::cout << x << std::endl; } } while(0)
#elif !defined(_VIEWER)
// Server: use vnclog
#define BRIDGE_LOG(x) do { std::ostringstream _oss; _oss << x; vnclog.Print(0, VNCLOG("%s"), _oss.str().c_str()); } while(0)
#define BRIDGE_LOG_LN(x) do { std::ostringstream _oss; _oss << x; vnclog.Print(0, VNCLOG("%s\n"), _oss.str().c_str()); } while(0)
#else
// Viewer: silent
#define BRIDGE_LOG(x) do { } while(0)
#define BRIDGE_LOG_LN(x) do { } while(0)
#endif

#ifdef _WIN32
#include <iphlpapi.h>
#include <intrin.h>  // For __cpuid
#include <windows.h> // For GetVolumeInformation
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#else
#include <ifaddrs.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netpacket/packet.h>
#include <errno.h>
#endif

// Static helper methods implementation
librats::NatTraversalConfig VncBridge::create_config() {
    librats::NatTraversalConfig config;
    config.enable_ice = true;
    config.enable_hole_punching = true;
    config.enable_turn_relay = false;
    config.stun_servers = { "stun.l.google.com:19302" };
    config.ice_connectivity_timeout_ms = 10000;
    return config;
}

// Get machine-specific identifier (MAC address + CPU ID + Disk Serial)
std::string VncBridge::get_machine_id() {
    std::string machine_id;
    
#ifdef _WIN32
    // Get MAC address on Windows
    IP_ADAPTER_INFO adapter_info[16];
    DWORD buf_len = sizeof(adapter_info);
    DWORD status = GetAdaptersInfo(adapter_info, &buf_len);
    
    if (status == ERROR_SUCCESS) {
        PIP_ADAPTER_INFO adapter = adapter_info;
        if (adapter) {
            for (int i = 0; i < 6; i++) {
                char hex[3];
                sprintf_s(hex, "%02X", adapter->Address[i]);
                machine_id += hex;
            }
        }
    }
    
    // Get CPU ID using CPUID instruction
    int cpu_info[4] = {0};
    __cpuid(cpu_info, 1);
    char cpu_id[32];
    sprintf_s(cpu_id, "%08X%08X", cpu_info[3], cpu_info[0]); // EDX + EAX
    machine_id += cpu_id;
    
    // Get disk serial number from C: drive
    char volume_name[MAX_PATH];
    char file_system[MAX_PATH];
    DWORD serial_number = 0;
    DWORD max_component_len = 0;
    DWORD file_system_flags = 0;
    
    if (GetVolumeInformation(
        "C:\\",
        volume_name, MAX_PATH,
        &serial_number,
        &max_component_len,
        &file_system_flags,
        file_system, MAX_PATH)) {
        char disk_serial[16];
        sprintf_s(disk_serial, "%08X", serial_number);
        machine_id += disk_serial;
    }
#else
    // Get MAC address on Linux/Unix
    struct ifaddrs *ifap, *ifaptr;
    if (getifaddrs(&ifap) == 0) {
        for (ifaptr = ifap; ifaptr != nullptr; ifaptr = ifaptr->ifa_next) {
            if (ifaptr->ifa_addr && ifaptr->ifa_addr->sa_family == AF_PACKET) {
                struct sockaddr_ll *s = (struct sockaddr_ll*)ifaptr->ifa_addr;
                for (int i = 0; i < 6; i++) {
                    char hex[3];
                    sprintf(hex, "%02X", s->sll_addr[i]);
                    machine_id += hex;
                }
                break;
            }
        }
        freeifaddrs(ifap);
    }
#endif
    
    // Fallback if MAC address not available
    if (machine_id.empty()) {
        machine_id = "FALLBACK123456";
    }
    
    return machine_id;
}

// Calculate 2-digit checksum
std::string VncBridge::calculate_checksum(const std::string& base_code) {
    uint32_t sum = 0;
    for (char c : base_code) {
        sum += static_cast<uint32_t>(c);
    }
    
    // Create 2-digit checksum (00-99)
    uint32_t checksum = sum % 100;
    char result[3];
    sprintf_s(result, "%02d", checksum);
    return std::string(result);
}

// Simple architecture-independent hash function
// Uses a custom algorithm that produces the same result on 32-bit and 64-bit
static uint64_t portable_hash(const std::string& str) {
    uint64_t hash = 0x123456789ABCDEF0ULL;  // Fixed seed
    const uint64_t prime = 0x100000001B3ULL;  // FNV prime
    
    for (size_t i = 0; i < str.length(); i++) {
        hash ^= static_cast<uint64_t>(str[i]);
        hash *= prime;
    }
    
    return hash;
}

// Generate deterministic code: 12 digits total (3+4+3+2 format, last 2 = checksum)
// Always generates the same code for the same hardware (CPU + MAC + Disk)
// Architecture-independent: produces same code on Win32 and x64
std::string VncBridge::generate_unique_code() {
    std::string machine_id = get_machine_id();
    
    // Create deterministic hash from machine hardware identifiers
    // Using custom hash that works the same on 32-bit and 64-bit
    uint64_t hardware_hash = portable_hash(machine_id);
    
    // Generate base code: 10 digits (3+4+3) from hardware hash
    std::string base_code;
    uint64_t hash_working = hardware_hash;
    
    // Extract 10 digits from hash in a deterministic way
    for (int i = 0; i < 10; i++) {
        base_code += std::to_string(hash_working % 10);
        hash_working /= 10;
        // If we run out of digits, rehash to get more
        if (hash_working == 0 && i < 9) {
            hash_working = portable_hash(machine_id + std::to_string(i));
        }
    }
    
    // Calculate checksum from base code only (for consistent validation)
    std::string checksum = calculate_checksum(base_code);
    
    // Return 12 digits total: base_code (10) + checksum (2)
    return base_code + checksum;
}

// Normalize discovery code by removing all non-digits
std::string VncBridge::normalize_discovery_code(const std::string& code) {
    std::string digits_only;
    
    // Remove all non-digits (spaces, dashes, etc.)
    for (char c : code) {
        if (std::isdigit(c)) {
            digits_only += c;
        }
    }
    
    return digits_only;
}

// Validate discovery code format and checksum
bool VncBridge::validate_discovery_code(const std::string& code) {
    // Normalize the code first (extract digits only)
    std::string digits_only = normalize_discovery_code(code);
    
    // Check if we have exactly 12 digits (10 code + 2 checksum)
    if (digits_only.length() != 12) return false;
    
    // Check all characters are digits
    for (char c : digits_only) {
        if (!std::isdigit(c)) return false;
    }
    
    // Extract base code (first 10 digits) and checksum (last 2 digits)
    std::string base_code = digits_only.substr(0, 10);
    std::string provided_checksum = digits_only.substr(10, 2);
    
    // Calculate expected checksum from base code only
    std::string expected_checksum = calculate_checksum(base_code);
    
    return provided_checksum == expected_checksum;
}

// Get detailed validation error message
std::string VncBridge::get_validation_error(const std::string& code) {
    if (code.empty()) {
        return "Code is empty";
    }
    
    // Normalize the code first (extract digits only)
    std::string digits_only = normalize_discovery_code(code);
    
    if (digits_only.length() != 12) {
        return "Invalid length. Expected\n"
               "Format: XXX-XXXX-XXX-##\n"
               "Format: XXX XXXX XXX ##\n"
               "Format: XXXXXXXXXX##\n"
               "Example: 123-1234-123-12";
    }
    
    // Check for non-digit characters
    for (size_t i = 0; i < digits_only.length(); i++) {
        if (!std::isdigit(digits_only[i])) {
            return "Invalid character '" + std::string(1, digits_only[i]) + "' at position " + std::to_string(i + 1) + 
                   ". Only digits allowed";
        }
    }
    
    // Check checksum
    std::string base_code = digits_only.substr(0, 10);
    std::string provided_checksum = digits_only.substr(10, 2);
    std::string expected_checksum = calculate_checksum(base_code);
    
    if (provided_checksum != expected_checksum) {
        return "Invalid checksum.";
    }
    
    return "Code has a valid format";
}

// Constructor implementation - simplified with fixed ports
VncBridge::VncBridge(const std::string& mode, const std::string& discovery_code,
                     const std::string& vnc_ip, int vnc_port)
    : // Use fixed ports based on mode
      client_(mode == "server" ? BRIDGE_SERVER_PORT : BRIDGE_CLIENT_PORT, 5, create_config()),
      mode_(mode),
      discovery_code_(discovery_code.empty() ? generate_unique_code() : normalize_discovery_code(discovery_code)),
      connected_(false), 
      tcp_listen_port_(mode == "client" ? BRIDGE_CLIENT_PORT : 0), 
      vnc_server_ip_(vnc_ip), 
      vnc_server_port_(vnc_port), 
      running_(true) {
    
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    
    setup_callbacks();
}

// Destructor implementation
VncBridge::~VncBridge() {
    running_ = false;
    
    // Close all TCP connections
    std::lock_guard<std::mutex> lock(connections_mutex_);
    for (int sock : tcp_connections_) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
    }
    tcp_connections_.clear();
    
#ifdef _WIN32
    WSACleanup();
#endif
}

// VNC Bridge implementation - Core functionality complete

void VncBridge::stop() {
    running_ = false;
    connected_ = false;
    
    // Stop librats client - this blocks until all internal threads are joined
    // This is necessary to prevent use-after-free when VncBridge is destroyed
    client_.stop();
}

std::string VncBridge::get_status() const {
    std::ostringstream status;
    status << "📊 VNC BRIDGE STATUS:\n";
    status << "🌉 Mode: " << mode_ << "\n";
    status << "🔑 Discovery code: " << discovery_code_ << "\n";
    status << "🔌 UDP port: " << BRIDGE_SERVER_PORT << "\n";
    status << "📡 Connected: " << (connected_ ? "Yes" : "No") << "\n";
    if (mode_ == "client") {
        status << "🔌 TCP listen port: " << BRIDGE_CLIENT_PORT << "\n";
    } else {
        status << "🎯 VNC server: " << vnc_server_ip_ << ":" << vnc_server_port_ << "\n";
    }
    return status.str();
}

// Placeholder implementations - need to copy from original vnc_bridge.cpp
void VncBridge::setup_callbacks() {
    // Connection callback
    client_.set_connection_callback([this](socket_t socket, const std::string& peer_id) {
        if (mode_ == "client") {
            // Client mode: single server connection
            connected_ = true;
            connected_peer_id_ = peer_id;
        } else {
            // Server mode: track multiple bridge clients
            std::lock_guard<std::mutex> lock(peers_mutex_);
            connected_peers_.insert(peer_id);
            connected_ = true;
        }
        
        BRIDGE_LOG_LN("\n🎉 UDP TUNNEL ESTABLISHED!");
        BRIDGE_LOG_LN("✅ Connected to bridge peer: " << peer_id.substr(0, 16) << "...");
        BRIDGE_LOG_LN("🔌 Socket: " << socket);
        
        if (mode_ == "client") {
            // Start keepalive thread (only needed for client mode)
            std::thread([this]() {
                while (running_ && connected_) {
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    if (connected_ && !connected_peer_id_.empty()) {
                        nlohmann::json ping;
                        ping["type"] = "keepalive";
                        ping["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch()).count();
                        client_.send("vnc_keepalive", ping);
                    }
                }
            }).detach();
        }
    });
    
    // Handle tunneled VNC data - BINARY PROTOCOL (much faster!)
    client_.set_binary_data_callback([this](socket_t socket, const std::string& peer_id, const std::vector<uint8_t>& data) {
        // First byte is connection_id, rest is VNC data
        if (data.size() >= 1) {
            int connection_id = data[0];
            // Forward data (skip first byte)
            forward_to_tcp_binary(data.data() + 1, data.size() - 1, connection_id);
        }
    });
    
    // Handle connection events
    client_.on("vnc_connect", [this](const std::string& peer_id, const nlohmann::json& data) {
        int connection_id = data["connection_id"];
        BRIDGE_LOG_LN("🔗 New VNC connection request from " << peer_id.substr(0, 8) << "... (ID: " << connection_id << ")");
        
        if (mode_ == "server") {
            // Map this connection_id to the peer that requested it
            {
                std::lock_guard<std::mutex> lock(peers_mutex_);
                connection_to_peer_[connection_id] = peer_id;
            }
            // Connect to actual VNC server
            connect_to_vnc_server(connection_id);
        }
    });
    
    client_.on("vnc_disconnect", [this](const std::string& peer_id, const nlohmann::json& data) {
        int connection_id = data["connection_id"];
        BRIDGE_LOG_LN("❌ VNC connection closed (ID: " << connection_id << ")");
        
        // Remove connection mapping
        {
            std::lock_guard<std::mutex> lock(peers_mutex_);
            connection_to_peer_.erase(connection_id);
        }
        
        // Close corresponding TCP connection
        close_tcp_connection(connection_id);
    });
    
    // Handle keepalive messages (just ignore them - they keep the connection alive)
    client_.on("vnc_keepalive", [this](const std::string& peer_id, const nlohmann::json& data) {
        // Keepalive received - connection is alive, nothing to do
    });
    
    // Disconnection callback
    client_.set_disconnect_callback([this](socket_t socket, const std::string& peer_id) {
        if (mode_ == "client") {
            // Client mode: single connection lost
            connected_ = false;
            connected_peer_id_.clear();
        } else {
            // Server mode: remove this peer from tracking
            std::lock_guard<std::mutex> lock(peers_mutex_);
            connected_peers_.erase(peer_id);
            
            // Close all connections from this peer
            std::vector<int> to_remove;
            for (const auto& pair : connection_to_peer_) {
                if (pair.second == peer_id) {
                    to_remove.push_back(pair.first);
                }
            }
            for (int conn_id : to_remove) {
                connection_to_peer_.erase(conn_id);
                close_tcp_connection(conn_id);
            }
            
            connected_ = !connected_peers_.empty();
        }
        
        BRIDGE_LOG_LN("❌ UDP tunnel disconnected: " << peer_id.substr(0, 16) << "...");
    });
}

bool VncBridge::start() {
    BRIDGE_LOG_LN("🚀 Starting VNC bridge in " << mode_ << " mode...");
    
    // Reduce librats logging to only show warnings and errors (for performance)
    // librats::Logger::getInstance().set_log_level(librats::LogLevel::WARN);
    
    if (!client_.start()) {
        BRIDGE_LOG_LN("❌ Failed to start librats client");
        return false;
    }
    
    BRIDGE_LOG_LN("✅ librats client started successfully!");
    return true;
}

void VncBridge::run_client_mode() {
    BRIDGE_LOG_LN("\n🌉 VNC BRIDGE CLIENT - Embedded mode");
    BRIDGE_LOG_LN("📋 VNC viewers connect to: localhost:" << tcp_listen_port_);
    BRIDGE_LOG_LN("🔑 Discovery code: " << discovery_code_);
    
    // Start the librats client first
    if (!start()) {
        BRIDGE_LOG_LN("❌ Failed to start bridge client");
        return;
    }
    
    // Wait for initialization
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Connect to bridge server using discovery code
    BRIDGE_LOG_LN("🎯 Attempting UDP tunnel connection...");
    bool success = client_.connect_to_peer("127.0.0.1", BRIDGE_SERVER_PORT, 
                                          librats::ConnectionStrategy::AUTO_ADAPTIVE);
    
    if (success) {
        BRIDGE_LOG_LN("✅ UDP tunnel connection initiated");
        
        // Wait for UDP connection
        for (int i = 0; i < 30 && !connected_; i++) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (i % 5 == 4) {
                BRIDGE_LOG_LN("⏳ Waiting for UDP tunnel... (" << (i + 1) << "/30)");
            }
        }
        
        if (connected_) {
            BRIDGE_LOG_LN("\n🎉 UDP tunnel established!");
            
            // Start TCP server for VNC viewers
            int server_socket = create_tcp_server(tcp_listen_port_);
            if (server_socket >= 0) {
                BRIDGE_LOG_LN("\n💡 Connect your VNC viewer to: localhost:" << tcp_listen_port_);
                BRIDGE_LOG_LN("🔄 Bridge is ready - forwarding VNC traffic through UDP tunnel");
                
                // Accept VNC viewer connections
                int connection_counter = 0;
                while (running_ && connected_) {
                    sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    
#ifdef _WIN32
                    SOCKET client_socket = accept(server_socket, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
                    if (client_socket != INVALID_SOCKET) {
#else
                    int client_socket = accept(server_socket, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
                    if (client_socket >= 0) {
#endif
                        // Enable TCP_NODELAY on accepted socket
                        int opt = 1;
                        setsockopt(client_socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&opt), sizeof(opt));
                        
                        // Set receive timeout so recv() doesn't block forever
#ifdef _WIN32
                        DWORD timeout_ms = 1000;  // 1 second timeout
                        setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout_ms), sizeof(timeout_ms));
#else
                        struct timeval tv;
                        tv.tv_sec = 1;  // 1 second timeout
                        tv.tv_usec = 0;
                        setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
                        
                        BRIDGE_LOG_LN("🎯 VNC viewer connected (ID: " << connection_counter << ")");
                        
                        // Store connection
                        {
                            std::lock_guard<std::mutex> lock(connections_mutex_);
                            if (connection_counter >= tcp_connections_.size()) {
                                tcp_connections_.resize(connection_counter + 1, -1);
                            }
                            tcp_connections_[connection_counter] = static_cast<int>(client_socket);
                        }
                        
                        // Notify bridge server
                        notify_connection(connection_counter);
                        
                        // Start forwarding thread for this connection
                        std::thread([this, client_socket, connection_counter]() {
                            handle_tcp_to_udp(static_cast<int>(client_socket), connection_counter);
                        }).detach();
                        
                        connection_counter++;
                    } else {
                        // Accept failed, wait a bit before trying again
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                }
                
#ifdef _WIN32
                closesocket(server_socket);
#else
                close(server_socket);
#endif
            }
        } else {
            BRIDGE_LOG_LN("❌ UDP tunnel connection failed after 30 seconds");
        }
    } else {
        BRIDGE_LOG_LN("❌ Failed to initiate UDP tunnel connection");
    }
    
    BRIDGE_LOG_LN("🛑 Bridge client mode stopped");
}

void VncBridge::run_server_mode() {
    BRIDGE_LOG_LN("\n🌉 VNC BRIDGE SERVER - Embedded mode");
    BRIDGE_LOG_LN("🎯 Will connect to VNC server: " << vnc_server_ip_ << ":" << vnc_server_port_);
    BRIDGE_LOG_LN("🔑 Discovery code: " << discovery_code_);
    
    // Start the librats client first
    if (!start()) {
        BRIDGE_LOG_LN("❌ Failed to start bridge server");
        return;
    }
    
    // Wait for initialization
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    BRIDGE_LOG_LN("\n💡 Client command: vnc_bridge client \"" << discovery_code_ << "\"");
    BRIDGE_LOG_LN("📢 Server is listening and ready for connections");
    BRIDGE_LOG_LN("⏳ Waiting for bridge client connections...");
    
    // Keep running and handle connections - check running_ frequently for responsive shutdown
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    BRIDGE_LOG_LN("🛑 Bridge server mode stopped");
}

void VncBridge::handle_tcp_to_udp(int tcp_socket, int connection_id) {
    std::vector<uint8_t> buffer(65536);  // 64KB buffer for better throughput
    
    while (running_) {
#ifdef _WIN32
        int received = recv(tcp_socket, reinterpret_cast<char*>(buffer.data()), buffer.size(), 0);
#else
        int received = recv(tcp_socket, buffer.data(), buffer.size(), 0);
#endif
        if (received > 0) {
            // Forward data from TCP to UDP tunnel
            forward_to_udp(reinterpret_cast<const char*>(buffer.data()), received, connection_id);
        } else if (received == 0) {
            BRIDGE_LOG_LN("📊 TCP connection " << connection_id << " closed by peer");
            break;
        } else {
            // Check if it's a timeout (not a real error)
#ifdef _WIN32
            int error = WSAGetLastError();
            if (error == WSAETIMEDOUT || error == WSAEWOULDBLOCK) {
                // Timeout - just continue to check running_ flag
                continue;
            }
#else
            int error = errno;
            if (error == EAGAIN || error == EWOULDBLOCK) {
                // Timeout - just continue to check running_ flag
                continue;
            }
#endif
            BRIDGE_LOG_LN("❌ TCP receive error on connection " << connection_id);
            break;
        }
    }
    
    // Cleanup
    BRIDGE_LOG_LN("🧹 Cleaning up TCP connection " << connection_id);
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        if (connection_id < tcp_connections_.size()) {
#ifdef _WIN32
            closesocket(tcp_connections_[connection_id]);
#else
            close(tcp_connections_[connection_id]);
#endif
            tcp_connections_[connection_id] = -1;
        }
    }
    notify_disconnection(connection_id);
}

void VncBridge::notify_connection(int connection_id) {
    if (!connected_ || connected_peer_id_.empty()) {
        return;
    }
    
    nlohmann::json message;
    message["connection_id"] = connection_id;
    
    client_.send("vnc_connect", message);
    BRIDGE_LOG_LN("📡 Notified bridge server of new connection (ID: " << connection_id << ")");
}

void VncBridge::notify_disconnection(int connection_id) {
    if (!connected_ || connected_peer_id_.empty()) {
        return;
    }
    
    nlohmann::json message;
    message["connection_id"] = connection_id;
    
    client_.send("vnc_disconnect", message);
    BRIDGE_LOG_LN("📡 Notified bridge server of disconnection (ID: " << connection_id << ")");
}

int VncBridge::create_tcp_server(int port) {
#ifdef _WIN32
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        BRIDGE_LOG_LN("❌ Failed to create TCP server socket");
        return -1;
    }
#else
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        BRIDGE_LOG_LN("❌ Failed to create TCP server socket");
        return -1;
    }
#endif
    
    // Set socket options
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
    
    // Enable TCP_NODELAY to disable Nagle's algorithm (critical for VNC performance)
    setsockopt(server_socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&opt), sizeof(opt));
    
    // Set accept timeout so the accept loop can check running_ flag periodically
#ifdef _WIN32
    DWORD accept_timeout_ms = 1000;  // 1 second timeout
    setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&accept_timeout_ms), sizeof(accept_timeout_ms));
#else
    struct timeval accept_tv;
    accept_tv.tv_sec = 1;  // 1 second timeout
    accept_tv.tv_usec = 0;
    setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO, &accept_tv, sizeof(accept_tv));
#endif
    
    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        BRIDGE_LOG_LN("❌ Failed to bind TCP server to port " << port);
#ifdef _WIN32
        closesocket(server_socket);
#else
        close(server_socket);
#endif
        return -1;
    }
    
    if (listen(server_socket, 5) < 0) {
        BRIDGE_LOG_LN("❌ Failed to listen on TCP port " << port);
#ifdef _WIN32
        closesocket(server_socket);
#else
        close(server_socket);
#endif
        return -1;
    }
    
    BRIDGE_LOG_LN("✅ TCP server listening on port " << port);
    return static_cast<int>(server_socket);
}

void VncBridge::forward_to_udp(const char* data, int size, int connection_id) {
    if (!connected_) {
        return;  // Silent drop for performance
    }
    
    // Binary protocol: [connection_id (1 byte)] [data...]
    std::vector<uint8_t> packet;
    packet.reserve(size + 1);
    packet.push_back(static_cast<uint8_t>(connection_id));
    packet.insert(packet.end(), reinterpret_cast<const uint8_t*>(data), reinterpret_cast<const uint8_t*>(data) + size);
    
    // Determine target peer based on mode
    std::string target_peer;
    if (mode_ == "client") {
        // Client mode: send to the single connected server
        target_peer = connected_peer_id_;
    } else {
        // Server mode: look up which peer owns this connection_id
        std::lock_guard<std::mutex> lock(peers_mutex_);
        auto it = connection_to_peer_.find(connection_id);
        if (it != connection_to_peer_.end()) {
            target_peer = it->second;
        }
    }
    
    if (target_peer.empty()) {
        return;  // No target peer found
    }
    
    // Send as raw binary - NO JSON encoding!
    client_.send_binary_to_peer_id(target_peer, packet, librats::MessageDataType::BINARY);
}

void VncBridge::forward_to_tcp_binary(const uint8_t* data, int size, int connection_id) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    if (connection_id < tcp_connections_.size() && tcp_connections_[connection_id] != -1) {
        int tcp_socket = tcp_connections_[connection_id];
        
#ifdef _WIN32
        int sent = send(tcp_socket, reinterpret_cast<const char*>(data), size, 0);
#else
        int sent = send(tcp_socket, data, size, 0);
#endif
        if (sent <= 0) {
            // Connection closed, clean up
            BRIDGE_LOG_LN("❌ TCP connection " << connection_id << " closed");
#ifdef _WIN32
            closesocket(tcp_socket);
#else
            close(tcp_socket);
#endif
            tcp_connections_[connection_id] = -1;
        }
    }
}

void VncBridge::forward_to_tcp(const std::vector<int>& data, int connection_id) {
    std::vector<uint8_t> tcp_data(data.begin(), data.end());
    forward_to_tcp_binary(tcp_data.data(), tcp_data.size(), connection_id);
}

int VncBridge::connect_to_vnc_server(int connection_id) {
#ifdef _WIN32
    SOCKET vnc_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (vnc_socket == INVALID_SOCKET) {
        BRIDGE_LOG_LN("❌ Failed to create VNC client socket");
        return -1;
    }
#else
    int vnc_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (vnc_socket < 0) {
        BRIDGE_LOG_LN("❌ Failed to create VNC client socket");
        return -1;
    }
#endif
    
    // Enable TCP_NODELAY for VNC connection
    int opt = 1;
    setsockopt(vnc_socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&opt), sizeof(opt));
    
    sockaddr_in vnc_addr = {};
    vnc_addr.sin_family = AF_INET;
    vnc_addr.sin_port = htons(vnc_server_port_);
    
    // Handle "localhost" specially - convert to 127.0.0.1
    std::string ip_to_use = vnc_server_ip_;
    if (ip_to_use == "localhost") {
        ip_to_use = "127.0.0.1";
    }
    
#ifdef _WIN32
    if (inet_pton(AF_INET, ip_to_use.c_str(), &vnc_addr.sin_addr) != 1) {
        BRIDGE_LOG_LN("❌ Invalid IP address: " << ip_to_use);
        closesocket(vnc_socket);
        return -1;
    }
#else
    if (inet_pton(AF_INET, ip_to_use.c_str(), &vnc_addr.sin_addr) != 1) {
        BRIDGE_LOG_LN("❌ Invalid IP address: " << ip_to_use);
        close(vnc_socket);
        return -1;
    }
#endif
    
    BRIDGE_LOG_LN("🔌 Connecting to VNC server at " << ip_to_use << ":" << vnc_server_port_ << "...");
    
    if (connect(vnc_socket, reinterpret_cast<sockaddr*>(&vnc_addr), sizeof(vnc_addr)) < 0) {
#ifdef _WIN32
        int error = WSAGetLastError();
        BRIDGE_LOG_LN("❌ Failed to connect to VNC server " << ip_to_use << ":" << vnc_server_port_ << " (error: " << error << ")");
        closesocket(vnc_socket);
#else
        int error = errno;
        BRIDGE_LOG_LN("❌ Failed to connect to VNC server " << ip_to_use << ":" << vnc_server_port_ << " (error: " << error << ")");
        close(vnc_socket);
#endif
        return -1;
    }
    
    BRIDGE_LOG_LN("✅ Connected to VNC server " << vnc_server_ip_ << ":" << vnc_server_port_);
    
    // Store connection
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        if (connection_id >= tcp_connections_.size()) {
            tcp_connections_.resize(connection_id + 1, -1);
        }
        tcp_connections_[connection_id] = static_cast<int>(vnc_socket);
    }
    
    // Start TCP->UDP forwarding thread for this connection
    std::thread([this, vnc_socket, connection_id]() {
        handle_tcp_to_udp(static_cast<int>(vnc_socket), connection_id);
    }).detach();
    
    return static_cast<int>(vnc_socket);
}

void VncBridge::close_tcp_connection(int connection_id) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    if (connection_id < tcp_connections_.size() && tcp_connections_[connection_id] != -1) {
        int tcp_socket = tcp_connections_[connection_id];
#ifdef _WIN32
        closesocket(tcp_socket);
#else
        close(tcp_socket);
#endif
        tcp_connections_[connection_id] = -1;
        BRIDGE_LOG_LN("🔒 Closed TCP connection " << connection_id);
    }
}
