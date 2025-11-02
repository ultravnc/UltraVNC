#include "socket.h"
#include "network_utils.h"
#include "logger.h"
#include <iostream>
#include <cstring>
#include <mutex>
#include <thread>
#include <chrono>
#ifndef _WIN32
    #include <fcntl.h>    // for O_NONBLOCK
    #include <errno.h>    // for errno
#endif

// Socket module logging macros
#define LOG_SOCKET_DEBUG(message) LOG_DEBUG("socket", message)
#define LOG_SOCKET_INFO(message)  LOG_INFO("socket", message)
#define LOG_SOCKET_WARN(message)  LOG_WARN("socket", message)
#define LOG_SOCKET_ERROR(message) LOG_ERROR("socket", message)

namespace librats {

// Static flag to track socket library initialization
static bool socket_library_initialized = false;
static std::mutex socket_init_mutex;

// Socket Library Initialization
bool init_socket_library() {
    std::lock_guard<std::mutex> lock(socket_init_mutex);
    
    if (socket_library_initialized) {
        return true; // Already initialized
    }
    
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        LOG_SOCKET_ERROR("WSAStartup failed: " << result);
        return false;
    }
    LOG_SOCKET_INFO("Windows Socket API initialized");
#endif
    
    socket_library_initialized = true;
    LOG_SOCKET_INFO("Socket library initialized");
    return true;
}

void cleanup_socket_library() {
    std::lock_guard<std::mutex> lock(socket_init_mutex);
    
    if (!socket_library_initialized) {
        return; // Not initialized or already cleaned up
    }
    
#ifdef _WIN32
    WSACleanup();
    LOG_SOCKET_INFO("Windows Socket API cleaned up");
#endif
    
    socket_library_initialized = false;
    LOG_SOCKET_INFO("Socket library cleaned up");
}

// Helper function for connection with timeout
bool connect_with_timeout(socket_t socket, struct sockaddr* addr, socklen_t addr_len, int timeout_ms) {
    // Set socket to non-blocking mode
    if (!set_socket_nonblocking(socket)) {
        LOG_SOCKET_ERROR("Failed to set socket to non-blocking mode for timeout connection");
        return false;
    }
    
    // Attempt to connect
    int result = connect(socket, addr, addr_len);
    
    if (result == 0) {
        // Connection succeeded immediately - this is rare but possible
        LOG_SOCKET_DEBUG("Connection succeeded immediately");
        return true;
    }
    
    // Check for expected non-blocking connect error
#ifdef _WIN32
    int error = WSAGetLastError();
    if (error != WSAEWOULDBLOCK) {
        LOG_SOCKET_ERROR("Connect failed immediately with error: " << error);
        return false;
    }
#else
    int error = errno;
    if (error != EINPROGRESS) {
        LOG_SOCKET_ERROR("Connect failed immediately with error: " << strerror(error));
        return false;
    }
#endif
    
    // Use select to wait for connection with timeout
    fd_set write_fds, error_fds;
    FD_ZERO(&write_fds);
    FD_ZERO(&error_fds);
    FD_SET(socket, &write_fds);
    FD_SET(socket, &error_fds);
    
    struct timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    
    LOG_SOCKET_DEBUG("Waiting for connection with timeout " << timeout_ms << "ms");
    int select_result = select(socket + 1, nullptr, &write_fds, &error_fds, &timeout);
    
    if (select_result == 0) {
        // Timeout occurred
        LOG_SOCKET_WARN("Connection timeout after " << timeout_ms << "ms");
        return false;
    } else if (select_result < 0) {
        // Select error
#ifdef _WIN32
        LOG_SOCKET_ERROR("Select error during connect: " << WSAGetLastError());
#else
        LOG_SOCKET_ERROR("Select error during connect: " << strerror(errno));
#endif
        return false;
    }
    
    // Check if connection completed successfully or failed
    if (FD_ISSET(socket, &error_fds)) {
        // Connection failed
        LOG_SOCKET_ERROR("Connection failed (error detected)");
        return false;
    }
    
    if (FD_ISSET(socket, &write_fds)) {
        // Check for actual connection success using getsockopt
        int sock_error;
        socklen_t len = sizeof(sock_error);
        if (getsockopt(socket, SOL_SOCKET, SO_ERROR, (char*)&sock_error, &len) < 0) {
            LOG_SOCKET_ERROR("Failed to get socket error status");
            return false;
        }
        
        if (sock_error != 0) {
#ifdef _WIN32
            LOG_SOCKET_ERROR("Connection failed with error: " << sock_error);
#else
            LOG_SOCKET_ERROR("Connection failed with error: " << strerror(sock_error));
#endif
            return false;
        }
        
        // Connection succeeded
        LOG_SOCKET_DEBUG("Connection succeeded within timeout");
        return true;
    }
    
    // This shouldn't happen
    LOG_SOCKET_ERROR("Unexpected select result state");
    return false;
}

// TCP Socket Functions
socket_t create_tcp_client(const std::string& host, int port, int timeout_ms) {
    if (timeout_ms > 0) {
        LOG_SOCKET_DEBUG("Creating TCP client socket (dual stack) for " << host << ":" << port << " with timeout " << timeout_ms << "ms");
    } else {
        LOG_SOCKET_DEBUG("Creating TCP client socket (dual stack) for " << host << ":" << port);
    }
    
    // Try IPv6 first
    socket_t client_socket = create_tcp_client_v6(host, port, timeout_ms);
    if (client_socket != INVALID_SOCKET_VALUE) {
        LOG_SOCKET_INFO("Successfully connected using IPv6");
        return client_socket;
    }
    
    // Fall back to IPv4
    LOG_SOCKET_DEBUG("IPv6 connection failed, trying IPv4");
    client_socket = create_tcp_client_v4(host, port, timeout_ms);
    if (client_socket != INVALID_SOCKET_VALUE) {
        LOG_SOCKET_INFO("Successfully connected using IPv4");
        return client_socket;
    }
    
    LOG_SOCKET_ERROR("Failed to connect using both IPv6 and IPv4");
    return INVALID_SOCKET_VALUE;
}

