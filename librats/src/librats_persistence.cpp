#include "librats.h"
#include "sha1.h"
#include "os.h"
#include "fs.h"
#include "json.hpp" // nlohmann::json
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
#else
#define LOG_CLIENT_DEBUG(message) LOG_DEBUG("client", message)
#define LOG_CLIENT_INFO(message)  LOG_INFO("client", message)
#define LOG_CLIENT_WARN(message)  LOG_WARN("client", message)
#define LOG_CLIENT_ERROR(message) LOG_ERROR("client", message)
#endif

namespace librats {

// =========================================================================
// Configuration Persistence Implementation
// =========================================================================

bool RatsClient::load_configuration() {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    LOG_CLIENT_INFO("Loading configuration from " << get_config_file_path());
    
    // Check if config file exists
    if (!file_or_directory_exists(get_config_file_path())) {
        LOG_CLIENT_INFO("No existing configuration found, generating new peer ID");
        our_peer_id_ = generate_persistent_peer_id();
        
        // Save the new configuration immediately
        {
            nlohmann::json config;
            config["peer_id"] = our_peer_id_;
            config["version"] = RATS_PROTOCOL_VERSION;
            config["listen_port"] = listen_port_;
            config["max_peers"] = max_peers_;
            
            auto now = std::chrono::high_resolution_clock::now();
            auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
            config["created_at"] = timestamp;
            config["last_updated"] = timestamp;
            
            std::string config_data = config.dump(4); // Pretty print with 4 spaces
            if (create_file(get_config_file_path(), config_data)) {
                LOG_CLIENT_INFO("Created new configuration file with peer ID: " << our_peer_id_);
            } else {
                LOG_CLIENT_ERROR("Failed to create configuration file");
                return false;
            }
        }
        
        return true;
    }
    
    // Load existing configuration
    try {
        std::string config_data = read_file_text_cpp(get_config_file_path());
        if (config_data.empty()) {
            LOG_CLIENT_ERROR("Configuration file is empty");
            return false;
        }
        
        nlohmann::json config = nlohmann::json::parse(config_data);
        
        // Load peer ID
        our_peer_id_ = config.value("peer_id", "");
        if (our_peer_id_.empty()) {
            LOG_CLIENT_WARN("No peer ID in configuration, generating new one");
            our_peer_id_ = generate_persistent_peer_id();
            return save_configuration(); // Save the new peer ID
        }
        
        LOG_CLIENT_INFO("Loaded configuration with peer ID: " << our_peer_id_);
        
        // Load encryption settings
        if (config.contains("encryption_enabled")) {
            encryption_enabled_ = config.value("encryption_enabled", true);
        }
        
        if (config.contains("encryption_key")) {
            std::string key_hex = config.value("encryption_key", "");
            if (!key_hex.empty()) {
                NoiseKey loaded_key = EncryptedSocket::string_to_key(key_hex);
                // Validate key
                bool is_valid = false;
                for (uint8_t byte : loaded_key) {
                    if (byte != 0) {
                        is_valid = true;
                        break;
                    }
                }
                if (is_valid) {
                    static_encryption_key_ = loaded_key;
                    LOG_CLIENT_INFO("Loaded encryption key from configuration");
                } else {
                    LOG_CLIENT_WARN("Invalid encryption key in configuration, using generated key");
                }
            }
        }
        
        // Update last_updated timestamp
        auto now = std::chrono::high_resolution_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        config["last_updated"] = timestamp;
        
        // Save updated config
        std::string updated_config_data = config.dump(4);
        create_file(get_config_file_path(), updated_config_data);
        
        return true;
        
    } catch (const nlohmann::json::exception& e) {
        LOG_CLIENT_ERROR("Failed to parse configuration file: " << e.what());
        return false;
    } catch (const std::exception& e) {
        LOG_CLIENT_ERROR("Failed to load configuration: " << e.what());
        return false;
    }
}

bool RatsClient::save_configuration() {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    if (our_peer_id_.empty()) {
        LOG_CLIENT_WARN("No peer ID to save");
        return false;
    }
    
    LOG_CLIENT_DEBUG("Saving configuration to " << get_config_file_path());
    
    try {
        // Create configuration JSON
        nlohmann::json config;
        config["peer_id"] = our_peer_id_;
        config["version"] = RATS_PROTOCOL_VERSION;
        config["listen_port"] = listen_port_;
        config["max_peers"] = max_peers_;
        config["encryption_enabled"] = encryption_enabled_;
        config["encryption_key"] = get_encryption_key();
        
        auto now = std::chrono::high_resolution_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        config["last_updated"] = timestamp;
        
        // If config file exists, preserve created_at timestamp
        if (file_or_directory_exists(get_config_file_path())) {
            try {
                std::string existing_config_data = read_file_text_cpp(get_config_file_path());
                nlohmann::json existing_config = nlohmann::json::parse(existing_config_data);
                if (existing_config.contains("created_at")) {
                    config["created_at"] = existing_config["created_at"];
                }
            } catch (const std::exception&) {
                // If we can't read existing config, just use current timestamp
                config["created_at"] = timestamp;
            }
        } else {
            config["created_at"] = timestamp;
        }
        
        // Save configuration
        std::string config_data = config.dump(4);
        if (create_file(get_config_file_path(), config_data)) {
            LOG_CLIENT_DEBUG("Configuration saved successfully");
        } else {
            LOG_CLIENT_ERROR("Failed to save configuration file");
            return false;
        }
        
        // Save peers
        return save_peers_to_file();
        
    } catch (const nlohmann::json::exception& e) {
        LOG_CLIENT_ERROR("Failed to create configuration JSON: " << e.what());
        return false;
    } catch (const std::exception& e) {
        LOG_CLIENT_ERROR("Failed to save configuration: " << e.what());
        return false;
    }
}

bool RatsClient::save_peers_to_file() {
    // This method assumes config_mutex_ is already locked by save_configuration()
    
    LOG_CLIENT_DEBUG("Saving peers to " << get_peers_file_path());
    
    try {
        nlohmann::json peers_json = nlohmann::json::array();
        
        // Get validated peers for saving
        {
            std::lock_guard<std::mutex> peers_lock(peers_mutex_);
            for (const auto& pair : peers_) {
                const RatsPeer& peer = pair.second;
                // Only save peers that have completed handshake and have valid peer IDs
                if (peer.is_handshake_completed() && !peer.peer_id.empty()) {
                    // Don't save ourselves
                    if (peer.peer_id != our_peer_id_) {
                        peers_json.push_back(serialize_peer_for_persistence(peer));
                    }
                }
            }
        }
        
        LOG_CLIENT_INFO("Saving " << peers_json.size() << " peers to persistence file");
        
        // Save peers file
        std::string peers_data = peers_json.dump(4);
        if (create_file(get_peers_file_path(), peers_data)) {
            LOG_CLIENT_DEBUG("Peers saved successfully");
            return true;
        } else {
            LOG_CLIENT_ERROR("Failed to save peers file");
            return false;
        }
        
    } catch (const nlohmann::json::exception& e) {
        LOG_CLIENT_ERROR("Failed to serialize peers: " << e.what());
        return false;
    } catch (const std::exception& e) {
        LOG_CLIENT_ERROR("Failed to save peers: " << e.what());
        return false;
    }
}

int RatsClient::load_and_reconnect_peers() {
    if (!running_.load()) {
        LOG_CLIENT_DEBUG("Client not running, skipping peer reconnection");
        return 0;
    }
    
    std::string peers_file_path;
    {
        std::lock_guard<std::mutex> lock(config_mutex_);
        peers_file_path = get_peers_file_path();
    }

    LOG_CLIENT_INFO("Loading saved peers from " << peers_file_path);
    
    // Check if peers file exists
    if (!file_or_directory_exists(peers_file_path)) {
        LOG_CLIENT_INFO("No saved peers file found");
        return 0;
    }
    
    try {
        std::string peers_data = read_file_text_cpp(peers_file_path);
        if (peers_data.empty()) {
            LOG_CLIENT_INFO("Peers file is empty");
            return 0;
        }
        
        nlohmann::json peers_json = nlohmann::json::parse(peers_data);
        
        if (!peers_json.is_array()) {
            LOG_CLIENT_ERROR("Invalid peers file format - expected array");
            return 0;
        }
        
        int reconnect_attempts = 0;
        
        for (const auto& peer_json : peers_json) {
            std::string ip;
            int port;
            std::string peer_id;
            
            if (!deserialize_peer_from_persistence(peer_json, ip, port, peer_id)) {
                continue; // Skip invalid or old peers
            }
            
            // Don't connect to ourselves
            if (peer_id == get_our_peer_id()) {
                LOG_CLIENT_DEBUG("Skipping connection to ourselves: " << peer_id);
                continue;
            }
            
            // Check if we should ignore this peer (local interface)
            if (should_ignore_peer(ip, port)) {
                LOG_CLIENT_DEBUG("Ignoring saved peer " << ip << ":" << port << " - local interface address");
                continue;
            }
            
            // Check if we're already connected to this peer
            std::string normalized_peer_address = normalize_peer_address(ip, port);
            if (is_already_connected_to_address(normalized_peer_address)) {
                LOG_CLIENT_DEBUG("Already connected to saved peer " << normalized_peer_address);
                continue;
            }
            
            // Check if peer limit is reached
            if (is_peer_limit_reached()) {
                LOG_CLIENT_DEBUG("Peer limit reached, stopping reconnection attempts");
                break;
            }
            
            LOG_CLIENT_INFO("Attempting to reconnect to saved peer: " << ip << ":" << port << " (peer_id: " << peer_id << ")");
            
            // Attempt to connect (non-blocking)
            add_managed_thread(std::thread([this, ip, port, peer_id]() {
                if (connect_to_peer(ip, port)) {
                    LOG_CLIENT_INFO("Successfully reconnected to saved peer: " << ip << ":" << port);
                } else {
                    LOG_CLIENT_DEBUG("Failed to reconnect to saved peer: " << ip << ":" << port);
                }
            }), "peer-reconnect-" + peer_id.substr(0, 8));
            
            reconnect_attempts++;
            
            // Small delay between connection attempts to avoid overwhelming the network
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        LOG_CLIENT_INFO("Processed " << peers_json.size() << " saved peers, attempted " << reconnect_attempts << " reconnections");
        return reconnect_attempts;
        
    } catch (const nlohmann::json::exception& e) {
        LOG_CLIENT_ERROR("Failed to parse saved peers file: " << e.what());
        return 0;
    } catch (const std::exception& e) {
        LOG_CLIENT_ERROR("Failed to load saved peers: " << e.what());
        return 0;
    }
}

bool RatsClient::append_peer_to_historical_file(const RatsPeer& peer) {
    // Don't save ourselves
    if (peer.peer_id == our_peer_id_) {
        return true;
    }
    
    // Only save peers that have completed handshake and have valid peer IDs
    if (!peer.is_handshake_completed() || peer.peer_id.empty()) {
        return true;
    }
    
    try {
        // Create historical peer entry with timestamp
        nlohmann::json historical_peer = serialize_peer_for_persistence(peer);
        
        // Add current timestamp as last_seen
        auto now = std::chrono::high_resolution_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        historical_peer["last_seen"] = timestamp;
        
        // Check if file exists, if not create it as an empty array
        std::string file_path = get_peers_ever_file_path();
        nlohmann::json historical_peers;
        
        if (file_or_directory_exists(file_path)) {
            try {
                std::string existing_data = read_file_text_cpp(file_path);
                if (!existing_data.empty()) {
                    historical_peers = nlohmann::json::parse(existing_data);
                    if (!historical_peers.is_array()) {
                        LOG_CLIENT_WARN("Historical peers file format invalid, creating new array");
                        historical_peers = nlohmann::json::array();
                    }
                } else {
                    historical_peers = nlohmann::json::array();
                }
            } catch (const std::exception& e) {
                LOG_CLIENT_WARN("Failed to read historical peers file, creating new array: " << e.what());
                historical_peers = nlohmann::json::array();
            }
        } else {
            historical_peers = nlohmann::json::array();
        }
        
        // Check if this peer already exists in historical file
        std::string peer_address = peer.ip + ":" + std::to_string(peer.port);
        bool already_exists = false;
        
        for (auto& existing_peer : historical_peers) {
            std::string existing_ip = existing_peer.value("ip", "");
            int existing_port = existing_peer.value("port", 0);
            std::string existing_address = existing_ip + ":" + std::to_string(existing_port);
            
            if (existing_address == peer_address || existing_peer.value("peer_id", "") == peer.peer_id) {
                // Update timestamp for existing peer
                existing_peer["last_seen"] = timestamp;
                already_exists = true;
                break;
            }
        }
        
        // If peer doesn't exist, add it
        if (!already_exists) {
            historical_peers.push_back(historical_peer);
            LOG_CLIENT_DEBUG("Added new peer to historical file: " << peer.ip << ":" << peer.port << " (peer_id: " << peer.peer_id << ")");
        } else {
            LOG_CLIENT_DEBUG("Updated timestamp for existing historical peer: " << peer.ip << ":" << peer.port);
        }
        
        // Save updated historical peers file
        std::string historical_data = historical_peers.dump(4);
        if (create_file(file_path, historical_data)) {
            return true;
        } else {
            LOG_CLIENT_ERROR("Failed to save historical peers file");
            return false;
        }
        
    } catch (const nlohmann::json::exception& e) {
        LOG_CLIENT_ERROR("Failed to process historical peer: " << e.what());
        return false;
    } catch (const std::exception& e) {
        LOG_CLIENT_ERROR("Failed to append peer to historical file: " << e.what());
        return false;
    }
}

bool RatsClient::load_historical_peers() {
    LOG_CLIENT_INFO("Loading historical peers from " << get_peers_ever_file_path());
    
    // Check if historical peers file exists
    if (!file_or_directory_exists(get_peers_ever_file_path())) {
        LOG_CLIENT_INFO("No historical peers file found");
        return true; // Not an error
    }
    
    try {
        std::string historical_data = read_file_text_cpp(get_peers_ever_file_path());
        if (historical_data.empty()) {
            LOG_CLIENT_INFO("Historical peers file is empty");
            return true;
        }
        
        nlohmann::json historical_peers = nlohmann::json::parse(historical_data);
        
        if (!historical_peers.is_array()) {
            LOG_CLIENT_ERROR("Invalid historical peers file format - expected array");
            return false;
        }
        
        LOG_CLIENT_INFO("Loaded " << historical_peers.size() << " historical peers from file");
        return true;
        
    } catch (const nlohmann::json::exception& e) {
        LOG_CLIENT_ERROR("Failed to parse historical peers file: " << e.what());
        return false;
    } catch (const std::exception& e) {
        LOG_CLIENT_ERROR("Failed to load historical peers file: " << e.what());
        return false;
    }
}

bool RatsClient::save_historical_peers() {
    try {
        // Get current peers and save them to historical file
        std::vector<RatsPeer> current_peers = get_validated_peers();
        
        for (const auto& peer : current_peers) {
            if (!append_peer_to_historical_file(peer)) {
                LOG_CLIENT_WARN("Failed to save peer to historical file: " << peer.ip << ":" << peer.port);
            }
        }
        
        LOG_CLIENT_INFO("Saved " << current_peers.size() << " current peers to historical file");
        return true;
        
    } catch (const std::exception& e) {
        LOG_CLIENT_ERROR("Failed to save historical peers: " << e.what());
        return false;
    }
}

void RatsClient::clear_historical_peers() {
    std::string file_path = get_peers_ever_file_path();
    
    try {
        // Create empty array and save to file
        nlohmann::json empty_array = nlohmann::json::array();
        std::string empty_data = empty_array.dump(4);
        
        if (create_file(file_path, empty_data)) {
            LOG_CLIENT_INFO("Cleared historical peers file");
        } else {
            LOG_CLIENT_ERROR("Failed to clear historical peers file");
        }
        
    } catch (const std::exception& e) {
        LOG_CLIENT_ERROR("Failed to clear historical peers: " << e.what());
    }
}

std::vector<RatsPeer> RatsClient::get_historical_peers() const {
    std::vector<RatsPeer> historical_peers;
    
    // Check if historical peers file exists
    if (!file_or_directory_exists(get_peers_ever_file_path())) {
        return historical_peers; // Return empty vector
    }
    
    try {
        std::string historical_data = read_file_text_cpp(get_peers_ever_file_path());
        if (historical_data.empty()) {
            return historical_peers;
        }
        
        nlohmann::json peers_json = nlohmann::json::parse(historical_data);
        
        if (!peers_json.is_array()) {
            LOG_CLIENT_ERROR("Invalid historical peers file format - expected array");
            return historical_peers;
        }
        
        for (const auto& peer_json : peers_json) {
            std::string ip;
            int port;
            std::string peer_id;
            
            if (deserialize_peer_from_persistence(peer_json, ip, port, peer_id)) {
                // Create a RatsPeer object for the historical peer
                // Note: This won't have all the runtime fields populated
                RatsPeer historical_peer(peer_id, ip, static_cast<uint16_t>(port), 
                                       INVALID_SOCKET_VALUE, ip + ":" + std::to_string(port), false);
                
                // Set additional fields from JSON if available
                historical_peer.version = peer_json.value("version", "");
                
                historical_peers.push_back(historical_peer);
            }
        }
        
        LOG_CLIENT_DEBUG("Retrieved " << historical_peers.size() << " historical peers");
        
    } catch (const nlohmann::json::exception& e) {
        LOG_CLIENT_ERROR("Failed to parse historical peers file: " << e.what());
    } catch (const std::exception& e) {
        LOG_CLIENT_ERROR("Failed to read historical peers file: " << e.what());
    }
    
    return historical_peers;
}

int RatsClient::load_and_reconnect_historical_peers() {
    if (!running_.load()) {
        LOG_CLIENT_DEBUG("Client not running, skipping historical peer reconnection");
        return 0;
    }
    
    LOG_CLIENT_INFO("Loading historical peers from " << get_peers_ever_file_path());
    
    // Check if historical peers file exists
    if (!file_or_directory_exists(get_peers_ever_file_path())) {
        LOG_CLIENT_INFO("No historical peers file found");
        return 0;
    }
    
    try {
        std::string historical_data = read_file_text_cpp(get_peers_ever_file_path());
        if (historical_data.empty()) {
            LOG_CLIENT_INFO("Historical peers file is empty");
            return 0;
        }
        
        nlohmann::json historical_peers = nlohmann::json::parse(historical_data);
        
        if (!historical_peers.is_array()) {
            LOG_CLIENT_ERROR("Invalid historical peers file format - expected array");
            return 0;
        }
        
        int reconnect_attempts = 0;
        
        for (const auto& peer_json : historical_peers) {
            std::string ip;
            int port;
            std::string peer_id;
            
            if (!deserialize_peer_from_persistence(peer_json, ip, port, peer_id)) {
                continue; // Skip invalid or old peers
            }
            
            // Don't connect to ourselves
            if (peer_id == get_our_peer_id()) {
                LOG_CLIENT_DEBUG("Skipping connection to ourselves: " << peer_id);
                continue;
            }
            
            // Check if we should ignore this peer (local interface)
            if (should_ignore_peer(ip, port)) {
                LOG_CLIENT_DEBUG("Ignoring historical peer " << ip << ":" << port << " - local interface address");
                continue;
            }
            
            // Check if we're already connected to this peer
            std::string normalized_peer_address = normalize_peer_address(ip, port);
            if (is_already_connected_to_address(normalized_peer_address)) {
                LOG_CLIENT_DEBUG("Already connected to historical peer " << normalized_peer_address);
                continue;
            }
            
            // Check if peer limit is reached
            if (is_peer_limit_reached()) {
                LOG_CLIENT_DEBUG("Peer limit reached, stopping historical reconnection attempts");
                break;
            }
            
            LOG_CLIENT_INFO("Attempting to reconnect to historical peer: " << ip << ":" << port << " (peer_id: " << peer_id << ")");
            
            // Attempt to connect (non-blocking)
            add_managed_thread(std::thread([this, ip, port, peer_id]() {
                if (connect_to_peer(ip, port)) {
                    LOG_CLIENT_INFO("Successfully reconnected to historical peer: " << ip << ":" << port);
                } else {
                    LOG_CLIENT_DEBUG("Failed to reconnect to historical peer: " << ip << ":" << port);
                }
            }), "historical-reconnect-" + peer_id.substr(0, 8));
            
            reconnect_attempts++;
            
            // Small delay between connection attempts to avoid overwhelming the network
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }
        
        LOG_CLIENT_INFO("Processed " << historical_peers.size() << " historical peers, attempted " << reconnect_attempts << " reconnections");
        return reconnect_attempts;
        
    } catch (const nlohmann::json::exception& e) {
        LOG_CLIENT_ERROR("Failed to parse historical peers file: " << e.what());
        return 0;
    } catch (const std::exception& e) {
        LOG_CLIENT_ERROR("Failed to load historical peers: " << e.what());
        return 0;
    }
}

// Configuration persistence implementation
std::string RatsClient::generate_persistent_peer_id() const {
    // Generate a unique peer ID using SHA1 hash of timestamp, random data, and hostname
    auto now = std::chrono::high_resolution_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    
    // Get system information for uniqueness
    SystemInfo sys_info = get_system_info();
    
    // Create random component
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    // Build unique string
    std::ostringstream unique_stream;
    unique_stream << timestamp << "_" << sys_info.hostname << "_" << listen_port_ << "_";
    
    // Add random component
    for (int i = 0; i < 16; ++i) {
        unique_stream << std::setfill('0') << std::setw(2) << std::hex << dis(gen);
    }
    
    // Generate SHA1 hash of the unique string
    std::string unique_string = unique_stream.str();
    std::string peer_id = SHA1::hash(unique_string);
    
    LOG_CLIENT_INFO("Generated new persistent peer ID: " << peer_id);
    return peer_id;
}

nlohmann::json RatsClient::serialize_peer_for_persistence(const RatsPeer& peer) const {
    nlohmann::json peer_json;
    peer_json["ip"] = peer.ip;
    peer_json["port"] = peer.port;
    peer_json["peer_id"] = peer.peer_id;
    peer_json["normalized_address"] = peer.normalized_address;
    peer_json["is_outgoing"] = peer.is_outgoing;
    peer_json["version"] = peer.version;
    
    // Add timestamp for cleanup of old peers
    auto now = std::chrono::high_resolution_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    peer_json["last_seen"] = timestamp;
    
    return peer_json;
}

bool RatsClient::deserialize_peer_from_persistence(const nlohmann::json& json, std::string& ip, int& port, std::string& peer_id) const {
    try {
        ip = json.value("ip", "");
        port = json.value("port", 0);
        peer_id = json.value("peer_id", "");
        
        // Validate required fields
        if (ip.empty() || port <= 0 || port > 65535 || peer_id.empty()) {
            return false;
        }
        
        // Check if peer data is not too old (optional - remove peers older than 7 days)
        if (json.contains("last_seen")) {
            auto now = std::chrono::high_resolution_clock::now();
            auto current_timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
            int64_t last_seen = json.value("last_seen", current_timestamp);
            
            const int64_t MAX_PEER_AGE_SECONDS = 7 * 24 * 60 * 60; // 7 days
            if (current_timestamp - last_seen > MAX_PEER_AGE_SECONDS) {
                LOG_CLIENT_DEBUG("Skipping old peer " << ip << ":" << port << " (last seen " << (current_timestamp - last_seen) << " seconds ago)");
                return false;
            }
        }
        
        return true;
        
    } catch (const nlohmann::json::exception& e) {
        LOG_CLIENT_ERROR("Failed to deserialize peer: " << e.what());
        return false;
    }
}

std::string RatsClient::get_config_file_path() const {
    #ifdef TESTING
        // For testing with ephemeral ports (port 0), use a unique identifier to avoid conflicts
        if (listen_port_ == 0) {
            // Generate a unique file path based on object pointer to ensure uniqueness during testing
            std::ostringstream oss;
            oss << "config_" << this << ".json";
            return oss.str();
        }
        return "config_" + std::to_string(listen_port_) + ".json";
    #else
        return data_directory_ + "/" + CONFIG_FILE_NAME;
    #endif
}

std::string RatsClient::get_peers_file_path() const {
    #ifdef TESTING
        // For testing with ephemeral ports (port 0), use a unique identifier to avoid conflicts
        if (listen_port_ == 0) {
            // Generate a unique file path based on object pointer to ensure uniqueness during testing
            std::ostringstream oss;
            oss << "peers_" << this << ".json";
            return oss.str();
        }
        return "peers_" + std::to_string(listen_port_) + ".json";
    #else
        return data_directory_ + "/" + PEERS_FILE_NAME;
    #endif
}

std::string RatsClient::get_peers_ever_file_path() const {
    #ifdef TESTING
        // For testing with ephemeral ports (port 0), use a unique identifier to avoid conflicts
        if (listen_port_ == 0) {
            // Generate a unique file path based on object pointer to ensure uniqueness during testing
            std::ostringstream oss;
            oss << "peers_ever_" << this << ".json";
            return oss.str();
        }
        return "peers_ever_" + std::to_string(listen_port_) + ".json";
    #else
        return data_directory_ + "/" + PEERS_EVER_FILE_NAME;
    #endif
}

// =========================================================================
// Data directory management
// =========================================================================

bool RatsClient::set_data_directory(const std::string& directory_path) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    // Normalize the path (remove trailing slashes)
    std::string normalized_path = directory_path;
    while (!normalized_path.empty() && (normalized_path.back() == '/' || normalized_path.back() == '\\')) {
        normalized_path.pop_back();
    }
    
    // Use current directory if empty
    if (normalized_path.empty()) {
        normalized_path = ".";
    }
    
    // Check if directory exists
    if (!directory_exists(normalized_path)) {
        // Try to create the directory
        if (!create_directories(normalized_path.c_str())) {
            LOG_CLIENT_ERROR("Failed to create data directory: " << normalized_path);
            return false;
        }
        LOG_CLIENT_INFO("Created data directory: " << normalized_path);
    }
    
    // Test if we can write to the directory by creating a temporary file
    std::string test_file = normalized_path + "/test_write_access.tmp";
    if (!create_file(test_file, "test")) {
        LOG_CLIENT_ERROR("Cannot write to data directory: " << normalized_path);
        return false;
    }
    
    // Clean up test file
    delete_file(test_file.c_str());
    
    data_directory_ = normalized_path;
    LOG_CLIENT_INFO("Data directory set to: " << data_directory_);
    return true;
}

std::string RatsClient::get_data_directory() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return data_directory_;
}

} // namespace librats
