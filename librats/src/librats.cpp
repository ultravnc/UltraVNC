#include "librats.h"
#include "gossipsub.h"
#include "sha1.h"
#include "os.h"
#include "network_utils.h"
#include "fs.h"
#include "json.hpp" // nlohmann::json
#include "version.h"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <memory>
#include <random>
#include <sstream>
#include <iomanip>
#include <stdexcept>

#ifdef TESTING
#define LOG_CLIENT_DEBUG(message) LOG_DEBUG("client", "[pointer: " << this << "] " << message)
#define LOG_CLIENT_INFO(message)  LOG_INFO("client", "[pointer: " << this << "] " << message)
#define LOG_CLIENT_WARN(message)  LOG_WARN("client", "[pointer: " << this << "] " << message)
#define LOG_CLIENT_ERROR(message) LOG_ERROR("client", "[pointer: " << this << "] " << message)

#define LOG_SERVER_DEBUG(message) LOG_DEBUG("server", "[pointer: " << this << "] " << message)
#define LOG_SERVER_INFO(message)  LOG_INFO("server", "[pointer: " << this << "] " << message)
#define LOG_SERVER_WARN(message)  LOG_WARN("server", "[pointer: " << this << "] " << message)
#define LOG_SERVER_ERROR(message) LOG_ERROR("server", "[pointer: " << this << "] " << message)
#else
#define LOG_CLIENT_DEBUG(message) LOG_DEBUG("client", message)
#define LOG_CLIENT_INFO(message)  LOG_INFO("client", message)
#define LOG_CLIENT_WARN(message)  LOG_WARN("client", message)
#define LOG_CLIENT_ERROR(message) LOG_ERROR("client", message)

#define LOG_SERVER_DEBUG(message) LOG_DEBUG("server", message)
#define LOG_SERVER_INFO(message)  LOG_INFO("server", message)
#define LOG_SERVER_WARN(message)  LOG_WARN("server", message)
#define LOG_SERVER_ERROR(message) LOG_ERROR("server", message)
#endif

