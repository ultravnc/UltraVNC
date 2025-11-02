#include "gossipsub.h"
#include "librats.h"
#include "logger.h"
#include "sha1.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

// GossipSub logging macros
#define LOG_GOSSIPSUB_DEBUG(message) LOG_DEBUG("gossipsub", message)
#define LOG_GOSSIPSUB_INFO(message)  LOG_INFO("gossipsub", message)
#define LOG_GOSSIPSUB_WARN(message)  LOG_WARN("gossipsub", message)
#define LOG_GOSSIPSUB_ERROR(message) LOG_ERROR("gossipsub", message)

namespace librats {

// Helper function to convert GossipSubMessageType to string
std::string gossipsub_message_type_to_string(GossipSubMessageType type) {
    switch (type) {
        case GossipSubMessageType::SUBSCRIBE: return "subscribe";
        case GossipSubMessageType::UNSUBSCRIBE: return "unsubscribe";
        case GossipSubMessageType::PUBLISH: return "publish";
        case GossipSubMessageType::GOSSIP: return "gossip";
        case GossipSubMessageType::GRAFT: return "graft";
        case GossipSubMessageType::PRUNE: return "prune";
        case GossipSubMessageType::IHAVE: return "ihave";
        case GossipSubMessageType::IWANT: return "iwant";
        case GossipSubMessageType::HEARTBEAT: return "heartbeat";
        default: return "unknown";
    }
}

GossipSubMessageType string_to_gossipsub_message_type(const std::string& str) {
    if (str == "subscribe") return GossipSubMessageType::SUBSCRIBE;
    if (str == "unsubscribe") return GossipSubMessageType::UNSUBSCRIBE;
    if (str == "publish") return GossipSubMessageType::PUBLISH;
    if (str == "gossip") return GossipSubMessageType::GOSSIP;
    if (str == "graft") return GossipSubMessageType::GRAFT;
    if (str == "prune") return GossipSubMessageType::PRUNE;
    if (str == "ihave") return GossipSubMessageType::IHAVE;
    if (str == "iwant") return GossipSubMessageType::IWANT;
    if (str == "heartbeat") return GossipSubMessageType::HEARTBEAT;
    return GossipSubMessageType::HEARTBEAT; // Default fallback
}

//=============================================================================
// PeerScore Implementation
//=============================================================================

void PeerScore::update_score() {
    auto now = std::chrono::steady_clock::now();
    auto time_since_update = std::chrono::duration_cast<std::chrono::seconds>(now - last_updated).count();
    auto connection_duration = std::chrono::duration_cast<std::chrono::seconds>(now - connected_since).count();
    
    // Update topic score based on participation
    topic_score = (std::min)(10.0, connection_duration / 60.0); // Up to 10 points for long connections
    
    // Update delivery score
    if (messages_delivered > 0) {
        delivery_score = (std::min)(20.0, static_cast<double>(messages_delivered) / 10.0);
    }
    
    // Update mesh behavior score
    double graft_prune_ratio = (graft_requests + prune_requests > 0) ? 
        static_cast<double>(graft_requests) / (graft_requests + prune_requests) : 0.5;
    mesh_behavior_score = (graft_prune_ratio - 0.5) * 10.0; // -5 to +5 points
    
    // Update invalid message penalty
    if (messages_invalid > 0) {
        invalid_message_score = -static_cast<double>(messages_invalid) * 5.0; // -5 points per invalid message
    }
    
    // Calculate total score
    score = topic_score + delivery_score + mesh_behavior_score + invalid_message_score;
    
    last_updated = now;
}

//=============================================================================
// GossipSub Implementation
//=============================================================================

GossipSub::GossipSub(RatsClient& rats_client, const GossipSubConfig& config)
    : rats_client_(rats_client), config_(config), running_(false), rng_(std::random_device{}()) {
    
    // Register message handler for gossipsub messages
    rats_client_.on("gossipsub", [this](const std::string& peer_id, const nlohmann::json& message) {
        handle_gossipsub_message(peer_id, message);
    });
}

GossipSub::~GossipSub() {
    stop();
}

bool GossipSub::start() {
    if (running_.load()) {
        return false; // Already running
    }
    
    running_.store(true);
    
    // Start heartbeat thread
    heartbeat_thread_ = std::thread(&GossipSub::heartbeat_loop, this);
    
    LOG_GOSSIPSUB_INFO("GossipSub service started");
    return true;
}

void GossipSub::stop() {
    if (!running_.load()) {
        return; // Already stopped
    }

    LOG_GOSSIPSUB_INFO("GossipSub service stopping");

    {
        // Unsubscribe from all topics
        std::lock_guard<std::mutex> topics_lock(topics_mutex_);
        for (const auto& topic : subscribed_topics_) {
            nlohmann::json payload;
            payload["topic"] = topic;
            broadcast_gossipsub_message(GossipSubMessageType::UNSUBSCRIBE, payload);
        }
        subscribed_topics_.clear();
    }

    running_.store(false);
    
    // Notify the heartbeat thread to wake up immediately
    {
        std::lock_guard<std::mutex> lock(heartbeat_mutex_);
        heartbeat_cv_.notify_all();
    }
    
    // Join heartbeat thread with timeout to avoid infinite hang
    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }
    
