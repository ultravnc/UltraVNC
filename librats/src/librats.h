#pragma once

#include "socket.h"
#include "dht.h"
#include "stun.h"
#include "mdns.h"
#include "ice.h"
#include "logger.h"
#include "encrypted_socket.h"
#include "threadmanager.h"
#include "gossipsub.h" // For ValidationResult enum and GossipSub types
#include "file_transfer.h" // File transfer functionality
#include "json.hpp" // nlohmann::json
#include <string>
#include <functional>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <condition_variable>
#include <unordered_set> // Added for unordered_set
#include <cstdint>
#include <cstring>
#include "rats_export.h"

namespace librats {

// Forward declarations
class IceAgent;

/**
 * RatsPeer struct - comprehensive information about a connected rats peer
 */
struct RatsPeer {
    std::string peer_id;                    // Unique hash ID for the peer
    std::string ip;                         // IP address
    uint16_t port;                          // Port number  
    socket_t socket;                        // Socket handle
    std::string normalized_address;         // Normalized address for duplicate detection (ip:port)
    std::chrono::steady_clock::time_point connected_at; // Connection timestamp
    bool is_outgoing;                       // True if we initiated the connection, false if incoming
    
    // Handshake-related fields
    enum class HandshakeState {
        PENDING,        // Handshake not started
        SENT,          // Handshake sent, waiting for response
        COMPLETED,     // Handshake completed successfully
        FAILED         // Handshake failed
    };
    
    HandshakeState handshake_state;         // Current handshake state
    std::string version;                    // Protocol version of remote peer
    int peer_count;                         // Number of peers connected to remote peer
    std::chrono::steady_clock::time_point handshake_start_time; // When handshake started
    
    // Encryption-related fields
    bool encryption_enabled;                // Whether encryption is enabled for this peer
    bool noise_handshake_completed;         // Whether noise handshake is completed
    NoiseKey remote_static_key;             // Remote peer's static public key (after handshake)
    
    // NAT traversal fields
    bool ice_enabled;                       // Whether ICE is enabled for this peer
    std::string ice_ufrag;                  // ICE username fragment
    std::string ice_pwd;                    // ICE password
    std::vector<IceCandidate> ice_candidates; // ICE candidates for this peer
    IceConnectionState ice_state;           // Current ICE connection state
    NatType detected_nat_type;              // Detected NAT type for this peer
    std::string connection_method;          // How connection was established (direct, stun, turn, ice)
    
    // Connection quality metrics
    uint32_t rtt_ms;                        // Round-trip time in milliseconds
    uint32_t packet_loss_percent;           // Packet loss percentage
    std::string transport_protocol;         // UDP, TCP, etc.
    
    RatsPeer() : handshake_state(HandshakeState::PENDING), 
                 peer_count(0), encryption_enabled(false), noise_handshake_completed(false),
                 ice_enabled(false), ice_state(IceConnectionState::NEW),
                 detected_nat_type(NatType::UNKNOWN), rtt_ms(0), packet_loss_percent(0),
                 transport_protocol("UDP") {
        connected_at = std::chrono::steady_clock::now();
        handshake_start_time = connected_at;
    }
    
    RatsPeer(const std::string& id, const std::string& peer_ip, uint16_t peer_port, 
             socket_t sock, const std::string& norm_addr, bool outgoing)
        : peer_id(id), ip(peer_ip), port(peer_port), socket(sock), 
          normalized_address(norm_addr), is_outgoing(outgoing),
          handshake_state(HandshakeState::PENDING), peer_count(0),
          encryption_enabled(false), noise_handshake_completed(false),
          ice_enabled(false), ice_state(IceConnectionState::NEW),
          detected_nat_type(NatType::UNKNOWN), rtt_ms(0), packet_loss_percent(0),
          transport_protocol("UDP") {
        connected_at = std::chrono::steady_clock::now();
        handshake_start_time = connected_at;
    }
    
    // Helper methods
    bool is_handshake_completed() const { return handshake_state == HandshakeState::COMPLETED; }
    bool is_handshake_failed() const { return handshake_state == HandshakeState::FAILED; }
    bool is_ice_connected() const { 
        return ice_state == IceConnectionState::CONNECTED || 
               ice_state == IceConnectionState::COMPLETED; 
    }
    bool is_fully_connected() const {
        return is_handshake_completed() && (!ice_enabled || is_ice_connected());
    }
};

// NAT Traversal Configuration
struct NatTraversalConfig {
    bool enable_ice;                        // Enable ICE for NAT traversal
    bool enable_upnp;                       // Enable UPnP for port mapping
    bool enable_hole_punching;              // Enable UDP/TCP hole punching
    bool enable_turn_relay;                 // Enable TURN relay as last resort
    bool prefer_ipv6;                       // Prefer IPv6 connections when available
    
    // ICE configuration
    std::vector<std::string> stun_servers;
    std::vector<std::string> turn_servers;
    std::vector<std::string> turn_usernames;
    std::vector<std::string> turn_passwords;
    
    // Timeouts and limits
    int ice_gathering_timeout_ms;
    int ice_connectivity_timeout_ms;
    int hole_punch_attempts;
    int turn_allocation_timeout_ms;
    
    // Priority settings
    int host_candidate_priority;
    int server_reflexive_priority;
    int relay_candidate_priority;
    
    NatTraversalConfig() 
        : enable_ice(true), enable_upnp(false), enable_hole_punching(true),
          enable_turn_relay(true), prefer_ipv6(false),
          ice_gathering_timeout_ms(10000), ice_connectivity_timeout_ms(30000),
          hole_punch_attempts(5), turn_allocation_timeout_ms(10000),
          host_candidate_priority(65535), server_reflexive_priority(65534),
          relay_candidate_priority(65533) {
        
        // Default STUN servers
        stun_servers.push_back("stun.l.google.com:19302");
        stun_servers.push_back("stun1.l.google.com:19302");
        stun_servers.push_back("stun.stunprotocol.org:3478");
    }
};

// Connection establishment strategies
enum class ConnectionStrategy {
    DIRECT_ONLY,        // Try direct connection only
    STUN_ASSISTED,      // Use STUN for public IP discovery
    ICE_FULL,           // Full ICE with candidate gathering
    TURN_RELAY,         // Force TURN relay usage
    AUTO_ADAPTIVE       // Automatically choose best strategy
};

// Connection attempt result
struct ConnectionAttemptResult {
    bool success;
    std::string method;                     // "direct", "stun", "ice", "turn", "hole_punch"
    std::chrono::milliseconds duration;
    std::string error_message;
    NatType local_nat_type;
    NatType remote_nat_type;
    std::vector<IceCandidate> used_candidates;
};

// Enhanced connection callbacks
using AdvancedConnectionCallback = std::function<void(socket_t, const std::string&, const ConnectionAttemptResult&)>;
using NatTraversalProgressCallback = std::function<void(const std::string&, const std::string&)>; // peer_id, status
using IceCandidateDiscoveredCallback = std::function<void(const std::string&, const IceCandidate&)>; // peer_id, candidate

/**
 * Message data types for librats message headers
 */
enum class MessageDataType : uint8_t {
    BINARY = 0x01,      // Raw binary data
    STRING = 0x02,      // UTF-8 string data  
    JSON = 0x03         // JSON formatted data
};

/**
 * Message header structure for librats messages
 * Fixed 8-byte header format:
 * [0-3]: Magic number "RATS" (4 bytes)
 * [4]: Message data type (1 byte)
 * [5-7]: Reserved for future use (3 bytes)
 */
struct MessageHeader {
    static constexpr uint32_t MAGIC_NUMBER = 0x52415453; // "RATS" in ASCII
    static constexpr size_t HEADER_SIZE = 8;
    