namespace librats {

// Configuration file constants
const std::string RatsClient::CONFIG_FILE_NAME = "config.json";
const std::string RatsClient::PEERS_FILE_NAME = "peers.rats";
const std::string RatsClient::PEERS_EVER_FILE_NAME = "peers_ever.rats";

// =========================================================================
// Constructor and Destructor
// =========================================================================

RatsClient::RatsClient(int listen_port, int max_peers, const NatTraversalConfig& nat_config, const std::string& bind_address) 
    : listen_port_(listen_port), 
      bind_address_(bind_address),
      max_peers_(max_peers),
      nat_config_(nat_config),
      server_socket_(INVALID_SOCKET_VALUE),
      running_(false),
      auto_discovery_running_(false),
      encryption_enabled_(false),
      detected_nat_type_(NatType::UNKNOWN),
      data_directory_("."),
      custom_protocol_name_("rats"),
      custom_protocol_version_("1.0") {
    // Initialize STUN client
    stun_client_ = std::make_unique<StunClient>();
    
    // Initialize NAT detector for advanced NAT characteristics detection
    nat_detector_ = std::make_unique<AdvancedNatDetector>();
    
    // Initialize ICE agent if enabled
    initialize_ice_agent();
    
    // Generate encryption key
    static_encryption_key_ = encrypted_communication::generate_node_key();
    
    // Load configuration (this will generate peer ID if needed)
    load_configuration();
    
    // Initialize modules
    initialize_modules();
    
    // Initialize NAT traversal
    initialize_nat_traversal();
}

RatsClient::~RatsClient() {
    stop();
    // Destroy modules
    destroy_modules();
}

// =========================================================================
// Modules Initialization and Destruction
// =========================================================================

void RatsClient::initialize_modules() {
    // Initialize GossipSub
    if (!gossipsub_) {
        LOG_CLIENT_INFO("Initializing GossipSub");
        gossipsub_ = std::make_unique<GossipSub>(*this);
    }

    // Initialize File Transfer Manager
    if (!file_transfer_manager_) {
        LOG_CLIENT_INFO("Initializing File Transfer Manager");
        file_transfer_manager_ = std::make_unique<FileTransferManager>(*this);
    }
}

void RatsClient::destroy_modules() {
    if (gossipsub_) {
        LOG_CLIENT_INFO("Destroying GossipSub");
        gossipsub_.reset();
    }

    if (file_transfer_manager_) {
        LOG_CLIENT_INFO("Destroying File Transfer Manager");
        file_transfer_manager_.reset();
    }
}

// =========================================================================
// Core Lifecycle Management
// =========================================================================


bool RatsClient::start() {
    if (running_.load()) {
        LOG_CLIENT_WARN("RatsClient is already running");
        return false;
    }

    LOG_CLIENT_INFO("Starting RatsClient on port " << listen_port_ <<
                   (bind_address_.empty() ? "" : " bound to " + bind_address_));
    
    // Print system information for debugging and log analysis
    SystemInfo sys_info = get_system_info();
    LOG_CLIENT_INFO("=== System Information ===");
    LOG_CLIENT_INFO("OS: " << sys_info.os_name << " " << sys_info.os_version);
    LOG_CLIENT_INFO("Architecture: " << sys_info.architecture);
    LOG_CLIENT_INFO("Hostname: " << sys_info.hostname);
    LOG_CLIENT_INFO("CPU: " << sys_info.cpu_model);
    LOG_CLIENT_INFO("CPU Cores: " << sys_info.cpu_cores << " physical, " << sys_info.cpu_logical_cores << " logical");
    LOG_CLIENT_INFO("Memory: " << sys_info.total_memory_mb << " MB total, " << sys_info.available_memory_mb << " MB available");
    LOG_CLIENT_INFO("===========================");
    
    // Initialize socket library first (required for all socket operations)
    init_socket_library();
    
    // Initialize encryption
    if (!initialize_encryption(encryption_enabled_)) {
        LOG_CLIENT_ERROR("Failed to initialize encryption");
        return false;
    }
    
    // Initialize local interface addresses for connection blocking
    initialize_local_addresses();
    
    // Discover public IP address via STUN and add to ignore list
    if (!discover_and_ignore_public_ip()) {
        LOG_CLIENT_WARN("Failed to discover public IP via STUN - continuing without it");
    }
    
    // Create dual-stack server socket (supports both IPv4 and IPv6)
   server_socket_ = create_tcp_server(listen_port_, 5, bind_address_);
    if (!is_valid_socket(server_socket_)) {
        LOG_CLIENT_ERROR("Failed to create dual-stack server socket on port " << listen_port_ <<
                        (bind_address_.empty() ? "" : " bound to " + bind_address_));
        return false;
    }
    
    // Update listen_port_ with actual bound port if ephemeral port was requested
    if (listen_port_ == 0) {
        listen_port_ = get_ephemeral_port(server_socket_);
        if (listen_port_ == 0) {
            LOG_CLIENT_WARN("Failed to get actual bound port - using port 0");
        } else {
            LOG_CLIENT_INFO("Server bound to ephemeral port " << listen_port_);
        }
    }
    
    running_.store(true);
    
    // Start server thread
    server_thread_ = std::thread(&RatsClient::server_loop, this);
    
    // Start management thread
    management_thread_ = std::thread(&RatsClient::management_loop, this);
    
    // Start GossipSub
    if (gossipsub_ && !gossipsub_->start()) {
        LOG_CLIENT_WARN("Failed to start GossipSub - continuing without it");
    }
    
    LOG_CLIENT_INFO("RatsClient started successfully on port " << listen_port_);
    
    // Attempt to reconnect to saved peers
    add_managed_thread(std::thread([this]() {
        // Give the server some time to fully initialize
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        int reconnect_attempts = load_and_reconnect_peers();
        if (reconnect_attempts > 0) {
            LOG_CLIENT_INFO("Attempted to reconnect to " << reconnect_attempts << " saved peers");
        }
        
        // Also attempt to reconnect to historical peers if not at peer limit
        if (!is_peer_limit_reached()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Give current peers time to connect
            int historical_attempts = load_and_reconnect_historical_peers();
            if (historical_attempts > 0) {
                LOG_CLIENT_INFO("Attempted to reconnect to " << historical_attempts << " historical peers");
            }
        }
    }), "peer-reconnection");
    
    return true;
}

void RatsClient::stop() {
    if (!running_.load()) {
        return;
    }
    
    LOG_CLIENT_INFO("Stopping RatsClient");
    
    // Stop GossipSub (can broadcast stop message)
    if (gossipsub_) {
        gossipsub_->stop();
    }


    // Trigger immediate shutdown of all background threads
    shutdown_all_threads();
    
    // Stop DHT discovery (this will also stop automatic discovery)
    stop_dht_discovery();
    
    // Stop mDNS discovery
    stop_mdns_discovery();
    
    // Cleanup ICE resources
    cleanup_ice_resources();
    
    // Close server socket to break accept loop
    if (is_valid_socket(server_socket_)) {
        close_socket(server_socket_, true);
        server_socket_ = INVALID_SOCKET_VALUE;
    }
    
    // Close all peer connections
    {
        std::lock_guard<std::mutex> lock(peers_mutex_);
        LOG_CLIENT_INFO("Closing " << peers_.size() << " peer connections");
        for (const auto& pair : peers_) {
            const RatsPeer& peer = pair.second;
            close_socket(peer.socket, true);
        }
        peers_.clear();
        socket_to_peer_id_.clear();
        address_to_peer_id_.clear();
    }
    

    
    // Wait for server thread to finish
    if (server_thread_.joinable()) {
        LOG_CLIENT_DEBUG("Waiting for server thread to finish");
        server_thread_.join();
    }
    
    // Wait for management thread to finish
    if (management_thread_.joinable()) {
        LOG_CLIENT_DEBUG("Waiting for management thread to finish");
        management_thread_.join();
    }
    
    // Join all managed threads for graceful cleanup
    join_all_active_threads();
    
    cleanup_socket_library();

    // Save configuration before stopping
    save_configuration();
    
    LOG_CLIENT_INFO("RatsClient stopped successfully");
}

void RatsClient::shutdown_all_threads() {
    LOG_CLIENT_INFO("Initiating shutdown of all background threads");
    
    // Signal all threads to stop
    running_.store(false);

    // Call parent class to handle thread management shutdown
    ThreadManager::shutdown_all_threads();
}

bool RatsClient::is_running() const {
    return running_.load();
}

// =========================================================================
// Utility Methods
// =========================================================================

int RatsClient::get_listen_port() const {
    return listen_port_;
}

std::string RatsClient::get_bind_address() const {
    return bind_address_;
}

// =========================================================================
// Managment loops
// =========================================================================

void RatsClient::server_loop() {
    LOG_SERVER_INFO("Server loop started");
    
    while (running_.load()) {
        socket_t client_socket = accept_client(server_socket_);
        if (!is_valid_socket(client_socket)) {
            if (running_.load()) {
                LOG_SERVER_ERROR("Failed to accept client connection");
            }
            break;
        }
        
        // Get peer address information
        std::string peer_address = get_peer_address(client_socket);
        if (peer_address.empty()) {
            LOG_SERVER_ERROR("Failed to get peer address for incoming connection");
            close_socket(client_socket);
            continue;
        }
        
        // Parse IP and port from peer_address
        std::string ip;
        int port = 0;
        if (!parse_address_string(peer_address, ip, port)) {
            LOG_SERVER_ERROR("Failed to parse peer address from incoming connection: " << peer_address);
            close_socket(client_socket);
            continue;
        }
        
        std::string normalized_peer_address = normalize_peer_address(ip, port);
        
        // Check if peer limit is reached
        if (is_peer_limit_reached()) {
            LOG_SERVER_INFO("Peer limit reached (" << max_peers_ << "), rejecting connection from " << normalized_peer_address);
            close_socket(client_socket);
            continue;
        }
        
        // Check if we're already connected to this peer
        if (is_already_connected_to_address(normalized_peer_address)) {
            LOG_SERVER_INFO("Already connected to peer " << normalized_peer_address << ", rejecting duplicate connection");
            close_socket(client_socket);
            continue;
        }

        // Initialize encryption for incoming connection
        if (is_encryption_enabled()) {
            if (!encrypted_communication::initialize_incoming_connection(client_socket)) {
                LOG_SERVER_ERROR("Failed to initialize encryption for incoming connection from " << normalized_peer_address);
                close_socket(client_socket);
                continue;
            }
        }

        // Generate unique hash ID for this incoming client
        std::string connection_info = "incoming_from_" + peer_address;
        std::string peer_hash_id = generate_peer_hash_id(client_socket, connection_info); // Temporary hash ID (real hash ID will be set after handshake)
        
        // Create RatsPeer object for incoming connection
        {
            std::lock_guard<std::mutex> lock(peers_mutex_);
            RatsPeer new_peer(peer_hash_id, ip, port, client_socket, normalized_peer_address, false); // false = incoming connection
            new_peer.encryption_enabled = is_encryption_enabled();
            add_peer_unlocked(new_peer);
        }
        
        // Start a thread to handle this client
        LOG_SERVER_DEBUG("Starting thread for client " << peer_hash_id << " from " << peer_address);
        add_managed_thread(std::thread(&RatsClient::handle_client, this, client_socket, peer_hash_id), 
                          "client-handler-" + peer_hash_id.substr(0, 8));
        
        // Note: Connection callback will be called after handshake completion in handle_client
    }
    
    LOG_SERVER_INFO("Server loop ended");
}

void RatsClient::management_loop() {
    LOG_CLIENT_INFO("Management loop started");
    
    while (running_.load()) {
        std::unique_lock<std::mutex> lock(shutdown_mutex_);
        if (shutdown_cv_.wait_for(lock, std::chrono::seconds(30), [this] { return !running_.load(); })) {
            break; // Exit if shutdown requested
        }
        
        // Periodically cleanup finished threads
        try {
            cleanup_finished_threads();
            LOG_CLIENT_DEBUG("Periodic thread cleanup completed. Active threads: " << get_active_thread_count());
        } catch (const std::exception& e) {
            LOG_CLIENT_ERROR("Exception during thread cleanup: " << e.what());
        }
    }
    
    LOG_CLIENT_INFO("Management loop ended");
}


void RatsClient::handle_client(socket_t client_socket, const std::string& peer_hash_id) {
    LOG_CLIENT_INFO("Started handling client: " << peer_hash_id);
    
    bool handshake_completed = false;
    bool noise_handshake_completed = false;
    auto last_timeout_check = std::chrono::steady_clock::now();
    
    // Check if encryption is enabled for this connection
    bool encryption_enabled = is_encryption_enabled();
    
    while (running_.load()) {
        std::string data;
        
        // Handle encryption handshake first if enabled
        LOG_CLIENT_DEBUG("Checking handshake state: encryption_enabled=" << encryption_enabled << ", noise_handshake_completed=" << noise_handshake_completed);
        if (encryption_enabled && !noise_handshake_completed) {
            LOG_CLIENT_DEBUG("Entering encryption handshake handling for socket " << client_socket);
            // Check if noise handshake is completed
            if (encrypted_communication::is_handshake_completed(client_socket)) {
                noise_handshake_completed = true;
                LOG_CLIENT_INFO("Noise handshake completed for peer " << peer_hash_id);
                // Update peer state
                {
                    std::lock_guard<std::mutex> lock(peers_mutex_);
                    auto it = socket_to_peer_id_.find(client_socket);
                    if (it != socket_to_peer_id_.end()) {
                        auto peer_it = peers_.find(it->second);
                        if (peer_it != peers_.end()) {
                            peer_it->second.noise_handshake_completed = true;
                            // For outgoing connections, send application handshake after noise handshake completes
                            if (peer_it->second.is_outgoing) {
                                if (!send_handshake_unlocked(client_socket, get_our_peer_id())) {
                                    LOG_CLIENT_ERROR("Failed to send application handshake after noise completion for peer " << peer_hash_id);
                                    break;
                                }
                            }
                        }
                    }
                }
            } else {
                // For responders and ongoing handshake processing, try to continue the handshake
                LOG_CLIENT_DEBUG("Attempting to process handshake for socket " << client_socket);
                if (!encrypted_communication::perform_handshake(client_socket)) {
                    LOG_CLIENT_ERROR("Encryption handshake failed for peer " << peer_hash_id);
                    break;
                }
                // Continue to check if handshake completed
                continue;
            }
        }
        
        // Receive data (encrypted or unencrypted based on settings)
        LOG_CLIENT_DEBUG("Receiving data: encryption_enabled=" << encryption_enabled << ", noise_handshake_completed=" << noise_handshake_completed);
        if (encryption_enabled && noise_handshake_completed) {
            LOG_CLIENT_DEBUG("Receiving encrypted data from socket " << client_socket);
            auto binary_data = encrypted_communication::receive_tcp_data_encrypted(client_socket);
            data = std::string(binary_data.begin(), binary_data.end());
        } else if (encryption_enabled && !noise_handshake_completed) {
            LOG_CLIENT_DEBUG("Processing handshake for socket " << client_socket);
            // During handshake, let the encrypted socket handle data reception and processing
            // The perform_handshake function will internally call receive_tcp_data and process handshake messages
            if (!encrypted_communication::perform_handshake(client_socket)) {
                LOG_CLIENT_ERROR("Encryption handshake failed for peer " << peer_hash_id);
                break;
            }
            // Continue to check if handshake completed
            continue;
        } else {
            LOG_CLIENT_DEBUG("Receiving unencrypted data from socket " << client_socket);
            // Always use framed message reception for reliable large message handling
            data = receive_tcp_string_framed(client_socket);
        }
        
        if (data.empty()) {
            // Check if this is a timeout or actual connection close
            if (!handshake_completed) {
                // Check for handshake timeout one more time before giving up
                auto now = std::chrono::steady_clock::now();
                if (now - last_timeout_check >= std::chrono::seconds(1)) {
                    check_handshake_timeouts();
                    last_timeout_check = now;
                    
                    // Check if handshake has failed due to timeout
                    std::lock_guard<std::mutex> lock(peers_mutex_);
                    auto it = socket_to_peer_id_.find(client_socket);
                    if (it != socket_to_peer_id_.end()) {
                        auto peer_it = peers_.find(it->second);
                        if (peer_it != peers_.end() && peer_it->second.is_handshake_failed()) {
                            LOG_CLIENT_ERROR("Handshake failed for peer " << peer_hash_id);
                            break;
                        }
                    }
                }
            }
            break; // Connection closed or error
        }
        
        LOG_CLIENT_DEBUG("Received data from " << peer_hash_id << ": " << data.substr(0, 50) << (data.length() > 50 ? "..." : ""));
        
        // Check for handshake timeout and failure
        if (!handshake_completed) {
            bool should_exit = false;
            
            {
                std::lock_guard<std::mutex> lock(peers_mutex_);
                auto it = socket_to_peer_id_.find(client_socket);
                if (it != socket_to_peer_id_.end()) {
                    auto peer_it = peers_.find(it->second);
                    if (peer_it != peers_.end()) {
                        const RatsPeer& peer = peer_it->second;
                        if (peer.is_handshake_failed()) {
                            LOG_CLIENT_ERROR("Handshake failed for peer " << peer_hash_id);
                            should_exit = true;
                        }
                    }
                }
            }
            
            if (should_exit) {
                break; // Exit loop to disconnect
            }
            
            // Check for handshake timeout
            auto now = std::chrono::steady_clock::now();
            if (now - last_timeout_check >= std::chrono::seconds(1)) {
                check_handshake_timeouts();
                last_timeout_check = now;
            }
        }
        
        // Handle handshake messages
        if (is_handshake_message(data)) {
            if (!handle_handshake_message(client_socket, peer_hash_id, data)) {
                LOG_CLIENT_ERROR("Failed to handle handshake message from " << peer_hash_id);
                break; // Exit loop to disconnect
            }
            
            // Check if handshake just completed and trigger notifications
            if (!handshake_completed) {
                RatsPeer peer_copy;
                bool should_notify_connection = false;
                bool should_broadcast_exchange = false;
                
                {
                    std::lock_guard<std::mutex> lock(peers_mutex_);
                    auto it = socket_to_peer_id_.find(client_socket);
                    if (it != socket_to_peer_id_.end()) {
                        auto peer_it = peers_.find(it->second);
                        if (peer_it != peers_.end()) {
                            const RatsPeer& peer = peer_it->second;
                            if (peer.is_handshake_completed()) {
                                handshake_completed = true;
                                should_notify_connection = true;
                                should_broadcast_exchange = true;
                                peer_copy = peer; // Copy peer data
                                
                                LOG_CLIENT_INFO("Handshake completed for peer " << peer_hash_id << " (peer_id: " << peer.peer_id << ")");
                            }
                        }
                    }
                }
                
                // Call callbacks and methods outside of mutex to avoid deadlock
                if (should_notify_connection) {
                    if (connection_callback_) {
                        connection_callback_(client_socket, peer_copy.peer_id);
                    }

                    if (gossipsub_) {
                        gossipsub_->handle_peer_connected(peer_copy.peer_id);
                    }
                    
                    // Broadcast peer exchange message to other peers
                    broadcast_peer_exchange_message(peer_copy);
                    
                    // Send peers request to the newly connected peer to discover more peers
                    if (peer_copy.is_outgoing) {
                        send_peers_request(client_socket, peer_copy.peer_id);
                    }
                    
                    // Initiate NAT traversal information exchange
                    send_nat_info_to_peer(client_socket, peer_copy.peer_id);
                    
                    // If ICE is enabled and this is an outgoing connection, initiate ICE coordination
                    if (ice_agent_ && ice_agent_->is_running() && peer_copy.is_outgoing) {
                        if (should_initiate_ice_coordination(peer_copy.peer_id)) {
                            mark_ice_coordination_in_progress(peer_copy.peer_id);
                            
                            // Start ICE coordination in background thread with proper exception handling
                            add_managed_thread(std::thread([this, peer_id = peer_copy.peer_id, host = peer_copy.ip, port = peer_copy.port]() {
                                try {
                                    // Use conditional variable for responsive shutdown
                                    {
                                        std::unique_lock<std::mutex> lock(shutdown_mutex_);
                                        if (shutdown_cv_.wait_for(lock, std::chrono::milliseconds(500), [this] { return !running_.load(); })) {
                                            // Clean up tracking before exit
                                            cleanup_ice_coordination_for_peer(peer_id);
                                            return; // Exit if shutdown requested
                                        }
                                    }
                                    
                                    // Initiate ICE coordination
                                    initiate_ice_with_peer(peer_id, host, port);
                                    
                                } catch (const std::exception& e) {
                                    LOG_CLIENT_ERROR("Exception in ICE coordination thread for peer " << peer_id << ": " << e.what());
                                } catch (...) {
                                    LOG_CLIENT_ERROR("Unknown exception in ICE coordination thread for peer " << peer_id);
                                }
                                
                                // Clean up tracking when done
                                cleanup_ice_coordination_for_peer(peer_id);
                            }), "ice-coordination-" + peer_copy.peer_id.substr(0, 8));
                        } else {
                            LOG_CLIENT_DEBUG("ICE coordination already in progress for peer " << peer_copy.peer_id << " - skipping duplicate attempt");
                        }
                    }
                    
                    // Save configuration after a new peer connects to keep peer list current
                    if (running_.load()) {
                        add_managed_thread(std::thread([this]() {
                            if (running_.load()) {
                                save_configuration();
                            }
                        }), "config-save");
                    }
                }
            }
            
            continue; // Don't process handshake messages as regular data
        }
        
        // Only process regular data after handshake is completed (and noise handshake if encryption is enabled)
        if (handshake_completed && (!encryption_enabled || noise_handshake_completed)) {
            // Convert string data to binary for header parsing
            std::vector<uint8_t> received_data(data.begin(), data.end());
            
            // Try to parse message header first
            MessageHeader header;
            std::vector<uint8_t> payload;
            if (parse_message_with_header(received_data, header, payload)) {
                // Message has valid header - call appropriate callback based on type
                std::string peer_id = get_peer_id(client_socket);
                
                switch (header.type) {
                    case MessageDataType::BINARY: {
                        LOG_CLIENT_DEBUG("Received BINARY message from " << peer_id << " (payload size: " << payload.size() << ")");
                        // Try to handle as file transfer data first
                        bool handled = false;
                        if (file_transfer_manager_) {
                            handled = file_transfer_manager_->handle_binary_data(peer_id, payload);
                        }
                        
                        // If not a file transfer chunk, call user's binary callback
                        if (!handled && binary_data_callback_) {
                            binary_data_callback_(client_socket, peer_id, payload);
                        }
                        break;
                    }
                        
                    case MessageDataType::STRING:
                        LOG_CLIENT_DEBUG("Received STRING message from " << peer_id << " (payload size: " << payload.size() << ")");
                        if (string_data_callback_) {
                            std::string string_data(payload.begin(), payload.end());
                            string_data_callback_(client_socket, peer_id, string_data);
                        }
                        break;
                        
                    case MessageDataType::JSON: {
                        LOG_CLIENT_DEBUG("Received JSON message from " << peer_id << " (payload size: " << payload.size() << ")");
                        // Parse JSON payload
                        std::string json_string(payload.begin(), payload.end());
                        nlohmann::json json_msg;
                        if (parse_json_message(json_string, json_msg)) {
                            // Check if it's a rats protocol message
                            if (json_msg.contains("rats_protocol") && json_msg["rats_protocol"] == true) {
                                handle_rats_message(client_socket, peer_id, json_msg);
                            } else {
                                // Regular JSON data - call JSON callback
                                if (json_data_callback_) {
                                    json_data_callback_(client_socket, peer_id, json_msg);
                                }
                            }
                        } else {
                            LOG_CLIENT_ERROR("Received invalid JSON in JSON message from " << peer_id);
                        }
                        break;
                    }
                        
                    default:
                        LOG_CLIENT_WARN("Received message with unknown data type " << static_cast<int>(header.type) << " from " << peer_id);
                        break;
                }
            } else {
                // No header found
                LOG_CLIENT_WARN("No header found in message from " << peer_hash_id);
            }
        } else {
            LOG_CLIENT_WARN("Received non-handshake data from " << peer_hash_id << " before handshake completion - ignoring");
        }
    }
    
    // Get current peer ID before cleanup for disconnect callback
    std::string current_peer_id = get_peer_id(client_socket);
    
    // Clean up
    remove_peer(client_socket);
    
    // Clean up encryption state
    if (encryption_enabled) {
        encrypted_communication::cleanup_socket(client_socket);
    }
    
    close_socket(client_socket);
    
    // Notify disconnect callback only if handshake was completed
    if (handshake_completed && disconnect_callback_) {
        disconnect_callback_(client_socket, current_peer_id);
    }

    if (handshake_completed && gossipsub_) {
        gossipsub_->handle_peer_disconnected(current_peer_id);
    }
    
    // Save configuration after a validated peer disconnects to update the saved peer list
    if (handshake_completed && running_.load()) {
        // Save configuration in a separate thread to avoid blocking
        add_managed_thread(std::thread([this]() {
            if (running_.load()) {
                save_configuration();
            }
        }), "config-save-disconnect");
    }
    
    LOG_CLIENT_INFO("Client disconnected: " << peer_hash_id);
}

// Handshake protocol implementation
std::string RatsClient::create_handshake_message(const std::string& message_type, const std::string& our_peer_id) const {
    auto now = std::chrono::high_resolution_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    
    // Use nlohmann::json for proper JSON serialization
    nlohmann::json handshake_msg;
    {
        std::lock_guard<std::mutex> lock(protocol_config_mutex_);
        handshake_msg["protocol"] = custom_protocol_name_;
        handshake_msg["version"] = custom_protocol_version_;
    }
    handshake_msg["peer_id"] = our_peer_id;
    handshake_msg["message_type"] = message_type;
    handshake_msg["timestamp"] = timestamp;
    
    return handshake_msg.dump();
}

bool RatsClient::parse_handshake_message(const std::string& message, HandshakeMessage& out_msg) const {
    try {
        // Use nlohmann::json for proper JSON parsing
        nlohmann::json json_msg = nlohmann::json::parse(message);
        
        // Clear the output structure
        out_msg = HandshakeMessage{};
        
        // Extract fields using nlohmann::json
        out_msg.protocol = json_msg.value("protocol", "");
        out_msg.version = json_msg.value("version", "");
        out_msg.peer_id = json_msg.value("peer_id", "");
        out_msg.message_type = json_msg.value("message_type", "");
        // Tolerate missing timestamp to avoid hard dependency on remote system clock
        out_msg.timestamp = json_msg.value("timestamp", static_cast<int64_t>(0));
        
        return true;
        
    } catch (const nlohmann::json::exception& e) {
        LOG_CLIENT_ERROR("Failed to parse handshake message: " << e.what());
        return false;
    } catch (const std::exception& e) {
        LOG_CLIENT_ERROR("Failed to parse handshake message: " << e.what());
        return false;
    }
}

bool RatsClient::validate_handshake_message(const HandshakeMessage& msg) const {
    std::string expected_protocol;
    std::string expected_version;
    {
        std::lock_guard<std::mutex> lock(protocol_config_mutex_);
        expected_protocol = custom_protocol_name_;
        expected_version = custom_protocol_version_;
    }
    
    // Validate protocol
    if (msg.protocol != expected_protocol) {
        LOG_CLIENT_WARN("Invalid handshake protocol: " << msg.protocol << " (expected: " << expected_protocol << ")");
        return false;
    }
    
    // Validate version (for now, only accept exact version match)
    if (msg.version != expected_version) {
        LOG_CLIENT_WARN("Unsupported protocol version: " << msg.version << " (expected: " << expected_version << ")");
        return false;
    }
    
    // Validate message type
    if (msg.message_type != "handshake") {
        LOG_CLIENT_WARN("Invalid handshake message type: " << msg.message_type);
        return false;
    }
    
    // Validate peer_id (must not be empty)
    if (msg.peer_id.empty()) {
        LOG_CLIENT_WARN("Empty peer_id in handshake message");
        return false;
    }
    
    // Soft-validate timestamp to avoid rejecting valid peers due to clock skew
    if (msg.timestamp == 0) {
        LOG_CLIENT_WARN("Handshake missing timestamp; accepting to avoid clock-skew rejection");
    } else {
        auto now = std::chrono::high_resolution_clock::now();
        auto current_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        int64_t time_diff = std::abs(current_timestamp - msg.timestamp);
        const int64_t allowed_skew_ms = 10LL * 60LL * 1000LL; // 10 minutes tolerance
        if (time_diff > allowed_skew_ms) {
            LOG_CLIENT_WARN("Handshake timestamp skew " << time_diff << "ms exceeds " << allowed_skew_ms << "ms; accepting to be tolerant of clock skew");
        }
    }
    
    return true;
}

bool RatsClient::is_handshake_message(const std::string& message) const {
    try {
        std::string json_to_parse = message;
        
        // Check if message has our message header (starts with "RATS" magic)
        std::vector<uint8_t> message_data(message.begin(), message.end());
        MessageHeader header;
        std::vector<uint8_t> payload;
        
        if (parse_message_with_header(message_data, header, payload)) {
            // Message has valid header - extract the JSON payload
            if (header.type == MessageDataType::STRING || header.type == MessageDataType::JSON) {
                json_to_parse = std::string(payload.begin(), payload.end());
            } else {
                // Handshake messages should be string/JSON type
                return false;
            }
        } else {
            // Message has no header
            return false;
        }
        
        // Parse the JSON message
        nlohmann::json json_msg = nlohmann::json::parse(json_to_parse);
        std::string expected_protocol;
        {
            std::lock_guard<std::mutex> lock(protocol_config_mutex_);
            expected_protocol = custom_protocol_name_;
        }
        return json_msg.value("protocol", "") == expected_protocol && 
               json_msg.value("message_type", "") == "handshake";
    } catch (const std::exception&) {
        return false;
    }
}

// Add this private helper function before send_handshake
bool RatsClient::send_handshake_unlocked(socket_t socket, const std::string& our_peer_id) {
    std::string handshake_msg = create_handshake_message("handshake", our_peer_id);
    LOG_CLIENT_DEBUG("Sending handshake to socket " << socket << ": " << handshake_msg);
    
    if (!send_string_to_peer(socket, handshake_msg)) {
        LOG_CLIENT_ERROR("Failed to send handshake to socket " << socket);
        return false;
    }
    
    // Update peer state (assumes peers_mutex_ is already locked)
    auto it = socket_to_peer_id_.find(socket);
    if (it != socket_to_peer_id_.end()) {
        auto peer_it = peers_.find(it->second);
        if (peer_it != peers_.end()) {
            peer_it->second.handshake_state = RatsPeer::HandshakeState::SENT;
            peer_it->second.handshake_start_time = std::chrono::steady_clock::now();
        }
    }
    
    return true;
}

bool RatsClient::send_handshake(socket_t socket, const std::string& our_peer_id) {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    return send_handshake_unlocked(socket, our_peer_id);
}

bool RatsClient::handle_handshake_message(socket_t socket, const std::string& peer_hash_id, const std::string& message) {
    // Extract JSON payload from message header if present
    std::string json_to_parse = message;
    std::vector<uint8_t> message_data(message.begin(), message.end());
    MessageHeader header;
    std::vector<uint8_t> payload;
    
    if (parse_message_with_header(message_data, header, payload)) {
        // Message has valid header - extract the JSON payload
        if (header.type == MessageDataType::STRING || header.type == MessageDataType::JSON) {
            json_to_parse = std::string(payload.begin(), payload.end());
        } else {
            LOG_CLIENT_ERROR("Invalid message type for handshake: " << static_cast<int>(header.type));
            return false;
        }
    } else {
        LOG_CLIENT_ERROR("Failed to parse handshake message header from " << peer_hash_id);
        return false;
    }
    
    HandshakeMessage handshake_msg;
    if (!parse_handshake_message(json_to_parse, handshake_msg)) {
        LOG_CLIENT_ERROR("Failed to parse handshake message from " << peer_hash_id);
        return false;
    }
    
    if (!validate_handshake_message(handshake_msg)) {
        LOG_CLIENT_ERROR("Invalid handshake message from " << peer_hash_id);
        return false;
    }

    if (handshake_msg.peer_id == get_our_peer_id()) {
        LOG_CLIENT_INFO("Received handshake from ourselves, ignoring");
        return false;
    }
    
    LOG_CLIENT_INFO("Received valid handshake from " << peer_hash_id 
                    << " (peer_id: " << handshake_msg.peer_id << ")");
    
    std::lock_guard<std::mutex> lock(peers_mutex_);
    auto it = socket_to_peer_id_.find(socket);
    if (it == socket_to_peer_id_.end()) {
        LOG_CLIENT_ERROR("Socket " << socket << " not found in peer mapping");
        return false;
    }
    
    auto peer_it = peers_.find(it->second);
    if (peer_it == peers_.end()) {
        LOG_CLIENT_ERROR("Peer " << peer_hash_id << " not found in peers");
        return false;
    }

    if (peers_.find(handshake_msg.peer_id) != peers_.end()) {
        LOG_CLIENT_INFO("Peer " << handshake_msg.peer_id << " already connected, closing duplicate connection");
        // This is a duplicate connection - the existing connection should remain stable
        // Return false to close this duplicate connection, but this is expected behavior
        return false;
    }
    
    // Store old peer ID for mapping updates
    std::string old_peer_id = peer_it->second.peer_id;
    
    // Update peer mappings with new peer_id if it changed
    if (old_peer_id != handshake_msg.peer_id) {
        // Create a copy of the peer object before erasing it.
        RatsPeer peer_copy = peer_it->second;
        
        // Erase the old entry from the main peers map.
        peers_.erase(peer_it);
        
        // Update the peer_id within the copied object.
        peer_copy.peer_id = handshake_msg.peer_id;
        
        // Insert the updated peer object back into the maps with the new peer_id.
        peers_[peer_copy.peer_id] = peer_copy;
        socket_to_peer_id_[socket] = peer_copy.peer_id;
        address_to_peer_id_[peer_copy.normalized_address] = peer_copy.peer_id;

        // Find the iterator for the newly inserted peer.
        peer_it = peers_.find(peer_copy.peer_id);
    }

    RatsPeer& peer = peer_it->second;

    // Store remote peer information
    peer.version = handshake_msg.version;
    
    // Simplified handshake logic - just one message type
    if (peer.handshake_state == RatsPeer::HandshakeState::PENDING) {
        // This is an incoming handshake - send our handshake back
        if (send_handshake_unlocked(socket, get_our_peer_id())) {
            peer.handshake_state = RatsPeer::HandshakeState::COMPLETED;
            log_handshake_completion_unlocked(peer);
            
            // Append to historical peers file after successful connection
            append_peer_to_historical_file(peer);
            
            return true;
        } else {
            peer.handshake_state = RatsPeer::HandshakeState::FAILED;
            LOG_CLIENT_ERROR("Failed to send handshake response to " << peer_hash_id);
            return false;
        }
    } else if (peer.handshake_state == RatsPeer::HandshakeState::SENT) {
        // This is a response to our handshake
        peer.handshake_state = RatsPeer::HandshakeState::COMPLETED;
        log_handshake_completion_unlocked(peer);
        
        // Append to historical peers file after successful connection
        append_peer_to_historical_file(peer);
        
        return true;
    } else {
        LOG_CLIENT_WARN("Received handshake from " << peer_hash_id << " but handshake state is " << static_cast<int>(peer.handshake_state));
        return false;
    }
}

void RatsClient::check_handshake_timeouts() {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    auto now = std::chrono::steady_clock::now();
    
    std::vector<std::string> peers_to_remove;
    
    for (auto& pair : peers_) {
        RatsPeer& peer = pair.second;
        
        if (peer.handshake_state != RatsPeer::HandshakeState::COMPLETED && 
            peer.handshake_state != RatsPeer::HandshakeState::FAILED) {
            
            auto handshake_duration = std::chrono::duration_cast<std::chrono::seconds>(now - peer.handshake_start_time);
            
            if (handshake_duration.count() > HANDSHAKE_TIMEOUT_SECONDS) {
                LOG_CLIENT_WARN("Handshake timeout for peer " << peer.peer_id << " after " << handshake_duration.count() << " seconds");
                peer.handshake_state = RatsPeer::HandshakeState::FAILED;
                peers_to_remove.push_back(peer.peer_id);
            }
        }
    }
    
    // Remove timed out peers
    for (const auto& peer_id : peers_to_remove) {
        auto peer_it = peers_.find(peer_id);
        if (peer_it != peers_.end()) {
            socket_t socket = peer_it->second.socket;
            LOG_CLIENT_INFO("Disconnecting peer " << peer_id << " due to handshake timeout");
            
            // Clean up peer data
            remove_peer_by_id_unlocked(peer_id);
            close_socket(socket);
        }
    }
}

// =========================================================================
// Connection Management
// =========================================================================

bool RatsClient::connect_to_peer(const std::string& host, int port, ConnectionStrategy strategy) {
    if (!running_.load()) {
        LOG_CLIENT_ERROR("RatsClient is not running");
        return false;
    }
    
    LOG_CLIENT_INFO("Connecting to peer " << host << ":" << port << " using strategy: " << static_cast<int>(strategy));
    
    // Create connection attempt result to track the attempt
    ConnectionAttemptResult result;
    result.local_nat_type = detect_nat_type();
    result.remote_nat_type = NatType::UNKNOWN;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    bool success = false;
    
    // Execute connection strategy
    switch (strategy) {
        case ConnectionStrategy::DIRECT_ONLY:
            success = attempt_direct_connection(host, port, result);
            result.method = "direct";
            break;
            
        case ConnectionStrategy::STUN_ASSISTED:
            success = attempt_stun_assisted_connection(host, port, result);
            result.method = "stun";
            break;
            
        case ConnectionStrategy::ICE_FULL:
            success = attempt_ice_connection(host, port, result);
            result.method = "ice";
            break;
            
        case ConnectionStrategy::TURN_RELAY:
            success = attempt_turn_relay_connection(host, port, result);
            result.method = "turn";
            break;
            
        case ConnectionStrategy::AUTO_ADAPTIVE:
            // Select best strategy based on NAT type
            std::string best_strategy = select_best_connection_strategy(host, port);
            result.method = best_strategy;
            
            if (best_strategy == "direct") {
                success = attempt_direct_connection(host, port, result);
            } else if (best_strategy == "stun") {
                success = attempt_stun_assisted_connection(host, port, result);
            } else if (best_strategy == "ice") {
                success = attempt_ice_connection(host, port, result);
            } else if (best_strategy == "turn") {
                success = attempt_turn_relay_connection(host, port, result);
            } else {
                success = attempt_hole_punch_connection(host, port, result);
            }
            break;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    result.success = success;
    
    // Log connection attempt result
    LOG_CLIENT_INFO("Connection attempt to " << host << ":" << port << " via " << result.method 
                   << " completed: " << (success ? "SUCCESS" : "FAILED") 
                   << " in " << result.duration.count() << "ms");
    
    if (!result.error_message.empty()) {
        LOG_CLIENT_DEBUG("Error details: " << result.error_message);
    }
    
    // Update connection statistics
    std::string peer_address = normalize_peer_address(host, port);
    update_connection_statistics(peer_address, result);
    
    // Call advanced connection callback if set
    if (success && advanced_connection_callback_) {
        // Find the socket for this connection to pass to callback
        socket_t connected_socket = INVALID_SOCKET_VALUE;
        std::string peer_id;
        
        {
            std::lock_guard<std::mutex> lock(peers_mutex_);
            auto addr_it = address_to_peer_id_.find(peer_address);
            if (addr_it != address_to_peer_id_.end()) {
                auto peer_it = peers_.find(addr_it->second);
                if (peer_it != peers_.end()) {
                    connected_socket = peer_it->second.socket;
                    peer_id = peer_it->second.peer_id;
                }
            }
        }
        
        if (is_valid_socket(connected_socket)) {
            advanced_connection_callback_(connected_socket, peer_id, result);
        }
    }
    
    return success;
}

void RatsClient::disconnect_peer(socket_t socket) {
    remove_peer(socket);
    
    // Clean up encryption state
    if (is_encryption_enabled()) {
        encrypted_communication::cleanup_socket(socket);
    }
    
    close_socket(socket);
}

void RatsClient::disconnect_peer_by_id(const std::string& peer_id) {
    socket_t socket = get_peer_socket_by_id(peer_id);
    if (is_valid_socket(socket)) {
        disconnect_peer(socket);
    }
}

// Helper methods for peer management
void RatsClient::add_peer(const RatsPeer& peer) {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    add_peer_unlocked(peer);
}

void RatsClient::add_peer_unlocked(const RatsPeer& peer) {
    // Assumes peers_mutex_ is already locked
    peers_[peer.peer_id] = peer;
    socket_to_peer_id_[peer.socket] = peer.peer_id;
    address_to_peer_id_[peer.normalized_address] = peer.peer_id;
}

void RatsClient::remove_peer(socket_t socket) {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    auto it = socket_to_peer_id_.find(socket);
    if (it != socket_to_peer_id_.end()) {
        remove_peer_by_id_unlocked(it->second);
    }
}

void RatsClient::remove_peer_by_id(const std::string& peer_id) {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    remove_peer_by_id_unlocked(peer_id);
}

void RatsClient::remove_peer_by_id_unlocked(const std::string& peer_id) {
    // Assumes peers_mutex_ is already locked
    
    // Make a copy of peer_id to avoid use-after-free if the reference points to memory that gets freed
    std::string peer_id_copy = peer_id;
    
    auto it = peers_.find(peer_id_copy);
    if (it != peers_.end()) {
        // Copy the values we need before erasing to avoid use-after-free
        socket_t peer_socket = it->second.socket;
        std::string peer_normalized_address = it->second.normalized_address;
        
        socket_to_peer_id_.erase(peer_socket);
        address_to_peer_id_.erase(peer_normalized_address);
        peers_.erase(it);
        
        // Clean up socket-specific mutex
        cleanup_socket_send_mutex(peer_socket);

        // Clean up ICE coordination tracking for this peer
        cleanup_ice_coordination_for_peer(peer_id_copy);
    }
}

bool RatsClient::is_already_connected_to_address(const std::string& normalized_address) const {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    return address_to_peer_id_.find(normalized_address) != address_to_peer_id_.end();
}

void RatsClient::add_ignored_address(const std::string& ip_address) {
    std::lock_guard<std::mutex> lock(local_addresses_mutex_);
    
    // Check if already in the list
    if (std::find(local_interface_addresses_.begin(), local_interface_addresses_.end(), ip_address) == local_interface_addresses_.end()) {
        local_interface_addresses_.push_back(ip_address);
        LOG_CLIENT_INFO("Added " << ip_address << " to ignore list");
    } else {
        LOG_CLIENT_DEBUG("IP address " << ip_address << " already in ignore list");
    }
}


// Local interface address blocking methods
void RatsClient::initialize_local_addresses() {
    LOG_CLIENT_INFO("Initializing local interface addresses for connection blocking");
    
    std::lock_guard<std::mutex> lock(local_addresses_mutex_);
    
    // Get all local interface addresses using network_utils
    local_interface_addresses_ = network_utils::get_local_interface_addresses();
    
    // Add common localhost addresses if not already present
    std::vector<std::string> localhost_addrs = {"127.0.0.1", "::1", "0.0.0.0", "::"};
    for (const auto& addr : localhost_addrs) {
        if (std::find(local_interface_addresses_.begin(), local_interface_addresses_.end(), addr) == local_interface_addresses_.end()) {
            local_interface_addresses_.push_back(addr);
        }
    }
    
    LOG_CLIENT_INFO("Found " << local_interface_addresses_.size() << " local addresses to block:");
    for (const auto& addr : local_interface_addresses_) {
        LOG_CLIENT_INFO("  - " << addr);
    }
}

void RatsClient::refresh_local_addresses() {
    LOG_CLIENT_DEBUG("Refreshing local interface addresses");
    
    std::lock_guard<std::mutex> lock(local_addresses_mutex_);
    
    // Clear old addresses and get fresh ones
    local_interface_addresses_.clear();
    local_interface_addresses_ = network_utils::get_local_interface_addresses();
    
    // Add common localhost addresses if not already present
    std::vector<std::string> localhost_addrs = {"127.0.0.1", "::1", "0.0.0.0", "::"};
    for (const auto& addr : localhost_addrs) {
        if (std::find(local_interface_addresses_.begin(), local_interface_addresses_.end(), addr) == local_interface_addresses_.end()) {
            local_interface_addresses_.push_back(addr);
        }
    }
    
    LOG_CLIENT_DEBUG("Refreshed " << local_interface_addresses_.size() << " local addresses");
}

bool RatsClient::is_blocked_address(const std::string& ip_address) const {
    std::lock_guard<std::mutex> lock(local_addresses_mutex_);
    
    // Check against our stored local addresses
    for (const auto& local_addr : local_interface_addresses_) {
        if (local_addr == ip_address) {
            return true;
        }
    }
    
    return false;
}

bool RatsClient::should_ignore_peer(const std::string& ip, int port) const {
    // Always block connections to ourselves (same port)
    if (port == listen_port_) {
        if (ip == "127.0.0.1" || ip == "::1" || ip == "localhost" || ip == "0.0.0.0" || ip == "::") {
            LOG_CLIENT_DEBUG("Ignoring peer " << ip << ":" << port << " - localhost with same port");
            return true;
        }
    }
    
    // For localhost addresses on different ports, allow the connection (for testing)
    if (ip == "127.0.0.1" || ip == "::1" || ip == "localhost") {
        LOG_CLIENT_DEBUG("Allowing localhost peer " << ip << ":" << port << " on different port");
        return false;
    }
    
    // Check if the IP is a non-localhost local interface address
    if (is_blocked_address(ip)) {
        LOG_CLIENT_DEBUG("Ignoring peer " << ip << ":" << port << " - matches local interface address");
        return true;
    }
    
    return false;
}

// =========================================================================
// Data Transmission Methods
// =========================================================================

// Helper method to create a message with header
std::vector<uint8_t> RatsClient::create_message_with_header(const std::vector<uint8_t>& payload, MessageDataType type) {
    MessageHeader header(type);
    std::vector<uint8_t> header_bytes = header.serialize();
    
    // Combine header + payload
    std::vector<uint8_t> message;
    message.reserve(header_bytes.size() + payload.size());
    message.insert(message.end(), header_bytes.begin(), header_bytes.end());
    message.insert(message.end(), payload.begin(), payload.end());
    
    return message;
}

// Helper method to parse message header and extract payload
bool RatsClient::parse_message_with_header(const std::vector<uint8_t>& message, MessageHeader& header, std::vector<uint8_t>& payload) const {
    // Check if message is large enough to contain header
    if (message.size() < MessageHeader::HEADER_SIZE) {
        LOG_CLIENT_DEBUG("Message too small to contain header: " << message.size() << " bytes");
        return false;
    }
    
    // Extract header bytes
    std::vector<uint8_t> header_bytes(message.begin(), message.begin() + MessageHeader::HEADER_SIZE);
    
    // Parse header
    if (!MessageHeader::deserialize(header_bytes, header)) {
        LOG_CLIENT_DEBUG("Failed to parse message header - invalid magic number or format");
        return false;
    }
    
    // Validate header
    if (!header.is_valid_type()) {
        LOG_CLIENT_WARN("Invalid message data type: " << static_cast<int>(header.type));
        return false;
    }
    
    // Extract payload
    payload.assign(message.begin() + MessageHeader::HEADER_SIZE, message.end());
    
    LOG_CLIENT_DEBUG("Parsed message header: type=" << static_cast<int>(header.type) << ", payload_size=" << payload.size());
    return true;
}

bool RatsClient::send_binary_to_peer(socket_t socket, const std::vector<uint8_t>& data, MessageDataType message_type) {
    if (!running_.load()) {
        return false;
    }
    
    // Get socket-specific mutex for thread-safe sending
    // Prevent framed messages corruption (like two-times sending the number of bytes instead number of bytes + message)
    auto socket_mutex = get_socket_send_mutex(socket);
    std::lock_guard<std::mutex> send_lock(*socket_mutex);
    
    // Create message with specified header type
    std::vector<uint8_t> message_with_header = create_message_with_header(data, message_type);
    
    // Use encrypted communication if encryption is enabled
    if (is_encryption_enabled()) {
        // Check if handshake is completed before sending data
        if (!encrypted_communication::is_handshake_completed(socket)) {
            std::string type_name = (message_type == MessageDataType::BINARY) ? "binary" : 
                                   (message_type == MessageDataType::STRING) ? "string" : "JSON";
            LOG_CLIENT_WARN("Cannot send " << type_name << " data to socket " << socket << " - encryption handshake not completed");
            return false;
        }
        
        int sent = encrypted_communication::send_tcp_data_encrypted(socket, message_with_header);
        return sent > 0;
    } else {
        // Use framed messages for reliable large message handling
        int sent = send_tcp_message_framed(socket, message_with_header);
        return sent > 0;
    }
}

bool RatsClient::send_string_to_peer(socket_t socket, const std::string& data) {
    // Convert string to binary and use the primary send_binary_to_peer method
    std::vector<uint8_t> binary_data(data.begin(), data.end());
    return send_binary_to_peer(socket, binary_data, MessageDataType::STRING);
}

bool RatsClient::send_json_to_peer(socket_t socket, const nlohmann::json& data) {
    try {
        // Serialize JSON and convert to binary, then use the primary send_binary_to_peer method
        std::string json_string = data.dump();
        std::vector<uint8_t> binary_data(json_string.begin(), json_string.end());
        return send_binary_to_peer(socket, binary_data, MessageDataType::JSON);
    } catch (const nlohmann::json::exception& e) {
        LOG_CLIENT_ERROR("Failed to serialize JSON message: " << e.what());
        return false;
    }
}

bool RatsClient::send_binary_to_peer_id(const std::string& peer_hash_id, const std::vector<uint8_t>& data, MessageDataType message_type) {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    auto it = peers_.find(peer_hash_id);
    if (it == peers_.end() || !it->second.is_handshake_completed()) {
        return false;
    }
    
    return send_binary_to_peer(it->second.socket, data, message_type);
}

bool RatsClient::send_string_to_peer_id(const std::string& peer_hash_id, const std::string& data) {
    // Convert string to binary and use primary binary method with STRING type
    std::vector<uint8_t> binary_data(data.begin(), data.end());
    return send_binary_to_peer_id(peer_hash_id, binary_data, MessageDataType::STRING);
}

bool RatsClient::send_json_to_peer_id(const std::string& peer_hash_id, const nlohmann::json& data) {
    try {
        // Serialize JSON and convert to binary, then use primary binary method with JSON type
        std::string json_string = data.dump();
        std::vector<uint8_t> binary_data(json_string.begin(), json_string.end());
        return send_binary_to_peer_id(peer_hash_id, binary_data, MessageDataType::JSON);
    } catch (const nlohmann::json::exception& e) {
        LOG_CLIENT_ERROR("Failed to serialize JSON message: " << e.what());
        return false;
    }
}

int RatsClient::broadcast_json_to_peers(const nlohmann::json& data) {
    try {
        // Serialize JSON and convert to binary, then use primary binary method with JSON type
        std::string json_string = data.dump();
        std::vector<uint8_t> binary_data(json_string.begin(), json_string.end());
        return broadcast_binary_to_peers(binary_data, MessageDataType::JSON);
    } catch (const nlohmann::json::exception& e) {
        LOG_CLIENT_ERROR("Failed to serialize JSON message for broadcast: " << e.what());
        return 0;
    }
}

int RatsClient::broadcast_binary_to_peers(const std::vector<uint8_t>& data, MessageDataType message_type) {
    if (!running_.load()) {
        return 0;
    }
    
    int sent_count = 0;
    std::lock_guard<std::mutex> lock(peers_mutex_);
    
    for (const auto& pair : peers_) {
        const RatsPeer& peer = pair.second;
        // Only send to peers that have completed handshake
        if (peer.is_handshake_completed()) {
            if (send_binary_to_peer(peer.socket, data, message_type)) {
                sent_count++;
            }
        }
    }
    
    return sent_count;
}

int RatsClient::broadcast_string_to_peers(const std::string& data) {
    // Convert string to binary and use primary binary method with STRING type
    std::vector<uint8_t> binary_data(data.begin(), data.end());
    return broadcast_binary_to_peers(binary_data, MessageDataType::STRING);
}

bool RatsClient::parse_json_message(const std::string& message, nlohmann::json& out_json) {
    try {
        out_json = nlohmann::json::parse(message);
        return true;
    } catch (const nlohmann::json::exception& e) {
        LOG_CLIENT_ERROR("Failed to parse JSON message: " << e.what());
        return false;
    }
}

// Helpers

// Per-socket synchronization helpers
std::shared_ptr<std::mutex> RatsClient::get_socket_send_mutex(socket_t socket) {
    std::lock_guard<std::mutex> lock(socket_send_mutexes_mutex_);
    auto it = socket_send_mutexes_.find(socket);
    if (it == socket_send_mutexes_.end()) {
        // Create new mutex for this socket
        socket_send_mutexes_[socket] = std::make_shared<std::mutex>();
        return socket_send_mutexes_[socket];
    }
    return it->second;
}

void RatsClient::cleanup_socket_send_mutex(socket_t socket) {
    std::lock_guard<std::mutex> lock(socket_send_mutexes_mutex_);
    socket_send_mutexes_.erase(socket);
}

// =========================================================================
// Peer Information and Management
// =========================================================================

std::string RatsClient::get_our_peer_id() const {
    return our_peer_id_;
}

int RatsClient::get_peer_count_unlocked() const {
    // Assumes peers_mutex_ is already locked
    int count = 0;
    for (const auto& pair : peers_) {
        if (pair.second.is_handshake_completed()) {
            count++;
        }
    }
    return count;
}

int RatsClient::get_peer_count() const {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    return get_peer_count_unlocked();
}

std::string RatsClient::get_peer_id(socket_t socket) const {
    const RatsPeer* peer = get_peer_by_socket(socket);
    return peer ? peer->peer_id : "";
}

socket_t RatsClient::get_peer_socket_by_id(const std::string& peer_id) const {
    const RatsPeer* peer = get_peer_by_id(peer_id);
    return peer ? peer->socket : INVALID_SOCKET_VALUE;
}

std::vector<RatsPeer> RatsClient::get_all_peers() const {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    std::vector<RatsPeer> result;
    result.reserve(peers_.size());
    
    for (const auto& pair : peers_) {
        result.push_back(pair.second);
    }
    
    return result;
}

std::vector<RatsPeer> RatsClient::get_validated_peers() const {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    std::vector<RatsPeer> result;
    
    for (const auto& pair : peers_) {
        if (pair.second.is_handshake_completed()) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}


std::vector<RatsPeer> RatsClient::get_random_peers(int max_count, const std::string& exclude_peer_id) const {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    
    std::vector<RatsPeer> all_validated_peers;
    
    // Get all validated peers excluding the specified peer
    for (const auto& pair : peers_) {
        const RatsPeer& peer = pair.second;
        if (peer.is_handshake_completed() && peer.peer_id != exclude_peer_id) {
            all_validated_peers.push_back(peer);
        }
    }
    
    // If we have fewer peers than requested, return all
    if (all_validated_peers.size() <= static_cast<size_t>(max_count)) {
        return all_validated_peers;
    }
    
    // Randomly select peers
    std::vector<RatsPeer> selected_peers;
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Use random sampling to select peers
    std::sample(all_validated_peers.begin(), all_validated_peers.end(),
                std::back_inserter(selected_peers), max_count, gen);
    
    return selected_peers;
}

const RatsPeer* RatsClient::get_peer_by_id(const std::string& peer_id) const {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    auto it = peers_.find(peer_id);
    return (it != peers_.end()) ? &it->second : nullptr;
}

const RatsPeer* RatsClient::get_peer_by_socket(socket_t socket) const {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    auto it = socket_to_peer_id_.find(socket);
    if (it != socket_to_peer_id_.end()) {
        auto peer_it = peers_.find(it->second);
        return (peer_it != peers_.end()) ? &peer_it->second : nullptr;
    }
    return nullptr;
}


// Peer limit management methods
int RatsClient::get_max_peers() const {
    return max_peers_;
}

void RatsClient::set_max_peers(int max_peers) {
    max_peers_ = max_peers;
    LOG_CLIENT_INFO("Maximum peers set to " << max_peers_);
}

bool RatsClient::is_peer_limit_reached() const {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    // Connected peers only enforcement (exclude handshake peers)
    int connected_peers = get_peer_count_unlocked();
    if (connected_peers >= max_peers_) {
        return true;
    }
    return false;
}

std::string RatsClient::generate_peer_hash_id(socket_t socket, const std::string& connection_info) {
    // Generate unique hash ID using timestamp, socket, connection info, and random component
    auto now = std::chrono::high_resolution_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    
    // Create a random component
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    // Build hash string
    std::ostringstream hash_stream;
    hash_stream << std::hex << timestamp << "_" << socket << "_";
    
    // Add connection info hash
    std::hash<std::string> hasher;
    hash_stream << hasher(connection_info) << "_";
    
    // Add random component
    for (int i = 0; i < 8; ++i) {
        hash_stream << std::setfill('0') << std::setw(2) << dis(gen);
    }
    
    return hash_stream.str();
}

std::string RatsClient::normalize_peer_address(const std::string& ip, int port) const {
    // Normalize IPv6 addresses and create consistent format
    std::string normalized_ip = ip;
    
    // Remove brackets from IPv6 addresses if present
    if (!normalized_ip.empty() && normalized_ip.front() == '[' && normalized_ip.back() == ']') {
        normalized_ip = normalized_ip.substr(1, normalized_ip.length() - 2);
    }
    
    // Handle localhost variations
    if (normalized_ip == "localhost" || normalized_ip == "::1") {
        normalized_ip = "127.0.0.1";
    }
    
    // For IPv6 addresses, add brackets for consistency
    if (normalized_ip.find(':') != std::string::npos && normalized_ip.find('.') == std::string::npos) {
        // This is likely an IPv6 address (contains colons but no dots)
        return "[" + normalized_ip + "]:" + std::to_string(port);
    }
    
    return normalized_ip + ":" + std::to_string(port);
}

// =========================================================================
// Callback Registration
// ========================================================================

void RatsClient::set_connection_callback(ConnectionCallback callback) {
    connection_callback_ = callback;
}

void RatsClient::set_binary_data_callback(BinaryDataCallback callback) {
    binary_data_callback_ = callback;
}

void RatsClient::set_string_data_callback(StringDataCallback callback) {
    string_data_callback_ = callback;
}

void RatsClient::set_json_data_callback(JsonDataCallback callback) {
    json_data_callback_ = callback;
}

void RatsClient::set_disconnect_callback(DisconnectCallback callback) {
    disconnect_callback_ = callback;
}

// NAT Traversal and ICE Callback Registration
void RatsClient::set_advanced_connection_callback(AdvancedConnectionCallback callback) {
    advanced_connection_callback_ = callback;
}

void RatsClient::set_nat_traversal_progress_callback(NatTraversalProgressCallback callback) {
    nat_progress_callback_ = callback;
}

void RatsClient::set_ice_candidate_callback(IceCandidateDiscoveredCallback callback) {
    ice_candidate_callback_ = callback;
}

// =========================================================================
// Peer Discovery Methods
// =========================================================================

bool RatsClient::start_dht_discovery(int dht_port) {
    if (dht_client_ && dht_client_->is_running()) {
        LOG_CLIENT_WARN("DHT discovery is already running");
        return true;
    }
    
    LOG_CLIENT_INFO("Starting DHT discovery on port " << dht_port <<
                   (bind_address_.empty() ? "" : " bound to " + bind_address_));
    
    dht_client_ = std::make_unique<DhtClient>(dht_port, bind_address_);
    if (!dht_client_->start()) {
        LOG_CLIENT_ERROR("Failed to start DHT client");
        dht_client_.reset();
        return false;
    }
    
    // Bootstrap with default nodes
    auto bootstrap_nodes = DhtClient::get_default_bootstrap_nodes();
    if (!dht_client_->bootstrap(bootstrap_nodes)) {
        LOG_CLIENT_WARN("Failed to bootstrap DHT");
    }
    
    // Start automatic peer discovery
    start_automatic_peer_discovery();
    
    LOG_CLIENT_INFO("DHT discovery started successfully");
    return true;
}

void RatsClient::stop_dht_discovery() {
    if (!dht_client_) {
        return;
    }
    
    LOG_CLIENT_INFO("Stopping DHT discovery");
    
    // Stop automatic peer discovery
    stop_automatic_peer_discovery();
    
    dht_client_->stop();
    dht_client_.reset();
    LOG_CLIENT_INFO("DHT discovery stopped");
}

bool RatsClient::find_peers_by_hash(const std::string& content_hash, std::function<void(const std::vector<std::string>&)> callback, int iteration_max) {
    if (!dht_client_ || !dht_client_->is_running()) {
        LOG_CLIENT_ERROR("DHT client not running");
        return false;
    }
    
    if (content_hash.length() != 40) {  // 160-bit hash as hex string
        LOG_CLIENT_ERROR("Invalid content hash length: " << content_hash.length() << " (expected 40)");
        return false;
    }
    
    LOG_CLIENT_INFO("Finding peers for content hash: " << content_hash << " with iteration max: " << iteration_max);
    
    InfoHash info_hash = hex_to_node_id(content_hash);
    
    return dht_client_->find_peers(info_hash, [this, callback](const std::vector<Peer>& peers, const InfoHash& info_hash) {
        handle_dht_peer_discovery(peers, info_hash);
        
        // Convert Peer to string addresses for callback
        std::vector<std::string> peer_addresses;
        for (const auto& peer : peers) {
            peer_addresses.push_back(peer.ip + ":" + std::to_string(peer.port));
        }
        
        if (callback) {
            callback(peer_addresses);
        }
    }, iteration_max);
}

bool RatsClient::announce_for_hash(const std::string& content_hash, uint16_t port) {
    if (!dht_client_ || !dht_client_->is_running()) {
        LOG_CLIENT_ERROR("DHT client not running");
        return false;
    }
    
    if (content_hash.length() != 40) {  // 160-bit hash as hex string
        LOG_CLIENT_ERROR("Invalid content hash length: " << content_hash.length() << " (expected 40)");
        return false;
    }
    
    if (port == 0) {
        port = listen_port_;
    }
    
    LOG_CLIENT_INFO("Announcing for content hash: " << content_hash << " on port " << port);
    
    InfoHash info_hash = hex_to_node_id(content_hash);
    return dht_client_->announce_peer(info_hash, port);
}

bool RatsClient::is_dht_running() const {
    return dht_client_ && dht_client_->is_running();
}

size_t RatsClient::get_dht_routing_table_size() const {
    if (!dht_client_) {
        return 0;
    }
    return dht_client_->get_routing_table_size();
}

void RatsClient::handle_dht_peer_discovery(const std::vector<Peer>& peers, const InfoHash& info_hash) {
    LOG_CLIENT_INFO("DHT discovered " << peers.size() << " peers for info hash: " << node_id_to_hex(info_hash));
    
    // Auto-connect to discovered peers (optional behavior)
    for (const auto& peer : peers) {
        // Check if this peer should be ignored (local interface)
        if (should_ignore_peer(peer.ip, peer.port)) {
            LOG_CLIENT_DEBUG("Ignoring discovered peer " << peer.ip << ":" << peer.port << " - local interface address");
            continue;
        }
        
        // Check if we're already connected to this peer
        std::string normalized_peer_address = normalize_peer_address(peer.ip, peer.port);
        bool already_connected = is_already_connected_to_address(normalized_peer_address);
        
        if (!already_connected) {
            // Check if peer limit is reached
            if (is_peer_limit_reached()) {
                LOG_CLIENT_DEBUG("Peer limit reached, not connecting to DHT discovered peer " << peer.ip << ":" << peer.port);
                continue;
            }
            
            LOG_CLIENT_DEBUG("Attempting to connect to discovered peer: " << peer.ip << ":" << peer.port);
            
            // Try to connect to the peer (non-blocking)
            std::thread([this, peer]() {
                if (connect_to_peer(peer.ip, peer.port)) {
                    LOG_CLIENT_INFO("Successfully connected to DHT discovered peer: " << peer.ip << ":" << peer.port);
                } else {
                    LOG_CLIENT_DEBUG("Failed to connect to DHT discovered peer: " << peer.ip << ":" << peer.port);
                }
            }).detach();
        } else {
            LOG_CLIENT_DEBUG("Already connected to discovered peer: " << normalized_peer_address);
        }
    }
}

void RatsClient::start_automatic_peer_discovery() {
    if (auto_discovery_running_.load()) {
        LOG_CLIENT_WARN("Automatic peer discovery is already running");
        return;
    }
    
    LOG_CLIENT_INFO("Starting automatic rats peer discovery");
    auto_discovery_running_.store(true);
    auto_discovery_thread_ = std::thread(&RatsClient::automatic_discovery_loop, this);
}

void RatsClient::stop_automatic_peer_discovery() {
    if (!auto_discovery_running_.load()) {
        return;
    }
    
    LOG_CLIENT_INFO("Stopping automatic peer discovery");
    auto_discovery_running_.store(false);
    
    if (auto_discovery_thread_.joinable()) {
        auto_discovery_thread_.join();
    }
    
    LOG_CLIENT_INFO("Automatic peer discovery stopped");
}

bool RatsClient::is_automatic_discovery_running() const {
    return auto_discovery_running_.load();
}

// This function will be removed - implementation moved to end of file

void RatsClient::automatic_discovery_loop() {
    LOG_CLIENT_INFO("Automatic peer discovery loop started");
    
    // Initial delay to let DHT bootstrap
    {
        std::unique_lock<std::mutex> lock(shutdown_mutex_);
        if (shutdown_cv_.wait_for(lock, std::chrono::seconds(5), [this] { return !auto_discovery_running_.load() || !running_.load(); })) {
            LOG_CLIENT_INFO("Automatic peer discovery loop stopped during initial delay");
            return;
        }
    }

    // Search immediately
    search_rats_peers(5);
    
    {
        std::unique_lock<std::mutex> lock(shutdown_mutex_);
        if (shutdown_cv_.wait_for(lock, std::chrono::seconds(10), [this] { return !auto_discovery_running_.load() || !running_.load(); })) {
            LOG_CLIENT_INFO("Automatic peer discovery loop stopped during search delay");
            return;
        }
    }
    
    // Announce immediately
    announce_rats_peer();

    auto last_announce = std::chrono::steady_clock::now();
    auto last_search = std::chrono::steady_clock::now();
    
    while (auto_discovery_running_.load()) {
        auto now = std::chrono::steady_clock::now();
        
        if (get_peer_count() == 0) {
            // No peers: aggressive search and announce
            if (now - last_search >= std::chrono::seconds(2)) {
                search_rats_peers();
                last_search = now;
            }
            if (now - last_announce >= std::chrono::seconds(10)) {
                announce_rats_peer();
                last_announce = now;
            }
        } else {
            // Peers connected: less aggressive, similar to original logic
            if (now - last_search >= std::chrono::minutes(5)) {
                search_rats_peers();
                last_search = now;
            }
            if (now - last_announce >= std::chrono::minutes(10)) {
                announce_rats_peer();
                last_announce = now;
            }
        }
        
        // Use conditional variable for responsive shutdown
        {
            std::unique_lock<std::mutex> lock(shutdown_mutex_);
            if (shutdown_cv_.wait_for(lock, std::chrono::milliseconds(500), [this] { return !auto_discovery_running_.load() || !running_.load(); })) {
                break;
            }
        }
    }
    
    LOG_CLIENT_INFO("Automatic peer discovery loop stopped");
}

void RatsClient::announce_rats_peer() {
    if (!dht_client_ || !dht_client_->is_running()) {
        LOG_CLIENT_WARN("DHT client not running, cannot announce peer");
        return;
    }
    
    std::string discovery_hash = get_discovery_hash();
    LOG_CLIENT_INFO("Announcing peer for discovery hash: " << discovery_hash << " on port " << listen_port_);
    
    if (announce_for_hash(discovery_hash, listen_port_)) {
        LOG_CLIENT_DEBUG("Successfully announced peer for discovery");
    } else {
        LOG_CLIENT_WARN("Failed to announce peer for discovery");
    }
}

void RatsClient::search_rats_peers(int iteration_max) {
    if (!dht_client_ || !dht_client_->is_running()) {
        LOG_CLIENT_WARN("DHT client not running, cannot search for peers");
        return;
    }
    
    std::string discovery_hash = get_discovery_hash();
    LOG_CLIENT_INFO("Searching for peers using discovery hash: " << discovery_hash << " with iteration max: " << iteration_max);
    
    find_peers_by_hash(discovery_hash, [this](const std::vector<std::string>& peers) {
        LOG_CLIENT_INFO("Found " << peers.size() << " peers through DHT discovery");
        // Note: Connection attempts are handled by handle_dht_peer_discovery() which is called
        // automatically by find_peers_by_hash(), so no need to duplicate connection logic here
        for (const auto& peer_address : peers) {
            LOG_CLIENT_DEBUG("Discovered peer: " << peer_address);
        }
    }, iteration_max);
}

std::string RatsClient::get_discovery_hash() const {
    std::lock_guard<std::mutex> lock(protocol_config_mutex_);
    // Generate discovery hash based on current protocol configuration
    std::string discovery_string = custom_protocol_name_ + "_peer_discovery_v" + custom_protocol_version_;
    return SHA1::hash(discovery_string);
}

std::string RatsClient::get_rats_peer_discovery_hash() {
    // Well-known hash for rats peer discovery
    // Compute SHA1 hash of "rats_peer_discovery_v1.0"
    return SHA1::hash("rats_peer_discovery_v1.0");
}



// =========================================================================
// Protocol Configuration
// =========================================================================

void RatsClient::set_protocol_name(const std::string& protocol_name) {
    std::lock_guard<std::mutex> lock(protocol_config_mutex_);
    custom_protocol_name_ = protocol_name;
    LOG_CLIENT_INFO("Protocol name set to: " << protocol_name);
}

void RatsClient::set_protocol_version(const std::string& protocol_version) {
    std::lock_guard<std::mutex> lock(protocol_config_mutex_);
    custom_protocol_version_ = protocol_version;
    LOG_CLIENT_INFO("Protocol version set to: " << protocol_version);
}

std::string RatsClient::get_protocol_name() const {
    std::lock_guard<std::mutex> lock(protocol_config_mutex_);
    return custom_protocol_name_;
}

std::string RatsClient::get_protocol_version() const {
    std::lock_guard<std::mutex> lock(protocol_config_mutex_);
    return custom_protocol_version_;
}

// =========================================================================
// Message Exchange API
// =========================================================================


void RatsClient::on(const std::string& message_type, MessageCallback callback) {
    std::lock_guard<std::mutex> lock(message_handlers_mutex_);
    message_handlers_[message_type].emplace_back(callback, false); // false = not once
    LOG_CLIENT_INFO("Registered persistent handler for message type: " << message_type << " (total handlers: " << message_handlers_[message_type].size() << ")");
}

void RatsClient::once(const std::string& message_type, MessageCallback callback) {
    std::lock_guard<std::mutex> lock(message_handlers_mutex_);
    message_handlers_[message_type].emplace_back(callback, true); // true = once
    LOG_CLIENT_DEBUG("Registered one-time handler for message type: " << message_type);
}

void RatsClient::off(const std::string& message_type) {
    std::lock_guard<std::mutex> lock(message_handlers_mutex_);
    auto it = message_handlers_.find(message_type);
    if (it != message_handlers_.end()) {
        size_t removed_count = it->second.size();
        message_handlers_.erase(it);
        LOG_CLIENT_DEBUG("Removed " << removed_count << " handlers for message type: " << message_type);
    }
}

void RatsClient::send(const std::string& message_type, const nlohmann::json& data, SendCallback callback) {
    if (!running_.load()) {
        LOG_CLIENT_ERROR("Cannot send message '" << message_type << "' - client is not running");
        if (callback) {
            callback(false, "Client is not running");
        }
        return;
    }
    
    LOG_CLIENT_INFO("Sending broadcast message type '" << message_type << "' with data: " << data.dump());
    
    // Create rats message
    nlohmann::json message = create_rats_message(message_type, data, get_our_peer_id());
    
    // Broadcast to all validated peers
    int sent_count = broadcast_rats_message_to_validated_peers(message);
    
    LOG_CLIENT_INFO("Broadcasted message type '" << message_type << "' to " << sent_count << " peers");
    
    if (callback) {
        if (sent_count > 0) {
            callback(true, "");
        } else {
            LOG_CLIENT_WARN("No peers to send message to");
            callback(false, "No peers to send message to");
        }
    }
}

void RatsClient::send(const std::string& peer_id, const std::string& message_type, const nlohmann::json& data, SendCallback callback) {
    if (!running_.load()) {
        LOG_CLIENT_ERROR("Cannot send message '" << message_type << "' to peer " << peer_id << " - client is not running");
        if (callback) {
            callback(false, "Client is not running");
        }
        return;
    }
    
    LOG_CLIENT_INFO("Sending targeted message type '" << message_type << "' to peer " << peer_id << " with data: " << data.dump());
    
    // Create rats message
    nlohmann::json message = create_rats_message(message_type, data, get_our_peer_id());
    
    // Send to specific peer
    socket_t target_socket = INVALID_SOCKET_VALUE;
    bool peer_found = false;
    bool handshake_completed = false;
    
    {
        std::lock_guard<std::mutex> lock(peers_mutex_);
        auto it = peers_.find(peer_id);
        if (it != peers_.end()) {
            peer_found = true;
            handshake_completed = it->second.is_handshake_completed();
            if (handshake_completed) {
                target_socket = it->second.socket;
            }
        }
    }
    
    if (!peer_found) {
        LOG_CLIENT_ERROR("Cannot send message '" << message_type << "' - peer not found: " << peer_id);
        if (callback) {
            callback(false, "Peer not found: " + peer_id);
        }
        return;
    }
    
    if (!handshake_completed) {
        LOG_CLIENT_ERROR("Cannot send message '" << message_type << "' - peer handshake not completed: " << peer_id);
        if (callback) {
            callback(false, "Peer handshake not completed: " + peer_id);
        }
        return;
    }
    
    bool success = send_json_to_peer(target_socket, message);
    
    LOG_CLIENT_INFO("Sent message type '" << message_type << "' to peer " << peer_id << " - " << (success ? "success" : "failed"));
    
    if (callback) {
        if (success) {
            callback(true, "");
        } else {
            callback(false, "Failed to send message to peer: " + peer_id);
        }
    }
}

// Message exchange system helpers
void RatsClient::call_message_handlers(const std::string& message_type, const std::string& peer_id, const nlohmann::json& data) {
    std::vector<MessageHandler> handlers_to_call;
    std::vector<MessageHandler> remaining_handlers;
    
    LOG_CLIENT_INFO("Calling message handlers for type '" << message_type << "' from peer " << peer_id << " with data: " << data.dump());
    
    // Get handlers to call and identify once handlers
    {
        std::lock_guard<std::mutex> lock(message_handlers_mutex_);
        auto it = message_handlers_.find(message_type);
        if (it != message_handlers_.end()) {
            handlers_to_call = it->second; // Copy handlers
            
            // Keep only non-once handlers for the remaining list
            for (const auto& handler : it->second) {
                if (!handler.is_once) {
                    remaining_handlers.push_back(handler);
                }
            }
            
            // Update the handlers list (removes once handlers)
            it->second = remaining_handlers;
        } else {
            LOG_CLIENT_WARN("No handlers registered for message type '" << message_type << "'");
        }
    }
    
    LOG_CLIENT_INFO("Found " << handlers_to_call.size() << " handlers for message type '" << message_type << "'");
    
    // Call handlers outside of mutex to avoid deadlock
    for (const auto& handler : handlers_to_call) {
        try {
            LOG_CLIENT_INFO("Calling handler for message type '" << message_type << "'");
            handler.callback(peer_id, data);
            LOG_CLIENT_INFO("Handler for message type '" << message_type << "' completed successfully");
        } catch (const std::exception& e) {
            LOG_CLIENT_ERROR("Exception in message handler for type '" << message_type << "': " << e.what());
        } catch (...) {
            LOG_CLIENT_ERROR("Unknown exception in message handler for type '" << message_type << "'");
        }
    }
    
    if (!handlers_to_call.empty()) {
        LOG_CLIENT_INFO("Called " << handlers_to_call.size() << " handlers for message type '" << message_type << "'");
    }
}

void RatsClient::remove_once_handlers(const std::string& message_type) {
    std::lock_guard<std::mutex> lock(message_handlers_mutex_);
    auto it = message_handlers_.find(message_type);
    if (it != message_handlers_.end()) {
        auto& handlers = it->second;
        auto new_end = std::remove_if(handlers.begin(), handlers.end(), 
                                     [](const MessageHandler& handler) { return handler.is_once; });
        handlers.erase(new_end, handlers.end());
        
        // Remove the entire entry if no handlers remain
        if (handlers.empty()) {
            message_handlers_.erase(it);
        }
    }
}

// =========================================================================
// Rats messages protocol / Message handling system
// =========================================================================

nlohmann::json RatsClient::create_rats_message(const std::string& type, const nlohmann::json& payload, const std::string& sender_peer_id) {
    nlohmann::json message;
    message["rats_protocol"] = true;
    message["type"] = type;
    message["payload"] = payload;
    message["sender_peer_id"] = sender_peer_id;
    message["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    
    return message;
}

void RatsClient::handle_rats_message(socket_t socket, const std::string& peer_hash_id, const nlohmann::json& message) {
    try {
        std::string message_type = message.value("type", "");
        nlohmann::json payload = message.value("payload", nlohmann::json::object());
        std::string sender_peer_id = message.value("sender_peer_id", "");
        
        LOG_CLIENT_DEBUG("Received rats message type '" << message_type << "' from " << peer_hash_id);
        
        // Call registered message handlers for all message types (including custom ones)
        call_message_handlers(message_type, sender_peer_id.empty() ? peer_hash_id : sender_peer_id, payload);
        
        // Handle built-in message types for internal functionality
        if (message_type == "peer") {
            handle_peer_exchange_message(socket, peer_hash_id, payload);
        } 
        else if (message_type == "peers_request") {
            handle_peers_request_message(socket, peer_hash_id, payload);
        }
        else if (message_type == "peers_response") {
            handle_peers_response_message(socket, peer_hash_id, payload);
        }
        // ICE coordination messages
        else if (message_type == "ice_offer") {
            handle_ice_offer_message(socket, peer_hash_id, payload);
        }
        else if (message_type == "ice_answer") {
            handle_ice_answer_message(socket, peer_hash_id, payload);
        }
        else if (message_type == "ice_candidate") {
            handle_ice_candidate_message(socket, peer_hash_id, payload);
        }
        // NAT traversal coordination messages
        else if (message_type == "hole_punch_coordination") {
            handle_hole_punch_coordination_message(socket, peer_hash_id, payload);
        }
        else if (message_type == "nat_info_exchange") {
            handle_nat_info_exchange_message(socket, peer_hash_id, payload);
        }
        // Custom message types are now handled by registered handlers above
        // No need for else clause - all message types are valid if they have registered handlers
        
    } catch (const nlohmann::json::exception& e) {
        LOG_CLIENT_ERROR("Failed to handle rats message: " << e.what());
    }
}

void RatsClient::handle_peer_exchange_message(socket_t socket, const std::string& peer_hash_id, const nlohmann::json& payload) {
    try {
        std::string peer_ip = payload.value("ip", "");
        int peer_port = payload.value("port", 0);
        std::string peer_id = payload.value("peer_id", "");
        
        if (peer_ip.empty() || peer_port <= 0 || peer_id.empty()) {
            LOG_CLIENT_WARN("Invalid peer exchange message from " << peer_hash_id);
            return;
        }
        
        LOG_CLIENT_INFO("Received peer exchange: " << peer_ip << ":" << peer_port << " (peer_id: " << peer_id << ")");
        
        // Check if we should ignore this peer (local interface)
        if (should_ignore_peer(peer_ip, peer_port)) {
            LOG_CLIENT_DEBUG("Ignoring exchanged peer " << peer_ip << ":" << peer_port << " - local interface address");
            return;
        }
        
        // Check if we're already connected to this peer
        std::string normalized_peer_address = normalize_peer_address(peer_ip, peer_port);
        if (is_already_connected_to_address(normalized_peer_address)) {
            LOG_CLIENT_DEBUG("Already connected to exchanged peer " << normalized_peer_address);
            return;
        }
        
        // Check if peer limit is reached
        if (is_peer_limit_reached()) {
            LOG_CLIENT_DEBUG("Peer limit reached, not connecting to exchanged peer " << peer_ip << ":" << peer_port);
            return;
        }
        
        // Try to connect to the exchanged peer (non-blocking)
        add_managed_thread(std::thread([this, peer_ip, peer_port, peer_id]() {
            if (connect_to_peer(peer_ip, peer_port)) {
                LOG_CLIENT_INFO("Successfully connected to exchanged peer: " << peer_ip << ":" << peer_port);
            } else {
                LOG_CLIENT_DEBUG("Failed to connect to exchanged peer: " << peer_ip << ":" << peer_port);
            }
        }), "peer-exchange-connect-" + peer_id.substr(0, 8));
        
    } catch (const nlohmann::json::exception& e) {
        LOG_CLIENT_ERROR("Failed to handle peer exchange message: " << e.what());
    }
}

// General broadcasting functions
int RatsClient::broadcast_rats_message(const nlohmann::json& message, const std::string& exclude_peer_id) {
    int sent_count = 0;
    {
        std::lock_guard<std::mutex> lock(peers_mutex_);
        for (const auto& pair : peers_) {
            const RatsPeer& peer = pair.second;
            // Don't send to excluded peer
            if (!exclude_peer_id.empty() && peer.peer_id == exclude_peer_id) {
                continue;
            }
            
            if (send_json_to_peer(peer.socket, message)) {
                sent_count++;
            }
        }
    }
    return sent_count;
}

int RatsClient::broadcast_rats_message_to_validated_peers(const nlohmann::json& message, const std::string& exclude_peer_id) {
    int sent_count = 0;
    {
        std::lock_guard<std::mutex> lock(peers_mutex_);
        for (const auto& pair : peers_) {
            const RatsPeer& peer = pair.second;
            // Don't send to excluded peer and only send to peers with completed handshake
            if ((!exclude_peer_id.empty() && peer.peer_id == exclude_peer_id) || 
                !peer.is_handshake_completed()) {
                continue;
            }
            
            if (send_json_to_peer(peer.socket, message)) {
                sent_count++;
            }
        }
    }
    return sent_count;
}

// Specific message creation functions
nlohmann::json RatsClient::create_peer_exchange_message(const RatsPeer& peer) {
    // Create peer exchange payload
    nlohmann::json payload;
    payload["ip"] = peer.ip;
    payload["port"] = peer.port;
    payload["peer_id"] = peer.peer_id;
    payload["connection_type"] = peer.is_outgoing ? "outgoing" : "incoming";
    
    // Create rats message
    return create_rats_message("peer", payload, peer.peer_id);
}

void RatsClient::broadcast_peer_exchange_message(const RatsPeer& new_peer) {
    // Don't broadcast exchange messages for ourselves
    if (new_peer.peer_id.empty()) {
        return;
    }
    
    // Create peer exchange message
    nlohmann::json message = create_peer_exchange_message(new_peer);
    
    // Broadcast to all validated peers except the new peer
    int sent_count = broadcast_rats_message_to_validated_peers(message, new_peer.peer_id);
    
    LOG_CLIENT_INFO("Broadcasted peer exchange message for " << new_peer.ip << ":" << new_peer.port 
                    << " to " << sent_count << " peers");
}

// Peers request/response system implementation
nlohmann::json RatsClient::create_peers_request_message(const std::string& sender_peer_id) {
    nlohmann::json payload;
    payload["max_peers"] = 5;  // Request up to 5 peers
    payload["requester_info"] = {
        {"listen_port", listen_port_},
        {"peer_count", get_peer_count()}
    };
    
    return create_rats_message("peers_request", payload, sender_peer_id);
}

nlohmann::json RatsClient::create_peers_response_message(const std::vector<RatsPeer>& peers, const std::string& sender_peer_id) {
    nlohmann::json payload;
    nlohmann::json peers_array = nlohmann::json::array();
    
    for (const auto& peer : peers) {
        nlohmann::json peer_info;
        peer_info["ip"] = peer.ip;
        peer_info["port"] = peer.port;
        peer_info["peer_id"] = peer.peer_id;
        peer_info["connection_type"] = peer.is_outgoing ? "outgoing" : "incoming";
        peers_array.push_back(peer_info);
    }
    
    payload["peers"] = peers_array;
    payload["total_peers"] = get_peer_count();
    
    return create_rats_message("peers_response", payload, sender_peer_id);
}


void RatsClient::handle_peers_request_message(socket_t socket, const std::string& peer_hash_id, const nlohmann::json& payload) {
    try {
        int max_peers = payload.value("max_peers", 5);
        
        LOG_CLIENT_INFO("Received peers request from " << peer_hash_id << " for up to " << max_peers << " peers");
        
        // Get random peers excluding the requester
        std::vector<RatsPeer> random_peers = get_random_peers(max_peers, peer_hash_id);
        
        LOG_CLIENT_DEBUG("Sending " << random_peers.size() << " peers to " << peer_hash_id);
        
        // Create and send peers response
        nlohmann::json response_message = create_peers_response_message(random_peers, peer_hash_id);
        
        if (!send_json_to_peer(socket, response_message)) {
            LOG_CLIENT_ERROR("Failed to send peers response to " << peer_hash_id);
        } else {
            LOG_CLIENT_DEBUG("Sent peers response with " << random_peers.size() << " peers to " << peer_hash_id);
        }
        
    } catch (const nlohmann::json::exception& e) {
        LOG_CLIENT_ERROR("Failed to handle peers request message: " << e.what());
    }
}

void RatsClient::handle_peers_response_message(socket_t socket, const std::string& peer_hash_id, const nlohmann::json& payload) {
    try {
        nlohmann::json peers_array = payload.value("peers", nlohmann::json::array());
        int total_peers = payload.value("total_peers", 0);
        
        LOG_CLIENT_INFO("Received peers response from " << peer_hash_id << " with " << peers_array.size() 
                        << " peers (total: " << total_peers << ")");
        
        // Process each peer in the response
        for (const auto& peer_info : peers_array) {
            std::string peer_ip = peer_info.value("ip", "");
            int peer_port = peer_info.value("port", 0);
            std::string peer_id = peer_info.value("peer_id", "");
            
            if (peer_ip.empty() || peer_port <= 0 || peer_id.empty()) {
                LOG_CLIENT_WARN("Invalid peer info in peers response from " << peer_hash_id);
                continue;
            }
            
            LOG_CLIENT_DEBUG("Processing peer from response: " << peer_ip << ":" << peer_port << " (peer_id: " << peer_id << ")");
            
            // Check if we should ignore this peer (local interface)
            if (should_ignore_peer(peer_ip, peer_port)) {
                LOG_CLIENT_DEBUG("Ignoring peer from response " << peer_ip << ":" << peer_port << " - local interface address");
                continue;
            }
            
            // Check if we're already connected to this peer
            std::string normalized_peer_address = normalize_peer_address(peer_ip, peer_port);
            if (is_already_connected_to_address(normalized_peer_address)) {
                LOG_CLIENT_DEBUG("Already connected to peer from response " << normalized_peer_address);
                continue;
            }
            
            // Check if peer limit is reached
            if (is_peer_limit_reached()) {
                LOG_CLIENT_DEBUG("Peer limit reached, not connecting to peer from response " << peer_ip << ":" << peer_port);
                continue;
            }
            
            // Try to connect to the peer (non-blocking)
            LOG_CLIENT_INFO("Attempting to connect to peer from response: " << peer_ip << ":" << peer_port);
            add_managed_thread(std::thread([this, peer_ip, peer_port, peer_id]() {
                if (connect_to_peer(peer_ip, peer_port)) {
                    LOG_CLIENT_INFO("Successfully connected to peer from response: " << peer_ip << ":" << peer_port);
                } else {
                    LOG_CLIENT_DEBUG("Failed to connect to peer from response: " << peer_ip << ":" << peer_port);
                }
            }), "peer-response-connect-" + peer_id.substr(0, 8));
        }
        
    } catch (const nlohmann::json::exception& e) {
        LOG_CLIENT_ERROR("Failed to handle peers response message: " << e.what());
    }
}

void RatsClient::send_peers_request(socket_t socket, const std::string& our_peer_id) {
    nlohmann::json request_message = create_peers_request_message(our_peer_id);
    
    if (send_json_to_peer(socket, request_message)) {
        LOG_CLIENT_INFO("Sent peers request to socket " << socket);
    } else {
        LOG_CLIENT_ERROR("Failed to send peers request to socket " << socket);
    }
}

// =========================================================================
// Statistics and Information
// =========================================================================

nlohmann::json RatsClient::get_connection_statistics() const {
    nlohmann::json stats;
    
    {
        std::lock_guard<std::mutex> lock(peers_mutex_);
        stats["total_peers"] = peers_.size();
        stats["validated_peers"] = get_peer_count_unlocked();
        stats["max_peers"] = max_peers_;
    }
    
    stats["running"] = is_running();
    stats["listen_port"] = listen_port_;
    stats["our_peer_id"] = get_our_peer_id();
    stats["encryption_enabled"] = is_encryption_enabled();
    stats["public_ip"] = get_public_ip();
    
    // DHT statistics
    if (dht_client_ && dht_client_->is_running()) {
        stats["dht_running"] = true;
        stats["dht_routing_table_size"] = get_dht_routing_table_size();
    } else {
        stats["dht_running"] = false;
    }
    
    // mDNS statistics
    stats["mdns_running"] = is_mdns_running();
    
    return stats;
}


// =========================================================================
// Helper functions
// =========================================================================

std::unique_ptr<RatsClient> create_rats_client(int listen_port) {
    auto client = std::make_unique<RatsClient>(listen_port, 10); // Default 10 max peers
    if (!client->start()) {
        return nullptr;
    }
    return client;
}

// Version query functions
const char* rats_get_library_version_string() {
    return librats::version::STRING;
}

void rats_get_library_version(int* major, int* minor, int* patch, int* build) {
    if (major) *major = librats::version::MAJOR;
    if (minor) *minor = librats::version::MINOR;
    if (patch) *patch = librats::version::PATCH;
    if (build) *build = librats::version::BUILD;
}

const char* rats_get_library_git_describe() {
    return librats::version::GIT_DESCRIBE;
}

uint32_t rats_get_library_abi() {
    // ABI policy: MAJOR bumps on breaking changes; MINOR for additive; PATCH ignored in ABI id
    return (static_cast<uint32_t>(librats::version::MAJOR) << 16) |
           (static_cast<uint32_t>(librats::version::MINOR) << 8) |
           (static_cast<uint32_t>(librats::version::PATCH));
}

bool RatsClient::parse_address_string(const std::string& address_str, std::string& out_ip, int& out_port) {
    if (address_str.empty()) {
        return false;
    }

    size_t colon_pos;
    if (address_str.front() == '[') {
        // IPv6 format: [ip]:port
        size_t bracket_end = address_str.find(']');
        if (bracket_end == std::string::npos || bracket_end < 2) { // Must be at least [a]
            return false;
        }
        out_ip = address_str.substr(1, bracket_end - 1);
        colon_pos = address_str.find(':', bracket_end);
    } else {
        // IPv4 or IPv6 without brackets
        colon_pos = address_str.find_last_of(':');
        if (colon_pos == std::string::npos || colon_pos == 0) {
            return false;
        }
        out_ip = address_str.substr(0, colon_pos);
    }

    if (colon_pos == std::string::npos || colon_pos + 1 >= address_str.length()) {
        return false;
    }

    try {
        out_port = std::stoi(address_str.substr(colon_pos + 1));
    } catch (const std::exception&) {
        return false;
    }

    return !out_ip.empty() && out_port > 0 && out_port <= 65535;
}

std::string get_box_separator() {
    return supports_unicode() ? 
        "════════════════════════════════════════════════════════════════════" :
        "=====================================================================";
}

std::string get_box_vertical() {
    return supports_unicode() ? "│" : "|";
}

std::string get_checkmark() {
    return supports_unicode() ? "✓" : "[*]";
}

void RatsClient::log_handshake_completion_unlocked(const RatsPeer& peer) {
    // Calculate connection duration
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - peer.connected_at);
    
    // Get current peer count (assumes peers_mutex_ is already locked)
    int current_peer_count = get_peer_count_unlocked();
    
    // Create visually appealing log output
    std::string connection_type = peer.is_outgoing ? "OUTGOING" : "INCOMING";
    std::string separator = get_box_separator();
    std::string vertical = get_box_vertical();
    std::string checkmark = get_checkmark();
    
    LOG_CLIENT_INFO("");
    LOG_CLIENT_INFO(separator);
    LOG_CLIENT_INFO(checkmark << " HANDSHAKE COMPLETED - NEW PEER CONNECTED");
    LOG_CLIENT_INFO(separator);
    LOG_CLIENT_INFO(vertical << " Peer ID       : " << peer.peer_id);
    LOG_CLIENT_INFO(vertical << " Address       : " << peer.ip << ":" << peer.port);
    LOG_CLIENT_INFO(vertical << " Connection    : " << connection_type);
    LOG_CLIENT_INFO(vertical << " Protocol Ver. : " << peer.version);
    LOG_CLIENT_INFO(vertical << " Socket        : " << peer.socket);
    LOG_CLIENT_INFO(vertical << " Duration      : " << duration.count() << "ms");
    LOG_CLIENT_INFO(vertical << " Network Peers : " << current_peer_count << "/" << max_peers_);
    
    LOG_CLIENT_INFO(separator);
    LOG_CLIENT_INFO("");
}

void RatsClient::log_handshake_completion(const RatsPeer& peer) {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    log_handshake_completion_unlocked(peer);
}

} // namespace librats