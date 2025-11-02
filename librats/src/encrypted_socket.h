#pragma once

#include "socket.h"
#include "noise.h"
#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <unordered_map>

namespace librats {

/**
 * Encrypted socket wrapper that provides Noise Protocol encryption
 * for all socket communications in librats
 */
class EncryptedSocket {
public:
    EncryptedSocket();
    ~EncryptedSocket();
    
    // Initialize encryption for a socket
    bool initialize_as_initiator(socket_t socket, const NoiseKey& static_private_key);
    bool initialize_as_responder(socket_t socket, const NoiseKey& static_private_key);
    
    // Check encryption status
    bool is_encrypted(socket_t socket) const;
    bool is_handshake_completed(socket_t socket) const;
    bool has_handshake_failed(socket_t socket) const;
    
    // Handshake operations
    bool send_handshake_message(socket_t socket, const std::vector<uint8_t>& payload = {});
    std::vector<uint8_t> receive_handshake_message(socket_t socket);
    
    // Encrypted communication (available after handshake completion) - binary primary
    bool send_encrypted_data(socket_t socket, const std::vector<uint8_t>& data);
    std::vector<uint8_t> receive_encrypted_data(socket_t socket);
    
    // Encrypted communication - string convenience wrappers
    bool send_encrypted_data(socket_t socket, const std::string& data);
    std::string receive_encrypted_data_string(socket_t socket);
    
    // Fallback to unencrypted communication - binary primary
    bool send_unencrypted_data(socket_t socket, const std::vector<uint8_t>& data);
    std::vector<uint8_t> receive_unencrypted_data(socket_t socket);
    
    // Fallback to unencrypted communication - string convenience wrappers
    bool send_unencrypted_data(socket_t socket, const std::string& data);
    std::string receive_unencrypted_data_string(socket_t socket);
    
    // Socket management
    void remove_socket(socket_t socket);
    void clear_all_sockets();
    
    // Key management
    static NoiseKey generate_static_key();
    static std::string key_to_string(const NoiseKey& key);
    static NoiseKey string_to_key(const std::string& key_str);
    
    // Utility functions
    NoiseRole get_socket_role(socket_t socket) const;
    const NoiseKey& get_remote_static_key(socket_t socket) const;
    
private:
    struct SocketSession {
        std::unique_ptr<NoiseSession> session;
        socket_t socket;
        bool is_encrypted;
        
        SocketSession(socket_t sock) : socket(sock), is_encrypted(false) {
            session = std::make_unique<NoiseSession>();
        }
    };
    
    std::unordered_map<socket_t, std::unique_ptr<SocketSession>> sessions_;
    mutable std::mutex sessions_mutex_;
    
public:
    // Helper methods  
    SocketSession* get_session(socket_t socket);
    const SocketSession* get_session(socket_t socket) const;

private:
    
    // Helper to read exact number of bytes from socket
    std::string receive_exact_bytes(socket_t socket, size_t byte_count);
    
    // Message framing for transport
    static std::vector<uint8_t> frame_message(const std::vector<uint8_t>& message);
    static std::vector<uint8_t> unframe_message(const std::vector<uint8_t>& framed_message);
    static bool is_noise_handshake_message(const std::vector<uint8_t>& data);
};

/**
 * Encrypted socket manager - singleton for managing all encrypted sockets
 */
class EncryptedSocketManager {
public:
    static EncryptedSocketManager& getInstance();
    
    // Socket initialization
    bool initialize_socket_as_initiator(socket_t socket, const NoiseKey& static_private_key);
    bool initialize_socket_as_responder(socket_t socket, const NoiseKey& static_private_key);
    
    // Communication methods (binary - primary)
    bool send_data(socket_t socket, const std::vector<uint8_t>& data);
    std::vector<uint8_t> receive_data(socket_t socket);
    
    // Communication methods (string - convenience wrappers)
    bool send_data(socket_t socket, const std::string& data);
    std::string receive_data_string(socket_t socket);
    