    uint32_t magic;         // Magic number for validation
    MessageDataType type;   // Message data type
    uint8_t reserved[3];    // Reserved bytes for future use
    
    MessageHeader(MessageDataType data_type) : magic(MAGIC_NUMBER), type(data_type) {
        reserved[0] = reserved[1] = reserved[2] = 0;
    }
    
    MessageHeader() : magic(MAGIC_NUMBER), type(MessageDataType::BINARY) {
        reserved[0] = reserved[1] = reserved[2] = 0;
    }
    
    // Serialize header to bytes
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> data(HEADER_SIZE);
        uint32_t network_magic = htonl(magic);
        memcpy(data.data(), &network_magic, 4);
        data[4] = static_cast<uint8_t>(type);
        data[5] = reserved[0];
        data[6] = reserved[1]; 
        data[7] = reserved[2];
        return data;
    }
    
    // Deserialize header from bytes
    static bool deserialize(const std::vector<uint8_t>& data, MessageHeader& header) {
        if (data.size() < HEADER_SIZE) {
            return false;
        }
        
        uint32_t network_magic;
        memcpy(&network_magic, data.data(), 4);
        header.magic = ntohl(network_magic);
        
        if (header.magic != MAGIC_NUMBER) {
            return false;
        }
        
        header.type = static_cast<MessageDataType>(data[4]);
        header.reserved[0] = data[5];
        header.reserved[1] = data[6];
        header.reserved[2] = data[7];
        
        return true;
    }
    
    // Validate data type
    bool is_valid_type() const {
        return type == MessageDataType::BINARY || 
               type == MessageDataType::STRING || 
               type == MessageDataType::JSON;
    }
};

/**
 * Enhanced RatsClient with comprehensive NAT traversal capabilities
 */
class RATS_API RatsClient : public ThreadManager {
public:
    // =========================================================================
    // Type Definitions and Callbacks
    // =========================================================================
    using ConnectionCallback = std::function<void(socket_t, const std::string&)>;
    using BinaryDataCallback = std::function<void(socket_t, const std::string&, const std::vector<uint8_t>&)>;
    using StringDataCallback = std::function<void(socket_t, const std::string&, const std::string&)>;
    using JsonDataCallback = std::function<void(socket_t, const std::string&, const nlohmann::json&)>;
    using DisconnectCallback = std::function<void(socket_t, const std::string&)>;
    using MessageCallback = std::function<void(const std::string&, const nlohmann::json&)>;
    using SendCallback = std::function<void(bool, const std::string&)>;

    // =========================================================================
    // Constructor and Destructor
    // =========================================================================
    
    /**
     * Constructor
     * @param listen_port Port to listen on for incoming connections
     * @param max_peers Maximum number of concurrent peers (default: 10)
     * @param nat_config NAT traversal configuration
     * @param bind_address Interface IP address to bind to (empty for all interfaces)
     */
    RatsClient(int listen_port, int max_peers = 10, const NatTraversalConfig& nat_config = NatTraversalConfig(), const std::string& bind_address = "");
    
    /**
     * Destructor
     */
    ~RatsClient();

    // =========================================================================
    // Core Lifecycle Management
    // =========================================================================
    
    /**
     * Start the RatsClient and begin listening for connections
     * @return true if successful, false otherwise
     */
    bool start();

    /**
     * Stop the RatsClient and close all connections
     */
    void stop();

    /**
     * Shutdown all background threads
     */
    void shutdown_all_threads();

    /**
     * Check if the client is currently running
     * @return true if running, false otherwise
     */
    bool is_running() const;


    // =========================================================================
    // Utility Methods
    // =========================================================================

    int get_listen_port() const;
    
    /**
     * Get the bind address being used
     * @return Bind address (empty string if binding to all interfaces)
     */
    std::string get_bind_address() const;

    // =========================================================================
    // Connection Management
    // =========================================================================
    
    /**
     * Connect to a peer with automatic NAT traversal
     * @param host Target host/IP address
     * @param port Target port
     * @param strategy Connection strategy to use
     * @return true if connection initiated successfully
     */
    bool connect_to_peer(const std::string& host, int port, 
                        ConnectionStrategy strategy = ConnectionStrategy::AUTO_ADAPTIVE);
    
    /**
     * Connect to a peer using ICE coordination
     * @param peer_id Target peer ID
     * @param ice_offer ICE offer from remote peer
     * @return true if ICE connection initiated successfully
     */
    bool connect_with_ice(const std::string& peer_id, const nlohmann::json& ice_offer);
    
    /**
     * Create ICE offer for a peer
     * @param peer_id Target peer ID
     * @return ICE offer JSON that can be sent to the peer
     */
    nlohmann::json create_ice_offer(const std::string& peer_id);
    
    /**
     * Handle ICE answer from a peer
     * @param peer_id Source peer ID
     * @param ice_answer ICE answer from the peer
     * @return true if successfully processed
     */
    bool handle_ice_answer(const std::string& peer_id, const nlohmann::json& ice_answer);
    
    /**
     * Disconnect from a specific peer
     * @param socket Peer socket to disconnect
     */
    void disconnect_peer(socket_t socket);

    /**
     * Disconnect from a peer by peer_id (preferred)
     * @param peer_id Peer ID to disconnect
     */
    void disconnect_peer_by_id(const std::string& peer_id);

    // =========================================================================
    // Data Transmission Methods
    // =========================================================================
    
