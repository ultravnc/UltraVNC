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
    config.hole_punch_attempts = 3;
    config.stun_servers = { "stun.l.google.com:19302" };
    config.ice_gathering_timeout_ms = 5000;
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
        connected_ = true;
        connected_peer_id_ = peer_id;
        std::cout << "\n🎉 UDP TUNNEL ESTABLISHED!" << std::endl;
        std::cout << "✅ Connected to bridge peer: " << peer_id.substr(0, 16) << "..." << std::endl;
        std::cout << "🔌 Socket: " << socket << std::endl;
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
        std::cout << "🔗 New VNC connection request (ID: " << connection_id << ")" << std::endl;
        
        if (mode_ == "server") {
            // Connect to actual VNC server
            connect_to_vnc_server(connection_id);
        }
    });
    
    client_.on("vnc_disconnect", [this](const std::string& peer_id, const nlohmann::json& data) {
        int connection_id = data["connection_id"];
        std::cout << "❌ VNC connection closed (ID: " << connection_id << ")" << std::endl;
        
        // Close corresponding TCP connection
        close_tcp_connection(connection_id);
    });
    
    // Disconnection callback
    client_.set_disconnect_callback([this](socket_t socket, const std::string& peer_id) {
        connected_ = false;
        connected_peer_id_.clear();
        std::cout << "❌ UDP tunnel disconnected: " << peer_id.substr(0, 16) << "..." << std::endl;
        std::cout << "🔌 Socket: " << socket << " closed" << std::endl;
    });
}

bool VncBridge::start() {
    std::cout << "🚀 Starting VNC bridge in " << mode_ << " mode..." << std::endl;
    
    // Reduce librats logging to only show warnings and errors (for performance)
    // librats::Logger::getInstance().set_log_level(librats::LogLevel::WARN);
    
    if (!client_.start()) {
        std::cout << "❌ Failed to start librats client" << std::endl;
        return false;
    }
    
    std::cout << "✅ librats client started successfully!" << std::endl;
    return true;
}

void VncBridge::run_client_mode() {
    std::cout << "\n🌉 VNC BRIDGE CLIENT - Embedded mode" << std::endl;
    std::cout << "📋 VNC viewers connect to: localhost:" << tcp_listen_port_ << std::endl;
    std::cout << "🔑 Discovery code: " << discovery_code_ << std::endl;
    
    // Start the librats client first
    if (!start()) {
        std::cout << "❌ Failed to start bridge client" << std::endl;
        return;
    }
    
    // Wait for initialization
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Connect to bridge server using discovery code
    std::cout << "🎯 Attempting UDP tunnel connection..." << std::endl;
    bool success = client_.connect_to_peer("127.0.0.1", BRIDGE_SERVER_PORT, 
                                          librats::ConnectionStrategy::AUTO_ADAPTIVE);
    
    if (success) {
        std::cout << "✅ UDP tunnel connection initiated" << std::endl;
        
        // Wait for UDP connection
        for (int i = 0; i < 30 && !connected_; i++) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (i % 5 == 4) {
                std::cout << "⏳ Waiting for UDP tunnel... (" << (i + 1) << "/30)" << std::endl;
            }
        }
        
        if (connected_) {
            std::cout << "\n🎉 UDP tunnel established!" << std::endl;
            
            // Start TCP server for VNC viewers
            int server_socket = create_tcp_server(tcp_listen_port_);
            if (server_socket >= 0) {
                std::cout << "\n💡 Connect your VNC viewer to: localhost:" << tcp_listen_port_ << std::endl;
                std::cout << "🔄 Bridge is ready - forwarding VNC traffic through UDP tunnel" << std::endl;
                
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
                        
                        std::cout << "🎯 VNC viewer connected (ID: " << connection_counter << ")" << std::endl;
                        
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
            std::cout << "❌ UDP tunnel connection failed after 30 seconds" << std::endl;
        }
    } else {
        std::cout << "❌ Failed to initiate UDP tunnel connection" << std::endl;
    }
    
    // Keep running until interrupted
    std::cout << "\n⏳ Client is running. Press Ctrl+C to exit." << std::endl;
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        if (connected_) {
            std::cout << "📊 Bridge client active - UDP tunnel established" << std::endl;
            // Test if we can actually send data
            std::cout << "🔍 Connection status: ACTIVE" << std::endl;
        } else {
            std::cout << "📊 Waiting for UDP tunnel connection..." << std::endl;
            std::cout << "🔍 Connection status: DISCONNECTED" << std::endl;
        }
    }
    
    std::cout << "🛑 Bridge client mode stopped" << std::endl;
}

