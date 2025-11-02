#include "librats.h"
// Logging macros for RatsClient
#define LOG_CLIENT_DEBUG(message) LOG_DEBUG("client", message)
#define LOG_CLIENT_INFO(message)  LOG_INFO("client", message)
#define LOG_CLIENT_WARN(message)  LOG_WARN("client", message)
#define LOG_CLIENT_ERROR(message) LOG_ERROR("client", message)

namespace librats {

//=============================================================================
// GossipSub Integration Methods
//=============================================================================

GossipSub& RatsClient::get_gossipsub() {
    if (!gossipsub_) {
        throw std::runtime_error("GossipSub not initialized");
    }
    return *gossipsub_;
}

bool RatsClient::is_gossipsub_available() const {
    return gossipsub_ != nullptr;
}

//=============================================================================
// GossipSub Convenience Methods - Topic Management
//=============================================================================

bool RatsClient::subscribe_to_topic(const std::string& topic) {
    if (!is_gossipsub_available()) {
        LOG_CLIENT_ERROR("GossipSub not available for topic subscription: " << topic);
        return false;
    }
    
    try {
        return gossipsub_->subscribe(topic);
    } catch (const std::exception& e) {
        LOG_CLIENT_ERROR("Failed to subscribe to topic '" << topic << "': " << e.what());
        return false;
    }
}

bool RatsClient::unsubscribe_from_topic(const std::string& topic) {
    if (!is_gossipsub_available()) {
        LOG_CLIENT_ERROR("GossipSub not available for topic unsubscription: " << topic);
        return false;
    }
    
    try {
        return gossipsub_->unsubscribe(topic);
    } catch (const std::exception& e) {
        LOG_CLIENT_ERROR("Failed to unsubscribe from topic '" << topic << "': " << e.what());
        return false;
    }
}

bool RatsClient::is_subscribed_to_topic(const std::string& topic) const {
    if (!is_gossipsub_available()) {
        return false;
    }
    
    try {
        return gossipsub_->is_subscribed(topic);
    } catch (const std::exception& e) {
        LOG_CLIENT_ERROR("Failed to check subscription for topic '" << topic << "': " << e.what());
        return false;
    }
}

std::vector<std::string> RatsClient::get_subscribed_topics() const {
    if (!is_gossipsub_available()) {
        return {};
    }
    
    try {
        return gossipsub_->get_subscribed_topics();
    } catch (const std::exception& e) {
        LOG_CLIENT_ERROR("Failed to get subscribed topics: " << e.what());
        return {};
    }
}

//=============================================================================
// GossipSub Convenience Methods - Publishing
//=============================================================================

bool RatsClient::publish_to_topic(const std::string& topic, const std::string& message) {
    if (!is_gossipsub_available()) {
        LOG_CLIENT_ERROR("GossipSub not available for publishing to topic: " << topic);
        return false;
    }
    
    try {
        return gossipsub_->publish(topic, message);
    } catch (const std::exception& e) {
        LOG_CLIENT_ERROR("Failed to publish to topic '" << topic << "': " << e.what());
        return false;
    }
}

bool RatsClient::publish_json_to_topic(const std::string& topic, const nlohmann::json& message) {
    if (!is_gossipsub_available()) {
        LOG_CLIENT_ERROR("GossipSub not available for publishing JSON to topic: " << topic);
        return false;
    }
    
    try {
        return gossipsub_->publish(topic, message);
    } catch (const std::exception& e) {
        LOG_CLIENT_ERROR("Failed to publish JSON to topic '" << topic << "': " << e.what());
        return false;
    }
}

//=============================================================================
// GossipSub Convenience Methods - Event Handlers (Unified API)
//=============================================================================

void RatsClient::on_topic_message(const std::string& topic, std::function<void(const std::string&, const std::string&, const std::string&)> callback) {
    if (!is_gossipsub_available()) {
        LOG_CLIENT_ERROR("GossipSub not available for setting message handler on topic: " << topic);
        return;
    }
    
    try {
        // Convert unified callback signature to GossipSub's MessageHandler signature
        gossipsub_->set_message_handler(topic, [callback](const std::string& topic, const std::string& message, const std::string& sender_peer_id) {
            callback(sender_peer_id, topic, message);
        });
        LOG_CLIENT_INFO("Set message handler for topic: " << topic);
    } catch (const std::exception& e) {
        LOG_CLIENT_ERROR("Failed to set message handler for topic '" << topic << "': " << e.what());
    }
}

void RatsClient::on_topic_json_message(const std::string& topic, std::function<void(const std::string&, const std::string&, const nlohmann::json&)> callback) {
    if (!is_gossipsub_available()) {
        LOG_CLIENT_ERROR("GossipSub not available for setting JSON message handler on topic: " << topic);
        return;
    }
    
    try {
        // Convert unified callback signature to GossipSub's MessageHandler signature with JSON parsing
        gossipsub_->set_message_handler(topic, [callback](const std::string& topic, const std::string& message, const std::string& sender_peer_id) {
            try {
                nlohmann::json json_message = nlohmann::json::parse(message);
                callback(sender_peer_id, topic, json_message);
            } catch (const nlohmann::json::exception& e) {
                LOG_WARN("client", "Failed to parse JSON message on topic '" << topic << "': " << e.what());
                // Still call callback with empty JSON object to maintain consistency
                callback(sender_peer_id, topic, nlohmann::json{});
            }
        });
        LOG_CLIENT_INFO("Set JSON message handler for topic: " << topic);
    } catch (const std::exception& e) {
        LOG_CLIENT_ERROR("Failed to set JSON message handler for topic '" << topic << "': " << e.what());
    }
}

void RatsClient::on_topic_peer_joined(const std::string& topic, std::function<void(const std::string&, const std::string&)> callback) {
    if (!is_gossipsub_available()) {
        LOG_CLIENT_ERROR("GossipSub not available for setting peer joined handler on topic: " << topic);
        return;
    }
    
    try {
        // Convert unified callback signature to GossipSub's PeerJoinedHandler signature
        gossipsub_->set_peer_joined_handler(topic, [callback](const std::string& topic, const std::string& peer_id) {
            callback(peer_id, topic);
        });
        LOG_CLIENT_INFO("Set peer joined handler for topic: " << topic);
    } catch (const std::exception& e) {
        LOG_CLIENT_ERROR("Failed to set peer joined handler for topic '" << topic << "': " << e.what());
    }
}

void RatsClient::on_topic_peer_left(const std::string& topic, std::function<void(const std::string&, const std::string&)> callback) {
    if (!is_gossipsub_available()) {
        LOG_CLIENT_ERROR("GossipSub not available for setting peer left handler on topic: " << topic);
        return;
    }
    
    try {
        // Convert unified callback signature to GossipSub's PeerLeftHandler signature
        gossipsub_->set_peer_left_handler(topic, [callback](const std::string& topic, const std::string& peer_id) {
            callback(peer_id, topic);
        });
        LOG_CLIENT_INFO("Set peer left handler for topic: " << topic);
    } catch (const std::exception& e) {
        LOG_CLIENT_ERROR("Failed to set peer left handler for topic '" << topic << "': " << e.what());
    }
}

void RatsClient::set_topic_message_validator(const std::string& topic, std::function<ValidationResult(const std::string&, const std::string&, const std::string&)> validator) {
    if (!is_gossipsub_available()) {
        LOG_CLIENT_ERROR("GossipSub not available for setting message validator on topic: " << topic);
        return;
    }
    
    try {
        // Convert unified callback signature to GossipSub's MessageValidator signature
        gossipsub_->set_message_validator(topic, [validator](const std::string& topic, const std::string& message, const std::string& sender_peer_id) {
            return validator(topic, message, sender_peer_id);
        });
        LOG_CLIENT_INFO("Set message validator for topic: " << topic);
    } catch (const std::exception& e) {
        LOG_CLIENT_ERROR("Failed to set message validator for topic '" << topic << "': " << e.what());
    }
}

void RatsClient::off_topic(const std::string& topic) {
    if (!is_gossipsub_available()) {
        LOG_CLIENT_ERROR("GossipSub not available for removing handlers from topic: " << topic);
        return;
    }
    
    try {
        // Remove all handlers by setting them to nullptr
        gossipsub_->set_message_handler(topic, nullptr);
        gossipsub_->set_peer_joined_handler(topic, nullptr);
        gossipsub_->set_peer_left_handler(topic, nullptr);
        gossipsub_->set_message_validator(topic, nullptr);
        LOG_CLIENT_INFO("Removed all handlers for topic: " << topic);
    } catch (const std::exception& e) {
        LOG_CLIENT_ERROR("Failed to remove handlers for topic '" << topic << "': " << e.what());
    }
}

//=============================================================================
// GossipSub Convenience Methods - Information
//=============================================================================

std::vector<std::string> RatsClient::get_topic_peers(const std::string& topic) const {
    if (!is_gossipsub_available()) {
        return {};
    }
    
    try {
        return gossipsub_->get_topic_peers(topic);
    } catch (const std::exception& e) {
        LOG_CLIENT_ERROR("Failed to get topic peers for '" << topic << "': " << e.what());
        return {};
    }
}

std::vector<std::string> RatsClient::get_topic_mesh_peers(const std::string& topic) const {
    if (!is_gossipsub_available()) {
        return {};
    }
    
    try {
        return gossipsub_->get_mesh_peers(topic);
    } catch (const std::exception& e) {
        LOG_CLIENT_ERROR("Failed to get mesh peers for topic '" << topic << "': " << e.what());
        return {};
    }
}

nlohmann::json RatsClient::get_gossipsub_statistics() const {
    if (!is_gossipsub_available()) {
        nlohmann::json empty_stats;
        empty_stats["available"] = false;
        empty_stats["error"] = "GossipSub not initialized";
        return empty_stats;
    }
    
    try {
        nlohmann::json stats = gossipsub_->get_statistics();
        stats["available"] = true;
        stats["running"] = gossipsub_->is_running();
        return stats;
    } catch (const std::exception& e) {
        nlohmann::json error_stats;
        error_stats["available"] = true;
        error_stats["error"] = e.what();
        return error_stats;
    }
}

bool RatsClient::is_gossipsub_running() const {
    if (!is_gossipsub_available()) {
        return false;
    }
    
    try {
        return gossipsub_->is_running();
    } catch (const std::exception& e) {
        LOG_CLIENT_ERROR("Failed to check GossipSub running status: " << e.what());
        return false;
    }
}

}