    // Send to specific peer by socket
    /**
     * Send binary data to a specific peer (primary method)
     * @param socket Target peer socket
     * @param data Binary data to send
     * @param message_type Type of message data (BINARY, STRING, JSON)
     * @return true if sent successfully
     */
    bool send_binary_to_peer(socket_t socket, const std::vector<uint8_t>& data, MessageDataType message_type = MessageDataType::BINARY);

    /**
     * Send string data to a specific peer
     * @param socket Target peer socket
     * @param data String data to send
     * @return true if sent successfully
     */
    bool send_string_to_peer(socket_t socket, const std::string& data);

    /**
     * Send JSON data to a specific peer
     * @param socket Target peer socket
     * @param data JSON data to send
     * @return true if sent successfully
     */
    bool send_json_to_peer(socket_t socket, const nlohmann::json& data);

    // Send to specific peer by ID
    /**
     * Send binary data to a peer by peer_id (preferred)
     * @param peer_id Target peer ID
     * @param data Binary data to send
     * @param message_type Type of message data (BINARY, STRING, JSON)
     * @return true if sent successfully
     */
    bool send_binary_to_peer_id(const std::string& peer_id, const std::vector<uint8_t>& data, MessageDataType message_type = MessageDataType::BINARY);

    /**
     * Send string data to a peer by peer_id (preferred)
     * @param peer_id Target peer ID
     * @param data String data to send
     * @return true if sent successfully
     */
    bool send_string_to_peer_id(const std::string& peer_id, const std::string& data);

    /**
     * Send JSON data to a peer by peer_id (preferred)
     * @param peer_id Target peer ID
     * @param data JSON data to send
     * @return true if sent successfully
     */
    bool send_json_to_peer_id(const std::string& peer_id, const nlohmann::json& data);

    // Broadcast to all peers
    /**
     * Broadcast binary data to all connected peers (primary method)
     * @param data Binary data to broadcast
     * @param message_type Type of message data (BINARY, STRING, JSON)
     * @return Number of peers the data was sent to
     */
    int broadcast_binary_to_peers(const std::vector<uint8_t>& data, MessageDataType message_type = MessageDataType::BINARY);

    /**
     * Broadcast string data to all connected peers
     * @param data String data to broadcast
     * @return Number of peers the data was sent to
     */
    int broadcast_string_to_peers(const std::string& data);

    /**
     * Broadcast JSON data to all connected peers
     * @param data JSON data to broadcast
     * @return Number of peers the data was sent to
     */
    int broadcast_json_to_peers(const nlohmann::json& data);

    // =========================================================================
    // Peer Information and Management
    // =========================================================================
    
    /**
     * Get the number of currently connected peers
     * @return Number of connected peers
     */
    int get_peer_count() const;


    /**
     * Get peer_id for a peer by socket (preferred)
     * @param socket Peer socket
     * @return Peer ID or empty string if not found
     */
    std::string get_peer_id(socket_t socket) const;

    /**
     * Get socket for a peer by peer_id (preferred)
     * @param peer_id Peer ID
     * @return Peer socket or INVALID_SOCKET_VALUE if not found
     */
    socket_t get_peer_socket_by_id(const std::string& peer_id) const;

    /**
     * Get our own peer ID
     * @return Our persistent peer ID
     */
    std::string get_our_peer_id() const;
    
    /**
     * Get all connected peers
     * @return Vector of RatsPeer objects
     */
    std::vector<RatsPeer> get_all_peers() const;
    
    /**
     * Get all peers that have completed handshake
     * @return Vector of RatsPeer objects with completed handshake
     */
    std::vector<RatsPeer> get_validated_peers() const;
    
    /**
     * Get peer information by peer ID
     * @param peer_id The peer ID to look up
     * @return Pointer to RatsPeer object, or nullptr if not found
     */
    const RatsPeer* get_peer_by_id(const std::string& peer_id) const;
    
    /**
     * Get peer information by socket
     * @param socket The socket handle to look up
     * @return Pointer to RatsPeer object, or nullptr if not found  
     */
    const RatsPeer* get_peer_by_socket(socket_t socket) const;
    
    /**
     * Get maximum number of peers
     * @return Maximum peer count
     */
    int get_max_peers() const;

    /**
     * Set maximum number of peers
     * @param max_peers New maximum peer count
     */
    void set_max_peers(int max_peers);

    /**
     * Check if peer limit has been reached
     * @return true if at limit, false otherwise
     */
    bool is_peer_limit_reached() const;

    // =========================================================================
    // Callback Registration
    // =========================================================================
    
    /**
     * Set connection callback (called when a new peer connects)
     * @param callback Function to call on new connections
     */
    void set_connection_callback(ConnectionCallback callback);
    
    /**
     * Set advanced connection callback with NAT traversal info
     * @param callback Function to call on new connections with detailed info
     */
    void set_advanced_connection_callback(AdvancedConnectionCallback callback);

    /**
     * Set binary data callback (called when binary data is received)
     * @param callback Function to call when binary data is received
     */
    void set_binary_data_callback(BinaryDataCallback callback);

    /**
     * Set string data callback (called when string data is received)
     * @param callback Function to call when string data is received
     */
    void set_string_data_callback(StringDataCallback callback);

    /**
     * Set JSON data callback (called when JSON data is received)
     * @param callback Function to call when JSON data is received
     */
    void set_json_data_callback(JsonDataCallback callback);

    /**
     * Set disconnect callback (called when a peer disconnects)
     * @param callback Function to call on disconnections
     */
    void set_disconnect_callback(DisconnectCallback callback);
    
    /**
     * Set NAT traversal progress callback
     * @param callback Function to call with NAT traversal progress updates
     */
    void set_nat_traversal_progress_callback(NatTraversalProgressCallback callback);
    
    /**
     * Set ICE candidate discovered callback
     * @param callback Function to call when ICE candidates are discovered
     */
    void set_ice_candidate_callback(IceCandidateDiscoveredCallback callback);

    // =========================================================================
    // Peer Discovery Methods
    // =========================================================================
    
    // DHT Discovery
    /**
     * Start DHT discovery on specified port
     * @param dht_port Port for DHT communication (default: 6881)
     * @return true if started successfully
     */
    bool start_dht_discovery(int dht_port = 6881);

    /**
     * Stop DHT discovery
     */
    void stop_dht_discovery();

    /**
     * Find peers by content hash using DHT
     * @param content_hash Hash to search for (40-character hex string)
     * @param callback Function to call with discovered peers
     * @param iteration_max Maximum DHT iterations (default: 1)
     * @return true if search initiated successfully
     */
    bool find_peers_by_hash(const std::string& content_hash, 
                           std::function<void(const std::vector<std::string>&)> callback,
                           int iteration_max = 1);