    // Handshake management
    bool perform_handshake_step(socket_t socket, const std::vector<uint8_t>& received_data = {});
    bool is_handshake_completed(socket_t socket) const;
    bool has_handshake_failed(socket_t socket) const;
    
    // Socket lifecycle
    void remove_socket(socket_t socket);
    void cleanup_all_sockets();
    
    // Key management
    void set_static_key(const NoiseKey& key) { static_key_ = key; }
    const NoiseKey& get_static_key() const { return static_key_; }
    
    // Configuration
    void set_encryption_enabled(bool enabled) { encryption_enabled_ = enabled; }
    bool is_encryption_enabled() const { return encryption_enabled_; }
    
private:
    EncryptedSocketManager();
    ~EncryptedSocketManager();
    
    // Prevent copying
    EncryptedSocketManager(const EncryptedSocketManager&) = delete;
    EncryptedSocketManager& operator=(const EncryptedSocketManager&) = delete;
    
    EncryptedSocket encrypted_socket_;
    NoiseKey static_key_;
    bool encryption_enabled_;
    mutable std::mutex key_mutex_;
};

// High-level encrypted communication functions that replace the standard socket functions
namespace encrypted_communication {
    
    /**
     * Initialize encryption for librats with a static key
     * @param static_key The static private key for this node
     * @return true if successful, false otherwise
     */
    bool initialize_encryption(const NoiseKey& static_key);
    
    /**
     * Generate a new static key pair for this node
     * @return The generated static private key
     */
    NoiseKey generate_node_key();
    
    /**
     * Enable or disable encryption globally
     * @param enabled Whether encryption should be enabled
     */
    void set_encryption_enabled(bool enabled);
    
    /**
     * Check if encryption is enabled
     * @return true if encryption is enabled, false otherwise
     */
    bool is_encryption_enabled();
    
    /**
     * Send binary data through an encrypted socket (primary method)
     * @param socket The socket handle
     * @param data The binary data to send
     * @return Number of bytes sent, or -1 on error
     */
    int send_tcp_data_encrypted(socket_t socket, const std::vector<uint8_t>& data);

    /**
     * Send data through an encrypted socket (convenience wrapper for strings)
     * @param socket The socket handle
     * @param data The data to send
     * @return Number of bytes sent, or -1 on error
     */
    int send_tcp_data_encrypted(socket_t socket, const std::string& data);
    
    /**
     * Receive binary data from an encrypted socket (primary method)
     * @param socket The socket handle
     * @param buffer_size Maximum number of bytes to receive
     * @return Received binary data, empty vector on error
     */
    std::vector<uint8_t> receive_tcp_data_encrypted(socket_t socket, size_t buffer_size = 1024);

    /**
     * Receive data from an encrypted socket (convenience wrapper for strings)
     * @param socket The socket handle
     * @param buffer_size Maximum number of bytes to receive
     * @return Received data as string, empty string on error
     */
    std::string receive_tcp_data_encrypted_string(socket_t socket, size_t buffer_size = 1024);
    
    /**
     * Initialize encryption for an outgoing connection
     * @param socket The socket handle for the outgoing connection
     * @return true if successful, false otherwise
     */
    bool initialize_outgoing_connection(socket_t socket);
    
    /**
     * Initialize encryption for an incoming connection
     * @param socket The socket handle for the incoming connection
     * @return true if successful, false otherwise
     */
    bool initialize_incoming_connection(socket_t socket);
    
    /**
     * Perform handshake step for a socket
     * @param socket The socket handle
     * @return true if handshake is progressing, false if failed
     */
    bool perform_handshake(socket_t socket);
    
    /**
     * Check if handshake is completed for a socket
     * @param socket The socket handle
     * @return true if handshake is completed, false otherwise
     */
    bool is_handshake_completed(socket_t socket);
    
    /**
     * Clean up encryption state for a socket
     * @param socket The socket handle
     */
    void cleanup_socket(socket_t socket);
}

} // namespace librats 