    LOG_GOSSIPSUB_INFO("GossipSub service stopped");
}

bool GossipSub::is_running() const {
    return running_.load();
}

//=============================================================================
// Topic Management
//=============================================================================

bool GossipSub::subscribe(const std::string& topic) {
    std::lock_guard<std::mutex> lock(topics_mutex_);
    
    if (subscribed_topics_.count(topic)) {
        return false; // Already subscribed
    }
    
    subscribed_topics_.insert(topic);
    
    // Get or create topic subscription
    TopicSubscription* topic_sub = get_or_create_topic(topic);
    if (!topic_sub) {
        subscribed_topics_.erase(topic);
        return false;
    }
    
    // Broadcast subscription to all peers
    nlohmann::json payload;
    payload["topic"] = topic;
    broadcast_gossipsub_message(GossipSubMessageType::SUBSCRIBE, payload);
    
    // Start building mesh for this topic
    maintain_mesh(topic);
    
            LOG_GOSSIPSUB_INFO("Subscribed to topic: " << topic);
    return true;
}

bool GossipSub::unsubscribe(const std::string& topic) {
    std::lock_guard<std::mutex> lock(topics_mutex_);
    
    if (!subscribed_topics_.count(topic)) {
        return false; // Not subscribed
    }
    
    subscribed_topics_.erase(topic);
    
    // Broadcast unsubscription to all peers
    nlohmann::json payload;
    payload["topic"] = topic;
    broadcast_gossipsub_message(GossipSubMessageType::UNSUBSCRIBE, payload);
    
    // Leave mesh for this topic
    auto topic_it = topics_.find(topic);
    if (topic_it != topics_.end()) {
        TopicSubscription* topic_sub = topic_it->second.get();
        
        // Send PRUNE to all mesh peers
        for (const auto& peer_id : topic_sub->mesh_peers) {
            nlohmann::json prune_payload;
            prune_payload["topic"] = topic;
            send_gossipsub_message(peer_id, GossipSubMessageType::PRUNE, prune_payload);
        }
        
        topic_sub->mesh_peers.clear();
    }
    
            LOG_GOSSIPSUB_INFO("Unsubscribed from topic: " << topic);
    return true;
}

bool GossipSub::is_subscribed(const std::string& topic) const {
    std::lock_guard<std::mutex> lock(topics_mutex_);
    return subscribed_topics_.count(topic) > 0;
}

std::vector<std::string> GossipSub::get_subscribed_topics() const {
    std::lock_guard<std::mutex> lock(topics_mutex_);
    return std::vector<std::string>(subscribed_topics_.begin(), subscribed_topics_.end());
}

//=============================================================================
// Message Publishing
//=============================================================================