    /**
     * Announce our presence for a content hash
     * @param content_hash Hash to announce for (40-character hex string)
     * @param port Port to announce (default: our listen port)
     * @return true if announced successfully
     */
    bool announce_for_hash(const std::string& content_hash, uint16_t port = 0);

    /**
     * Check if DHT is currently running
     * @return true if DHT is running
     */
    bool is_dht_running() const;

    /**
     * Get the size of the DHT routing table
     * @return Number of nodes in routing table
     */
    size_t get_dht_routing_table_size() const;

    // mDNS Discovery
    /**
     * Start mDNS service discovery and announcement
     * @param service_instance_name Service instance name (optional)
     * @param txt_records Additional TXT records for service announcement
     * @return true if started successfully
     */
    bool start_mdns_discovery(const std::string& service_instance_name = "", 
                             const std::map<std::string, std::string>& txt_records = {});

    /**
     * Stop mDNS discovery
     */
    void stop_mdns_discovery();

    /**
     * Check if mDNS is currently running
     * @return true if mDNS is running
     */
    bool is_mdns_running() const;

    /**
     * Set mDNS service discovery callback
     * @param callback Function to call when services are discovered
     */
    void set_mdns_callback(std::function<void(const std::string&, int, const std::string&)> callback);

    /**
     * Get recently discovered mDNS services
     * @return Vector of discovered services
     */
    std::vector<MdnsService> get_mdns_services() const;

    /**
     * Manually query for mDNS services
     * @return true if query sent successfully
     */
    bool query_mdns_services();

    // Automatic Discovery
    /**
     * Start automatic peer discovery
     */
    void start_automatic_peer_discovery();
    
    /**
     * Stop automatic peer discovery
     */
    void stop_automatic_peer_discovery();
    
    /**
     * Check if automatic discovery is running
     * @return true if automatic discovery is running
     */
    bool is_automatic_discovery_running() const;
    
    /**
     * Get the discovery hash for current protocol configuration
     * @return Discovery hash based on current protocol name and version
     */
    std::string get_discovery_hash() const;
    
    /**
     * Get the well-known RATS peer discovery hash
     * @return Standard RATS discovery hash
     */
    static std::string get_rats_peer_discovery_hash();

    // =========================================================================
    // NAT Traversal and STUN Functionality
    // =========================================================================
    
    /**
     * Discover public IP address using STUN and add to ignore list
     * @param stun_server STUN server hostname (default: Google STUN)
     * @param stun_port STUN server port (default: 19302)
     * @return true if successful, false otherwise
     */
    bool discover_and_ignore_public_ip(const std::string& stun_server = "stun.l.google.com", int stun_port = 19302);
    
    /**
     * Detect NAT type using STUN servers
     * @return Detected NAT type
     */
    NatType detect_nat_type();
    
    /**
     * Get detailed NAT characteristics
     * @return Detailed NAT information
     */
    NatTypeInfo get_nat_characteristics();
    
    /**
     * Get the discovered public IP address
     * @return Public IP address string or empty if not discovered
     */
    std::string get_public_ip() const;
    
    /**
     * Add an IP address to the ignore list
     * @param ip_address IP address to ignore
     */
    void add_ignored_address(const std::string& ip_address);
    
    /**
     * Perform coordinated hole punching with a peer
     * @param peer_ip Peer IP address
     * @param peer_port Peer port
     * @param coordination_data Coordination data from peer
     * @return true if successful
     */
    bool coordinate_hole_punching(const std::string& peer_ip, uint16_t peer_port,
                                 const nlohmann::json& coordination_data);
    
    /**
     * Get NAT traversal statistics
     * @return JSON object with NAT traversal statistics
     */
    nlohmann::json get_nat_traversal_statistics() const;

    // =========================================================================
    // Protocol Configuration
    // =========================================================================
    
    /**
     * Set custom protocol name for handshakes and DHT discovery
     * @param protocol_name Custom protocol name (default: "rats")
     */
    void set_protocol_name(const std::string& protocol_name);

    /**
     * Set custom protocol version for handshakes
     * @param protocol_version Custom protocol version (default: "1.0")
     */
    void set_protocol_version(const std::string& protocol_version);

    /**
     * Get current protocol name
     * @return Current protocol name
     */
    std::string get_protocol_name() const;

    /**
     * Get current protocol version
     * @return Current protocol version
     */
    std::string get_protocol_version() const;

    // =========================================================================
    // Message Exchange API
    // =========================================================================
    
    /**
     * Register a persistent message handler
     * @param message_type Type of message to handle
     * @param callback Function to call when message is received
     */
    void on(const std::string& message_type, MessageCallback callback);

    /**
     * Register a one-time message handler
     * @param message_type Type of message to handle
     * @param callback Function to call when message is received (once only)
     */
    void once(const std::string& message_type, MessageCallback callback);

    /**
     * Remove all handlers for a message type
     * @param message_type Type of message to stop handling
     */
    void off(const std::string& message_type);

    /**
     * Send a message to all peers
     * @param message_type Type of message
     * @param data Message data
     * @param callback Optional callback for send result
     */
    void send(const std::string& message_type, const nlohmann::json& data, SendCallback callback = nullptr);

    /**
     * Send a message to a specific peer
     * @param peer_id Target peer ID
     * @param message_type Type of message
     * @param data Message data
     * @param callback Optional callback for send result
     */
    void send(const std::string& peer_id, const std::string& message_type, const nlohmann::json& data, SendCallback callback = nullptr);

    /**
     * Parse a JSON message
     * @param message Raw message string
     * @param out_json Parsed JSON output
     * @return true if parsed successfully
     */
    bool parse_json_message(const std::string& message, nlohmann::json& out_json);

    // =========================================================================
    // Encryption Functionality
    // =========================================================================
    
    /**
     * Initialize encryption system
     * @param enable Whether to enable encryption
     * @return true if successful
     */
    bool initialize_encryption(bool enable);

    /**
     * Set encryption enabled/disabled
     * @param enabled Whether encryption should be enabled
     */
    void set_encryption_enabled(bool enabled);

    /**
     * Check if encryption is enabled
     * @return true if encryption is enabled
     */
    bool is_encryption_enabled() const;

    /**
     * Get the encryption key as hex string
     * @return Encryption key in hex format
     */
    std::string get_encryption_key() const;

    /**
     * Set encryption key from hex string
     * @param key_hex Encryption key in hex format
     * @return true if key was valid and set
     */
    bool set_encryption_key(const std::string& key_hex);

    /**
     * Generate a new encryption key
     * @return New encryption key in hex format
     */
    std::string generate_new_encryption_key();

