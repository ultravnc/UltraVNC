#include "librats.h"
#include "ice.h"
#include <algorithm>
#include <random>

// Logging macros for ICE operations
#ifdef TESTING
#define LOG_ICE_DEBUG(message) LOG_DEBUG("ice", "[pointer: " << this << "] " << message)
#define LOG_ICE_INFO(message)  LOG_INFO("ice", "[pointer: " << this << "] " << message)
#define LOG_ICE_WARN(message)  LOG_WARN("ice", "[pointer: " << this << "] " << message)
#define LOG_ICE_ERROR(message) LOG_ERROR("ice", "[pointer: " << this << "] " << message)
#else
#define LOG_ICE_DEBUG(message) LOG_DEBUG("ice", message)
#define LOG_ICE_INFO(message)  LOG_INFO("ice", message)
#define LOG_ICE_WARN(message)  LOG_WARN("ice", message)
#define LOG_ICE_ERROR(message) LOG_ERROR("ice", message)
#endif

namespace librats {

//=============================================================================
// ICE Connection Attempts
//=============================================================================

bool RatsClient::attempt_ice_connection(const std::string& host, int port, ConnectionAttemptResult& result) {
    LOG_ICE_DEBUG("Attempting ICE connection to " << host << ":" << port);
    
    if (!ice_agent_ || !ice_agent_->is_running()) {
        result.error_message = "ICE agent not available or not running";
        return false;
    }
    
    try {
        // For now, use direct connection and enhance with ICE coordination later
        // Full ICE implementation would require signaling channel for candidate exchange
        LOG_ICE_INFO("ICE coordination requires signaling channel - using enhanced direct connection");
        
        bool success = attempt_direct_connection(host, port, result);
        if (success) {
            result.method = "ice_direct";
            // In a full implementation, we would:
            // 1. Gather local candidates
            // 2. Exchange candidates via signaling
            // 3. Perform connectivity checks
            // 4. Establish connection on best candidate pair
        }
        
        return success;
        
    } catch (const std::exception& e) {
        result.error_message = "ICE connection failed: " + std::string(e.what());
        return false;
    }
}

//=============================================================================
// ICE Offer/Answer Processing
//=============================================================================

nlohmann::json RatsClient::create_ice_offer(const std::string& peer_id) {
    LOG_ICE_INFO("Creating ICE offer for peer " << peer_id);
    
    nlohmann::json offer;
    offer["type"] = "ice_offer";
    offer["peer_id"] = get_our_peer_id();
    offer["target_peer_id"] = peer_id;
    
    if (ice_agent_ && ice_agent_->is_running()) {
        // Get local credentials
        auto credentials = ice_agent_->get_local_credentials();
        offer["ice_ufrag"] = credentials.first;
        offer["ice_pwd"] = credentials.second;
        
        // Get local candidates
        auto candidates = ice_agent_->get_local_candidates();
        nlohmann::json candidates_array = nlohmann::json::array();
        
        for (const auto& candidate : candidates) {
            nlohmann::json cand_json;
            cand_json["ip"] = candidate.ip;
            cand_json["port"] = candidate.port;
            cand_json["type"] = static_cast<int>(candidate.type);
            cand_json["priority"] = candidate.priority;
            cand_json["foundation"] = candidate.foundation;
            candidates_array.push_back(cand_json);
        }
        
        offer["candidates"] = candidates_array;
    } else {
        LOG_ICE_WARN("ICE agent not available for ICE offer");
        offer["error"] = "ICE agent not available";
    }
    
    return offer;
}

bool RatsClient::connect_with_ice(const std::string& peer_id, const nlohmann::json& ice_offer) {
    LOG_ICE_INFO("Connecting with ICE to peer " << peer_id);
    
    if (!ice_agent_ || !ice_agent_->is_running()) {
        LOG_ICE_ERROR("ICE agent not available for ICE connection");
        return false;
    }
    
    try {
        // Extract ICE credentials
        std::string remote_ufrag = ice_offer.value("ice_ufrag", "");
        std::string remote_pwd = ice_offer.value("ice_pwd", "");
        
        if (remote_ufrag.empty() || remote_pwd.empty()) {
            LOG_ICE_ERROR("Invalid ICE offer - missing credentials");
            return false;
        }
        
        // Set remote credentials
        ice_agent_->set_remote_credentials(remote_ufrag, remote_pwd);
        
        // Add remote candidates
        if (ice_offer.contains("candidates")) {
            for (const auto& cand_json : ice_offer["candidates"]) {
                IceCandidate candidate;
                candidate.ip = cand_json.value("ip", "");
                candidate.port = cand_json.value("port", 0);
                candidate.type = static_cast<IceCandidateType>(cand_json.value("type", 0));
                candidate.priority = cand_json.value("priority", 0);
                candidate.foundation = cand_json.value("foundation", "");
                
                if (!candidate.ip.empty() && candidate.port > 0) {
                    ice_agent_->add_remote_candidate(candidate);
                    LOG_ICE_DEBUG("Added remote ICE candidate: " << candidate.ip << ":" << candidate.port);
                }
            }
        }
        
        // Start connectivity checks
        ice_agent_->start_connectivity_checks();
        
        LOG_ICE_INFO("ICE connectivity checks started for peer " << peer_id);
        return true;
        
    } catch (const std::exception& e) {
        LOG_ICE_ERROR("ICE connection failed: " << e.what());
        return false;
    }
}

bool RatsClient::handle_ice_answer(const std::string& peer_id, const nlohmann::json& ice_answer) {
    LOG_ICE_INFO("Handling ICE answer from peer " << peer_id);
    
    // ICE answer handling is similar to ICE offer processing
    return connect_with_ice(peer_id, ice_answer);
}

//=============================================================================
// ICE Coordination and Initiation
//=============================================================================

void RatsClient::initiate_ice_with_peer(const std::string& peer_id, const std::string& host, int port) {
    if (!ice_agent_ || !ice_agent_->is_running()) {
        LOG_ICE_WARN("ICE agent not available for ICE initiation");
        return;
    }
    
    LOG_ICE_INFO("Initiating ICE coordination with peer " << peer_id);
    
    try {
        // Gather local candidates if not already done
        ice_agent_->gather_candidates();
        
        // Create ICE offer
        nlohmann::json ice_offer = create_ice_offer(peer_id);
        
        // Send ICE offer via message system
        nlohmann::json offer_message = create_rats_message("ice_offer", ice_offer, get_our_peer_id());
        
        // Find peer socket to send offer
        socket_t peer_socket = get_peer_socket_by_id(peer_id);
        
        if (is_valid_socket(peer_socket)) {
            if (send_json_to_peer(peer_socket, offer_message)) {
                LOG_ICE_INFO("Sent ICE offer to peer " << peer_id);
            } else {
                LOG_ICE_ERROR("Failed to send ICE offer to peer " << peer_id);
            }
        } else {
            LOG_ICE_ERROR("Cannot send ICE offer - peer socket not found for " << peer_id);
        }
        
    } catch (const std::exception& e) {
        LOG_ICE_ERROR("Failed to initiate ICE with peer: " << e.what());
    }
}

//=============================================================================
// ICE Message Handlers
//=============================================================================

void RatsClient::handle_ice_offer_message(socket_t socket, const std::string& peer_hash_id, const nlohmann::json& payload) {
    LOG_ICE_INFO("Received ICE offer from peer " << peer_hash_id);
    
    if (!ice_agent_ || !ice_agent_->is_running()) {
        LOG_ICE_WARN("ICE agent not available - cannot handle ICE offer");
        return;
    }
    
    try {
        // Extract offer details
        std::string target_peer_id = payload.value("target_peer_id", "");
        std::string offer_peer_id = payload.value("peer_id", "");
        
        // Verify this offer is for us
        if (target_peer_id != get_our_peer_id()) {
            LOG_ICE_WARN("ICE offer not intended for us (target: " << target_peer_id << ", us: " << get_our_peer_id() << ")");
            return;
        }
        
        // Update peer with ICE information
        update_peer_ice_info(socket, payload);
        
        // Process the ICE offer
        connect_with_ice(offer_peer_id, payload);
        
        // Create and send ICE answer
        nlohmann::json ice_answer = create_ice_offer(offer_peer_id); // Create our offer as answer
        ice_answer["type"] = "ice_answer";
        ice_answer["target_peer_id"] = offer_peer_id;
        ice_answer["peer_id"] = get_our_peer_id();
        
        nlohmann::json answer_message = create_rats_message("ice_answer", ice_answer, get_our_peer_id());
        
        if (send_json_to_peer(socket, answer_message)) {
            LOG_ICE_INFO("Sent ICE answer to peer " << peer_hash_id);
        } else {
            LOG_ICE_ERROR("Failed to send ICE answer to peer " << peer_hash_id);
        }
        
    } catch (const std::exception& e) {
        LOG_ICE_ERROR("Failed to handle ICE offer: " << e.what());
    }
}

void RatsClient::handle_ice_answer_message(socket_t socket, const std::string& peer_hash_id, const nlohmann::json& payload) {
    LOG_ICE_INFO("Received ICE answer from peer " << peer_hash_id);
    
    if (!ice_agent_ || !ice_agent_->is_running()) {
        LOG_ICE_WARN("ICE agent not available - cannot handle ICE answer");
        return;
    }
    
    try {
        std::string answer_peer_id = payload.value("peer_id", "");
        
        // Update peer with ICE information
        update_peer_ice_info(socket, payload);
        
        // Set ICE state to checking
        {
            std::lock_guard<std::mutex> lock(peers_mutex_);
            auto socket_it = socket_to_peer_id_.find(socket);
            if (socket_it != socket_to_peer_id_.end()) {
                auto peer_it = peers_.find(socket_it->second);
                if (peer_it != peers_.end()) {
                    peer_it->second.ice_state = IceConnectionState::CHECKING;
                }
            }
        }
        
        // Process the ICE answer
        handle_ice_answer(answer_peer_id, payload);
        
    } catch (const std::exception& e) {
        LOG_ICE_ERROR("Failed to handle ICE answer: " << e.what());
    }
}

void RatsClient::handle_ice_candidate_message(socket_t socket, const std::string& peer_hash_id, const nlohmann::json& payload) {
    LOG_ICE_DEBUG("Received ICE candidate from peer " << peer_hash_id);
    
    if (!ice_agent_ || !ice_agent_->is_running()) {
        LOG_ICE_WARN("ICE agent not available - cannot handle ICE candidate");
        return;
    }
    
    try {
        // Extract candidate information
        IceCandidate candidate;
        candidate.ip = payload.value("ip", "");
        candidate.port = payload.value("port", 0);
        candidate.type = static_cast<IceCandidateType>(payload.value("type", 0));
        candidate.priority = payload.value("priority", 0);
        candidate.foundation = payload.value("foundation", "");
        candidate.component_id = payload.value("component_id", 1);
        
        if (!candidate.ip.empty() && candidate.port > 0) {
            // Add candidate to ICE agent
            ice_agent_->add_remote_candidate(candidate);
            
            // Update peer candidate list
            add_candidate_to_peer(socket, candidate);
            
            LOG_ICE_DEBUG("Added remote ICE candidate: " << candidate.ip << ":" << candidate.port 
                         << " type=" << static_cast<int>(candidate.type));
        } else {
            LOG_ICE_WARN("Invalid ICE candidate received from peer " << peer_hash_id);
        }
        
    } catch (const std::exception& e) {
        LOG_ICE_ERROR("Failed to handle ICE candidate: " << e.what());
    }
}

//=============================================================================
// ICE Helper Functions
//=============================================================================

void RatsClient::update_peer_ice_info(socket_t socket, const nlohmann::json& payload) {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    auto socket_it = socket_to_peer_id_.find(socket);
    if (socket_it != socket_to_peer_id_.end()) {
        auto peer_it = peers_.find(socket_it->second);
        if (peer_it != peers_.end()) {
            peer_it->second.ice_enabled = true;
            peer_it->second.ice_ufrag = payload.value("ice_ufrag", "");
            peer_it->second.ice_pwd = payload.value("ice_pwd", "");
        }
    }
}

void RatsClient::add_candidate_to_peer(socket_t socket, const IceCandidate& candidate) {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    auto socket_it = socket_to_peer_id_.find(socket);
    if (socket_it != socket_to_peer_id_.end()) {
        auto peer_it = peers_.find(socket_it->second);
        if (peer_it != peers_.end()) {
            peer_it->second.ice_candidates.push_back(candidate);
        }
    }
}

//=============================================================================
// ICE Coordination Management
//=============================================================================

bool RatsClient::should_initiate_ice_coordination(const std::string& peer_id) {
    std::lock_guard<std::mutex> ice_lock(ice_coordination_mutex_);
    return ice_coordination_in_progress_.find(peer_id) == ice_coordination_in_progress_.end();
}

void RatsClient::mark_ice_coordination_in_progress(const std::string& peer_id) {
    std::lock_guard<std::mutex> ice_lock(ice_coordination_mutex_);
    ice_coordination_in_progress_.insert(peer_id);
}

void RatsClient::remove_ice_coordination_tracking(const std::string& peer_id) {
    std::lock_guard<std::mutex> ice_lock(ice_coordination_mutex_);
    ice_coordination_in_progress_.erase(peer_id);
}

void RatsClient::cleanup_ice_coordination_for_peer(const std::string& peer_id) {
    try {
        remove_ice_coordination_tracking(peer_id);
    } catch (...) {
        // Ignore cleanup errors to prevent recursive exceptions
        LOG_ICE_DEBUG("Exception during ICE coordination cleanup for peer " << peer_id);
    }
}

//=============================================================================
// ICE Callbacks and State Management
//=============================================================================

void RatsClient::setup_ice_callbacks() {
    if (!ice_agent_) {
        return;
    }
    
    // Set ICE candidate discovery callback
    ice_agent_->set_candidate_callback([this](const IceCandidate& candidate) {
        LOG_ICE_INFO("ICE candidate discovered: " << candidate.ip << ":" << candidate.port 
                   << " type=" << static_cast<int>(candidate.type) 
                   << " priority=" << candidate.priority);
        
        if (ice_candidate_callback_) {
            ice_candidate_callback_("", candidate); // Empty peer_id for local candidates
        }
    });
    
    // Set ICE state change callback
    ice_agent_->set_state_change_callback([this](IceConnectionState state) {
        LOG_ICE_INFO("ICE connection state changed: " << static_cast<int>(state));
        
        if (nat_progress_callback_) {
            std::string status = "ICE state: " + std::to_string(static_cast<int>(state));
            nat_progress_callback_("", status); // Empty peer_id for general ICE state
        }
    });
}

void RatsClient::initialize_ice_agent() {
    if (!nat_config_.enable_ice) {
        LOG_ICE_DEBUG("ICE is disabled in configuration");
        return;
    }
    
    LOG_ICE_INFO("Initializing ICE agent");
    
    IceConfig ice_config;
    ice_config.stun_servers = nat_config_.stun_servers;
    ice_config.turn_servers = nat_config_.turn_servers;
    ice_config.turn_usernames = nat_config_.turn_usernames;
    ice_config.turn_passwords = nat_config_.turn_passwords;
    ice_config.stun_timeout_ms = 5000;
    ice_config.turn_timeout_ms = nat_config_.turn_allocation_timeout_ms;
    ice_config.connectivity_check_timeout_ms = nat_config_.ice_connectivity_timeout_ms;
    ice_config.enable_host_candidates = true;
    ice_config.enable_server_reflexive_candidates = true;
    ice_config.enable_relay_candidates = nat_config_.enable_turn_relay;
    
    ice_agent_ = std::make_unique<IceAgent>(IceRole::CONTROLLING, ice_config);
    
    // Setup callbacks
    setup_ice_callbacks();
    
    LOG_ICE_INFO("ICE agent initialized successfully");
}

//=============================================================================
// ICE Statistics and Information
//=============================================================================

nlohmann::json RatsClient::get_ice_statistics() const {
    nlohmann::json stats;
    
    if (ice_agent_) {
        stats["available"] = true;
        stats["running"] = ice_agent_->is_running();
        stats["state"] = static_cast<int>(ice_agent_->get_connection_state());
        
        if (ice_agent_->is_running()) {
            auto local_candidates = ice_agent_->get_local_candidates();
            stats["local_candidates"] = local_candidates.size();
            
            // Get candidate details
            nlohmann::json candidates_array = nlohmann::json::array();
            for (const auto& candidate : local_candidates) {
                nlohmann::json cand_json;
                cand_json["ip"] = candidate.ip;
                cand_json["port"] = candidate.port;
                cand_json["type"] = static_cast<int>(candidate.type);
                cand_json["priority"] = candidate.priority;
                candidates_array.push_back(cand_json);
            }
            stats["local_candidates_details"] = candidates_array;
        }
        
        // Get peer ICE information
        nlohmann::json ice_peers = nlohmann::json::array();
        {
            std::lock_guard<std::mutex> lock(peers_mutex_);
            for (const auto& pair : peers_) {
                const RatsPeer& peer = pair.second;
                if (peer.ice_enabled) {
                    nlohmann::json peer_ice;
                    peer_ice["peer_id"] = peer.peer_id;
                    peer_ice["ice_state"] = static_cast<int>(peer.ice_state);
                    peer_ice["ufrag"] = peer.ice_ufrag;
                    peer_ice["candidates_count"] = peer.ice_candidates.size();
                    ice_peers.push_back(peer_ice);
                }
            }
        }
        stats["ice_peers"] = ice_peers;
        
    } else {
        stats["available"] = false;
    }
    
    return stats;
}

bool RatsClient::is_ice_enabled() const {
    return ice_agent_ && ice_agent_->is_running();
}

bool RatsClient::is_peer_ice_connected(const std::string& peer_id) const {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    auto it = peers_.find(peer_id);
    if (it != peers_.end()) {
        return it->second.is_ice_connected();
    }
    return false;
}

//=============================================================================
// ICE Cleanup and Shutdown
//=============================================================================

void RatsClient::cleanup_ice_resources() {
    LOG_ICE_DEBUG("Cleaning up ICE resources");
    
    // Stop ICE agent
    if (ice_agent_ && ice_agent_->is_running()) {
        ice_agent_->stop();
    }
    
    // Clear ICE coordination tracking
    {
        std::lock_guard<std::mutex> ice_lock(ice_coordination_mutex_);
        ice_coordination_in_progress_.clear();
    }
    
    LOG_ICE_DEBUG("ICE resources cleaned up");
}

} // namespace librats