socket_t create_tcp_client_v4(const std::string& host, int port, int timeout_ms) {
    if (timeout_ms > 0) {
        LOG_SOCKET_DEBUG("Creating TCP client socket for " << host << ":" << port << " with timeout " << timeout_ms << "ms");
    } else {
        LOG_SOCKET_DEBUG("Creating TCP client socket for " << host << ":" << port);
    }
    
    // Validate port number
    if (port < 0 || port > 65535) {
        LOG_SOCKET_ERROR("Invalid port number: " << port << " (must be 0-65535)");
        return INVALID_SOCKET_VALUE;
    }
    
    socket_t client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET_VALUE) {
        LOG_SOCKET_ERROR("Failed to create client socket");
        return INVALID_SOCKET_VALUE;
    }

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    // Resolve hostname to IP address
    std::string resolved_ip = network_utils::resolve_hostname(host);
    if (resolved_ip.empty()) {
        LOG_SOCKET_ERROR("Failed to resolve hostname: " << host);
        close_socket(client_socket);
        return INVALID_SOCKET_VALUE;
    }
    
    // Convert IP address from string to binary form
    if (inet_pton(AF_INET, resolved_ip.c_str(), &server_addr.sin_addr) <= 0) {
        LOG_SOCKET_ERROR("Invalid address: " << resolved_ip);
        close_socket(client_socket);
        return INVALID_SOCKET_VALUE;
    }

    // Connect to server
    LOG_SOCKET_DEBUG("Connecting to " << resolved_ip << ":" << port);
    bool connection_success;
    
    if (timeout_ms > 0) {
        // Use timeout connection
        connection_success = connect_with_timeout(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr), timeout_ms);
    } else {
        // Use blocking connection
        connection_success = (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) != SOCKET_ERROR_VALUE);
    }
    
    if (!connection_success) {
        if (timeout_ms > 0) {
            LOG_SOCKET_ERROR("Connection to " << resolved_ip << ":" << port << " failed or timed out after " << timeout_ms << "ms");
        } else {
            LOG_SOCKET_ERROR("Connection to " << resolved_ip << ":" << port << " failed");
        }
        close_socket(client_socket);
        return INVALID_SOCKET_VALUE;
    }

    LOG_SOCKET_INFO("Successfully connected to " << resolved_ip << ":" << port);
    return client_socket;
}

socket_t create_tcp_client_v6(const std::string& host, int port, int timeout_ms) {
    if (timeout_ms > 0) {
        LOG_SOCKET_DEBUG("Creating TCP client socket for IPv6 " << host << ":" << port << " with timeout " << timeout_ms << "ms");
    } else {
        LOG_SOCKET_DEBUG("Creating TCP client socket for IPv6 " << host << ":" << port);
    }
    
    // Validate port number
    if (port < 0 || port > 65535) {
        LOG_SOCKET_ERROR("Invalid port number: " << port << " (must be 0-65535)");
        return INVALID_SOCKET_VALUE;
    }
    
    socket_t client_socket = socket(AF_INET6, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET_VALUE) {
        LOG_SOCKET_ERROR("Failed to create IPv6 client socket");
        return INVALID_SOCKET_VALUE;
    }

    sockaddr_in6 server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(port);
    
    // Resolve hostname to IPv6 address
    std::string resolved_ip = network_utils::resolve_hostname_v6(host);
    if (resolved_ip.empty()) {
        LOG_SOCKET_ERROR("Failed to resolve hostname to IPv6: " << host);
        close_socket(client_socket);
        return INVALID_SOCKET_VALUE;
    }
    
    // Convert IPv6 address from string to binary form
    if (inet_pton(AF_INET6, resolved_ip.c_str(), &server_addr.sin6_addr) <= 0) {
        LOG_SOCKET_ERROR("Invalid IPv6 address: " << resolved_ip);
        close_socket(client_socket);
        return INVALID_SOCKET_VALUE;
    }

    // Connect to server
    LOG_SOCKET_DEBUG("Connecting to IPv6 " << resolved_ip << ":" << port);
    bool connection_success;
    
    if (timeout_ms > 0) {
        // Use timeout connection
        connection_success = connect_with_timeout(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr), timeout_ms);
    } else {
        // Use blocking connection
        connection_success = (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) != SOCKET_ERROR_VALUE);
    }
    
    if (!connection_success) {
        if (timeout_ms > 0) {
            LOG_SOCKET_ERROR("Connection to IPv6 " << resolved_ip << ":" << port << " failed or timed out after " << timeout_ms << "ms");
        } else {
            LOG_SOCKET_ERROR("Connection to IPv6 " << resolved_ip << ":" << port << " failed");
        }
        close_socket(client_socket);
        return INVALID_SOCKET_VALUE;
    }

    LOG_SOCKET_INFO("Successfully connected to IPv6 " << resolved_ip << ":" << port);
    return client_socket;
}

socket_t create_tcp_server(int port, int backlog, const std::string& bind_address) {
    LOG_SOCKET_DEBUG("Creating TCP server socket (dual stack) on port " << port <<
                     (bind_address.empty() ? "" : " bound to " + bind_address));
    
    // Validate port number
    if (port < 0 || port > 65535) {
        LOG_SOCKET_ERROR("Invalid port number: " << port << " (must be 0-65535)");
        return INVALID_SOCKET_VALUE;
    }
    
    socket_t server_socket = socket(AF_INET6, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET_VALUE) {
        LOG_SOCKET_ERROR("Failed to create dual stack server socket");
        return INVALID_SOCKET_VALUE;
    }

    // Set socket option to reuse address
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, 
                   (char*)&opt, sizeof(opt)) == SOCKET_ERROR_VALUE) {
        LOG_SOCKET_ERROR("Failed to set dual stack socket options");
        close_socket(server_socket);
        return INVALID_SOCKET_VALUE;
    }

    // Disable IPv6-only mode to allow IPv4 connections
    int ipv6_only = 0;
    if (setsockopt(server_socket, IPPROTO_IPV6, IPV6_V6ONLY,
                   (char*)&ipv6_only, sizeof(ipv6_only)) == SOCKET_ERROR_VALUE) {
        LOG_SOCKET_WARN("Failed to disable IPv6-only mode, will be IPv6 only");
    }

    sockaddr_in6 server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(port);
    
    // Set bind address (default to all interfaces if empty)
    if (bind_address.empty()) {
        server_addr.sin6_addr = in6addr_any;
    } else {
        if (inet_pton(AF_INET6, bind_address.c_str(), &server_addr.sin6_addr) != 1) {
            LOG_SOCKET_ERROR("Invalid IPv6 bind address: " << bind_address);
            close_socket(server_socket);
            return INVALID_SOCKET_VALUE;
        }
    }

    // Bind socket to address
    LOG_SOCKET_DEBUG("Binding dual stack server socket to port " << port);
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR_VALUE) {
        LOG_SOCKET_ERROR("Failed to bind dual stack server socket to port " << port);
        close_socket(server_socket);
        return INVALID_SOCKET_VALUE;
    }

    // Listen for connections
    if (listen(server_socket, backlog) == SOCKET_ERROR_VALUE) {
        LOG_SOCKET_ERROR("Failed to listen on dual stack server socket");
        close_socket(server_socket);
        return INVALID_SOCKET_VALUE;
    }

    LOG_SOCKET_INFO("Dual stack server listening on port " << port << " (backlog: " << backlog << ")");
    return server_socket;
}