void VncBridge::run_server_mode() {
    std::cout << "\n🌉 VNC BRIDGE SERVER - Embedded mode" << std::endl;
    std::cout << "🎯 Will connect to VNC server: " << vnc_server_ip_ << ":" << vnc_server_port_ << std::endl;
    std::cout << "🔑 Discovery code: " << discovery_code_ << std::endl;
    
    // Start the librats client first
    if (!start()) {
        std::cout << "❌ Failed to start bridge server" << std::endl;
        return;
    }
    
    // Wait for initialization
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    std::cout << "\n💡 Client command: vnc_bridge client \"" << discovery_code_ << "\"" << std::endl;
    std::cout << "📢 Server is listening and ready for connections" << std::endl;
    std::cout << "⏳ Waiting for bridge client connections..." << std::endl;
    
    // Keep running and handle connections
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        if (connected_) {
            std::cout << "📊 Bridge server active - UDP tunnel established" << std::endl;
            std::cout << "🔍 Server status: CONNECTED" << std::endl;
        } else {
            std::cout << "📊 Waiting for bridge client..." << std::endl;
            std::cout << "🔍 Server status: LISTENING" << std::endl;
        }
    }
    
    std::cout << "🛑 Bridge server mode stopped" << std::endl;
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
            std::cout << "📊 TCP connection " << connection_id << " closed by peer" << std::endl;
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
            std::cout << "❌ TCP receive error on connection " << connection_id << std::endl;
            break;
        }
    }
    
    // Cleanup
    std::cout << "🧹 Cleaning up TCP connection " << connection_id << std::endl;
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
    std::cout << "📡 Notified bridge server of new connection (ID: " << connection_id << ")" << std::endl;
}

void VncBridge::notify_disconnection(int connection_id) {
    if (!connected_ || connected_peer_id_.empty()) {
        return;
    }
    
    nlohmann::json message;
    message["connection_id"] = connection_id;
    
    client_.send("vnc_disconnect", message);
    std::cout << "📡 Notified bridge server of disconnection (ID: " << connection_id << ")" << std::endl;
}

int VncBridge::create_tcp_server(int port) {
#ifdef _WIN32
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        std::cout << "❌ Failed to create TCP server socket" << std::endl;
        return -1;
    }
#else
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cout << "❌ Failed to create TCP server socket" << std::endl;
        return -1;
    }
#endif
    
    // Set socket options
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
    
    // Enable TCP_NODELAY to disable Nagle's algorithm (critical for VNC performance)
    setsockopt(server_socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&opt), sizeof(opt));
    
    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        std::cout << "❌ Failed to bind TCP server to port " << port << std::endl;
#ifdef _WIN32
        closesocket(server_socket);
#else
        close(server_socket);
#endif
        return -1;
    }
    
    if (listen(server_socket, 5) < 0) {
        std::cout << "❌ Failed to listen on TCP port " << port << std::endl;
#ifdef _WIN32
        closesocket(server_socket);
#else
        close(server_socket);
#endif
        return -1;
    }
    
    std::cout << "✅ TCP server listening on port " << port << std::endl;
    return static_cast<int>(server_socket);
}

void VncBridge::forward_to_udp(const char* data, int size, int connection_id) {
    if (!connected_ || connected_peer_id_.empty()) {
        return;  // Silent drop for performance
    }
    
    // Binary protocol: [connection_id (1 byte)] [data...]
    std::vector<uint8_t> packet;
    packet.reserve(size + 1);
    packet.push_back(static_cast<uint8_t>(connection_id));
    packet.insert(packet.end(), reinterpret_cast<const uint8_t*>(data), reinterpret_cast<const uint8_t*>(data) + size);
    
    // Send as raw binary - NO JSON encoding!
    client_.send_binary_to_peer_id(connected_peer_id_, packet, librats::MessageDataType::BINARY);
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
            std::cout << "❌ TCP connection " << connection_id << " closed" << std::endl;
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
        std::cout << "❌ Failed to create VNC client socket" << std::endl;
        return -1;
    }
#else
    int vnc_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (vnc_socket < 0) {
        std::cout << "❌ Failed to create VNC client socket" << std::endl;
        return -1;
    }
#endif
    
    // Enable TCP_NODELAY for VNC connection
    int opt = 1;
    setsockopt(vnc_socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&opt), sizeof(opt));
    
    sockaddr_in vnc_addr = {};
    vnc_addr.sin_family = AF_INET;
    vnc_addr.sin_port = htons(vnc_server_port_);
    
#ifdef _WIN32
    inet_pton(AF_INET, vnc_server_ip_.c_str(), &vnc_addr.sin_addr);
#else
    inet_pton(AF_INET, vnc_server_ip_.c_str(), &vnc_addr.sin_addr);
#endif
    
    if (connect(vnc_socket, reinterpret_cast<sockaddr*>(&vnc_addr), sizeof(vnc_addr)) < 0) {
        std::cout << "❌ Failed to connect to VNC server " << vnc_server_ip_ << ":" << vnc_server_port_ << std::endl;
#ifdef _WIN32
        closesocket(vnc_socket);
#else
        close(vnc_socket);
#endif
        return -1;
    }
    
    std::cout << "✅ Connected to VNC server " << vnc_server_ip_ << ":" << vnc_server_port_ << std::endl;
    
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
        std::cout << "🔒 Closed TCP connection " << connection_id << std::endl;
    }
}