bool GossipSub::publish(const std::string& topic, const std::string& message) {
    if (!running_.load()) {
        return false;
    }
    
    std::string our_peer_id = rats_client_.get_our_peer_id();
    std::string message_id = generate_message_id(topic, message, our_peer_id);
    
    // Check if we've already seen this message
    if (is_message_seen(message_id)) {
        return false;
    }
    
    // Cache the message
    cache_message(message_id, topic, message, our_peer_id);
    
    // Create publish message
    nlohmann::json payload;
    payload["topic"] = topic;
    payload["message"] = message;
    payload["message_id"] = message_id;
    payload["sender_peer_id"] = our_peer_id;
    payload["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    std::lock_guard<std::mutex> topics_lock(topics_mutex_);
    
    // If we're subscribed to this topic, send to mesh peers
    if (subscribed_topics_.count(topic)) {
        auto topic_it = topics_.find(topic);
        if (topic_it != topics_.end()) {
            TopicSubscription* topic_sub = topic_it->second.get();
            
            for (const auto& peer_id : topic_sub->mesh_peers) {
                if (is_peer_score_acceptable(peer_id, config_.score_threshold_publish)) {
                    send_gossipsub_message(peer_id, GossipSubMessageType::PUBLISH, payload);
                }
            }
        }
    } else {
        // If not subscribed, use fanout
        TopicSubscription* topic_sub = get_or_create_topic(topic);
        if (topic_sub) {
            // Select fanout peers if we don't have enough
            if (topic_sub->fanout_peers.size() < static_cast<size_t>(config_.fanout_size)) {
                std::vector<std::string> candidates = select_peers_for_gossip(topic, 
                    config_.fanout_size - static_cast<int>(topic_sub->fanout_peers.size()), topic_sub->fanout_peers);
                for (const auto& peer_id : candidates) {
                    topic_sub->fanout_peers.insert(peer_id);
                }
            }
            
            // Send to fanout peers
            for (const auto& peer_id : topic_sub->fanout_peers) {
                if (is_peer_score_acceptable(peer_id, config_.score_threshold_publish)) {
                    send_gossipsub_message(peer_id, GossipSubMessageType::PUBLISH, payload);
                }
            }
        }
    }
    
    LOG_GOSSIPSUB_DEBUG("Published message to topic: " << topic << " (ID: " << message_id << ")");
    return true;
}

bool GossipSub::publish(const std::string& topic, const nlohmann::json& message) {
    return publish(topic, message.dump());
}

//=============================================================================
// Message Handling
//=============================================================================

void GossipSub::handle_gossipsub_message(const std::string& peer_id, const nlohmann::json& message) {
    try {
        std::string type_str = message.value("type", "");
        GossipSubMessageType type = string_to_gossipsub_message_type(type_str);
        nlohmann::json payload = message.value("payload", nlohmann::json::object());
        
        LOG_GOSSIPSUB_DEBUG("Received gossipsub " << type_str << " message from " << peer_id);
        
        switch (type) {
            case GossipSubMessageType::SUBSCRIBE:
                handle_subscribe(peer_id, payload);
                break;
            case GossipSubMessageType::UNSUBSCRIBE:
                handle_unsubscribe(peer_id, payload);
                break;
            case GossipSubMessageType::PUBLISH:
                handle_publish(peer_id, payload);
                break;
            case GossipSubMessageType::GOSSIP:
                handle_gossip(peer_id, payload);
                break;
            case GossipSubMessageType::GRAFT:
                handle_graft(peer_id, payload);
                break;
            case GossipSubMessageType::PRUNE:
                handle_prune(peer_id, payload);
                break;
            case GossipSubMessageType::IHAVE:
                handle_ihave(peer_id, payload);
                break;
            case GossipSubMessageType::IWANT:
                handle_iwant(peer_id, payload);
                break;
            case GossipSubMessageType::HEARTBEAT:
                handle_heartbeat(peer_id, payload);
                break;
        }
        
    } catch (const std::exception& e) {
        LOG_GOSSIPSUB_ERROR("Failed to handle gossipsub message from " << peer_id << ": " << e.what());
        
        // Update peer score for invalid message
        std::lock_guard<std::mutex> scores_lock(scores_mutex_);
        auto score_it = peer_scores_.find(peer_id);
        if (score_it != peer_scores_.end()) {
            score_it->second->messages_invalid++;
            score_it->second->update_score();
        }
    }
}

void GossipSub::handle_subscribe(const std::string& peer_id, const nlohmann::json& payload) {
    std::string topic = payload.value("topic", "");
    if (topic.empty()) {
        return;
    }
    
    std::lock_guard<std::mutex> topics_lock(topics_mutex_);
    TopicSubscription* topic_sub = get_or_create_topic(topic);
    if (!topic_sub) {
        return;
    }
    
    // Add peer to subscribers
    bool was_new = topic_sub->subscribers.insert(peer_id).second;
    
    if (was_new) {
                    LOG_GOSSIPSUB_DEBUG("Peer " << peer_id << " subscribed to topic: " << topic);
        
        // Call peer joined handler
        std::lock_guard<std::mutex> handlers_lock(handlers_mutex_);
        auto handler_it = peer_joined_handlers_.find(topic);
        if (handler_it != peer_joined_handlers_.end()) {
            try {
                handler_it->second(topic, peer_id);
            } catch (const std::exception& e) {
                LOG_GOSSIPSUB_ERROR("Exception in peer joined handler for topic '" << topic << "': " << e.what());
            }
        }
        
        // If we're subscribed to this topic, consider adding peer to mesh
        if (subscribed_topics_.count(topic)) {
            maintain_mesh(topic);
        }
    }
}

void GossipSub::handle_unsubscribe(const std::string& peer_id, const nlohmann::json& payload) {
    std::string topic = payload.value("topic", "");
    if (topic.empty()) {
        return;
    }
    
    std::lock_guard<std::mutex> topics_lock(topics_mutex_);
    auto topic_it = topics_.find(topic);
    if (topic_it == topics_.end()) {
        return;
    }
    
    TopicSubscription* topic_sub = topic_it->second.get();
    
    // Remove peer from subscribers and mesh
    bool was_subscribed = topic_sub->subscribers.erase(peer_id) > 0;
    bool was_in_mesh = topic_sub->mesh_peers.erase(peer_id) > 0;
    topic_sub->fanout_peers.erase(peer_id);
    
    if (was_subscribed) {
                    LOG_GOSSIPSUB_DEBUG("Peer " << peer_id << " unsubscribed from topic: " << topic);
        
        // Call peer left handler
        std::lock_guard<std::mutex> handlers_lock(handlers_mutex_);
        auto handler_it = peer_left_handlers_.find(topic);
        if (handler_it != peer_left_handlers_.end()) {
            try {
                handler_it->second(topic, peer_id);
            } catch (const std::exception& e) {
                LOG_GOSSIPSUB_ERROR("Exception in peer left handler for topic '" << topic << "': " << e.what());
            }
        }
        
        // If peer was in mesh and we're subscribed, maintain mesh
        if (was_in_mesh && subscribed_topics_.count(topic)) {
            maintain_mesh(topic);
        }
    }
    
    // Clean up topic if no subscribers
    if (topic_sub->subscribers.empty()) {
        cleanup_topic(topic);
    }
}

void GossipSub::handle_publish(const std::string& peer_id, const nlohmann::json& payload) {
    std::string topic = payload.value("topic", "");
    std::string message = payload.value("message", "");
    std::string message_id = payload.value("message_id", "");
    std::string sender_peer_id = payload.value("sender_peer_id", peer_id);
    
    if (topic.empty() || message.empty() || message_id.empty()) {
        return;
    }
    
    // Check if we've already seen this message
    if (is_message_seen(message_id)) {
        return;
    }
    
    // Validate message
    ValidationResult validation = validate_message(topic, message, sender_peer_id);
    if (validation == ValidationResult::REJECT) {
        // Update peer score for invalid message
        std::lock_guard<std::mutex> scores_lock(scores_mutex_);
        auto score_it = peer_scores_.find(peer_id);
        if (score_it != peer_scores_.end()) {
            score_it->second->messages_invalid++;
            score_it->second->update_score();
        }
        return;
    }
    
    if (validation == ValidationResult::IGNORE_MSG) {
        return;
    }
    
    // Cache the message
    cache_message(message_id, topic, message, sender_peer_id);
    
    // Update peer score for valid message delivery
    {
        std::lock_guard<std::mutex> scores_lock(scores_mutex_);
        auto score_it = peer_scores_.find(peer_id);
        if (score_it != peer_scores_.end()) {
            score_it->second->messages_delivered++;
            score_it->second->update_score();
        }
    }
    
    std::lock_guard<std::mutex> topics_lock(topics_mutex_);
    
    // Forward message to mesh peers (except sender)
    if (subscribed_topics_.count(topic)) {
        auto topic_it = topics_.find(topic);
        if (topic_it != topics_.end()) {
            TopicSubscription* topic_sub = topic_it->second.get();
            
            for (const auto& forward_peer_id : topic_sub->mesh_peers) {
                if (forward_peer_id != peer_id && 
                    is_peer_score_acceptable(forward_peer_id, config_.score_threshold_gossip)) {
                    send_gossipsub_message(forward_peer_id, GossipSubMessageType::PUBLISH, payload);
                }
            }
        }
        
        // Call local message handler
        std::lock_guard<std::mutex> handlers_lock(handlers_mutex_);
        auto handler_it = message_handlers_.find(topic);
        if (handler_it != message_handlers_.end()) {
            try {
                handler_it->second(topic, message, sender_peer_id);
            } catch (const std::exception& e) {
                LOG_GOSSIPSUB_ERROR("Exception in message handler for topic '" << topic << "': " << e.what());
            }
        }
    }
    
    LOG_GOSSIPSUB_DEBUG("Processed published message for topic: " << topic << " (ID: " << message_id << ")");
}

void GossipSub::handle_gossip(const std::string& peer_id, const nlohmann::json& payload) {
    // Handle gossip messages (IHAVE/IWANT are sent as gossip)
    LOG_GOSSIPSUB_DEBUG("Received gossip message from " << peer_id);
}

void GossipSub::handle_graft(const std::string& peer_id, const nlohmann::json& payload) {
    std::string topic = payload.value("topic", "");
    if (topic.empty()) {
        return;
    }
    
    // Update peer score
    {
        std::lock_guard<std::mutex> scores_lock(scores_mutex_);
        auto score_it = peer_scores_.find(peer_id);
        if (score_it != peer_scores_.end()) {
            score_it->second->graft_requests++;
            score_it->second->update_score();
        }
    }
    
    std::lock_guard<std::mutex> topics_lock(topics_mutex_);
    
    // Only accept graft if we're subscribed and peer score is acceptable
    if (subscribed_topics_.count(topic) && 
        is_peer_score_acceptable(peer_id, config_.score_threshold_mesh)) {
        
        TopicSubscription* topic_sub = get_or_create_topic(topic);
        if (topic_sub) {
            topic_sub->mesh_peers.insert(peer_id);
            LOG_GOSSIPSUB_DEBUG("Added peer " << peer_id << " to mesh for topic: " << topic);
        }
    } else {
        // Send PRUNE in response to reject the graft
        nlohmann::json prune_payload;
        prune_payload["topic"] = topic;
        send_gossipsub_message(peer_id, GossipSubMessageType::PRUNE, prune_payload);
    }
}

void GossipSub::handle_prune(const std::string& peer_id, const nlohmann::json& payload) {
    std::string topic = payload.value("topic", "");
    if (topic.empty()) {
        return;
    }
    
    // Update peer score
    {
        std::lock_guard<std::mutex> scores_lock(scores_mutex_);
        auto score_it = peer_scores_.find(peer_id);
        if (score_it != peer_scores_.end()) {
            score_it->second->prune_requests++;
            score_it->second->update_score();
        }
    }
    
    std::lock_guard<std::mutex> topics_lock(topics_mutex_);
    auto topic_it = topics_.find(topic);
    if (topic_it != topics_.end()) {
        TopicSubscription* topic_sub = topic_it->second.get();
        topic_sub->mesh_peers.erase(peer_id);
        LOG_GOSSIPSUB_DEBUG("Removed peer " << peer_id << " from mesh for topic: " << topic);
    }
}

void GossipSub::handle_ihave(const std::string& peer_id, const nlohmann::json& payload) {
    std::vector<std::string> message_ids = payload.value("message_ids", std::vector<std::string>());
    std::string topic = payload.value("topic", "");
    
    if (message_ids.empty() || topic.empty()) {
        return;
    }
    
    // Check which messages we want
    std::vector<std::string> wanted_messages;
    for (const auto& msg_id : message_ids) {
        if (!is_message_seen(msg_id)) {
            wanted_messages.push_back(msg_id);
        }
    }
    
    // Send IWANT for messages we don't have
    if (!wanted_messages.empty()) {
        nlohmann::json iwant_payload;
        iwant_payload["message_ids"] = wanted_messages;
        iwant_payload["topic"] = topic;
        send_gossipsub_message(peer_id, GossipSubMessageType::IWANT, iwant_payload);
    }
}

void GossipSub::handle_iwant(const std::string& peer_id, const nlohmann::json& payload) {
    std::vector<std::string> message_ids = payload.value("message_ids", std::vector<std::string>());
    std::string topic = payload.value("topic", "");
    
    if (message_ids.empty() || topic.empty()) {
        return;
    }
    
    // Send requested messages that we have
    std::lock_guard<std::mutex> cache_lock(message_cache_mutex_);
    for (const auto& msg_id : message_ids) {
        auto cache_it = message_cache_.find(msg_id);
        if (cache_it != message_cache_.end()) {
            MessageMetadata* metadata = cache_it->second.get();
            
            // TODO: Store actual message content in cache
            // For now, we'll send a placeholder response
            nlohmann::json publish_payload;
            publish_payload["topic"] = metadata->topic;
            publish_payload["message"] = ""; // Would contain actual message
            publish_payload["message_id"] = msg_id;
            publish_payload["sender_peer_id"] = metadata->sender_peer_id;
            
            send_gossipsub_message(peer_id, GossipSubMessageType::PUBLISH, publish_payload);
        }
    }
}

void GossipSub::handle_heartbeat(const std::string& peer_id, const nlohmann::json& payload) {
    // Process any control messages in the heartbeat
    if (payload.contains("graft")) {
        auto graft_messages = payload["graft"];
        for (const auto& graft : graft_messages) {
            handle_graft(peer_id, graft);
        }
    }
    
    if (payload.contains("prune")) {
        auto prune_messages = payload["prune"];
        for (const auto& prune : prune_messages) {
            handle_prune(peer_id, prune);
        }
    }
    
    if (payload.contains("ihave")) {
        auto ihave_messages = payload["ihave"];
        for (const auto& ihave : ihave_messages) {
            handle_ihave(peer_id, ihave);
        }
    }
    
    if (payload.contains("iwant")) {
        auto iwant_messages = payload["iwant"];
        for (const auto& iwant : iwant_messages) {
            handle_iwant(peer_id, iwant);
        }
    }
}

//=============================================================================
// Utility Functions
//=============================================================================

std::string GossipSub::generate_message_id(const std::string& topic, const std::string& message, const std::string& sender_peer_id) {
    std::string combined = topic + message + sender_peer_id + std::to_string(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    
    return SHA1::hash(combined);
}

bool GossipSub::is_message_seen(const std::string& message_id) {
    std::lock_guard<std::mutex> lock(message_cache_mutex_);
    return message_cache_.count(message_id) > 0;
}

void GossipSub::cache_message(const std::string& message_id, const std::string& topic, const std::string& message, const std::string& sender_peer_id) {
    std::lock_guard<std::mutex> lock(message_cache_mutex_);
    
    auto metadata = std::make_unique<MessageMetadata>(message_id, topic, sender_peer_id);
    message_cache_[message_id] = std::move(metadata);
    message_ids_seen_[message_id] = std::chrono::steady_clock::now();
}

TopicSubscription* GossipSub::get_or_create_topic(const std::string& topic) {
    auto topic_it = topics_.find(topic);
    if (topic_it == topics_.end()) {
        auto topic_sub = std::make_unique<TopicSubscription>(topic);
        TopicSubscription* ptr = topic_sub.get();
        topics_[topic] = std::move(topic_sub);
        return ptr;
    }
    return topic_it->second.get();
}

void GossipSub::cleanup_topic(const std::string& topic) {
    auto topic_it = topics_.find(topic);
    if (topic_it != topics_.end() && topic_it->second->subscribers.empty()) {
        topics_.erase(topic_it);
    }
}

bool GossipSub::send_gossipsub_message(const std::string& peer_id, GossipSubMessageType type, const nlohmann::json& payload) {
    nlohmann::json message;
    message["type"] = gossipsub_message_type_to_string(type);
    message["payload"] = payload;
    
    try {
        rats_client_.send(peer_id, "gossipsub", message);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool GossipSub::broadcast_gossipsub_message(GossipSubMessageType type, const nlohmann::json& payload, const std::unordered_set<std::string>& exclude) {
    nlohmann::json message;
    message["type"] = gossipsub_message_type_to_string(type);
    message["payload"] = payload;
    
    // Get all connected peers
    auto all_peers = rats_client_.get_all_peers();
    bool any_sent = false;
    
    for (const auto& peer : all_peers) {
        if (exclude.count(peer.peer_id) == 0) {
            try {
                rats_client_.send(peer.peer_id, "gossipsub", message);
                any_sent = true;
            } catch (const std::exception&) {
                // Continue with other peers
            }
        }
    }
    
    return any_sent;
}

ValidationResult GossipSub::validate_message(const std::string& topic, const std::string& message, const std::string& sender_peer_id) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    
    // Check topic-specific validator first
    auto validator_it = message_validators_.find(topic);
    if (validator_it != message_validators_.end()) {
        return validator_it->second(topic, message, sender_peer_id);
    }
    
    // Check global validator
    if (global_validator_) {
        return global_validator_(topic, message, sender_peer_id);
    }
    
    // Default: accept all messages
    return ValidationResult::ACCEPT;
}

bool GossipSub::is_peer_score_acceptable(const std::string& peer_id, double threshold) {
    std::lock_guard<std::mutex> lock(scores_mutex_);
    auto score_it = peer_scores_.find(peer_id);
    if (score_it == peer_scores_.end()) {
        return true; // Unknown peer, accept for now
    }
    return score_it->second->score >= threshold;
}

void GossipSub::handle_peer_connected(const std::string& peer_id) {
    // Initialize peer score
    std::lock_guard<std::mutex> lock(scores_mutex_);
    if (peer_scores_.find(peer_id) == peer_scores_.end()) {
        peer_scores_[peer_id] = std::make_unique<PeerScore>(peer_id);
    }
}

void GossipSub::handle_peer_disconnected(const std::string& peer_id) {
    // Remove peer from all topics
    std::lock_guard<std::mutex> topics_lock(topics_mutex_);
    for (auto& topic_pair : topics_) {
        TopicSubscription* topic_sub = topic_pair.second.get();
        topic_sub->subscribers.erase(peer_id);
        topic_sub->mesh_peers.erase(peer_id);
        topic_sub->fanout_peers.erase(peer_id);
    }
    
    // Remove peer score
    std::lock_guard<std::mutex> scores_lock(scores_mutex_);
    peer_scores_.erase(peer_id);
}

//=============================================================================
// Heartbeat and Mesh Maintenance
//=============================================================================

void GossipSub::heartbeat_loop() {
    while (running_.load()) {
        try {
            process_heartbeat();
        } catch (const std::exception& e) {
            LOG_GOSSIPSUB_ERROR("Exception in heartbeat loop: " << e.what());
        }
        
        // Use condition variable for interruptible sleep
        std::unique_lock<std::mutex> lock(heartbeat_mutex_);
        heartbeat_cv_.wait_for(lock, config_.heartbeat_interval, [this] { 
            return !running_.load(); 
        });
    }
}

void GossipSub::process_heartbeat() {
    cleanup_message_cache();
    
    std::lock_guard<std::mutex> topics_lock(topics_mutex_);
    
    for (const auto& topic : subscribed_topics_) {
        maintain_mesh(topic);
    }
    
    // Process fanout cleanup
    auto now = std::chrono::steady_clock::now();
    for (auto& topic_pair : topics_) {
        TopicSubscription* topic_sub = topic_pair.second.get();
        
        // Clean up old fanout peers if we're not subscribed
        if (!subscribed_topics_.count(topic_pair.first)) {
            auto time_since_prune = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - topic_sub->last_fanout_prune);
            
            if (time_since_prune >= config_.fanout_ttl) {
                topic_sub->fanout_peers.clear();
                topic_sub->last_fanout_prune = now;
            }
        }
    }
    
    // Update peer scores
    std::lock_guard<std::mutex> scores_lock(scores_mutex_);
    for (auto& score_pair : peer_scores_) {
        score_pair.second->update_score();
    }
}

void GossipSub::maintain_mesh(const std::string& topic) {
    auto topic_it = topics_.find(topic);
    if (topic_it == topics_.end()) {
        return;
    }
    
    TopicSubscription* topic_sub = topic_it->second.get();
    int current_mesh_size = static_cast<int>(topic_sub->mesh_peers.size());
    
    // Remove low-scoring peers from mesh
    std::vector<std::string> to_remove;
    for (const auto& peer_id : topic_sub->mesh_peers) {
        if (!is_peer_score_acceptable(peer_id, config_.score_threshold_mesh)) {
            to_remove.push_back(peer_id);
        }
    }
    
    for (const auto& peer_id : to_remove) {
        remove_peer_from_mesh(topic, peer_id);
        current_mesh_size--;
    }
    
    // Add peers if below optimal
    if (current_mesh_size < config_.mesh_optimal) {
        int needed = config_.mesh_optimal - current_mesh_size;
        std::vector<std::string> candidates = select_peers_for_mesh(topic, needed);
        
        for (const auto& peer_id : candidates) {
            add_peer_to_mesh(topic, peer_id);
        }
    }
    
    // Remove excess peers if above high threshold
    if (current_mesh_size > config_.mesh_high) {
        int excess = current_mesh_size - config_.mesh_optimal;
        std::vector<std::string> mesh_peers_vec(topic_sub->mesh_peers.begin(), topic_sub->mesh_peers.end());
        std::vector<std::string> to_prune = random_sample(mesh_peers_vec, excess);
        
        for (const auto& peer_id : to_prune) {
            remove_peer_from_mesh(topic, peer_id);
        }
    }
}

void GossipSub::add_peer_to_mesh(const std::string& topic, const std::string& peer_id) {
    auto topic_it = topics_.find(topic);
    if (topic_it == topics_.end()) {
        return;
    }
    
    TopicSubscription* topic_sub = topic_it->second.get();
    
    // Check if peer is subscribed to the topic
    if (topic_sub->subscribers.count(peer_id) == 0) {
        return;
    }
    
    // Check peer score
    if (!is_peer_score_acceptable(peer_id, config_.score_threshold_mesh)) {
        return;
    }
    
    // Add to mesh
    if (topic_sub->mesh_peers.insert(peer_id).second) {
        // Send GRAFT message
        nlohmann::json graft_payload;
        graft_payload["topic"] = topic;
        send_gossipsub_message(peer_id, GossipSubMessageType::GRAFT, graft_payload);
        
        LOG_GOSSIPSUB_DEBUG("Added peer " << peer_id << " to mesh for topic: " << topic);
    }
}

void GossipSub::remove_peer_from_mesh(const std::string& topic, const std::string& peer_id) {
    auto topic_it = topics_.find(topic);
    if (topic_it == topics_.end()) {
        return;
    }
    
    TopicSubscription* topic_sub = topic_it->second.get();
    
    if (topic_sub->mesh_peers.erase(peer_id) > 0) {
        // Send PRUNE message
        nlohmann::json prune_payload;
        prune_payload["topic"] = topic;
        send_gossipsub_message(peer_id, GossipSubMessageType::PRUNE, prune_payload);
        
        LOG_GOSSIPSUB_DEBUG("Removed peer " << peer_id << " from mesh for topic: " << topic);
    }
}

std::vector<std::string> GossipSub::select_peers_for_mesh(const std::string& topic, int count) {
    auto topic_it = topics_.find(topic);
    if (topic_it == topics_.end()) {
        return {};
    }
    
    TopicSubscription* topic_sub = topic_it->second.get();
    
    // Get candidates (subscribers not in mesh)
    std::vector<std::string> candidates;
    for (const auto& peer_id : topic_sub->subscribers) {
        if (topic_sub->mesh_peers.count(peer_id) == 0 && 
            is_peer_score_acceptable(peer_id, config_.score_threshold_mesh)) {
            candidates.push_back(peer_id);
        }
    }
    
    return random_sample(candidates, count);
}

std::vector<std::string> GossipSub::select_peers_for_gossip(const std::string& topic, int count, const std::unordered_set<std::string>& exclude) {
    // Get all connected peers with acceptable scores
    auto all_peers = rats_client_.get_all_peers();
    std::vector<std::string> candidates;
    
    for (const auto& peer : all_peers) {
        if (exclude.count(peer.peer_id) == 0 && 
            is_peer_score_acceptable(peer.peer_id, config_.score_threshold_gossip)) {
            candidates.push_back(peer.peer_id);
        }
    }
    
    return random_sample(candidates, count);
}

std::vector<std::string> GossipSub::random_sample(const std::vector<std::string>& peers, int count) {
    if (peers.empty() || count <= 0) {
        return {};
    }
    
    std::lock_guard<std::mutex> rng_lock(rng_mutex_);
    
    std::vector<std::string> result = peers;
    if (static_cast<int>(result.size()) > count) {
        std::shuffle(result.begin(), result.end(), rng_);
        result.resize(count);
    }
    
    return result;
}

std::vector<std::string> GossipSub::random_sample(const std::unordered_set<std::string>& peers, int count) {
    std::vector<std::string> vec(peers.begin(), peers.end());
    return random_sample(vec, count);
}

void GossipSub::cleanup_message_cache() {
    std::lock_guard<std::mutex> lock(message_cache_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    
    // Clean up old message cache entries
    auto cache_it = message_cache_.begin();
    while (cache_it != message_cache_.end()) {
        auto time_since_first_seen = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - cache_it->second->first_seen);
        
        if (time_since_first_seen >= config_.message_cache_ttl) {
            cache_it = message_cache_.erase(cache_it);
        } else {
            ++cache_it;
        }
    }
    
    // Clean up message IDs seen
    auto ids_it = message_ids_seen_.begin();
    while (ids_it != message_ids_seen_.end()) {
        auto time_since_seen = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - ids_it->second);
        
        if (time_since_seen >= config_.message_cache_ttl) {
            ids_it = message_ids_seen_.erase(ids_it);
        } else {
            ++ids_it;
        }
    }
}

//=============================================================================
// Public API Implementation (remaining methods)
//=============================================================================

void GossipSub::set_message_validator(const std::string& topic, MessageValidator validator) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    if (topic.empty()) {
        global_validator_ = validator;
    } else {
        message_validators_[topic] = validator;
    }
}