socket_t create_tcp_server_v4(int port, int backlog, const std::string& bind_address) {
    LOG_SOCKET_DEBUG("Creating TCP server socket on port " << port <<
                     (bind_address.empty() ? "" : " bound to " + bind_address));
    
    // Validate port number
    if (port < 0 || port > 65535) {
        LOG_SOCKET_ERROR("Invalid port number: " << port << " (must be 0-65535)");
        return INVALID_SOCKET_VALUE;
    }
    
    socket_t server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET_VALUE) {
        LOG_SOCKET_ERROR("Failed to create server socket");
        return INVALID_SOCKET_VALUE;
    }

    // Set socket option to reuse address
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, 
                   (char*)&opt, sizeof(opt)) == SOCKET_ERROR_VALUE) {
        LOG_SOCKET_ERROR("Failed to set socket options");
        close_socket(server_socket);
        return INVALID_SOCKET_VALUE;
    }

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    // Set bind address (default to all interfaces if empty)
    if (bind_address.empty()) {
        server_addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_pton(AF_INET, bind_address.c_str(), &server_addr.sin_addr) != 1) {
            LOG_SOCKET_ERROR("Invalid IPv4 bind address: " << bind_address);
            close_socket(server_socket);
            return INVALID_SOCKET_VALUE;
        }
    }

    // Bind socket to address
    LOG_SOCKET_DEBUG("Binding server socket to port " << port);
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR_VALUE) {
        LOG_SOCKET_ERROR("Failed to bind server socket to port " << port);
        close_socket(server_socket);
        return INVALID_SOCKET_VALUE;
    }

    // Listen for connections
    if (listen(server_socket, backlog) == SOCKET_ERROR_VALUE) {
        LOG_SOCKET_ERROR("Failed to listen on server socket");
        close_socket(server_socket);
        return INVALID_SOCKET_VALUE;
    }

    LOG_SOCKET_INFO("Server listening on port " << port << " (backlog: " << backlog << ")");
    return server_socket;
}

socket_t create_tcp_server_v6(int port, int backlog, const std::string& bind_address) {
    LOG_SOCKET_DEBUG("Creating TCP server socket on IPv6 port " << port <<
                     (bind_address.empty() ? "" : " bound to " + bind_address));
    
    // Validate port number
    if (port < 0 || port > 65535) {
        LOG_SOCKET_ERROR("Invalid port number: " << port << " (must be 0-65535)");
        return INVALID_SOCKET_VALUE;
    }
    
    socket_t server_socket = socket(AF_INET6, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET_VALUE) {
        LOG_SOCKET_ERROR("Failed to create IPv6 server socket");
        return INVALID_SOCKET_VALUE;
    }

    // Set socket option to reuse address
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, 
                   (char*)&opt, sizeof(opt)) == SOCKET_ERROR_VALUE) {
        LOG_SOCKET_ERROR("Failed to set IPv6 socket options");
        close_socket(server_socket);
        return INVALID_SOCKET_VALUE;
    }

    sockaddr_in6 server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(port);
    
    // Set bind address (default to all interfaces if empty)
    if (bind_address.empty()) {
        server_addr.sin6_addr = in6addr_any;
    } else {
        if (inet_pton(AF_INET6, bind_address.c_str(), &server_addr.sin6_addr) != 1) {
            LOG_SOCKET_ERROR("Invalid IPv6 bind address: " << bind_address);
            close_socket(server_socket);
            return INVALID_SOCKET_VALUE;
        }
    }

    // Bind socket to address
    LOG_SOCKET_DEBUG("Binding IPv6 server socket to port " << port);
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR_VALUE) {
        LOG_SOCKET_ERROR("Failed to bind IPv6 server socket to port " << port);
        close_socket(server_socket);
        return INVALID_SOCKET_VALUE;
    }

    // Listen for connections
    if (listen(server_socket, backlog) == SOCKET_ERROR_VALUE) {
        LOG_SOCKET_ERROR("Failed to listen on IPv6 server socket");
        close_socket(server_socket);
        return INVALID_SOCKET_VALUE;
    }

    LOG_SOCKET_INFO("IPv6 server listening on port " << port << " (backlog: " << backlog << ")");
    return server_socket;
}

socket_t accept_client(socket_t server_socket) {
    sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    socket_t client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
    if (client_socket == INVALID_SOCKET_VALUE) {
        LOG_SOCKET_ERROR("Failed to accept client connection");
        return INVALID_SOCKET_VALUE;
    }

    // Log client information based on address family
    if (client_addr.ss_family == AF_INET) {
        char client_ip[INET_ADDRSTRLEN];
        struct sockaddr_in* addr_in = (struct sockaddr_in*)&client_addr;
        inet_ntop(AF_INET, &addr_in->sin_addr, client_ip, INET_ADDRSTRLEN);
        LOG_SOCKET_INFO("Client connected from " << client_ip << ":" << ntohs(addr_in->sin_port));
    } else if (client_addr.ss_family == AF_INET6) {
        char client_ip[INET6_ADDRSTRLEN];
        struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*)&client_addr;
        inet_ntop(AF_INET6, &addr_in6->sin6_addr, client_ip, INET6_ADDRSTRLEN);
        LOG_SOCKET_INFO("Client connected from IPv6 [" << client_ip << "]:" << ntohs(addr_in6->sin6_port));
    } else {
        LOG_SOCKET_INFO("Client connected from unknown address family");
    }
    
    return client_socket;
}

std::string get_peer_address(socket_t socket) {
    sockaddr_storage peer_addr;
    socklen_t peer_addr_len = sizeof(peer_addr);
    
    if (getpeername(socket, (struct sockaddr*)&peer_addr, &peer_addr_len) == SOCKET_ERROR_VALUE) {
        LOG_SOCKET_ERROR("Failed to get peer address for socket " << socket);
        return "";
    }
    
    std::string peer_ip;
    uint16_t peer_port = 0;
    
    if (peer_addr.ss_family == AF_INET) {
        char ip_str[INET_ADDRSTRLEN];
        struct sockaddr_in* addr_in = (struct sockaddr_in*)&peer_addr;
        inet_ntop(AF_INET, &addr_in->sin_addr, ip_str, INET_ADDRSTRLEN);
        peer_ip = ip_str;
        peer_port = ntohs(addr_in->sin_port);
    } else if (peer_addr.ss_family == AF_INET6) {
        char ip_str[INET6_ADDRSTRLEN];
        struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*)&peer_addr;
        inet_ntop(AF_INET6, &addr_in6->sin6_addr, ip_str, INET6_ADDRSTRLEN);
        peer_ip = ip_str;
        peer_port = ntohs(addr_in6->sin6_port);
    } else {
        LOG_SOCKET_ERROR("Unknown address family for socket " << socket);
        return "";
    }
    
    return peer_ip + ":" + std::to_string(peer_port);
}

