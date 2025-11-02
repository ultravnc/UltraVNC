#include "librats.h"
#include "logger.h"
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
    std::string discovery_code_;  // Discovery code for connection
    int derived_port_;            // Port derived from discovery code
    bool connected_;
    std::string connected_peer_id_;
    
    // TCP server/client variables
    int tcp_listen_port_;    // Port to listen for VNC viewer connections (client mode)
    std::string vnc_server_ip_;  // VNC server IP to connect to (server mode)
    int vnc_server_port_;    // VNC server port to connect to (server mode)
    
    // Active connections
    std::vector<int> tcp_connections_;
    std::mutex connections_mutex_;
    bool running_;
    int listen_port_;  // UDP listen port
    
    
    static librats::NatTraversalConfig create_config() {
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
    
    // Generate a port number from discovery code (range 8000-9000)
    static int generate_port_from_code(const std::string& code) {
        std::hash<std::string> hasher;
        size_t hash_value = hasher(code);
        return 8000 + (hash_value % 1000);  // Port range: 8000-8999
    }
    
public:
    VncBridge(int port, const std::string& mode, const std::string& discovery_code,
              int tcp_port = 0, const std::string& vnc_ip = "", int vnc_port = 0)
        : // Initialize client_ FIRST using the parameter (not member variable!)
          client_(mode == "server" ? generate_port_from_code(discovery_code) : port, 5, create_config()),
          mode_(mode),
          discovery_code_(discovery_code),
          derived_port_(generate_port_from_code(discovery_code)),
          connected_(false), 
          tcp_listen_port_(tcp_port), 
          vnc_server_ip_(vnc_ip), 
          vnc_server_port_(vnc_port), 
          running_(true), 
          listen_port_(port) {
        
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
        
        setup_callbacks();
    }
    
    ~VncBridge() {
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
        
#ifdef _WIN32
        WSACleanup();
#endif
    }
    
    void setup_callbacks() {
        // Connection callback
        client_.set_connection_callback([this](socket_t socket, const std::string& peer_id) {
            connected_ = true;
            connected_peer_id_ = peer_id;
            std::cout << "\n🎉 UDP TUNNEL ESTABLISHED!" << std::endl;
            std::cout << "✅ Connected to bridge peer: " << peer_id.substr(0, 16) << "..." << std::endl;
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
        });
    }
    
    // Optimized version - BINARY PROTOCOL (no JSON overhead!)
    void forward_to_udp_direct(const uint8_t* data, int size, int connection_id) {
        if (!connected_ || connected_peer_id_.empty()) {
            return;  // Silent drop for performance
        }
        
        // Binary protocol: [connection_id (1 byte)] [data...]
        std::vector<uint8_t> packet;
        packet.reserve(size + 1);
        packet.push_back(static_cast<uint8_t>(connection_id));
        packet.insert(packet.end(), data, data + size);
        
        // Send as raw binary - NO JSON encoding!
        client_.send_binary_to_peer_id(connected_peer_id_, packet, librats::MessageDataType::BINARY);
    }
    
    // Legacy version kept for compatibility
    void forward_to_udp(const std::vector<uint8_t>& data, int connection_id) {
        forward_to_udp_direct(data.data(), data.size(), connection_id);
    }
    
    // BINARY version - direct send without conversion
    void forward_to_tcp_binary(const uint8_t* data, int size, int connection_id) {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        
        if (connection_id < tcp_connections_.size() && tcp_connections_[connection_id] != -1) {
            int tcp_socket = tcp_connections_[connection_id];
            
            int sent = send(tcp_socket, reinterpret_cast<const char*>(data), size, 0);
            if (sent <= 0) {
                close_tcp_connection(connection_id);
            }
        }
    }
    
    // Legacy JSON version
    void forward_to_tcp(const std::vector<int>& data, int connection_id) {
        std::vector<uint8_t> tcp_data(data.begin(), data.end());
        forward_to_tcp_binary(tcp_data.data(), tcp_data.size(), connection_id);
    }
    
    void notify_connection(int connection_id) {
        nlohmann::json message;
        message["connection_id"] = connection_id;
        
        client_.send("vnc_connect", message);
        std::cout << "✅ New VNC connection (ID: " << connection_id << ")" << std::endl;
    }
    
    void notify_disconnection(int connection_id) {
        nlohmann::json message;
        message["connection_id"] = connection_id;
        
        client_.send("vnc_disconnect", message);
        std::cout << "❌ VNC disconnection (ID: " << connection_id << ")" << std::endl;
    }
    
    int create_tcp_server(int port) {
        int server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0) {
            std::cout << "❌ Failed to create TCP server socket" << std::endl;
            return -1;
        }
        
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
        return server_socket;
    }
    
    int connect_to_vnc_server(int connection_id) {
        int vnc_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (vnc_socket < 0) {
            std::cout << "❌ Failed to create VNC client socket" << std::endl;
            return -1;
        }
        
        // Enable TCP_NODELAY for VNC connection
        int opt = 1;
        setsockopt(vnc_socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&opt), sizeof(opt));
        
        sockaddr_in vnc_addr = {};
        vnc_addr.sin_family = AF_INET;
        vnc_addr.sin_port = htons(vnc_server_port_);
        inet_pton(AF_INET, vnc_server_ip_.c_str(), &vnc_addr.sin_addr);
        
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
        std::lock_guard<std::mutex> lock(connections_mutex_);
        if (connection_id >= tcp_connections_.size()) {
            tcp_connections_.resize(connection_id + 1, -1);
        }
        tcp_connections_[connection_id] = vnc_socket;
        
        // Start TCP->UDP forwarding thread for this connection
        std::thread([this, vnc_socket, connection_id]() {
            handle_tcp_to_udp(vnc_socket, connection_id);
        }).detach();
        
        return vnc_socket;
    }
    
    void handle_tcp_to_udp(int tcp_socket, int connection_id) {
        std::vector<uint8_t> buffer(65536);  // 64KB buffer for better throughput
        
        while (running_) {
            int received = recv(tcp_socket, reinterpret_cast<char*>(buffer.data()), buffer.size(), 0);
            if (received > 0) {
                // Pass buffer directly with size - avoid unnecessary vector copy
                forward_to_udp_direct(buffer.data(), received, connection_id);
            } else if (received == 0) {
                std::cout << "📊 TCP connection " << connection_id << " closed by peer" << std::endl;
                break;
            } else {
                std::cout << "❌ TCP receive error on connection " << connection_id << std::endl;
                break;
            }
        }
        
        // Cleanup
        close_tcp_connection(connection_id);
        notify_disconnection(connection_id);
    }
    
    void close_tcp_connection(int connection_id) {
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
    
    bool start() {
        std::cout << "🚀 Starting VNC bridge in " << mode_ << " mode..." << std::endl;
        
        // Reduce librats logging to only show warnings and errors (for performance)
        librats::Logger::getInstance().set_log_level(librats::LogLevel::WARN);
        
        if (!client_.start()) {
            std::cout << "❌ Failed to start librats client" << std::endl;
            return false;
        }
        
        std::cout << "✅ librats client started successfully!" << std::endl;
        return true;
    }
    
    void print_status() {
        std::cout << "\n📊 VNC BRIDGE STATUS:" << std::endl;
        std::cout << "🆔 Peer ID: " << client_.get_our_peer_id().substr(0, 16) << "..." << std::endl;
        std::cout << "👥 Connected peers: " << client_.get_peer_count() << std::endl;
        std::cout << "🌉 Mode: " << mode_ << std::endl;
        
        if (mode_ == "client") {
            std::cout << "🔌 TCP listen port: " << tcp_listen_port_ << std::endl;
        } else {
            std::cout << "🎯 VNC server: " << vnc_server_ip_ << ":" << vnc_server_port_ << std::endl;
        }
        
        auto public_ip = client_.get_public_ip();
        if (!public_ip.empty()) {
            std::cout << "🌍 Public IP: " << public_ip << std::endl;
        }
    }
    
    void run_client_mode() {
        std::cout << "\n🌉 VNC BRIDGE CLIENT - Viewer side" << std::endl;
        std::cout << "📋 VNC viewers connect to: localhost:" << tcp_listen_port_ << std::endl;
        std::cout << "🔑 Discovery code: " << discovery_code_ << std::endl;
        std::cout << "🔢 Derived connection port: " << derived_port_ << std::endl;
        
        // Wait for initialization
        std::this_thread::sleep_for(std::chrono::seconds(2));
        print_status();
        
        // Connect to bridge server using derived port
        std::cout << "🎯 Attempting UDP tunnel connection to localhost:" << derived_port_ << "..." << std::endl;
        bool success = client_.connect_to_peer("127.0.0.1", derived_port_, 
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
                        
                        int client_socket = accept(server_socket, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
                        if (client_socket >= 0) {
                            // Enable TCP_NODELAY on accepted socket
                            int opt = 1;
                            setsockopt(client_socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&opt), sizeof(opt));
                            
                            std::cout << "🎯 VNC viewer connected (ID: " << connection_counter << ")" << std::endl;
                            
                            // Store connection
                            {
                                std::lock_guard<std::mutex> lock(connections_mutex_);
                                if (connection_counter >= tcp_connections_.size()) {
                                    tcp_connections_.resize(connection_counter + 1, -1);
                                }
                                tcp_connections_[connection_counter] = client_socket;
                            }
                            
                            // Notify bridge server
                            notify_connection(connection_counter);
                            
                            // Start forwarding thread
                            std::thread([this, client_socket, connection_counter]() {
                                handle_tcp_to_udp(client_socket, connection_counter);
                            }).detach();
                            
                            connection_counter++;
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
        }
    }
    
    void run_server_mode() {
        std::cout << "\n🌉 VNC BRIDGE SERVER - VNC server side" << std::endl;
        std::cout << "🎯 Will connect to VNC server: " << vnc_server_ip_ << ":" << vnc_server_port_ << std::endl;
        std::cout << "🔑 Discovery code: " << discovery_code_ << std::endl;
        std::cout << "🔢 Listening on derived port: " << derived_port_ << std::endl;
        
        // Wait for initialization
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        print_status();
        
        std::cout << "\n💡 Client command: vnc_bridge client " << listen_port_ << " <tcp_port> \"" << discovery_code_ << "\"" << std::endl;
        std::cout << "📢 Server is listening and ready for connections" << std::endl;
        std::cout << "⏳ Waiting for bridge client connections..." << std::endl;
        
        // Keep running and handle connections
        while (running_) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
            if (connected_) {
                std::cout << "📊 Bridge active - UDP tunnel established" << std::endl;
            } else {
                std::cout << "📊 Waiting for bridge client..." << std::endl;
            }
        }
    }
    
    void stop() {
        running_ = false;
        client_.stop();
    }
};