void GossipSub::set_message_handler(const std::string& topic, MessageHandler handler) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    message_handlers_[topic] = handler;
}

void GossipSub::set_peer_joined_handler(const std::string& topic, PeerJoinedHandler handler) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    peer_joined_handlers_[topic] = handler;
}

void GossipSub::set_peer_left_handler(const std::string& topic, PeerLeftHandler handler) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    peer_left_handlers_[topic] = handler;
}

std::vector<std::string> GossipSub::get_topic_peers(const std::string& topic) const {
    std::lock_guard<std::mutex> lock(topics_mutex_);
    auto topic_it = topics_.find(topic);
    if (topic_it == topics_.end()) {
        return {};
    }
    
    const auto& subscribers = topic_it->second->subscribers;
    return std::vector<std::string>(subscribers.begin(), subscribers.end());
}

std::vector<std::string> GossipSub::get_mesh_peers(const std::string& topic) const {
    std::lock_guard<std::mutex> lock(topics_mutex_);
    auto topic_it = topics_.find(topic);
    if (topic_it == topics_.end()) {
        return {};
    }
    
    const auto& mesh_peers = topic_it->second->mesh_peers;
    return std::vector<std::string>(mesh_peers.begin(), mesh_peers.end());
}

double GossipSub::get_peer_score(const std::string& peer_id) const {
    std::lock_guard<std::mutex> lock(scores_mutex_);
    auto score_it = peer_scores_.find(peer_id);
    if (score_it == peer_scores_.end()) {
        return 0.0;
    }
    return score_it->second->score;
}