int send_tcp_data(socket_t socket, const std::vector<uint8_t>& data) {
    LOG_SOCKET_DEBUG("Sending " << data.size() << " bytes to TCP socket " << socket);
    
    size_t total_sent = 0;
    const char* buffer = reinterpret_cast<const char*>(data.data());
    size_t remaining = data.size();
    
    while (remaining > 0) {
#ifdef _WIN32
        int bytes_sent = send(socket, buffer + total_sent, remaining, 0);
#else
        // Use MSG_NOSIGNAL to prevent SIGPIPE on broken connections
        int bytes_sent = send(socket, buffer + total_sent, remaining, MSG_NOSIGNAL);
#endif
        if (bytes_sent == SOCKET_ERROR_VALUE) {
#ifdef _WIN32
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) {
                // Non-blocking socket would block, try again
                continue;
            }
#else
            int error = errno;
            if (error == EAGAIN || error == EWOULDBLOCK) {
                // Non-blocking socket would block, try again
                continue;
            }
            if (error == EPIPE || error == ECONNRESET || error == ENOTCONN) {
                // Connection closed by peer - this is expected during shutdown
                LOG_SOCKET_DEBUG("Connection closed during send to socket " << socket << " (error: " << strerror(error) << ")");
                return -1;
            }
#endif
            LOG_SOCKET_ERROR("Failed to send TCP data to socket " << socket << " (error: " << error << ")");
            return -1;
        }
        
        if (bytes_sent == 0) {
            LOG_SOCKET_ERROR("Connection closed by peer during send on socket " << socket);
            return -1;
        }
        
        total_sent += bytes_sent;
        remaining -= bytes_sent;
        LOG_SOCKET_DEBUG("Sent " << bytes_sent << " bytes, " << remaining << " remaining");
    }
    
    LOG_SOCKET_DEBUG("Successfully sent all " << total_sent << " bytes to TCP socket " << socket);
    return static_cast<int>(total_sent);
}

std::vector<uint8_t> receive_tcp_data(socket_t socket, size_t buffer_size) {
    if (buffer_size == 0) {
        buffer_size = 1024; // Default buffer size
    }
    
    std::vector<uint8_t> buffer(buffer_size);
    
    int bytes_received = recv(socket, reinterpret_cast<char*>(buffer.data()), buffer_size, 0);
    if (bytes_received == SOCKET_ERROR_VALUE) {
#ifdef _WIN32
        int error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK) {
            // No data available on non-blocking socket - this is normal
            return std::vector<uint8_t>();
        }
        LOG_SOCKET_ERROR("Failed to receive TCP data from socket " << socket << " (error: " << error << ")");
#else
        int error = errno;
        if (error == EAGAIN || error == EWOULDBLOCK) {
            // No data available on non-blocking socket - this is normal
            return std::vector<uint8_t>();
        }
        LOG_SOCKET_ERROR("Failed to receive TCP data from socket " << socket << " (error: " << strerror(error) << ")");
#endif
        return std::vector<uint8_t>();
    }
    
    if (bytes_received == 0) {
        LOG_SOCKET_INFO("Connection closed by peer on socket " << socket);
        return std::vector<uint8_t>();
    }
    
    LOG_SOCKET_DEBUG("Received " << bytes_received << " bytes from TCP socket " << socket);
    
    // Resize to actual received size
    buffer.resize(bytes_received);
    return buffer;
}

// Large message handling with length-prefixed framing
int send_tcp_message_framed(socket_t socket, const std::vector<uint8_t>& message) {
    // Create length prefix (4 bytes, network byte order)
    uint32_t message_length = static_cast<uint32_t>(message.size());
    uint32_t length_prefix = htonl(message_length);
    
    // Send length prefix first
    std::vector<uint8_t> prefix_data(reinterpret_cast<const uint8_t*>(&length_prefix), 
                                     reinterpret_cast<const uint8_t*>(&length_prefix) + 4);
    int prefix_sent = send_tcp_data(socket, prefix_data);
    if (prefix_sent != 4) {
        LOG_SOCKET_ERROR("Failed to send message length prefix to socket " << socket);
        return -1;
    }
    
    // Send the actual message
    int message_sent = send_tcp_data(socket, message);
    if (message_sent != static_cast<int>(message.size())) {
        LOG_SOCKET_ERROR("Failed to send complete message to socket " << socket);
        return -1;
    }
    
    LOG_SOCKET_DEBUG("Successfully sent framed message (" << message.size() << " bytes) to socket " << socket);
    return prefix_sent + message_sent;
}

std::vector<uint8_t> receive_exact_bytes(socket_t socket, size_t num_bytes) {
    std::vector<uint8_t> result;
    result.reserve(num_bytes);
    
    size_t total_received = 0;
    auto start_time = std::chrono::steady_clock::now();
    const int timeout_seconds = 5;
    
    while (total_received < num_bytes) {
        std::vector<uint8_t> buffer(num_bytes - total_received);
        int bytes_received = recv(socket, reinterpret_cast<char*>(buffer.data()), buffer.size(), 0);
        
        if (bytes_received == SOCKET_ERROR_VALUE) {
#ifdef _WIN32
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) {
                // Check for timeout
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::steady_clock::now() - start_time).count();
                if (elapsed >= timeout_seconds) {
                    LOG_SOCKET_ERROR("Timeout waiting for data on socket " << socket);
                    return std::vector<uint8_t>();
                }
                // No data available on non-blocking socket - try again
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            LOG_SOCKET_ERROR("Failed to receive exact bytes from socket " << socket << " (error: " << error << ")");
#else
            int error = errno;
            if (error == EAGAIN || error == EWOULDBLOCK) {
                // Check for timeout
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::steady_clock::now() - start_time).count();
                if (elapsed >= timeout_seconds) {
                    LOG_SOCKET_ERROR("Timeout waiting for data on socket " << socket);
                    return std::vector<uint8_t>();
                }
                // No data available on non-blocking socket - try again
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            LOG_SOCKET_ERROR("Failed to receive exact bytes from socket " << socket << " (error: " << strerror(error) << ")");
#endif
            return std::vector<uint8_t>();
        }
        
        if (bytes_received == 0) {
            LOG_SOCKET_INFO("Connection closed by peer while receiving exact bytes on socket " << socket);
            return std::vector<uint8_t>();
        }
        
        result.insert(result.end(), buffer.begin(), buffer.begin() + bytes_received);
        total_received += bytes_received;
    }
    
    LOG_SOCKET_DEBUG("Successfully received " << total_received << " exact bytes from socket " << socket);
    return result;
}