    /**
     * Check if a peer connection is encrypted
     * @param peer_id Peer ID to check
     * @return true if peer connection is encrypted
     */
    bool is_peer_encrypted(const std::string& peer_id) const;

    // =========================================================================
    // Configuration Persistence
    // =========================================================================
    
    /**
     * Load configuration from files
     * @return true if successful, false otherwise
     */
    bool load_configuration();

    /**
     * Save configuration to files
     * @return true if successful, false otherwise
     */
    bool save_configuration();

    /**
     * Set directory where data files will be stored
     * @param directory_path Path to directory (default: current folder)
     * @return true if directory is accessible, false otherwise
     */
    bool set_data_directory(const std::string& directory_path);

    /**
     * Get current data directory path
     * @return Current data directory path
     */
    std::string get_data_directory() const;

    /**
     * Load saved peers and attempt to reconnect
     * @return Number of connection attempts made
     */
    int load_and_reconnect_peers();

    /**
     * Load historical peers from a file
     * @return true if successful, false otherwise
     */
    bool load_historical_peers();

    /**
     * Save current peers to a historical file
     * @return true if successful, false otherwise
     */
    bool save_historical_peers();

    /**
     * Clear all historical peers
     */
    void clear_historical_peers();

    /**
     * Get all historical peers
     * @return Vector of RatsPeer objects
     */
    std::vector<RatsPeer> get_historical_peers() const;

    // =========================================================================
    // Statistics and Information
    // =========================================================================
    
    /**
     * Get connection statistics
     * @return JSON object with detailed statistics
     */
    nlohmann::json get_connection_statistics() const;

    // =========================================================================
    // GossipSub Functionality
    // =========================================================================
    
    /**
     * Get GossipSub instance for publish-subscribe messaging
     * @return Reference to GossipSub instance
     */
    GossipSub& get_gossipsub();
    
    /**
     * Check if GossipSub is available
     * @return true if GossipSub is initialized
     */
    bool is_gossipsub_available() const;
    
    // Topic Management
    /**
     * Subscribe to a GossipSub topic
     * @param topic Topic name to subscribe to
     * @return true if subscription successful
     */
    bool subscribe_to_topic(const std::string& topic);
    
    /**
     * Unsubscribe from a GossipSub topic
     * @param topic Topic name to unsubscribe from
     * @return true if unsubscription successful
     */
    bool unsubscribe_from_topic(const std::string& topic);
    
    /**
     * Check if subscribed to a GossipSub topic
     * @param topic Topic name to check
     * @return true if subscribed
     */
    bool is_subscribed_to_topic(const std::string& topic) const;
    
    /**
     * Get list of subscribed GossipSub topics
     * @return Vector of topic names
     */
    std::vector<std::string> get_subscribed_topics() const;
    
    // Publishing
    /**
     * Publish a message to a GossipSub topic
     * @param topic Topic to publish to
     * @param message Message content
     * @return true if published successfully
     */
    bool publish_to_topic(const std::string& topic, const std::string& message);
    
    /**
     * Publish a JSON message to a GossipSub topic
     * @param topic Topic to publish to
     * @param message JSON message content
     * @return true if published successfully
     */
    bool publish_json_to_topic(const std::string& topic, const nlohmann::json& message);
    
    // Event Handlers (Unified API)
    /**
     * Set a message handler for a GossipSub topic using unified event API pattern
     * @param topic Topic name
     * @param callback Function to call when messages are received (peer_id, topic, message_content)
     */
    void on_topic_message(const std::string& topic, std::function<void(const std::string&, const std::string&, const std::string&)> callback);
    
    /**
     * Set a JSON message handler for a GossipSub topic using unified event API pattern
     * @param topic Topic name  
     * @param callback Function to call when JSON messages are received (peer_id, topic, json_message)
     */
    void on_topic_json_message(const std::string& topic, std::function<void(const std::string&, const std::string&, const nlohmann::json&)> callback);
    
    /**
     * Set a peer joined handler for a GossipSub topic using unified event API pattern
     * @param topic Topic name
     * @param callback Function to call when peers join the topic
     */
    void on_topic_peer_joined(const std::string& topic, std::function<void(const std::string&, const std::string&)> callback);
    
    /**
     * Set a peer left handler for a GossipSub topic using unified event API pattern  
     * @param topic Topic name
     * @param callback Function to call when peers leave the topic
     */
    void on_topic_peer_left(const std::string& topic, std::function<void(const std::string&, const std::string&)> callback);
    
    /**
     * Set a message validator for a GossipSub topic
     * @param topic Topic name (empty for global validator)
     * @param validator Validation function returning ACCEPT, REJECT, or IGNORE_MSG
     */
    void set_topic_message_validator(const std::string& topic, std::function<ValidationResult(const std::string&, const std::string&, const std::string&)> validator);
    
    /**
     * Remove all event handlers for a GossipSub topic
     * @param topic Topic name
     */
    void off_topic(const std::string& topic);
    
    // Information
    /**
     * Get peers subscribed to a GossipSub topic
     * @param topic Topic name
     * @return Vector of peer IDs
     */
    std::vector<std::string> get_topic_peers(const std::string& topic) const;
    
    /**
     * Get mesh peers for a GossipSub topic
     * @param topic Topic name  
     * @return Vector of peer IDs in the mesh
     */
    std::vector<std::string> get_topic_mesh_peers(const std::string& topic) const;
    
    /**
     * Get GossipSub statistics
     * @return JSON object with comprehensive GossipSub statistics
     */
    nlohmann::json get_gossipsub_statistics() const;
    
    /**
     * Check if GossipSub is running
     * @return true if GossipSub service is active
     */
    bool is_gossipsub_running() const;

    // =========================================================================
    // Logging Control API
    // =========================================================================
    
    /**
     * Enable or disable file logging
     * When enabled, logs will be written to "rats.log" by default
     * @param enabled Whether to enable file logging
     */
    void set_logging_enabled(bool enabled);
    
    /**
     * Check if file logging is currently enabled
     * @return true if file logging is enabled
     */
    bool is_logging_enabled() const;
    
    /**
     * Set the log file path
     * @param file_path Path to the log file (default: "rats.log")
     */
    void set_log_file_path(const std::string& file_path);
    
    /**
     * Get the current log file path
     * @return Current log file path
     */
    std::string get_log_file_path() const;
    
    /**
     * Set the minimum log level
     * @param level Minimum log level (DEBUG=0, INFO=1, WARN=2, ERROR=3)
     */
    void set_log_level(LogLevel level);
    
