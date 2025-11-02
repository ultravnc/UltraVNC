#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <memory>
#include <functional>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <random>
#include "json.hpp"

namespace librats {

// Forward declarations
class RatsClient;

/**
 * GossipSub message types
 */
enum class GossipSubMessageType {
    SUBSCRIBE,          // Subscribe to a topic
    UNSUBSCRIBE,        // Unsubscribe from a topic
    PUBLISH,            // Publish a message to a topic
    GOSSIP,             // Gossip message about published content
    GRAFT,              // Join mesh for a topic
    PRUNE,              // Leave mesh for a topic
    IHAVE,              // Announce having certain messages
    IWANT,              // Request specific messages
    HEARTBEAT           // Periodic heartbeat with control information
};

/**
 * Message metadata for tracking and deduplication
 */
struct MessageMetadata {
    std::string message_id;
    std::string topic;
    std::string sender_peer_id;
    std::chrono::steady_clock::time_point first_seen;
    std::chrono::steady_clock::time_point last_seen;
    std::vector<std::string> seen_from_peers;
    int hop_count;
    bool validated;

    MessageMetadata(const std::string& id, const std::string& t, const std::string& sender)
        : message_id(id), topic(t), sender_peer_id(sender),
          first_seen(std::chrono::steady_clock::now()),
          last_seen(std::chrono::steady_clock::now()),
          hop_count(0), validated(false) {}
};

/**
 * Peer scoring metrics for mesh management
 */
struct PeerScore {
    std::string peer_id;
    double score;
    
    // Score components
    double topic_score;           // Score based on topic participation
    double delivery_score;        // Score based on message delivery
    double mesh_behavior_score;   // Score based on mesh behavior
    double invalid_message_score; // Penalty for invalid messages
    
    // Timing metrics
    std::chrono::steady_clock::time_point last_updated;
    std::chrono::steady_clock::time_point connected_since;
    
    // Behavioral metrics
    int messages_delivered;
    int messages_invalid;
    int graft_requests;
    int prune_requests;
    
    PeerScore(const std::string& id) 
        : peer_id(id), score(0.0), topic_score(0.0), delivery_score(0.0),
          mesh_behavior_score(0.0), invalid_message_score(0.0),
          last_updated(std::chrono::steady_clock::now()),
          connected_since(std::chrono::steady_clock::now()),
          messages_delivered(0), messages_invalid(0),
          graft_requests(0), prune_requests(0) {}
    
    void update_score();
};

/**
 * Topic subscription information
 */
struct TopicSubscription {
    std::string topic;
    std::unordered_set<std::string> subscribers;        // All peers subscribed to this topic
    std::unordered_set<std::string> mesh_peers;         // Peers in our mesh for this topic
    std::unordered_set<std::string> fanout_peers;       // Peers in fanout (when we're not subscribed)
    std::chrono::steady_clock::time_point last_fanout_prune;
    
    TopicSubscription(const std::string& t) : topic(t), last_fanout_prune(std::chrono::steady_clock::now()) {}
};

/**
 * GossipSub configuration parameters
 */
struct GossipSubConfig {
    // Mesh parameters
    int mesh_low;                    // Minimum number of peers in mesh
    int mesh_high;                   // Maximum number of peers in mesh
    int mesh_optimal;                // Optimal number of peers in mesh
    
    // Fanout parameters
    int fanout_size;                 // Number of peers for fanout
    std::chrono::milliseconds fanout_ttl; // Time to live for fanout
    
    // Gossip parameters
    int gossip_factor;               // Number of peers to gossip to
    int gossip_lazy;                 // Lazy pull parameter
    std::chrono::milliseconds gossip_retransmit; // Gossip retransmission interval
    
    // Heartbeat parameters
    std::chrono::milliseconds heartbeat_interval; // Heartbeat interval
    
    // Message parameters
    std::chrono::milliseconds message_cache_ttl; // Message cache time to live
    int max_ihave_messages;          // Maximum messages in IHAVE
    int max_iwant_messages;          // Maximum messages in IWANT
    
    // Scoring parameters
    double score_threshold_accept;   // Minimum score to accept messages
    double score_threshold_gossip;   // Minimum score to gossip to peer
    double score_threshold_mesh;     // Minimum score to keep in mesh
    double score_threshold_publish;  // Minimum score to accept published messages
    