std::vector<uint8_t> receive_tcp_message_framed(socket_t socket) {
    // First, receive the 4-byte length prefix
    std::vector<uint8_t> length_data = receive_exact_bytes(socket, 4);
    if (length_data.size() != 4) {
        if (!length_data.empty()) {
            LOG_SOCKET_ERROR("Failed to receive complete length prefix from socket " << socket << " (got " << length_data.size() << " bytes)");
        }
        return std::vector<uint8_t>();
    }
    
    // Extract message length (convert from network byte order)
    uint32_t length_prefix;
    memcpy(&length_prefix, length_data.data(), 4);
    uint32_t message_length = ntohl(length_prefix);
    
    // Validate message length (prevent excessive memory allocation)
    if (message_length == 0) {
        LOG_SOCKET_DEBUG("Received keep-alive message (length 0) from socket " << socket);
        return std::vector<uint8_t>();
    }
    
    if (message_length > 100 * 1024 * 1024) { // 100MB limit
        LOG_SOCKET_ERROR("Message length too large: " << message_length << " bytes from socket " << socket);
        return std::vector<uint8_t>();
    }
    
    // Receive the actual message
    std::vector<uint8_t> message = receive_exact_bytes(socket, message_length);
    if (message.size() != message_length) {
        LOG_SOCKET_ERROR("Failed to receive complete message from socket " << socket << " (expected " << message_length << " bytes, got " << message.size() << ")");
        return std::vector<uint8_t>();
    }
    
    LOG_SOCKET_DEBUG("Successfully received framed message (" << message_length << " bytes) from socket " << socket);
    return message;
}

// Convenience functions for string compatibility
int send_tcp_string(socket_t socket, const std::string& data) {
    std::vector<uint8_t> binary_data(data.begin(), data.end());
    return send_tcp_data(socket, binary_data);
}

std::string receive_tcp_string(socket_t socket, size_t buffer_size) {
    std::vector<uint8_t> binary_data = receive_tcp_data(socket, buffer_size);
    if (binary_data.empty()) {
        return "";
    }
    return std::string(binary_data.begin(), binary_data.end());
}

int send_tcp_string_framed(socket_t socket, const std::string& message) {
    std::vector<uint8_t> binary_message(message.begin(), message.end());
    return send_tcp_message_framed(socket, binary_message);
}

std::string receive_tcp_string_framed(socket_t socket) {
    std::vector<uint8_t> binary_message = receive_tcp_message_framed(socket);
    if (binary_message.empty()) {
        return "";
    }
    return std::string(binary_message.begin(), binary_message.end());
}

// UDP Socket Functions
socket_t create_udp_socket(int port, const std::string& bind_address) {
    LOG_SOCKET_DEBUG("Creating dual stack UDP socket on port " << port <<
                     (bind_address.empty() ? "" : " bound to " + bind_address));
    
    // Validate port number
    if (port < 0 || port > 65535) {
        LOG_SOCKET_ERROR("Invalid port number: " << port << " (must be 0-65535)");
        return INVALID_SOCKET_VALUE;
    }
    
    socket_t udp_socket = socket(AF_INET6, SOCK_DGRAM, 0);
    if (udp_socket == INVALID_SOCKET_VALUE) {
#ifdef _WIN32
        LOG_SOCKET_ERROR("Failed to create dual stack UDP socket (error: " << WSAGetLastError() << ")");
#else
        LOG_SOCKET_ERROR("Failed to create dual stack UDP socket (error: " << strerror(errno) << ")");
#endif
        return INVALID_SOCKET_VALUE;
    }

    // Set socket option to reuse address
    int opt = 1;
    if (setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, 
                   (char*)&opt, sizeof(opt)) == SOCKET_ERROR_VALUE) {
        LOG_SOCKET_ERROR("Failed to set dual stack UDP socket options");
        close_socket(udp_socket);
        return INVALID_SOCKET_VALUE;
    }

    // Disable IPv6-only mode to allow IPv4 connections
    int ipv6_only = 0;
    if (setsockopt(udp_socket, IPPROTO_IPV6, IPV6_V6ONLY,
                   (char*)&ipv6_only, sizeof(ipv6_only)) == SOCKET_ERROR_VALUE) {
        LOG_SOCKET_WARN("Failed to disable IPv6-only mode, will be IPv6 only");
    }

    // Always bind the socket - this is required for receiving data
    sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);  // port=0 will get an ephemeral port
    
    // Set bind address (default to all interfaces if empty)
    if (bind_address.empty()) {
        addr.sin6_addr = in6addr_any;
    } else {
        if (inet_pton(AF_INET6, bind_address.c_str(), &addr.sin6_addr) != 1) {
            LOG_SOCKET_ERROR("Invalid IPv6 bind address: " << bind_address);
            close_socket(udp_socket);
            return INVALID_SOCKET_VALUE;
        }
    }

    if (bind(udp_socket, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR_VALUE) {
#ifdef _WIN32
        LOG_SOCKET_ERROR("Failed to bind dual stack UDP socket to port " << port << " (error: " << WSAGetLastError() << ")");
#else
        LOG_SOCKET_ERROR("Failed to bind dual stack UDP socket to port " << port << " (error: " << strerror(errno) << ")");
#endif
        close_socket(udp_socket);
        return INVALID_SOCKET_VALUE;
    }
    
    // Get the actual bound port if ephemeral port was requested
    if (port == 0) {
        sockaddr_in6 bound_addr;
        socklen_t addr_len = sizeof(bound_addr);
        if (getsockname(udp_socket, (struct sockaddr*)&bound_addr, &addr_len) == 0) {
            uint16_t actual_port = ntohs(bound_addr.sin6_port);
            LOG_SOCKET_INFO("Dual stack UDP socket bound to ephemeral port " << actual_port);
        } else {
            LOG_SOCKET_INFO("Dual stack UDP socket bound to ephemeral port (unknown)");
        }
    } else {
        LOG_SOCKET_INFO("Dual stack UDP socket bound to port " << port);
    }

    return udp_socket;
}