    /**
     * Set the minimum log level using string
     * @param level_str Log level as string ("DEBUG", "INFO", "WARN", "ERROR")
     */
    void set_log_level(const std::string& level_str);
    
    /**
     * Get the current log level
     * @return Current minimum log level
     */
    LogLevel get_log_level() const;
    
    /**
     * Enable or disable colored log output
     * @param enabled Whether to enable colored output
     */
    void set_log_colors_enabled(bool enabled);
    
    /**
     * Check if colored log output is enabled
     * @return true if colors are enabled
     */
    bool is_log_colors_enabled() const;
    
    /**
     * Enable or disable timestamps in log output
     * @param enabled Whether to enable timestamps
     */
    void set_log_timestamps_enabled(bool enabled);
    
    /**
     * Check if timestamps are enabled in log output
     * @return true if timestamps are enabled
     */
    bool is_log_timestamps_enabled() const;
    
    /**
     * Set log file rotation size
     * @param max_size_bytes Maximum size in bytes before log rotation (default: 10MB)
     */
    void set_log_rotation_size(size_t max_size_bytes);
    
    /**
     * Set the number of log files to retain during rotation
     * @param count Number of old log files to keep (default: 5)
     */
    void set_log_retention_count(int count);
    
    /**
     * Clear/reset the current log file
     */
    void clear_log_file();

    // =========================================================================
    // File Transfer API
    // =========================================================================
    
    /**
     * Get the file transfer manager instance
     * @return Reference to the file transfer manager
     */
    FileTransferManager& get_file_transfer_manager();
    
    /**
     * Check if file transfer is available
     * @return true if file transfer manager is initialized
     */
    bool is_file_transfer_available() const;
    
    // Sending and Requesting
    /**
     * Send a file to a peer
     * @param peer_id Target peer ID
     * @param file_path Local file path to send
     * @param remote_filename Optional remote filename (default: use local name)
     * @return Transfer ID if successful, empty string if failed
     */
    std::string send_file(const std::string& peer_id, const std::string& file_path, 
                         const std::string& remote_filename = "");
    
    /**
     * Send an entire directory to a peer
     * @param peer_id Target peer ID
     * @param directory_path Local directory path to send
     * @param remote_directory_name Optional remote directory name
     * @param recursive Whether to include subdirectories (default: true)
     * @return Transfer ID if successful, empty string if failed
     */
    std::string send_directory(const std::string& peer_id, const std::string& directory_path,
                              const std::string& remote_directory_name = "", bool recursive = true);
    
    /**
     * Request a file from a remote peer
     * @param peer_id Target peer ID
     * @param remote_file_path Path to file on remote peer
     * @param local_path Local path where file should be saved
     * @return Transfer ID if successful, empty string if failed
     */
    std::string request_file(const std::string& peer_id, const std::string& remote_file_path,
                            const std::string& local_path);
    
    /**
     * Request a directory from a remote peer
     * @param peer_id Target peer ID
     * @param remote_directory_path Path to directory on remote peer
     * @param local_directory_path Local path where directory should be saved
     * @param recursive Whether to include subdirectories (default: true)
     * @return Transfer ID if successful, empty string if failed
     */
    std::string request_directory(const std::string& peer_id, const std::string& remote_directory_path,
                                 const std::string& local_directory_path, bool recursive = true);
    
    // Accept/Reject Operations
    /**
     * Accept an incoming file transfer
     * @param transfer_id Transfer identifier from request
     * @param local_path Local path where file should be saved
     * @return true if accepted successfully
     */
    bool accept_file_transfer(const std::string& transfer_id, const std::string& local_path);
    
    /**
     * Reject an incoming file transfer
     * @param transfer_id Transfer identifier from request
     * @param reason Optional reason for rejection
     * @return true if rejected successfully
     */
    bool reject_file_transfer(const std::string& transfer_id, const std::string& reason = "");
    
    /**
     * Accept an incoming directory transfer
     * @param transfer_id Transfer identifier from request
     * @param local_path Local path where directory should be saved
     * @return true if accepted successfully
     */
    bool accept_directory_transfer(const std::string& transfer_id, const std::string& local_path);
    
    /**
     * Reject an incoming directory transfer
     * @param transfer_id Transfer identifier from request
     * @param reason Optional reason for rejection
     * @return true if rejected successfully
     */
    bool reject_directory_transfer(const std::string& transfer_id, const std::string& reason = "");
    
    // Transfer Control
    /**
     * Pause an active file transfer
     * @param transfer_id Transfer to pause
     * @return true if paused successfully
     */
    bool pause_file_transfer(const std::string& transfer_id);
    
    /**
     * Resume a paused file transfer
     * @param transfer_id Transfer to resume
     * @return true if resumed successfully
     */
    bool resume_file_transfer(const std::string& transfer_id);
    
    /**
     * Cancel an active or paused file transfer
     * @param transfer_id Transfer to cancel
     * @return true if cancelled successfully
     */
    bool cancel_file_transfer(const std::string& transfer_id);
    
    // Information and Monitoring
    /**
     * Get file transfer progress information
     * @param transfer_id Transfer to query
     * @return Progress information or nullptr if not found
     */
    std::shared_ptr<FileTransferProgress> get_file_transfer_progress(const std::string& transfer_id) const;
    
    /**
     * Get all active file transfers
     * @return Vector of transfer progress objects
     */
    std::vector<std::shared_ptr<FileTransferProgress>> get_active_file_transfers() const;
    
    /**
     * Get file transfer statistics
     * @return JSON object with transfer statistics
     */
    nlohmann::json get_file_transfer_statistics() const;
    
    /**
     * Set file transfer configuration
     * @param config Transfer configuration settings
     */
    void set_file_transfer_config(const FileTransferConfig& config);
    
    /**
     * Get current file transfer configuration
     * @return Current configuration settings
     */
    const FileTransferConfig& get_file_transfer_config() const;
    
    // Event Handlers
    /**
     * Set file transfer progress callback
     * @param callback Function to call with progress updates
     */
    void on_file_transfer_progress(FileTransferProgressCallback callback);
    
    /**
     * Set file transfer completion callback
     * @param callback Function to call when transfers complete
     */
    void on_file_transfer_completed(FileTransferCompletedCallback callback);
    
    /**
     * Set incoming file transfer request callback
     * @param callback Function to call when receiving transfer requests
     */
    void on_file_transfer_request(FileTransferRequestCallback callback);
    
    /**
     * Set directory transfer progress callback
     * @param callback Function to call with directory transfer progress
     */
    void on_directory_transfer_progress(DirectoryTransferProgressCallback callback);
    
    /**
     * Set file request callback (called when receiving file requests)
     * @param callback Function to call when receiving file requests
     */
    void on_file_request(FileRequestCallback callback);
    