    GossipSubConfig()
        : mesh_low(4), mesh_high(12), mesh_optimal(6),
          fanout_size(6), fanout_ttl(std::chrono::seconds(60)),
          gossip_factor(3), gossip_lazy(3), gossip_retransmit(std::chrono::seconds(3)),
          heartbeat_interval(std::chrono::seconds(1)),
          message_cache_ttl(std::chrono::minutes(5)),
          max_ihave_messages(5000), max_iwant_messages(5000),
          score_threshold_accept(-100.0), score_threshold_gossip(-1000.0),
          score_threshold_mesh(-10.0), score_threshold_publish(-50.0) {}
};

/**
 * Message validation result
 */
enum class ValidationResult {
    ACCEPT,
    REJECT,
    IGNORE_MSG
};

/**
 * Callback types for GossipSub
 */
using MessageValidator = std::function<ValidationResult(const std::string& topic, const std::string& message, const std::string& sender_peer_id)>;
using MessageHandler = std::function<void(const std::string& topic, const std::string& message, const std::string& sender_peer_id)>;
using PeerJoinedHandler = std::function<void(const std::string& topic, const std::string& peer_id)>;
using PeerLeftHandler = std::function<void(const std::string& topic, const std::string& peer_id)>;

/**
 * Main GossipSub implementation class
 */
class GossipSub {
    friend class RatsClient;
    
public:
    /**
     * Constructor
     * @param rats_client Reference to the RatsClient instance
     * @param config GossipSub configuration
     */
    explicit GossipSub(RatsClient& rats_client, const GossipSubConfig& config = GossipSubConfig());
    
    /**
     * Destructor
     */
    ~GossipSub();
    
    /**
     * Start the GossipSub service
     * @return true if started successfully
     */
    bool start();
    
    /**
     * Stop the GossipSub service
     */
    void stop();
    
    /**
     * Check if GossipSub is running
     * @return true if running
     */
    bool is_running() const;
    
    // Topic management
    /**
     * Subscribe to a topic
     * @param topic Topic name to subscribe to
     * @return true if subscription successful
     */
    bool subscribe(const std::string& topic);
    
    /**
     * Unsubscribe from a topic
     * @param topic Topic name to unsubscribe from
     * @return true if unsubscription successful
     */
    bool unsubscribe(const std::string& topic);
    
    /**
     * Check if subscribed to a topic
     * @param topic Topic name to check
     * @return true if subscribed
     */
    bool is_subscribed(const std::string& topic) const;
    
    /**
     * Get list of subscribed topics
     * @return Vector of topic names
     */
    std::vector<std::string> get_subscribed_topics() const;
    
    // Message publishing
    /**
     * Publish a message to a topic
     * @param topic Topic to publish to
     * @param message Message content
     * @return true if published successfully
     */
    bool publish(const std::string& topic, const std::string& message);
    
    /**
     * Publish a JSON message to a topic
     * @param topic Topic to publish to
     * @param message JSON message content
     * @return true if published successfully
     */
    bool publish(const std::string& topic, const nlohmann::json& message);
    
    // Callback registration
    /**
     * Set message validator for a topic
     * @param topic Topic name (empty for all topics)
     * @param validator Validation function
     */
    void set_message_validator(const std::string& topic, MessageValidator validator);
    
    /**
     * Set message handler for a topic
     * @param topic Topic name
     * @param handler Message handler function
     */
    void set_message_handler(const std::string& topic, MessageHandler handler);
    
    /**
     * Set peer joined handler for a topic
     * @param topic Topic name
     * @param handler Peer joined handler function
     */
    void set_peer_joined_handler(const std::string& topic, PeerJoinedHandler handler);
    
    /**
     * Set peer left handler for a topic
     * @param topic Topic name
     * @param handler Peer left handler function
     */
    void set_peer_left_handler(const std::string& topic, PeerLeftHandler handler);
    
    // Peer and mesh information
    /**
     * Get peers subscribed to a topic
     * @param topic Topic name
     * @return Vector of peer IDs
     */
    std::vector<std::string> get_topic_peers(const std::string& topic) const;
    
    /**
     * Get mesh peers for a topic
     * @param topic Topic name
     * @return Vector of peer IDs in the mesh
     */
    std::vector<std::string> get_mesh_peers(const std::string& topic) const;
    
    /**
     * Get peer score
     * @param peer_id Peer ID
     * @return Peer score
     */
    double get_peer_score(const std::string& peer_id) const;
    
    // Statistics and debugging
    /**
     * Get GossipSub statistics
     * @return JSON object with statistics
     */
    nlohmann::json get_statistics() const;
    
    /**
     * Get message cache statistics
     * @return JSON object with cache statistics
     */
    nlohmann::json get_cache_statistics() const;

private:
    RatsClient& rats_client_;
    GossipSubConfig config_;
    std::atomic<bool> running_;
    
