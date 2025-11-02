#include "librats.h"
#include "encrypted_socket.h"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <memory>
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

//=============================================================================
// Encryption Methods Implementation
//=============================================================================

bool RatsClient::initialize_encryption(bool enable) {
    std::lock_guard<std::mutex> lock(encryption_mutex_);
    
    encryption_enabled_ = enable;
    
    if (enable) {
        // Initialize the encryption system with our static key
        bool success = encrypted_communication::initialize_encryption(static_encryption_key_);
        if (!success) {
            LOG_CLIENT_ERROR("Failed to initialize encryption system");
            encryption_enabled_ = false;
            return false;
        }
        
        LOG_CLIENT_INFO("Encryption initialized and enabled");
    } else {
        encrypted_communication::set_encryption_enabled(false);
        LOG_CLIENT_INFO("Encryption disabled");
    }
    
    return true;
}

void RatsClient::set_encryption_enabled(bool enabled) {
    std::lock_guard<std::mutex> lock(encryption_mutex_);
    encryption_enabled_ = enabled;
    encrypted_communication::set_encryption_enabled(enabled);
    
    LOG_CLIENT_INFO("Encryption " << (enabled ? "enabled" : "disabled"));
}

bool RatsClient::is_encryption_enabled() const {
    std::lock_guard<std::mutex> lock(encryption_mutex_);
    return encryption_enabled_;
}

std::string RatsClient::get_encryption_key() const {
    std::lock_guard<std::mutex> lock(encryption_mutex_);
    return EncryptedSocket::key_to_string(static_encryption_key_);
}

bool RatsClient::set_encryption_key(const std::string& key_hex) {
    std::lock_guard<std::mutex> lock(encryption_mutex_);
    
    NoiseKey new_key = EncryptedSocket::string_to_key(key_hex);
    
    // Validate key (check if it's not all zeros)
    bool is_valid = false;
    for (uint8_t byte : new_key) {
        if (byte != 0) {
            is_valid = true;
            break;
        }
    }
    
    if (!is_valid) {
        LOG_CLIENT_ERROR("Invalid encryption key provided");
        return false;
    }
    
    static_encryption_key_ = new_key;
    
    // Reinitialize encryption system if it's enabled
    if (encryption_enabled_) {
        encrypted_communication::initialize_encryption(static_encryption_key_);
    }
    
    LOG_CLIENT_INFO("Updated encryption key");
    return true;
}

std::string RatsClient::generate_new_encryption_key() {
    std::lock_guard<std::mutex> lock(encryption_mutex_);
    
    static_encryption_key_ = encrypted_communication::generate_node_key();
    
    // Reinitialize encryption system if it's enabled
    if (encryption_enabled_) {
        encrypted_communication::initialize_encryption(static_encryption_key_);
    }
    
    std::string key_hex = EncryptedSocket::key_to_string(static_encryption_key_);
    LOG_CLIENT_INFO("Generated new encryption key: " << key_hex);
    
    return key_hex;
}

bool RatsClient::is_peer_encrypted(const std::string& peer_id) const {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    auto it = peers_.find(peer_id);
    if (it != peers_.end()) {
        return it->second.encryption_enabled && it->second.noise_handshake_completed;
    }
    return false;
}

} // namespace librats
