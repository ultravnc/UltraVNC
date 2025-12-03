#pragma once

#include "../librats/src/librats.h"
#include "../librats/src/logger.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <cstdint>
#include <mutex>
#include <queue>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif

// VNC Bridge: TCP <-> librats UDP tunnel
// Architecture: VNC Viewer (TCP) -> Bridge Client -> UDP Tunnel -> Bridge Server -> VNC Server (TCP)
class VncBridge {
private:
    librats::RatsClient client_;
    std::string mode_;
    std::string discovery_code_;  // Discovery code for peer identification
    bool connected_;
    std::string connected_peer_id_;
    
    // Fixed ports - no derivation needed
    static const int BRIDGE_SERVER_PORT = 50000;  // Bridge server UDP (high dynamic range)
    static const int BRIDGE_CLIENT_PORT = 5901;   // Bridge client TCP (VNC-related)
    
    // TCP server/client variables
    int tcp_listen_port_;    // Port to listen for VNC viewer connections (client mode)
    std::string vnc_server_ip_;  // VNC server IP to connect to (server mode)
    int vnc_server_port_;    // VNC server port to connect to (server mode)
    
    // Active connections
    std::vector<int> tcp_connections_;
    std::mutex connections_mutex_;
    bool running_;
    
    // Private helper methods
    static librats::NatTraversalConfig create_config();
    static std::string generate_unique_code();
    static std::string get_machine_id();
    static std::string calculate_checksum(const std::string& base_code);
    
    void setup_callbacks();
    void notify_connection(int connection_id);
    void notify_disconnection(int connection_id);
    int create_tcp_server(int port);
    int connect_to_vnc_server(int connection_id);
    void close_tcp_connection(int connection_id);
    void handle_tcp_to_udp(int tcp_socket, int connection_id);
    void forward_to_udp(const char* data, int size, int connection_id);
    void forward_to_tcp_binary(const uint8_t* data, int size, int connection_id);
    void forward_to_tcp(const std::vector<int>& data, int connection_id);
    
public:
    // Constructor - simplified with fixed ports
    VncBridge(const std::string& mode, const std::string& discovery_code = "",
              const std::string& vnc_ip = "", int vnc_port = 5900);
    
    // Destructor
    ~VncBridge();
    
    // Main execution methods
    bool start();
    void run_client_mode();
    void run_server_mode();
    
    // Control methods
    void stop();
    bool is_running() const { return running_; }
    bool is_connected() const { return connected_; }
    
    // Status methods
    std::string get_status() const;
    int get_tcp_listen_port() const { return tcp_listen_port_; }
    std::string get_discovery_code() const { return discovery_code_; }
    
    // Static utility methods for embedded mode
    static std::string get_auto_generated_code() { return generate_unique_code(); }
    static bool validate_discovery_code(const std::string& code);
    static std::string get_validation_error(const std::string& code);
    static std::string normalize_discovery_code(const std::string& code);
};
