#include "librats.h"
#include "stun.h"
#include "network_utils.h"
#include <algorithm>
#include <random>

// Logging macros for NAT operations
#ifdef TESTING
#define LOG_NAT_DEBUG(message) LOG_DEBUG("nat", "[pointer: " << this << "] " << message)
#define LOG_NAT_INFO(message)  LOG_INFO("nat", "[pointer: " << this << "] " << message)
#define LOG_NAT_WARN(message)  LOG_WARN("nat", "[pointer: " << this << "] " << message)
#define LOG_NAT_ERROR(message) LOG_ERROR("nat", "[pointer: " << this << "] " << message)
#else
#define LOG_NAT_DEBUG(message) LOG_DEBUG("nat", message)
#define LOG_NAT_INFO(message)  LOG_INFO("nat", message)
#define LOG_NAT_WARN(message)  LOG_WARN("nat", message)
#define LOG_NAT_ERROR(message) LOG_ERROR("nat", message)
#endif

namespace librats {

//=============================================================================
// NAT Traversal Initialization and Detection
//=============================================================================

void RatsClient::initialize_nat_traversal() {
    LOG_NAT_INFO("Initializing NAT traversal capabilities");
    
    // Detect and cache NAT type in background thread
    add_managed_thread(std::thread([this]() {
        detect_and_cache_nat_type();
    }), "nat-detection");
    
    // Initialize ICE if enabled
    if (ice_agent_) {
        LOG_NAT_INFO("Starting ICE agent for NAT traversal");
        if (!ice_agent_->start()) {
            LOG_NAT_ERROR("Failed to start ICE agent");
        } else {
            LOG_NAT_INFO("ICE agent started successfully");
        }
    }
}

void RatsClient::detect_and_cache_nat_type() {
    if (!nat_detector_) {
        LOG_NAT_ERROR("NAT detector not initialized");
        return;
    }
    
    // Check if client is still running before proceeding
    if (!running_.load()) {
        LOG_NAT_DEBUG("Client not running, skipping NAT detection");
        return;
    }
    
    LOG_NAT_INFO("Detecting NAT type and characteristics");
    
    try {
        // Use STUN servers from configuration
        std::vector<std::string> stun_servers = nat_config_.stun_servers;
        if (stun_servers.empty()) {
            stun_servers.push_back("stun.l.google.com:19302");
            stun_servers.push_back("stun1.l.google.com:19302");
        }
        
        // Check again before mutex access
        if (!running_.load()) {
            LOG_NAT_DEBUG("Client stopped during NAT detection setup");
            return;
        }
        
        // Detect detailed NAT characteristics
        {
            std::lock_guard<std::mutex> lock(nat_mutex_);
            nat_characteristics_ = nat_detector_->detect_nat_characteristics(stun_servers, 5000);
            
            // Map detailed characteristics to simple NAT type
            detected_nat_type_ = map_characteristics_to_nat_type(nat_characteristics_);
        }
        
        log_nat_detection_results();
        
    } catch (const std::exception& e) {
        LOG_NAT_ERROR("NAT detection failed: " << e.what());
        // Only try to update state if client is still running
        if (running_.load()) {
            std::lock_guard<std::mutex> lock(nat_mutex_);
            detected_nat_type_ = NatType::UNKNOWN;
        }
    }
}

NatType RatsClient::map_characteristics_to_nat_type(const NatTypeInfo& characteristics) {
    if (!characteristics.has_nat) {
        return NatType::OPEN_INTERNET;
    } else if (characteristics.mapping_behavior == NatBehavior::ENDPOINT_INDEPENDENT &&
              characteristics.filtering_behavior == NatBehavior::ENDPOINT_INDEPENDENT) {
        return NatType::FULL_CONE;
    } else if (characteristics.mapping_behavior == NatBehavior::ENDPOINT_INDEPENDENT &&
              characteristics.filtering_behavior == NatBehavior::ADDRESS_DEPENDENT) {
        return NatType::RESTRICTED_CONE;
    } else if (characteristics.mapping_behavior == NatBehavior::ENDPOINT_INDEPENDENT &&
              characteristics.filtering_behavior == NatBehavior::ADDRESS_PORT_DEPENDENT) {
        return NatType::PORT_RESTRICTED;
    } else if (characteristics.mapping_behavior == NatBehavior::ADDRESS_PORT_DEPENDENT) {
        return NatType::SYMMETRIC;
    } else {
        return NatType::UNKNOWN;
    }
}

void RatsClient::log_nat_detection_results() {
    LOG_NAT_INFO("NAT detection completed:");
    LOG_NAT_INFO("  Type: " << static_cast<int>(detected_nat_type_));
    LOG_NAT_INFO("  Has NAT: " << nat_characteristics_.has_nat);
    LOG_NAT_INFO("  Filtering: " << static_cast<int>(nat_characteristics_.filtering_behavior));
    LOG_NAT_INFO("  Mapping: " << static_cast<int>(nat_characteristics_.mapping_behavior));
    LOG_NAT_INFO("  Port Preservation: " << nat_characteristics_.preserves_port);
    LOG_NAT_INFO("  Hairpin Support: " << nat_characteristics_.hairpin_support);
}

NatType RatsClient::detect_nat_type() {
    std::lock_guard<std::mutex> lock(nat_mutex_);
    return detected_nat_type_;
}

NatTypeInfo RatsClient::get_nat_characteristics() {
    std::lock_guard<std::mutex> lock(nat_mutex_);
    return nat_characteristics_;
}

//=============================================================================
// STUN and Public IP Discovery
//=============================================================================

bool RatsClient::discover_and_ignore_public_ip(const std::string& stun_server, int stun_port) {
    if (!stun_client_) {
        LOG_NAT_ERROR("STUN client not initialized");
        return false;
    }
    
    LOG_NAT_INFO("Discovering public IP address using STUN server: " << stun_server << ":" << stun_port);
    
    StunAddress public_address;
    if (!stun_client_->get_public_address(stun_server, stun_port, public_address)) {
        LOG_NAT_ERROR("Failed to discover public IP address via STUN");
        return false;
    }
    
    // Store the discovered public IP
    {
        std::lock_guard<std::mutex> lock(public_ip_mutex_);
        public_ip_ = public_address.ip;
    }
    
    LOG_NAT_INFO("Discovered public IP address: " << public_address.ip << " (port: " << public_address.port << ")");
    
    // Add to ignore list
    add_ignored_address(public_address.ip);
    
    LOG_NAT_INFO("Added public IP " << public_address.ip << " to ignore list");
    return true;
}

std::string RatsClient::get_public_ip() const {
    std::lock_guard<std::mutex> lock(public_ip_mutex_);
    return public_ip_;
}

//=============================================================================
// Connection Strategy Selection
//=============================================================================

std::string RatsClient::select_best_connection_strategy(const std::string& host, int port) {
    NatType local_nat = detect_nat_type();
    
    // Strategy selection based on NAT type
    switch (local_nat) {
        case NatType::OPEN_INTERNET:
            return "direct";
            
        case NatType::FULL_CONE:
        case NatType::RESTRICTED_CONE:
            if (nat_config_.enable_ice && ice_agent_) {
                return "ice";
            } else {
                return "stun";
            }
            
        case NatType::PORT_RESTRICTED:
            if (nat_config_.enable_ice && ice_agent_) {
                return "ice";
            } else if (nat_config_.enable_hole_punching) {
                return "hole_punch";
            } else {
                return "stun";
            }
            
        case NatType::SYMMETRIC:
            if (nat_config_.enable_turn_relay) {
                return "turn";
            } else if (nat_config_.enable_ice && ice_agent_) {
                return "ice";
            } else {
                return "hole_punch";
            }
            
        default:
            // Unknown NAT type - try ICE if available, otherwise STUN
            if (nat_config_.enable_ice && ice_agent_) {
                return "ice";
            } else {
                return "stun";
            }
    }
}

//=============================================================================
// Connection Attempt Methods
//=============================================================================

bool RatsClient::attempt_direct_connection(const std::string& host, int port, ConnectionAttemptResult& result) {
    LOG_NAT_DEBUG("Attempting direct connection to " << host << ":" << port);
    
    // Check if this peer should be ignored (local interface)
    if (should_ignore_peer(host, port)) {
        result.error_message = "Target is local interface address";
        return false;
    }
    
    // Check peer limit
    if (is_peer_limit_reached()) {
        result.error_message = "Peer limit reached";
        return false;
    }
    
    // Check if already connected
    std::string peer_address = normalize_peer_address(host, port);
    if (is_already_connected_to_address(peer_address)) {
        result.error_message = "Already connected to this address";
        return true; // Not really an error
    }
    
    return perform_tcp_connection(host, port, result);
}

bool RatsClient::attempt_stun_assisted_connection(const std::string& host, int port, ConnectionAttemptResult& result) {
    LOG_NAT_DEBUG("Attempting STUN-assisted connection to " << host << ":" << port);
    
    // First ensure we have discovered our public IP
    if (get_public_ip().empty()) {
        if (!discover_and_ignore_public_ip()) {
            result.error_message = "Failed to discover public IP via STUN";
            return false;
        }
    }
    
    // For STUN-assisted, we still use direct TCP but with knowledge of public IP
    // The actual NAT traversal happens through the STUN binding discovery
    return attempt_direct_connection(host, port, result);
}

bool RatsClient::attempt_turn_relay_connection(const std::string& host, int port, ConnectionAttemptResult& result) {
    LOG_NAT_DEBUG("Attempting TURN relay connection to " << host << ":" << port);
    
    if (nat_config_.turn_servers.empty()) {
        result.error_message = "No TURN servers configured";
        return false;
    }
    
    try {
        // For now, fall back to direct connection
        // Full TURN implementation would require:
        // 1. TURN allocation
        // 2. Permission creation
        // 3. Data relay through TURN server
        LOG_NAT_INFO("TURN relay requires coordination - using direct connection as fallback");
        
        bool success = attempt_direct_connection(host, port, result);
        if (success) {
            result.method = "turn_fallback";
        }
        
        return success;
        
    } catch (const std::exception& e) {
        result.error_message = "TURN relay failed: " + std::string(e.what());
        return false;
    }
}

bool RatsClient::attempt_hole_punch_connection(const std::string& host, int port, ConnectionAttemptResult& result) {
    LOG_NAT_DEBUG("Attempting hole punch connection to " << host << ":" << port);
    
    if (!nat_config_.enable_hole_punching) {
        result.error_message = "Hole punching disabled in configuration";
        return false;
    }
    
    try {
        // Coordinated hole punching would require:
        // 1. Peer coordination through DHT or signaling
        // 2. Synchronized UDP hole punching attempts
        // 3. TCP hole punching (where supported)
        // 4. Connection establishment on successful punch
        
        LOG_NAT_INFO("Coordinated hole punching requires peer cooperation - using direct connection");
        
        bool success = attempt_direct_connection(host, port, result);
        if (success) {
            result.method = "hole_punch_direct";
        }
        
        return success;
        
    } catch (const std::exception& e) {
        result.error_message = "Hole punch failed: " + std::string(e.what());
        return false;
    }
}

bool RatsClient::perform_tcp_connection(const std::string& host, int port, ConnectionAttemptResult& result) {
    // Attempt TCP connection with 10-second timeout
    socket_t peer_socket = create_tcp_client(host, port, 10000); // 10 seconds = 10000ms
    if (!is_valid_socket(peer_socket)) {
        result.error_message = "Failed to create TCP connection (connection may have timed out after 10s)";
        return false;
    }
    
    // Initialize encryption if enabled
    if (is_encryption_enabled()) {
        if (!encrypted_communication::initialize_outgoing_connection(peer_socket)) {
            result.error_message = "Failed to initialize encryption";
            close_socket(peer_socket);
            return false;
        }
    }
    
    // Create peer and add to management
    std::string connection_info = host + ":" + std::to_string(port);
    std::string peer_hash_id = generate_peer_hash_id(peer_socket, connection_info);
    std::string peer_address = normalize_peer_address(host, port);
    
    {
        std::lock_guard<std::mutex> lock(peers_mutex_);
        RatsPeer new_peer(peer_hash_id, host, port, peer_socket, peer_address, true);
        new_peer.encryption_enabled = is_encryption_enabled();
        new_peer.connection_method = "direct";
        add_peer_unlocked(new_peer);
    }
    
    // Start handling this peer
    add_managed_thread(std::thread(&RatsClient::handle_client, this, peer_socket, peer_hash_id), 
                      "direct-connect-handler-" + peer_hash_id.substr(0, 8));
    
    // Send handshake if not encrypted
    if (!is_encryption_enabled()) {
        if (!send_handshake(peer_socket, get_our_peer_id())) {
            result.error_message = "Failed to send handshake";
            disconnect_peer(peer_socket);
            return false;
        }
    }
    
    return true;
}

//=============================================================================
// Hole Punching Coordination
//=============================================================================

bool RatsClient::coordinate_hole_punching(const std::string& peer_ip, uint16_t peer_port,
                                         const nlohmann::json& coordination_data) {
    LOG_NAT_INFO("Coordinating hole punching with " << peer_ip << ":" << peer_port);
    
    if (!nat_config_.enable_hole_punching) {
        LOG_NAT_ERROR("Hole punching disabled in configuration");
        return false;
    }
    
    try {
        std::string method = coordination_data.value("method", "");
        if (method == "udp_hole_punch") {
            // Use ICE agent for UDP hole punching if available
            if (ice_agent_ && ice_agent_->is_running()) {
                return ice_agent_->perform_hole_punching(peer_ip, peer_port);
            }
        }
        
        // For now, return success to indicate coordination was received
        LOG_NAT_INFO("Hole punching coordination received but not fully implemented");
        return true;
        
    } catch (const std::exception& e) {
        LOG_NAT_ERROR("Hole punching coordination failed: " << e.what());
        return false;
    }
}

//=============================================================================
// NAT Information Exchange
//=============================================================================

void RatsClient::send_nat_info_to_peer(socket_t socket, const std::string& peer_id) {
    LOG_NAT_DEBUG("Sending NAT info to peer " << peer_id);
    
    try {
        nlohmann::json nat_info;
        nat_info["nat_type"] = static_cast<int>(detect_nat_type());
        nat_info["has_nat"] = get_nat_characteristics().has_nat;
        nat_info["public_ip"] = get_public_ip();
        nat_info["filtering_behavior"] = static_cast<int>(get_nat_characteristics().filtering_behavior);
        nat_info["mapping_behavior"] = static_cast<int>(get_nat_characteristics().mapping_behavior);
        nat_info["preserves_port"] = get_nat_characteristics().preserves_port;
        nat_info["hairpin_support"] = get_nat_characteristics().hairpin_support;
        
        nlohmann::json nat_message = create_rats_message("nat_info_exchange", nat_info, get_our_peer_id());
        
        if (send_json_to_peer(socket, nat_message)) {
            LOG_NAT_DEBUG("Sent NAT info to peer " << peer_id);
        } else {
            LOG_NAT_ERROR("Failed to send NAT info to peer " << peer_id);
        }
        
    } catch (const std::exception& e) {
        LOG_NAT_ERROR("Failed to send NAT info: " << e.what());
    }
}

void RatsClient::handle_nat_info_exchange_message(socket_t socket, const std::string& peer_hash_id, const nlohmann::json& payload) {
    LOG_NAT_INFO("Received NAT info exchange from peer " << peer_hash_id);
    
    try {
        // Extract peer's NAT information
        NatType remote_nat_type = static_cast<NatType>(payload.value("nat_type", static_cast<int>(NatType::UNKNOWN)));
        bool has_nat = payload.value("has_nat", false);
        std::string public_ip = payload.value("public_ip", "");
        
        // Update peer with NAT information
        {
            std::lock_guard<std::mutex> lock(peers_mutex_);
            auto socket_it = socket_to_peer_id_.find(socket);
            if (socket_it != socket_to_peer_id_.end()) {
                auto peer_it = peers_.find(socket_it->second);
                if (peer_it != peers_.end()) {
                    peer_it->second.detected_nat_type = remote_nat_type;
                }
            }
        }
        
        LOG_NAT_INFO("Peer " << peer_hash_id << " NAT info: type=" << static_cast<int>(remote_nat_type) 
                   << ", has_nat=" << has_nat << ", public_ip=" << public_ip);
        
        // Send our NAT information in response if not already sent
        nlohmann::json our_nat_info;
        our_nat_info["nat_type"] = static_cast<int>(detect_nat_type());
        our_nat_info["has_nat"] = get_nat_characteristics().has_nat;
        our_nat_info["public_ip"] = get_public_ip();
        our_nat_info["response"] = true;
        
        nlohmann::json nat_message = create_rats_message("nat_info_exchange", our_nat_info, get_our_peer_id());
        
        // Only send if this wasn't already a response
        if (!payload.value("response", false)) {
            if (send_json_to_peer(socket, nat_message)) {
                LOG_NAT_DEBUG("Sent NAT info to peer " << peer_hash_id);
            } else {
                LOG_NAT_ERROR("Failed to send NAT info to peer " << peer_hash_id);
            }
        }
        
    } catch (const std::exception& e) {
        LOG_NAT_ERROR("Failed to handle NAT info exchange: " << e.what());
    }
}

void RatsClient::handle_hole_punch_coordination_message(socket_t socket, const std::string& peer_hash_id, const nlohmann::json& payload) {
    LOG_NAT_INFO("Received hole punch coordination from peer " << peer_hash_id);
    
    try {
        std::string method = payload.value("method", "");
        std::string peer_ip = payload.value("peer_ip", "");
        int peer_port = payload.value("peer_port", 0);
        
        if (method.empty() || peer_ip.empty() || peer_port <= 0) {
            LOG_NAT_WARN("Invalid hole punch coordination data from peer " << peer_hash_id);
            return;
        }
        
        LOG_NAT_INFO("Coordinating " << method << " hole punch with " << peer_ip << ":" << peer_port);
        
        // Perform hole punching coordination
        bool success = coordinate_hole_punching(peer_ip, peer_port, payload);
        
        // Send coordination response
        nlohmann::json response_payload;
        response_payload["success"] = success;
        response_payload["method"] = method;
        response_payload["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        
        nlohmann::json response_message = create_rats_message("hole_punch_response", response_payload, get_our_peer_id());
        
        if (send_json_to_peer(socket, response_message)) {
            LOG_NAT_DEBUG("Sent hole punch coordination response to peer " << peer_hash_id);
        } else {
            LOG_NAT_ERROR("Failed to send hole punch coordination response to peer " << peer_hash_id);
        }
        
    } catch (const std::exception& e) {
        LOG_NAT_ERROR("Failed to handle hole punch coordination: " << e.what());
    }
}

//=============================================================================
// Statistics and Connection Information
//=============================================================================

nlohmann::json RatsClient::get_nat_traversal_statistics() const {
    nlohmann::json stats;
    
    {
        std::lock_guard<std::mutex> lock(nat_mutex_);
        stats["detected_nat_type"] = static_cast<int>(detected_nat_type_);
        stats["has_nat"] = nat_characteristics_.has_nat;
        stats["filtering_behavior"] = static_cast<int>(nat_characteristics_.filtering_behavior);
        stats["mapping_behavior"] = static_cast<int>(nat_characteristics_.mapping_behavior);
        stats["preserves_port"] = nat_characteristics_.preserves_port;
        stats["hairpin_support"] = nat_characteristics_.hairpin_support;
        stats["description"] = nat_characteristics_.description;
    }
    
    // ICE statistics
    if (ice_agent_) {
        stats["ice_available"] = true;
        stats["ice_running"] = ice_agent_->is_running();
        stats["ice_state"] = static_cast<int>(ice_agent_->get_connection_state());
        
        if (ice_agent_->is_running()) {
            auto local_candidates = ice_agent_->get_local_candidates();
            stats["local_ice_candidates"] = local_candidates.size();
        }
    } else {
        stats["ice_available"] = false;
    }
    
    // NAT traversal configuration
    stats["config"] = {
        {"enable_ice", nat_config_.enable_ice},
        {"enable_upnp", nat_config_.enable_upnp},
        {"enable_hole_punching", nat_config_.enable_hole_punching},
        {"enable_turn_relay", nat_config_.enable_turn_relay},
        {"stun_servers", nat_config_.stun_servers},
        {"turn_servers", nat_config_.turn_servers}
    };
    
    return stats;
}

void RatsClient::update_connection_statistics(const std::string& peer_id, const ConnectionAttemptResult& result) {
    std::lock_guard<std::mutex> lock(connection_attempts_mutex_);
    connection_attempts_[peer_id].push_back(result);
    
    // Keep only the last 10 attempts per peer to avoid memory growth
    if (connection_attempts_[peer_id].size() > 10) {
        connection_attempts_[peer_id].erase(connection_attempts_[peer_id].begin());
    }
}

} // namespace librats