socket_t create_udp_socket_v4(int port, const std::string& bind_address) {
    LOG_SOCKET_DEBUG("Creating UDP socket on port " << port <<
                     (bind_address.empty() ? "" : " bound to " + bind_address));
    
    // Validate port number
    if (port < 0 || port > 65535) {
        LOG_SOCKET_ERROR("Invalid port number: " << port << " (must be 0-65535)");
        return INVALID_SOCKET_VALUE;
    }
    
    socket_t udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket == INVALID_SOCKET_VALUE) {
#ifdef _WIN32
        LOG_SOCKET_ERROR("Failed to create UDP socket (error: " << WSAGetLastError() << ")");
#else
        LOG_SOCKET_ERROR("Failed to create UDP socket (error: " << strerror(errno) << ")");
#endif
        return INVALID_SOCKET_VALUE;
    }

    // Set socket option to reuse address
    int opt = 1;
    if (setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, 
                   (char*)&opt, sizeof(opt)) == SOCKET_ERROR_VALUE) {
        LOG_SOCKET_ERROR("Failed to set UDP socket options");
        close_socket(udp_socket);
        return INVALID_SOCKET_VALUE;
    }

    // Always bind the socket - this is required for receiving data
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);  // port=0 will get an ephemeral port
    
    // Set bind address (default to all interfaces if empty)
    if (bind_address.empty()) {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_pton(AF_INET, bind_address.c_str(), &addr.sin_addr) != 1) {
            LOG_SOCKET_ERROR("Invalid IPv4 bind address: " << bind_address);
            close_socket(udp_socket);
            return INVALID_SOCKET_VALUE;
        }
    }

    if (bind(udp_socket, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR_VALUE) {
#ifdef _WIN32
        LOG_SOCKET_ERROR("Failed to bind UDP socket to port " << port << " (error: " << WSAGetLastError() << ")");
#else
        LOG_SOCKET_ERROR("Failed to bind UDP socket to port " << port << " (error: " << strerror(errno) << ")");
#endif
        close_socket(udp_socket);
        return INVALID_SOCKET_VALUE;
    }
    
    // Get the actual bound port if ephemeral port was requested
    if (port == 0) {
        sockaddr_in bound_addr;
        socklen_t addr_len = sizeof(bound_addr);
        if (getsockname(udp_socket, (struct sockaddr*)&bound_addr, &addr_len) == 0) {
            uint16_t actual_port = ntohs(bound_addr.sin_port);
            LOG_SOCKET_INFO("UDP socket bound to ephemeral port " << actual_port);
        } else {
            LOG_SOCKET_INFO("UDP socket bound to ephemeral port (unknown)");
        }
    } else {
        LOG_SOCKET_INFO("UDP socket bound to port " << port);
    }

    return udp_socket;
}

socket_t create_udp_socket_v6(int port, const std::string& bind_address) {
    LOG_SOCKET_DEBUG("Creating UDP socket on IPv6 port " << port <<
                     (bind_address.empty() ? "" : " bound to " + bind_address));
    
    // Validate port number
    if (port < 0 || port > 65535) {
        LOG_SOCKET_ERROR("Invalid port number: " << port << " (must be 0-65535)");
        return INVALID_SOCKET_VALUE;
    }
    
    socket_t udp_socket = socket(AF_INET6, SOCK_DGRAM, 0);
    if (udp_socket == INVALID_SOCKET_VALUE) {
#ifdef _WIN32
        LOG_SOCKET_ERROR("Failed to create IPv6 UDP socket (error: " << WSAGetLastError() << ")");
#else
        LOG_SOCKET_ERROR("Failed to create IPv6 UDP socket (error: " << strerror(errno) << ")");
#endif
        return INVALID_SOCKET_VALUE;
    }

    // Set socket option to reuse address
    int opt = 1;
    if (setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, 
                   (char*)&opt, sizeof(opt)) == SOCKET_ERROR_VALUE) {
        LOG_SOCKET_ERROR("Failed to set IPv6 UDP socket options");
        close_socket(udp_socket);
        return INVALID_SOCKET_VALUE;
    }

    // Always bind the socket - this is required for receiving data
    sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);  // port=0 will get an ephemeral port
    
    // Set bind address (default to all interfaces if empty)
    if (bind_address.empty()) {
        addr.sin6_addr = in6addr_any;
    } else {
        if (inet_pton(AF_INET6, bind_address.c_str(), &addr.sin6_addr) != 1) {
            LOG_SOCKET_ERROR("Invalid IPv6 bind address: " << bind_address);
            close_socket(udp_socket);
            return INVALID_SOCKET_VALUE;
        }
    }

    if (bind(udp_socket, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR_VALUE) {
#ifdef _WIN32
        LOG_SOCKET_ERROR("Failed to bind IPv6 UDP socket to port " << port << " (error: " << WSAGetLastError() << ")");
#else
        LOG_SOCKET_ERROR("Failed to bind IPv6 UDP socket to port " << port << " (error: " << strerror(errno) << ")");
#endif
        close_socket(udp_socket);
        return INVALID_SOCKET_VALUE;
    }
    
    // Get the actual bound port if ephemeral port was requested
    if (port == 0) {
        sockaddr_in6 bound_addr;
        socklen_t addr_len = sizeof(bound_addr);
        if (getsockname(udp_socket, (struct sockaddr*)&bound_addr, &addr_len) == 0) {
            uint16_t actual_port = ntohs(bound_addr.sin6_port);
            LOG_SOCKET_INFO("IPv6 UDP socket bound to ephemeral port " << actual_port);
        } else {
            LOG_SOCKET_INFO("IPv6 UDP socket bound to ephemeral port (unknown)");
        }
    } else {
        LOG_SOCKET_INFO("IPv6 UDP socket bound to port " << port);
    }

    return udp_socket;
}