void print_usage(const char* program_name) {
    std::cout << "VNC Bridge - TCP to UDP tunnel using librats with discovery codes\n" << std::endl;
    std::cout << "Architecture:" << std::endl;
    std::cout << "  VNC Viewer (TCP) -> Bridge Client -> UDP Tunnel -> Bridge Server -> VNC Server (TCP)\n" << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  " << program_name << " server <discovery_code> <vnc_server_ip> <vnc_server_port>" << std::endl;
    std::cout << "  " << program_name << " client <udp_port> <tcp_port> <discovery_code>" << std::endl;
    std::cout << "\nExample:" << std::endl;
    std::cout << "  # Bridge Server (near VNC server) - port auto-derived from code" << std::endl;
    std::cout << "  " << program_name << " server \"123 1234 123 12\" 127.0.0.1 5900" << std::endl;
    std::cout << "\n  # Bridge Client (near VNC viewer)" << std::endl;
    std::cout << "  " << program_name << " client 8081 5901 \"123 1234 123 12\"" << std::endl;
    std::cout << "\n  # Then connect VNC viewer to: localhost:5901" << std::endl;
    std::cout << "\n💡 Both sides must use the SAME discovery code!" << std::endl;
    std::cout << "💡 Server port is automatically derived from the discovery code!" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "🌉 VNC Bridge - TCP to UDP tunnel using librats\n" << std::endl;
    
    if (argc < 4) {
        print_usage(argv[0]);
        return 1;
    }
    
    std::string mode = argv[1];
    
    if (mode == "server") {
        if (argc < 5) {
            std::cerr << "❌ Server mode requires: <discovery_code> <vnc_server_ip> <vnc_server_port>" << std::endl;
            print_usage(argv[0]);
            return 1;
        }
        
        std::string discovery_code = argv[2];
        std::string vnc_ip = argv[3];
        int vnc_port = std::atoi(argv[4]);
        
        std::cout << "📋 Server Configuration:" << std::endl;
        std::cout << "  Discovery Code: " << discovery_code << std::endl;
        std::cout << "  VNC Server: " << vnc_ip << ":" << vnc_port << std::endl;
        
        // Server uses derived port from discovery code (not user-specified)
        std::cout << "🔨 Creating VNC bridge..." << std::endl;
        VncBridge bridge(0, mode, discovery_code, 0, vnc_ip, vnc_port);
        std::cout << "✅ VNC bridge created" << std::endl;
        
        if (bridge.start()) {
            std::cout << "\n💡 Press Ctrl+C to stop the server" << std::endl;
            bridge.run_server_mode();
        } else {
            std::cerr << "❌ Failed to start VNC bridge server" << std::endl;
            return 1;
        }
    } else if (mode == "client") {
        if (argc < 5) {
            std::cerr << "❌ Client mode requires: <udp_port> <tcp_port> <discovery_code>" << std::endl;
            print_usage(argv[0]);
            return 1;
        }
        
        int udp_port = std::atoi(argv[2]);
        int tcp_port = std::atoi(argv[3]);
        std::string discovery_code = argv[4];
        
        if (udp_port <= 0 || udp_port > 65535) {
            std::cerr << "❌ Invalid UDP port: " << argv[2] << std::endl;
            return 1;
        }
        
        VncBridge bridge(udp_port, mode, discovery_code, tcp_port);
        if (bridge.start()) {
            std::cout << "\n💡 Press Ctrl+C to stop the client" << std::endl;
            bridge.run_client_mode();
        } else {
            std::cerr << "❌ Failed to start VNC bridge client" << std::endl;
            return 1;
        }
    } else {
        std::cerr << "❌ Invalid mode: " << mode << std::endl;
        print_usage(argv[0]);
        return 1;
    }
    
    return 0;
}