    /**
     * Set directory request callback (called when receiving directory requests)
     * @param callback Function to call when receiving directory requests
     */
    void on_directory_request(DirectoryRequestCallback callback);

private:
    int listen_port_;
    std::string bind_address_;
    int max_peers_;
    socket_t server_socket_;
    std::atomic<bool> running_;
    
    // NAT traversal configuration
    NatTraversalConfig nat_config_;
    
    // Configuration persistence
    std::string our_peer_id_;                               // Our persistent peer ID
    std::string data_directory_;                            // Directory where data files are stored
    mutable std::mutex config_mutex_;                       // Protects configuration data
    static const std::string CONFIG_FILE_NAME;             // "config.json"
    static const std::string PEERS_FILE_NAME;              // "peers.rats"
    static const std::string PEERS_EVER_FILE_NAME;         // "peers_ever.rats"
    
    // Encryption state
    NoiseKey static_encryption_key_;                        // Our static encryption key
    bool encryption_enabled_;                               // Whether encryption is enabled
    mutable std::mutex encryption_mutex_;                   // Protects encryption state
    
    // ICE and NAT traversal
    std::unique_ptr<IceAgent> ice_agent_;                   // ICE agent for NAT traversal
    std::unique_ptr<AdvancedNatDetector> nat_detector_;     // Advanced NAT type detection
    NatType detected_nat_type_;                             // Our detected NAT type
    NatTypeInfo nat_characteristics_;                       // Detailed NAT information
    mutable std::mutex nat_mutex_;                          // Protects NAT-related data
    
    // ICE coordination tracking to prevent duplicate attempts
    std::unordered_set<std::string> ice_coordination_in_progress_;  // Set of peer_ids having ICE coordination
    mutable std::mutex ice_coordination_mutex_;                     // Protects ICE coordination state
    
    // Connection attempt tracking
    std::unordered_map<std::string, std::vector<ConnectionAttemptResult>> connection_attempts_;
    mutable std::mutex connection_attempts_mutex_;
    
    // Organized peer management using RatsPeer struct
    mutable std::mutex peers_mutex_;
    std::unordered_map<std::string, RatsPeer> peers_;          // keyed by peer_id
    std::unordered_map<socket_t, std::string> socket_to_peer_id_;  // for quick socket->peer_id lookup  
    std::unordered_map<std::string, std::string> address_to_peer_id_;  // for duplicate detection (normalized_address->peer_id)
    
    // Per-socket synchronization for thread-safe message sending
    mutable std::mutex socket_send_mutexes_mutex_;
    std::unordered_map<socket_t, std::shared_ptr<std::mutex>> socket_send_mutexes_;
    
    // Server and client management
    std::thread server_thread_;
    std::thread management_thread_;
    
    ConnectionCallback connection_callback_;
    AdvancedConnectionCallback advanced_connection_callback_;
    BinaryDataCallback binary_data_callback_;
    StringDataCallback string_data_callback_;
    JsonDataCallback json_data_callback_;
    DisconnectCallback disconnect_callback_;
    NatTraversalProgressCallback nat_progress_callback_;
    IceCandidateDiscoveredCallback ice_candidate_callback_;
    
    // DHT client for peer discovery
    std::unique_ptr<DhtClient> dht_client_;
    
    // STUN client for public IP discovery
    std::unique_ptr<StunClient> stun_client_;
    std::string public_ip_;
    mutable std::mutex public_ip_mutex_;
    
    // mDNS client for local network discovery
    std::unique_ptr<MdnsClient> mdns_client_;
    std::function<void(const std::string&, int, const std::string&)> mdns_callback_;
    
    // GossipSub for publish-subscribe messaging
    std::unique_ptr<GossipSub> gossipsub_;
    
    // File transfer manager
    std::unique_ptr<FileTransferManager> file_transfer_manager_;
    
    void initialize_modules();
    void destroy_modules();

    void server_loop();
    void management_loop();
    void handle_client(socket_t client_socket, const std::string& peer_hash_id);
    void remove_peer(socket_t socket);
    std::string generate_peer_hash_id(socket_t socket, const std::string& connection_info);
    void handle_dht_peer_discovery(const std::vector<Peer>& peers, const InfoHash& info_hash);
    void handle_mdns_service_discovery(const MdnsService& service, bool is_new);
    
    // Message header helpers
    std::vector<uint8_t> create_message_with_header(const std::vector<uint8_t>& payload, MessageDataType type);
    bool parse_message_with_header(const std::vector<uint8_t>& message, MessageHeader& header, std::vector<uint8_t>& payload) const;
    
    // Enhanced connection establishment
    bool attempt_direct_connection(const std::string& host, int port, ConnectionAttemptResult& result);
    bool attempt_stun_assisted_connection(const std::string& host, int port, ConnectionAttemptResult& result);
    bool attempt_ice_connection(const std::string& host, int port, ConnectionAttemptResult& result);
    bool attempt_turn_relay_connection(const std::string& host, int port, ConnectionAttemptResult& result);
    bool attempt_hole_punch_connection(const std::string& host, int port, ConnectionAttemptResult& result);
    
    // ICE coordination helpers
    void handle_ice_candidate_discovered(const std::string& peer_id, const IceCandidate& candidate);
    void handle_ice_connection_state_change(const std::string& peer_id, IceConnectionState state);
    void initiate_ice_with_peer(const std::string& peer_id, const std::string& host, int port);
    
    // NAT traversal message handlers
    void handle_ice_offer_message(socket_t socket, const std::string& peer_hash_id, const nlohmann::json& payload);
    void handle_ice_answer_message(socket_t socket, const std::string& peer_hash_id, const nlohmann::json& payload);
    void handle_ice_candidate_message(socket_t socket, const std::string& peer_hash_id, const nlohmann::json& payload);
    void handle_hole_punch_coordination_message(socket_t socket, const std::string& peer_hash_id, const nlohmann::json& payload);
    void handle_nat_info_exchange_message(socket_t socket, const std::string& peer_hash_id, const nlohmann::json& payload);
    void send_nat_info_to_peer(socket_t socket, const std::string& peer_id);
    
    // New peer management methods using RatsPeer
    void add_peer(const RatsPeer& peer);
    void add_peer_unlocked(const RatsPeer& peer);  // Assumes peers_mutex_ is already locked
    void remove_peer_by_id(const std::string& peer_id);
    void remove_peer_by_id_unlocked(const std::string& peer_id);  // Assumes peers_mutex_ is already locked
    bool is_already_connected_to_address(const std::string& normalized_address) const;
    std::string normalize_peer_address(const std::string& ip, int port) const;