int send_udp_data(socket_t socket, const std::vector<uint8_t>& data, const Peer& peer) {
    LOG_SOCKET_DEBUG("Sending " << data.size() << " bytes to " << peer.ip << ":" << peer.port);
    
    // Check if it's an IPv6 address
    if (network_utils::is_valid_ipv6(peer.ip)) {
        // Handle IPv6 address
        sockaddr_in6 addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin6_family = AF_INET6;
        addr.sin6_port = htons(peer.port);
        
        if (inet_pton(AF_INET6, peer.ip.c_str(), &addr.sin6_addr) <= 0) {
            LOG_SOCKET_ERROR("Invalid IPv6 address: " << peer.ip);
            return -1;
        }

        int bytes_sent = sendto(socket, (char*)data.data(), data.size(), 0, 
                               (struct sockaddr*)&addr, sizeof(addr));
        if (bytes_sent == SOCKET_ERROR_VALUE) {
            LOG_SOCKET_ERROR("Failed to send UDP data to IPv6 " << peer.ip << ":" << peer.port);
            return -1;
        }
        
        LOG_SOCKET_DEBUG("Successfully sent " << bytes_sent << " bytes to IPv6 " << peer.ip << ":" << peer.port);
        return bytes_sent;
    } else {
        // Handle IPv4 address or hostname
        std::string resolved_ip = network_utils::resolve_hostname(peer.ip);
        if (resolved_ip.empty()) {
            LOG_SOCKET_ERROR("Failed to resolve hostname: " << peer.ip);
            return -1;
        }
        
        // For dual-stack sockets, we need to use IPv6 address structure
        // and convert IPv4 to IPv4-mapped IPv6 address (::ffff:x.x.x.x)
        sockaddr_in6 addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin6_family = AF_INET6;
        addr.sin6_port = htons(peer.port);
        
        // Convert IPv4 to IPv4-mapped IPv6 address
        struct in_addr ipv4_addr;
        if (inet_pton(AF_INET, resolved_ip.c_str(), &ipv4_addr) <= 0) {
            LOG_SOCKET_ERROR("Invalid IPv4 address: " << resolved_ip);
            return -1;
        }
        
        // Create IPv4-mapped IPv6 address: ::ffff:x.x.x.x
        addr.sin6_addr.s6_addr[10] = 0xff;
        addr.sin6_addr.s6_addr[11] = 0xff;
        memcpy(&addr.sin6_addr.s6_addr[12], &ipv4_addr.s_addr, 4);
        
        LOG_SOCKET_DEBUG("Converting IPv4 " << resolved_ip << " to IPv4-mapped IPv6 for dual-stack socket");

        int bytes_sent = sendto(socket, (char*)data.data(), data.size(), 0, 
                               (struct sockaddr*)&addr, sizeof(addr));
        if (bytes_sent == SOCKET_ERROR_VALUE) {
#ifdef _WIN32
            LOG_SOCKET_ERROR("Failed to send UDP data to " << resolved_ip << ":" << peer.port << " (error: " << WSAGetLastError() << ")");
#else
            LOG_SOCKET_ERROR("Failed to send UDP data to " << resolved_ip << ":" << peer.port << " (error: " << strerror(errno) << ")");
#endif
            return -1;
        }
        
        LOG_SOCKET_DEBUG("Successfully sent " << bytes_sent << " bytes to " << resolved_ip << ":" << peer.port << " via IPv4-mapped IPv6");
        return bytes_sent;
    }
}

int send_udp_data_to(socket_t socket, const std::vector<uint8_t>& data, const std::string& hostname, int port) {
    LOG_SOCKET_DEBUG("Sending " << data.size() << " bytes to " << hostname << ":" << port);
    
    // Validate port number
    if (port < 0 || port > 65535) {
        LOG_SOCKET_ERROR("Invalid port number: " << port << " (must be 0-65535)");
        return -1;
    }
    
    // Resolve hostname to IP address
    std::string resolved_ip = network_utils::resolve_hostname(hostname);
    if (resolved_ip.empty()) {
        LOG_SOCKET_ERROR("Failed to resolve hostname: " << hostname);
        return -1;
    }
    
    // Check if it's an IPv6 address
    if (network_utils::is_valid_ipv6(resolved_ip)) {
        // Handle IPv6 address
        sockaddr_in6 addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin6_family = AF_INET6;
        addr.sin6_port = htons(port);
        
        if (inet_pton(AF_INET6, resolved_ip.c_str(), &addr.sin6_addr) <= 0) {
            LOG_SOCKET_ERROR("Invalid IPv6 address: " << resolved_ip);
            return -1;
        }

        int bytes_sent = sendto(socket, (char*)data.data(), data.size(), 0, 
                               (struct sockaddr*)&addr, sizeof(addr));
        if (bytes_sent == SOCKET_ERROR_VALUE) {
            LOG_SOCKET_ERROR("Failed to send UDP data to IPv6 " << resolved_ip << ":" << port);
            return -1;
        }
        
        LOG_SOCKET_DEBUG("Successfully sent " << bytes_sent << " bytes to IPv6 " << resolved_ip << ":" << port);
        return bytes_sent;
    } else {
        // Handle IPv4 address
        sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        
        if (inet_pton(AF_INET, resolved_ip.c_str(), &addr.sin_addr) <= 0) {
            LOG_SOCKET_ERROR("Invalid IPv4 address: " << resolved_ip);
            return -1;
        }

        int bytes_sent = sendto(socket, (char*)data.data(), data.size(), 0, 
                               (struct sockaddr*)&addr, sizeof(addr));
        if (bytes_sent == SOCKET_ERROR_VALUE) {
            LOG_SOCKET_ERROR("Failed to send UDP data to " << resolved_ip << ":" << port);
            return -1;
        }
        
        LOG_SOCKET_DEBUG("Successfully sent " << bytes_sent << " bytes to " << resolved_ip << ":" << port);
        return bytes_sent;
    }
}

std::vector<uint8_t> receive_udp_data(socket_t socket, size_t buffer_size, Peer& sender_peer) {
    std::vector<uint8_t> buffer(buffer_size);
    sockaddr_storage sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);
    
    int bytes_received = recvfrom(socket, (char*)buffer.data(), buffer_size, 0,
                                 (struct sockaddr*)&sender_addr, &sender_addr_len);
    
    if (bytes_received == SOCKET_ERROR_VALUE) {
        // Check if this is just a non-blocking socket with no data available
#ifdef _WIN32
        int error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK) {
            // No data available on non-blocking socket - this is normal
            return std::vector<uint8_t>();
        } else {
            LOG_SOCKET_ERROR("Failed to receive UDP data: " << error);
            return std::vector<uint8_t>();
        }
#else
        int error = errno;
        if (error == EAGAIN || error == EWOULDBLOCK) {
            // No data available on non-blocking socket - this is normal
            return std::vector<uint8_t>();
        } else {
            LOG_SOCKET_ERROR("Failed to receive UDP data: " << strerror(error));
            return std::vector<uint8_t>();
        }