nlohmann::json GossipSub::get_statistics() const {
    nlohmann::json stats;
    
    // Basic stats
    stats["running"] = running_.load();
    
    // Topic stats
    {
        std::lock_guard<std::mutex> topics_lock(topics_mutex_);
        stats["subscribed_topics_count"] = subscribed_topics_.size();
        stats["total_topics_count"] = topics_.size();
        
        nlohmann::json topics_detail;
        for (const auto& topic_pair : topics_) {
            nlohmann::json topic_stats;
            topic_stats["subscribers_count"] = topic_pair.second->subscribers.size();
            topic_stats["mesh_peers_count"] = topic_pair.second->mesh_peers.size();
            topic_stats["fanout_peers_count"] = topic_pair.second->fanout_peers.size();
            topic_stats["is_subscribed"] = subscribed_topics_.count(topic_pair.first) > 0;
            topics_detail[topic_pair.first] = topic_stats;
        }
        stats["topics"] = topics_detail;
    }
    
    // Peer scores
    {
        std::lock_guard<std::mutex> scores_lock(scores_mutex_);
        stats["peers_count"] = peer_scores_.size();
        
        double total_score = 0.0;
        double min_score = (std::numeric_limits<double>::max)();
        double max_score = (std::numeric_limits<double>::lowest)();
        
        for (const auto& score_pair : peer_scores_) {
            double score = score_pair.second->score;
            total_score += score;
            min_score = (std::min)(min_score, score);
            max_score = (std::max)(max_score, score);
        }
        
        if (!peer_scores_.empty()) {
            stats["average_peer_score"] = total_score / peer_scores_.size();
            stats["min_peer_score"] = min_score;
            stats["max_peer_score"] = max_score;
        }
    }
    
    return stats;
}

nlohmann::json GossipSub::get_cache_statistics() const {
    std::lock_guard<std::mutex> lock(message_cache_mutex_);
    
    nlohmann::json cache_stats;
    cache_stats["cached_messages_count"] = message_cache_.size();
    cache_stats["seen_message_ids_count"] = message_ids_seen_.size();
    
    return cache_stats;
}

} // namespace librats 