    // Thread management
    std::thread heartbeat_thread_;
    mutable std::mutex heartbeat_mutex_;
    std::condition_variable heartbeat_cv_;
    
    // Topic management
    mutable std::mutex topics_mutex_;
    std::unordered_map<std::string, std::unique_ptr<TopicSubscription>> topics_;
    std::unordered_set<std::string> subscribed_topics_;
    
    // Peer scoring
    mutable std::mutex scores_mutex_;
    std::unordered_map<std::string, std::unique_ptr<PeerScore>> peer_scores_;
    
    // Message cache and deduplication
    mutable std::mutex message_cache_mutex_;
    std::unordered_map<std::string, std::unique_ptr<MessageMetadata>> message_cache_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> message_ids_seen_;
    
    // Handlers and validators
    mutable std::mutex handlers_mutex_;
    std::unordered_map<std::string, MessageValidator> message_validators_;  // topic -> validator
    std::unordered_map<std::string, MessageHandler> message_handlers_;     // topic -> handler
    std::unordered_map<std::string, PeerJoinedHandler> peer_joined_handlers_; // topic -> handler
    std::unordered_map<std::string, PeerLeftHandler> peer_left_handlers_;   // topic -> handler
    MessageValidator global_validator_;  // Global validator for all topics
    
    // Control message queues for heartbeat
    mutable std::mutex control_queue_mutex_;
    std::vector<nlohmann::json> pending_grafts_;
    std::vector<nlohmann::json> pending_prunes_;
    std::vector<nlohmann::json> pending_ihaves_;
    std::vector<nlohmann::json> pending_iwants_;
    
    // Random number generation
    mutable std::mutex rng_mutex_;
    std::mt19937 rng_;
    
    // Internal methods
    void heartbeat_loop();
    void process_heartbeat();
    void handle_gossipsub_message(const std::string& peer_id, const nlohmann::json& message);
    
    // Message handling
    void handle_subscribe(const std::string& peer_id, const nlohmann::json& payload);
    void handle_unsubscribe(const std::string& peer_id, const nlohmann::json& payload);
    void handle_publish(const std::string& peer_id, const nlohmann::json& payload);
    void handle_gossip(const std::string& peer_id, const nlohmann::json& payload);
    void handle_graft(const std::string& peer_id, const nlohmann::json& payload);
    void handle_prune(const std::string& peer_id, const nlohmann::json& payload);
    void handle_ihave(const std::string& peer_id, const nlohmann::json& payload);
    void handle_iwant(const std::string& peer_id, const nlohmann::json& payload);
    void handle_heartbeat(const std::string& peer_id, const nlohmann::json& payload);
    
    // Mesh management
    void maintain_mesh(const std::string& topic);
    void add_peer_to_mesh(const std::string& topic, const std::string& peer_id);
    void remove_peer_from_mesh(const std::string& topic, const std::string& peer_id);
    std::vector<std::string> select_peers_for_mesh(const std::string& topic, int count);
    std::vector<std::string> select_peers_for_gossip(const std::string& topic, int count, const std::unordered_set<std::string>& exclude = {});
    
    // Message utilities
    std::string generate_message_id(const std::string& topic, const std::string& message, const std::string& sender_peer_id);
    bool is_message_seen(const std::string& message_id);
    void cache_message(const std::string& message_id, const std::string& topic, const std::string& message, const std::string& sender_peer_id);
    void cleanup_message_cache();
    
    // Peer management
    void update_peer_score(const std::string& peer_id);
    void handle_peer_connected(const std::string& peer_id);
    void handle_peer_disconnected(const std::string& peer_id);
    
    // Message sending utilities
    bool send_gossipsub_message(const std::string& peer_id, GossipSubMessageType type, const nlohmann::json& payload);
    bool broadcast_gossipsub_message(GossipSubMessageType type, const nlohmann::json& payload, const std::unordered_set<std::string>& exclude = {});
    
    // Validation
    ValidationResult validate_message(const std::string& topic, const std::string& message, const std::string& sender_peer_id);
    bool is_peer_score_acceptable(const std::string& peer_id, double threshold);
    
    // Topic utilities
    TopicSubscription* get_or_create_topic(const std::string& topic);
    void cleanup_topic(const std::string& topic);
    
    // Random selection utilities
    std::vector<std::string> random_sample(const std::vector<std::string>& peers, int count);
    std::vector<std::string> random_sample(const std::unordered_set<std::string>& peers, int count);
};

} // namespace librats 