#endif
    }
    
    if (bytes_received == 0) {
        LOG_SOCKET_DEBUG("Received empty UDP packet");
        return std::vector<uint8_t>();
    }
    
    // Extract sender information based on address family
    if (sender_addr.ss_family == AF_INET) {
        char sender_ip[INET_ADDRSTRLEN];
        struct sockaddr_in* addr_in = (struct sockaddr_in*)&sender_addr;
        inet_ntop(AF_INET, &addr_in->sin_addr, sender_ip, INET_ADDRSTRLEN);
        sender_peer.ip = sender_ip;
        sender_peer.port = ntohs(addr_in->sin_port);
        
        LOG_SOCKET_DEBUG("Received " << bytes_received << " bytes from " << sender_peer.ip << ":" << sender_peer.port);
    } else if (sender_addr.ss_family == AF_INET6) {
        char sender_ip[INET6_ADDRSTRLEN];
        struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*)&sender_addr;
        inet_ntop(AF_INET6, &addr_in6->sin6_addr, sender_ip, INET6_ADDRSTRLEN);
        sender_peer.ip = sender_ip;
        sender_peer.port = ntohs(addr_in6->sin6_port);
        
        LOG_SOCKET_DEBUG("Received " << bytes_received << " bytes from IPv6 [" << sender_peer.ip << "]:" << sender_peer.port);
    } else {
        LOG_SOCKET_WARN("Received UDP data from unknown address family");
        sender_peer.ip = "unknown";
        sender_peer.port = 0;
    }
    
    buffer.resize(bytes_received);
    return buffer;
}

std::vector<uint8_t> receive_udp_data_with_timeout(socket_t socket, size_t buffer_size, int timeout_ms, 
                                                   std::string* sender_ip, int* sender_port) {
    LOG_SOCKET_DEBUG("Receiving UDP data with timeout " << timeout_ms << "ms");
    
    std::vector<uint8_t> buffer(buffer_size);
    sockaddr_storage sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);
    
    // Handle timeout if specified
    if (timeout_ms > 0) {
        // Set up timeout using select
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(socket, &read_fds);
        
        struct timeval timeout;
        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;
        
        // Wait for data with timeout
        int result = select(socket + 1, &read_fds, nullptr, nullptr, &timeout);
        if (result == 0) {
            LOG_SOCKET_DEBUG("UDP receive timeout (" << timeout_ms << "ms)");
            return std::vector<uint8_t>();
        } else if (result < 0) {
            LOG_SOCKET_ERROR("Select error while waiting for UDP data");
            return std::vector<uint8_t>();
        }
    }
    
    int bytes_received = recvfrom(socket, (char*)buffer.data(), buffer_size, 0,
                                 (struct sockaddr*)&sender_addr, &sender_addr_len);
    
    if (bytes_received == SOCKET_ERROR_VALUE) {
        // Check if this is just a non-blocking socket with no data available
#ifdef _WIN32
        int error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK) {
            // No data available on non-blocking socket - this is normal
            LOG_SOCKET_DEBUG("No UDP data available (non-blocking)");
            return std::vector<uint8_t>();
        } else {
            LOG_SOCKET_ERROR("Failed to receive UDP data: " << error);
            return std::vector<uint8_t>();
        }
#else
        int error = errno;
        if (error == EAGAIN || error == EWOULDBLOCK) {
            // No data available on non-blocking socket - this is normal
            LOG_SOCKET_DEBUG("No UDP data available (non-blocking)");
            return std::vector<uint8_t>();
        } else {
            LOG_SOCKET_ERROR("Failed to receive UDP data: " << strerror(error));
            return std::vector<uint8_t>();
        }
#endif
    }
    
    if (bytes_received == 0) {
        LOG_SOCKET_DEBUG("Received empty UDP packet");
        return std::vector<uint8_t>();
    }
    
    // Extract sender information based on address family
    if (sender_addr.ss_family == AF_INET) {
        char ip_str[INET_ADDRSTRLEN];
        struct sockaddr_in* addr_in = (struct sockaddr_in*)&sender_addr;
        inet_ntop(AF_INET, &addr_in->sin_addr, ip_str, INET_ADDRSTRLEN);
        
        if (sender_ip) *sender_ip = ip_str;
        if (sender_port) *sender_port = ntohs(addr_in->sin_port);
        
        LOG_SOCKET_DEBUG("Received " << bytes_received << " bytes from " << ip_str << ":" << ntohs(addr_in->sin_port));
    } else if (sender_addr.ss_family == AF_INET6) {
        char ip_str[INET6_ADDRSTRLEN];
        struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*)&sender_addr;
        inet_ntop(AF_INET6, &addr_in6->sin6_addr, ip_str, INET6_ADDRSTRLEN);
        
        if (sender_ip) *sender_ip = ip_str;
        if (sender_port) *sender_port = ntohs(addr_in6->sin6_port);
        
        LOG_SOCKET_DEBUG("Received " << bytes_received << " bytes from IPv6 [" << ip_str << "]:" << ntohs(addr_in6->sin6_port));
    } else {
        LOG_SOCKET_WARN("Received UDP data from unknown address family");
        if (sender_ip) *sender_ip = "unknown";
        if (sender_port) *sender_port = 0;
    }
    
    buffer.resize(bytes_received);
    return buffer;
}

// Helper function to determine if a socket is TCP
bool is_tcp_socket(socket_t socket) {
    if (!is_valid_socket(socket)) {
        return false;
    }
    
    int sock_type;
    socklen_t opt_len = sizeof(sock_type);
    
    if (getsockopt(socket, SOL_SOCKET, SO_TYPE, (char*)&sock_type, &opt_len) == 0) {
        return sock_type == SOCK_STREAM;
    }
    
    return false; // Assume not TCP if we can't determine
}

// Common Socket Functions
void close_socket(socket_t socket, bool force) {
    if (is_valid_socket(socket)) {
        LOG_SOCKET_DEBUG("Closing socket " << socket);
        
        // Peform force shutdown if needed
        if (force) {
            LOG_SOCKET_DEBUG("Performing shutdown for TCP socket " << socket);
#ifdef _WIN32
            shutdown(socket, SD_BOTH);
#else
            shutdown(socket, SHUT_RDWR);
#endif
        }
        
        closesocket(socket);
    }
}

bool is_valid_socket(socket_t socket) {
    return socket != INVALID_SOCKET_VALUE;
}

bool set_socket_nonblocking(socket_t socket) {
#ifdef _WIN32
    unsigned long mode = 1;
    if (ioctlsocket(socket, FIONBIO, &mode) != 0) {
        LOG_SOCKET_ERROR("Failed to set socket to non-blocking mode");
        return false;
    }
#else
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags == -1) {
        LOG_SOCKET_ERROR("Failed to get socket flags");
        return false;
    }
    
    if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        LOG_SOCKET_ERROR("Failed to set socket to non-blocking mode");
        return false;
    }
#endif
    
    LOG_SOCKET_DEBUG("Socket set to non-blocking mode");
    return true;
}

int get_ephemeral_port(socket_t socket)
{
    sockaddr_in6 bound_addr;
    socklen_t addr_len = sizeof(bound_addr);
    if (getsockname(socket, (struct sockaddr*)&bound_addr, &addr_len) == 0) {
        return ntohs(bound_addr.sin6_port);
    }
    return 0;
}

} // namespace librats 