    // Local interface address blocking (ignore list)
    std::vector<std::string> local_interface_addresses_;
    mutable std::mutex local_addresses_mutex_;
    void initialize_local_addresses();
    void refresh_local_addresses();
    bool is_blocked_address(const std::string& ip_address) const;
    bool should_ignore_peer(const std::string& ip, int port) const;
    static bool parse_address_string(const std::string& address_str, std::string& out_ip, int& out_port);
    
    // Helper functions that assume mutex is already locked
    int get_peer_count_unlocked() const;  // Helper that assumes peers_mutex_ is already locked

    // Handshake protocol
    static constexpr const char* RATS_PROTOCOL_VERSION = "1.0";
    static constexpr int HANDSHAKE_TIMEOUT_SECONDS = 10;

    // Custom protocol configuration
    std::string custom_protocol_name_;          // Custom protocol name (default: "rats")
    std::string custom_protocol_version_;       // Custom protocol version (default: "1.0")
    mutable std::mutex protocol_config_mutex_;  // Protects protocol configuration

    struct HandshakeMessage {
        std::string protocol;
        std::string version;
        std::string peer_id;
        std::string message_type;
        int64_t timestamp;
    };

    std::string create_handshake_message(const std::string& message_type, const std::string& our_peer_id) const;
    bool parse_handshake_message(const std::string& message, HandshakeMessage& out_msg) const;
    bool validate_handshake_message(const HandshakeMessage& msg) const;
    bool is_handshake_message(const std::string& message) const;
    bool send_handshake(socket_t socket, const std::string& our_peer_id);
    bool send_handshake_unlocked(socket_t socket, const std::string& our_peer_id);
    bool handle_handshake_message(socket_t socket, const std::string& peer_hash_id, const std::string& message);
    void check_handshake_timeouts();
    void log_handshake_completion(const RatsPeer& peer);
    void log_handshake_completion_unlocked(const RatsPeer& peer);

    // Automatic discovery
    std::atomic<bool> auto_discovery_running_;
    std::thread auto_discovery_thread_;
    void automatic_discovery_loop();
    void announce_rats_peer();
    void search_rats_peers(int iteration_max = 1);

    // Message handling system
    nlohmann::json create_rats_message(const std::string& type, const nlohmann::json& payload, const std::string& sender_peer_id);
    void handle_rats_message(socket_t socket, const std::string& peer_hash_id, const nlohmann::json& message);

    // Specific message handlers
    void handle_peer_exchange_message(socket_t socket, const std::string& peer_hash_id, const nlohmann::json& payload);
    void handle_peers_request_message(socket_t socket, const std::string& peer_hash_id, const nlohmann::json& payload);
    void handle_peers_response_message(socket_t socket, const std::string& peer_hash_id, const nlohmann::json& payload);

    // Message creation and broadcasting
    nlohmann::json create_peer_exchange_message(const RatsPeer& peer);
    void broadcast_peer_exchange_message(const RatsPeer& new_peer);
    nlohmann::json create_peers_request_message(const std::string& sender_peer_id);
    nlohmann::json create_peers_response_message(const std::vector<RatsPeer>& peers, const std::string& sender_peer_id);
    std::vector<RatsPeer> get_random_peers(int max_count, const std::string& exclude_peer_id = "") const;
    void send_peers_request(socket_t socket, const std::string& our_peer_id);

    int broadcast_rats_message(const nlohmann::json& message, const std::string& exclude_peer_id = "");
    int broadcast_rats_message_to_validated_peers(const nlohmann::json& message, const std::string& exclude_peer_id = "");
    // Message exchange API implementation
    struct MessageHandler {
        MessageCallback callback;
        bool is_once;
        
        MessageHandler(MessageCallback cb, bool once) : callback(cb), is_once(once) {}
    };
    
    std::unordered_map<std::string, std::vector<MessageHandler>> message_handlers_;
    mutable std::mutex message_handlers_mutex_;
    
    void call_message_handlers(const std::string& message_type, const std::string& peer_id, const nlohmann::json& data);
    void call_callback_safely(const MessageCallback& callback, const std::string& peer_id, const nlohmann::json& data);
    void remove_once_handlers(const std::string& message_type);

    // Per-socket synchronization helpers
    std::shared_ptr<std::mutex> get_socket_send_mutex(socket_t socket);
    void cleanup_socket_send_mutex(socket_t socket);

    // Configuration persistence helpers
    std::string generate_persistent_peer_id() const;
    nlohmann::json serialize_peer_for_persistence(const RatsPeer& peer) const;
    bool deserialize_peer_from_persistence(const nlohmann::json& json, std::string& ip, int& port, std::string& peer_id) const;
    std::string get_config_file_path() const;
    std::string get_peers_file_path() const;
    std::string get_peers_ever_file_path() const;
    bool save_peers_to_file();
    bool append_peer_to_historical_file(const RatsPeer& peer);
    int load_and_reconnect_historical_peers();
    
    // NAT traversal helpers
    void initialize_nat_traversal();
    void detect_and_cache_nat_type();
    void update_connection_statistics(const std::string& peer_id, const ConnectionAttemptResult& result);
    std::string select_best_connection_strategy(const std::string& host, int port);
    NatType map_characteristics_to_nat_type(const NatTypeInfo& characteristics);
    void log_nat_detection_results();
    bool perform_tcp_connection(const std::string& host, int port, ConnectionAttemptResult& result);
    
    // ICE coordination helpers
    void initialize_ice_agent();
    void setup_ice_callbacks();
    void update_peer_ice_info(socket_t socket, const nlohmann::json& payload);
    void add_candidate_to_peer(socket_t socket, const IceCandidate& candidate);
    bool should_initiate_ice_coordination(const std::string& peer_id);
    void mark_ice_coordination_in_progress(const std::string& peer_id);
    void remove_ice_coordination_tracking(const std::string& peer_id);
    void cleanup_ice_coordination_for_peer(const std::string& peer_id);
    nlohmann::json get_ice_statistics() const;
    bool is_ice_enabled() const;
    bool is_peer_ice_connected(const std::string& peer_id) const;
    void cleanup_ice_resources();
};

// Utility functions
std::unique_ptr<RatsClient> create_rats_client(int listen_port);

// Library version query (stable, binding-friendly)
RATS_API const char* rats_get_library_version_string();
RATS_API void rats_get_library_version(int* major, int* minor, int* patch, int* build);
RATS_API const char* rats_get_library_git_describe();
RATS_API uint32_t rats_get_library_abi(); // packed as (major<<16)|(minor<<8)|patch

} // namespace librats 