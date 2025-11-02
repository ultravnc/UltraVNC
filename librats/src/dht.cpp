#include "dht.h"
#include "network_utils.h"
#include "logger.h"
#include "socket.h"
#include <random>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <cmath>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <arpa/inet.h>
    #include <netinet/in.h>
#endif

// DHT module logging macros
#define LOG_DHT_DEBUG(message) LOG_DEBUG("dht", message)
#define LOG_DHT_INFO(message)  LOG_INFO("dht", message)
#define LOG_DHT_WARN(message)  LOG_WARN("dht", message)
#define LOG_DHT_ERROR(message) LOG_ERROR("dht", message)

namespace librats {


DhtClient::DhtClient(int port, const std::string& bind_address) 
    : port_(port), bind_address_(bind_address), socket_(INVALID_SOCKET_VALUE), running_(false) {
    node_id_ = generate_node_id();
    routing_table_.resize(NODE_ID_SIZE * 8);  // 160 buckets for 160-bit node IDs
    
    LOG_DHT_INFO("DHT client created with node ID: " << node_id_to_hex(node_id_) <<
                 (bind_address_.empty() ? "" : " bind address: " + bind_address_));
}

DhtClient::~DhtClient() {
    stop();
}

bool DhtClient::start() {
    if (running_) {
        return true;
    }
    
    LOG_DHT_INFO("Starting DHT client on port " << port_ <<
                 (bind_address_.empty() ? "" : " bound to " + bind_address_));
    
    // Initialize socket library (safe to call multiple times)
    if (!init_socket_library()) {
        LOG_DHT_ERROR("Failed to initialize socket library");
        return false;
    }
    
    socket_ = create_udp_socket(port_, bind_address_);
    if (!is_valid_socket(socket_)) {
        LOG_DHT_ERROR("Failed to create dual-stack UDP socket");
        return false;
    }
    
    if (!set_socket_nonblocking(socket_)) {
        LOG_DHT_WARN("Failed to set socket to non-blocking mode");
    }
    
    running_ = true;
    
    // Start network and maintenance threads
    network_thread_ = std::thread(&DhtClient::network_loop, this);
    maintenance_thread_ = std::thread(&DhtClient::maintenance_loop, this);
    
    LOG_DHT_INFO("DHT client started successfully");
    return true;
}

void DhtClient::stop() {
    if (!running_) {
        return;
    }
    
    LOG_DHT_INFO("Stopping DHT client");
    
    // Trigger immediate shutdown of all background threads
    shutdown_immediate();
    
    // Wait for threads to finish
    if (network_thread_.joinable()) {
        network_thread_.join();
    }
    if (maintenance_thread_.joinable()) {
        maintenance_thread_.join();
    }
    
    // Close socket
    if (is_valid_socket(socket_)) {
        close_socket(socket_);
        socket_ = INVALID_SOCKET_VALUE;
    }
    
    LOG_DHT_INFO("DHT client stopped");
}

void DhtClient::shutdown_immediate() {
    LOG_DHT_INFO("Triggering immediate shutdown of DHT background threads");
    
    running_.store(false);
    
    // Notify all waiting threads to wake up immediately
    shutdown_cv_.notify_all();
}

bool DhtClient::bootstrap(const std::vector<Peer>& bootstrap_nodes) {
    if (!running_) {
        LOG_DHT_ERROR("DHT client not running");
        return false;
    }
    
    LOG_DHT_INFO("Bootstrapping DHT with " << bootstrap_nodes.size() << " nodes");
    LOG_DHT_DEBUG("Bootstrap nodes:");
    for (const auto& peer : bootstrap_nodes) {
        LOG_DHT_DEBUG("  - " << peer.ip << ":" << peer.port);
    }
    

    
    // Send ping to bootstrap nodes
    LOG_DHT_DEBUG("Sending PING to all bootstrap nodes");
    for (const auto& peer : bootstrap_nodes) {
        send_ping(peer);
    }
    
    // Start node discovery by finding our own node
    LOG_DHT_DEBUG("Starting node discovery by finding our own node ID: " << node_id_to_hex(node_id_));
    for (const auto& peer : bootstrap_nodes) {
        send_find_node(peer, node_id_);
    }
    
    LOG_DHT_DEBUG("Bootstrap process initiated");
    return true;
}

bool DhtClient::find_peers(const InfoHash& info_hash, PeerDiscoveryCallback callback, int iteration_max) {
    if (!running_) {
        LOG_DHT_ERROR("DHT client not running");
        return false;
    }
    
    LOG_DHT_INFO("Finding peers for info hash: " << node_id_to_hex(info_hash));
    
    {
        std::lock_guard<std::mutex> lock(active_searches_mutex_);
        std::string hash_key = node_id_to_hex(info_hash);
        active_searches_[hash_key] = callback;
    }
    
    // Start search by querying closest nodes
    auto closest_nodes = find_closest_nodes(info_hash, ALPHA);
    std::string hash_key = node_id_to_hex(info_hash);
    
    {
        std::lock_guard<std::mutex> lock(pending_searches_mutex_);
        // Create or get existing PendingSearch for this info_hash
        auto search_it = pending_searches_.find(hash_key);
        if (search_it == pending_searches_.end()) {
            pending_searches_.emplace(hash_key, PendingSearch(info_hash, iteration_max));
        }
    }
    
    for (const auto& node : closest_nodes) {
            // Generate transaction ID and track this as a pending search for KRPC
            std::string transaction_id = KrpcProtocol::generate_transaction_id();
            
            {
                std::lock_guard<std::mutex> lock(pending_searches_mutex_);
                transaction_to_search_[transaction_id] = hash_key;
                auto search_it = pending_searches_.find(hash_key);
                if (search_it != pending_searches_.end()) {
                    search_it->second.queried_nodes.insert(node_id_to_hex(node.id));
                }
            }
            
            auto message = KrpcProtocol::create_get_peers_query(transaction_id, node_id_, info_hash);
            send_krpc_message(message, node.peer);
    }
    
    return true;
}

bool DhtClient::announce_peer(const InfoHash& info_hash, uint16_t port) {
    if (!running_) {
        LOG_DHT_ERROR("DHT client not running");
        return false;
    }
    
    if (port == 0) {
        port = port_;
    }
    
    LOG_DHT_INFO("Announcing peer for info hash: " << node_id_to_hex(info_hash) << " on port " << port);
    
    // First find nodes close to the info hash and send get_peers to them
    // This is the proper BEP 5 flow: get_peers -> collect tokens -> announce_peer
    auto closest_nodes = find_closest_nodes(info_hash, ALPHA);
    for (const auto& node : closest_nodes) {
            // Generate transaction ID and track this as a pending announce for KRPC
            std::string transaction_id = KrpcProtocol::generate_transaction_id();
            
            {
                std::lock_guard<std::mutex> lock(pending_announces_mutex_);
                pending_announces_.emplace(transaction_id, PendingAnnounce(info_hash, port));
            }
            
            auto message = KrpcProtocol::create_get_peers_query(transaction_id, node_id_, info_hash);
            send_krpc_message(message, node.peer);
    }
    
    return true;
}

size_t DhtClient::get_routing_table_size() const {
    std::lock_guard<std::mutex> lock(routing_table_mutex_);
    size_t total = 0;
    for (const auto& bucket : routing_table_) {
        total += bucket.size();
    }
    return total;
}

size_t DhtClient::get_pending_ping_verifications_count() const {
    std::lock_guard<std::mutex> lock(pending_pings_mutex_);
    return pending_pings_.size();
}

std::vector<Peer> DhtClient::get_default_bootstrap_nodes() {
    return {
        {"router.bittorrent.com", 6881},
        {"dht.transmissionbt.com", 6881},
        {"router.utorrent.com", 6881},
        {"dht.aelitis.com", 6881}
    };
}

void DhtClient::network_loop() {
    LOG_DHT_DEBUG("Network loop started");
    
    while (running_) {
        Peer sender;
        auto data = receive_udp_data(socket_, 1500, sender);  // MTU size
        
        if (!data.empty()) {
            LOG_DHT_DEBUG("Received " << data.size() << " bytes from " << sender.ip << ":" << sender.port);
            handle_message(data, sender);
        }
        
        // Use conditional variable for responsive shutdown
        {
            std::unique_lock<std::mutex> lock(shutdown_mutex_);
            if (shutdown_cv_.wait_for(lock, std::chrono::milliseconds(10), [this] { return !running_.load(); })) {
                break;
            }
        }
    }
    
    LOG_DHT_DEBUG("Network loop stopped");
}

void DhtClient::maintenance_loop() {
    LOG_DHT_DEBUG("Maintenance loop started");
    
    auto last_bucket_refresh = std::chrono::steady_clock::now();
    auto last_ping_verification_cleanup = std::chrono::steady_clock::now();
    auto last_general_cleanup = std::chrono::steady_clock::now();
    
    while (running_) {
        auto now = std::chrono::steady_clock::now();

        
        // General cleanup operations every 1 minute (like previously)
        if (now - last_general_cleanup >= std::chrono::minutes(1)) {
            // Cleanup stale nodes every 1 minute
            cleanup_stale_nodes();
            
            // Cleanup stale pending announces
            cleanup_stale_announces();
            
            // Cleanup stale pending searches
            cleanup_stale_searches();
            
            // Cleanup stale announced peers
            cleanup_stale_announced_peers();
            
            last_general_cleanup = now;
        }
        
        // Refresh buckets every 30 minutes
        if (now - last_bucket_refresh >= std::chrono::minutes(30)) {
            refresh_buckets();
            last_bucket_refresh = now;
        }
        
        // Frequent maintenance: ping verifications time out at ~30s, so check often
        if (now - last_ping_verification_cleanup >= std::chrono::seconds(30)) {
            cleanup_stale_ping_verifications();
            last_ping_verification_cleanup = now;
        }
        
        // Execute maintenance loop every 5 seconds
        {
            std::unique_lock<std::mutex> lock(shutdown_mutex_);
            if (shutdown_cv_.wait_for(lock, std::chrono::seconds(5), [this] { return !running_.load(); })) {
                break;
            }
        }
    }
    
    LOG_DHT_DEBUG("Maintenance loop stopped");
}

void DhtClient::handle_message(const std::vector<uint8_t>& data, const Peer& sender) {
    LOG_DHT_DEBUG("Processing message of " << data.size() << " bytes from " << sender.ip << ":" << sender.port);
    
    LOG_DHT_DEBUG("Using BitTorrent DHT (KRPC) protocol from " << sender.ip << ":" << sender.port);

        // Handle KRPC message
        auto krpc_message = KrpcProtocol::decode_message(data);
        if (!krpc_message) {
            LOG_DHT_WARN("Failed to decode KRPC message from " << sender.ip << ":" << sender.port);
            return;
        }
        
        handle_krpc_message(*krpc_message, sender);
}

void DhtClient::add_node(const DhtNode& node) {
    std::lock_guard<std::mutex> lock(routing_table_mutex_);
    
    int bucket_index = get_bucket_index(node.id);
    auto& bucket = routing_table_[bucket_index];
    
    LOG_DHT_DEBUG("Adding node " << node_id_to_hex(node.id) << " at " << node.peer.ip << ":" << node.peer.port << " to bucket " << bucket_index);
    
    // Check if node already exists
    auto it = std::find_if(bucket.begin(), bucket.end(),
                          [&node](const DhtNode& existing) {
                              return existing.id == node.id;
                          });
    
    if (it != bucket.end()) {
        // Update existing node
        LOG_DHT_DEBUG("Node " << node_id_to_hex(node.id) << " already exists in bucket " << bucket_index << ", updating");
        
        it->peer = node.peer;
        it->last_seen = std::chrono::steady_clock::now();
    } else {
        // Add new node
        if (bucket.size() < K_BUCKET_SIZE) {
            bucket.push_back(node);
            LOG_DHT_DEBUG("Added new node " << node_id_to_hex(node.id) << " to bucket " << bucket_index << " (size: " << bucket.size() << "/" << K_BUCKET_SIZE << ")");
        } else {
            // Bucket is full, use ping-before-replace eviction (BEP 5)
            
            // Find the worst node that's not already being pinged for replacement
            auto worst_it = bucket.end();
            {
                std::lock_guard<std::mutex> nodes_lock(nodes_being_replaced_mutex_);
                for (auto it = bucket.begin(); it != bucket.end(); ++it) {
                    if (nodes_being_replaced_.find(it->id) == nodes_being_replaced_.end()) {
                        if (worst_it == bucket.end() || it->last_seen < worst_it->last_seen) {
                            worst_it = it;
                        }
                    }
                }
            }
            
            if (worst_it == bucket.end()) {
                LOG_DHT_DEBUG("Bucket " << bucket_index << " is full, but all nodes already have pending ping verifications - skipping new node " << node_id_to_hex(node.id));
                return;
            }
            
            LOG_DHT_DEBUG("Bucket " << bucket_index << " is full, initiating ping-before-replace for node " 
                          << node_id_to_hex(worst_it->id) << " (last_seen age: " 
                          << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - worst_it->last_seen).count() 
                          << "s) to potentially replace with " << node_id_to_hex(node.id));
            
            // Initiate ping verification instead of immediate replacement
            initiate_ping_verification(node, *worst_it, bucket_index);
        }
    }
}

std::vector<DhtNode> DhtClient::find_closest_nodes(const NodeId& target, size_t count) {
    std::lock_guard<std::mutex> lock(routing_table_mutex_);
    
    auto result = find_closest_nodes_unlocked(target, count);
    
    return result;
}

std::vector<DhtNode> DhtClient::find_closest_nodes_unlocked(const NodeId& target, size_t count) {
    LOG_DHT_DEBUG("Finding closest nodes to target " << node_id_to_hex(target) << " (max " << count << " nodes)");
    
    std::vector<DhtNode> all_nodes;
    size_t total_nodes = 0;
    for (const auto& bucket : routing_table_) {
        all_nodes.insert(all_nodes.end(), bucket.begin(), bucket.end());
        total_nodes += bucket.size();
    }
    
    LOG_DHT_DEBUG("Routing table contains " << total_nodes << " nodes across " << routing_table_.size() << " buckets");
    
    // Sort by distance to target
    std::sort(all_nodes.begin(), all_nodes.end(),
              [&target, this](const DhtNode& a, const DhtNode& b) {
                  return is_closer(a.id, b.id, target);
              });
    
    // Return up to 'count' closest nodes
    if (all_nodes.size() > count) {
        all_nodes.resize(count);
    }
    
    LOG_DHT_DEBUG("Found " << all_nodes.size() << " closest nodes to target " << node_id_to_hex(target));
    for (size_t i = 0; i < all_nodes.size(); ++i) {
        LOG_DHT_DEBUG("  [" << i << "] " << node_id_to_hex(all_nodes[i].id) << " at " << all_nodes[i].peer.ip << ":" << all_nodes[i].peer.port);
    }
    // Print the xor distance for each node
    // for (size_t i = 0; i < all_nodes.size(); ++i) {
    //     NodeId dist = xor_distance(all_nodes[i].id, target);
    //     // Convert the NodeId (20 bytes) to a single uint64_t for a simple distance metric (using the first 8 bytes)
    //     uint64_t dist_metric = 0;
    //     for (int j = 0; j < 8 && j < dist.size(); ++j) {
    //         dist_metric = (dist_metric << 8) | dist[j];
    //     }
    //     LOG_DHT_DEBUG("  [" << i << "] " << node_id_to_hex(all_nodes[i].id) 
    //         << " at " << all_nodes[i].peer.ip << ":" << all_nodes[i].peer.port
    //         << " xor_distance=" << dist_metric);
    // }
    
    return all_nodes;
}

int DhtClient::get_bucket_index(const NodeId& id) {
    NodeId distance = xor_distance(node_id_, id);
    
    // Find the position of the most significant bit
    for (int i = 0; i < NODE_ID_SIZE; ++i) {
        if (distance[i] != 0) {
            for (int j = 7; j >= 0; --j) {
                if (distance[i] & (1 << j)) {
                    return i * 8 + (7 - j);
                }
            }
        }
    }
    
    return NODE_ID_SIZE * 8 - 1;  // All bits are 0, maximum distance
}



// KRPC message handling
void DhtClient::handle_krpc_message(const KrpcMessage& message, const Peer& sender) {
    LOG_DHT_DEBUG("Handling KRPC message type " << static_cast<int>(message.type) << " from " << sender.ip << ":" << sender.port);
    
    switch (message.type) {
        case KrpcMessageType::Query:
            switch (message.query_type) {
                case KrpcQueryType::Ping:
                    handle_krpc_ping(message, sender);
                    break;
                case KrpcQueryType::FindNode:
                    handle_krpc_find_node(message, sender);
                    break;
                case KrpcQueryType::GetPeers:
                    handle_krpc_get_peers(message, sender);
                    break;
                case KrpcQueryType::AnnouncePeer:
                    handle_krpc_announce_peer(message, sender);
                    break;
            }
            break;
        case KrpcMessageType::Response:
            handle_krpc_response(message, sender);
            break;
        case KrpcMessageType::Error:
            handle_krpc_error(message, sender);
            break;
    }
}

void DhtClient::handle_krpc_ping(const KrpcMessage& message, const Peer& sender) {
    LOG_DHT_DEBUG("Handling KRPC PING from " << node_id_to_hex(message.sender_id) << " at " << sender.ip << ":" << sender.port);
    
    // Add sender to routing table
    KrpcNode krpc_node(message.sender_id, sender.ip, sender.port);
    DhtNode sender_node = krpc_node_to_dht_node(krpc_node);
    add_node(sender_node);
    
    // Respond with ping response
    auto response = KrpcProtocol::create_ping_response(message.transaction_id, node_id_);
    send_krpc_message(response, sender);
}

void DhtClient::handle_krpc_find_node(const KrpcMessage& message, const Peer& sender) {
    LOG_DHT_DEBUG("Handling KRPC FIND_NODE from " << node_id_to_hex(message.sender_id) << " at " << sender.ip << ":" << sender.port);
    
    // Add sender to routing table
    KrpcNode krpc_node(message.sender_id, sender.ip, sender.port);
    DhtNode sender_node = krpc_node_to_dht_node(krpc_node);
    add_node(sender_node);
    
    // Find closest nodes
    auto closest_nodes = find_closest_nodes(message.target_id, K_BUCKET_SIZE);
    auto krpc_nodes = dht_nodes_to_krpc_nodes(closest_nodes);
    
    // Respond with closest nodes
    auto response = KrpcProtocol::create_find_node_response(message.transaction_id, node_id_, krpc_nodes);
    send_krpc_message(response, sender);
}

void DhtClient::handle_krpc_get_peers(const KrpcMessage& message, const Peer& sender) {
    LOG_DHT_DEBUG("Handling KRPC GET_PEERS from " << node_id_to_hex(message.sender_id) << " at " << sender.ip << ":" << sender.port << " for info_hash " << node_id_to_hex(message.info_hash));
    
    // Add sender to routing table
    KrpcNode krpc_node(message.sender_id, sender.ip, sender.port);
    DhtNode sender_node = krpc_node_to_dht_node(krpc_node);
    add_node(sender_node);
    
    // Generate a token for this peer
    std::string token = generate_token(sender);
    
    // First check if we have announced peers for this info_hash
    auto announced_peers = get_announced_peers(message.info_hash);
    
    KrpcMessage response;
    if (!announced_peers.empty()) {
        // Return the peers we have stored
        response = KrpcProtocol::create_get_peers_response(message.transaction_id, node_id_, announced_peers, token);
        LOG_DHT_DEBUG("Responding to KRPC GET_PEERS with " << announced_peers.size() << " announced peers for info_hash " << node_id_to_hex(message.info_hash));
    } else {
        // Return closest nodes
        auto closest_nodes = find_closest_nodes(message.info_hash, K_BUCKET_SIZE);
        auto krpc_nodes = dht_nodes_to_krpc_nodes(closest_nodes);
        response = KrpcProtocol::create_get_peers_response_with_nodes(message.transaction_id, node_id_, krpc_nodes, token);
        LOG_DHT_DEBUG("Responding to KRPC GET_PEERS with " << krpc_nodes.size() << " closest nodes for info_hash " << node_id_to_hex(message.info_hash));
    }
    
    send_krpc_message(response, sender);
}

void DhtClient::handle_krpc_announce_peer(const KrpcMessage& message, const Peer& sender) {
    LOG_DHT_DEBUG("Handling KRPC ANNOUNCE_PEER from " << node_id_to_hex(message.sender_id) << " at " << sender.ip << ":" << sender.port);
    
    // Verify token
    if (!verify_token(sender, message.token)) {
        LOG_DHT_WARN("Invalid token from " << sender.ip << ":" << sender.port << " for KRPC ANNOUNCE_PEER");
        auto error = KrpcProtocol::create_error(message.transaction_id, KrpcErrorCode::ProtocolError, "Invalid token");
        send_krpc_message(error, sender);
        return;
    }
    
    // Add sender to routing table
    KrpcNode krpc_node(message.sender_id, sender.ip, sender.port);
    DhtNode sender_node = krpc_node_to_dht_node(krpc_node);
    add_node(sender_node);
    
    // Store the peer announcement
    Peer announcing_peer(sender.ip, message.port);
    store_announced_peer(message.info_hash, announcing_peer);
    
    // Respond with acknowledgment
    auto response = KrpcProtocol::create_announce_peer_response(message.transaction_id, node_id_);
    send_krpc_message(response, sender);
}

void DhtClient::handle_krpc_response(const KrpcMessage& message, const Peer& sender) {
    LOG_DHT_DEBUG("Handling KRPC response from " << sender.ip << ":" << sender.port);
    
    // Check if this is a ping verification response before normal processing
    handle_ping_verification_response(message.transaction_id, message.response_id, sender);
    
    // Add responder to routing table
    KrpcNode krpc_node(message.response_id, sender.ip, sender.port);
    DhtNode sender_node = krpc_node_to_dht_node(krpc_node);
    add_node(sender_node);
    
    // Add any nodes from the response
    for (const auto& node : message.nodes) {
        DhtNode dht_node = krpc_node_to_dht_node(node);
        add_node(dht_node);
    }
    
    // Check if this is a response to a pending search (get_peers with peers)
    if (!message.peers.empty()) {
        handle_get_peers_response_for_search(message.transaction_id, sender, message.peers);
    }
    // Check if this is a response to a pending search (get_peers with nodes)
    else if (!message.nodes.empty()) {
        handle_get_peers_response_with_nodes(message.transaction_id, sender, message.nodes);
    }
    
    // Check if this is a response to a pending announce (get_peers with token)
    if (!message.token.empty()) {
        handle_get_peers_response_for_announce(message.transaction_id, sender, message.token);
    }
}

void DhtClient::handle_krpc_error(const KrpcMessage& message, const Peer& sender) {
    LOG_DHT_WARN("Received KRPC error from " << sender.ip << ":" << sender.port 
                 << " - Code: " << static_cast<int>(message.error_code) 
                 << " Message: " << message.error_message);
}

// KRPC sending functions
bool DhtClient::send_krpc_message(const KrpcMessage& message, const Peer& peer) {
    auto data = KrpcProtocol::encode_message(message);
    if (data.empty()) {
        LOG_DHT_ERROR("Failed to encode KRPC message");
        return false;
    }
    
    LOG_DHT_DEBUG("Sending KRPC message (" << data.size() << " bytes) to " << peer.ip << ":" << peer.port);
    int result = send_udp_data(socket_, data, peer);
    
    if (result > 0) {
        LOG_DHT_DEBUG("Successfully sent KRPC message to " << peer.ip << ":" << peer.port);
    } else {
        LOG_DHT_ERROR("Failed to send KRPC message to " << peer.ip << ":" << peer.port);
    }
    
    return result > 0;
}

void DhtClient::send_krpc_ping(const Peer& peer) {
    std::string transaction_id = KrpcProtocol::generate_transaction_id();
    auto message = KrpcProtocol::create_ping_query(transaction_id, node_id_);
    send_krpc_message(message, peer);
}

void DhtClient::send_krpc_find_node(const Peer& peer, const NodeId& target) {
    std::string transaction_id = KrpcProtocol::generate_transaction_id();
    auto message = KrpcProtocol::create_find_node_query(transaction_id, node_id_, target);
    send_krpc_message(message, peer);
}

void DhtClient::send_krpc_get_peers(const Peer& peer, const InfoHash& info_hash) {
    std::string transaction_id = KrpcProtocol::generate_transaction_id();
    auto message = KrpcProtocol::create_get_peers_query(transaction_id, node_id_, info_hash);
    send_krpc_message(message, peer);
}

void DhtClient::send_krpc_announce_peer(const Peer& peer, const InfoHash& info_hash, uint16_t port, const std::string& token) {
    std::string transaction_id = KrpcProtocol::generate_transaction_id();
    auto message = KrpcProtocol::create_announce_peer_query(transaction_id, node_id_, info_hash, port, token);
    send_krpc_message(message, peer);
}

// KRPC protocol sending functions
void DhtClient::send_ping(const Peer& peer) {
        LOG_DHT_DEBUG("Sending KRPC PING to " << peer.ip << ":" << peer.port);
        send_krpc_ping(peer);
}

void DhtClient::send_find_node(const Peer& peer, const NodeId& target) {
        LOG_DHT_DEBUG("Sending KRPC FIND_NODE to " << peer.ip << ":" << peer.port);
        send_krpc_find_node(peer, target);
}

void DhtClient::send_get_peers(const Peer& peer, const InfoHash& info_hash) {
        LOG_DHT_DEBUG("Sending KRPC GET_PEERS to " << peer.ip << ":" << peer.port);
        send_krpc_get_peers(peer, info_hash);
}

void DhtClient::send_announce_peer(const Peer& peer, const InfoHash& info_hash, uint16_t port, const std::string& token) {
        LOG_DHT_DEBUG("Sending KRPC ANNOUNCE_PEER to " << peer.ip << ":" << peer.port);
        send_krpc_announce_peer(peer, info_hash, port, token);
}

// Conversion utilities
KrpcNode DhtClient::dht_node_to_krpc_node(const DhtNode& node) {
    return KrpcNode(node.id, node.peer.ip, node.peer.port);
}

DhtNode DhtClient::krpc_node_to_dht_node(const KrpcNode& node) {
    Peer peer(node.ip, node.port);
    return DhtNode(node.id, peer);
}

std::vector<KrpcNode> DhtClient::dht_nodes_to_krpc_nodes(const std::vector<DhtNode>& nodes) {
    std::vector<KrpcNode> krpc_nodes;
    krpc_nodes.reserve(nodes.size());
    for (const auto& node : nodes) {
        krpc_nodes.push_back(dht_node_to_krpc_node(node));
    }
    return krpc_nodes;
}

std::vector<DhtNode> DhtClient::krpc_nodes_to_dht_nodes(const std::vector<KrpcNode>& nodes) {
    std::vector<DhtNode> dht_nodes;
    dht_nodes.reserve(nodes.size());
    for (const auto& node : nodes) {
        dht_nodes.push_back(krpc_node_to_dht_node(node));
    }
    return dht_nodes;
}

NodeId DhtClient::generate_node_id() {
    NodeId id;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (size_t i = 0; i < NODE_ID_SIZE; ++i) {
        id[i] = dis(gen);
    }
    
    return id;
}

NodeId DhtClient::xor_distance(const NodeId& a, const NodeId& b) {
    NodeId result;
    for (size_t i = 0; i < NODE_ID_SIZE; ++i) {
        result[i] = a[i] ^ b[i];
    }
    return result;
}

bool DhtClient::is_closer(const NodeId& a, const NodeId& b, const NodeId& target) {
    NodeId dist_a = xor_distance(a, target);
    NodeId dist_b = xor_distance(b, target);
    
    return std::lexicographical_compare(dist_a.begin(), dist_a.end(),
                                       dist_b.begin(), dist_b.end());
}

std::string DhtClient::generate_token(const Peer& peer) {
    // Simple token generation (in real implementation, use proper cryptographic hash)
    std::string data = peer.ip + ":" + std::to_string(peer.port);
    std::hash<std::string> hasher;
    size_t hash = hasher(data);
    
    // Convert hash to hex string
    std::ostringstream oss;
    oss << std::hex << hash;
    
    // Store token for this peer
    {
        std::lock_guard<std::mutex> lock(peer_tokens_mutex_);
        peer_tokens_[peer] = oss.str();
    }
    
    return oss.str();
}

bool DhtClient::verify_token(const Peer& peer, const std::string& token) {
    std::lock_guard<std::mutex> lock(peer_tokens_mutex_);
    auto it = peer_tokens_.find(peer);
    if (it != peer_tokens_.end()) {
        return it->second == token;
    }
    return false;
}

void DhtClient::cleanup_stale_nodes() {
    std::lock_guard<std::mutex> routing_lock(routing_table_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    auto stale_threshold = std::chrono::minutes(15);
    
    size_t total_removed = 0;
    
    for (auto& bucket : routing_table_) {
        auto old_size = bucket.size();
        
        bucket.erase(std::remove_if(bucket.begin(), bucket.end(),
                                   [now, stale_threshold](const DhtNode& node) {
                                       bool should_remove = (now - node.last_seen > stale_threshold);
                                       
                                       if (should_remove) {
                                           LOG_DHT_DEBUG("Removing stale node " << node_id_to_hex(node.id) 
                                                       << " at " << node.peer.ip << ":" << node.peer.port);
                                       }
                                       
                                       return should_remove;
                                   }), bucket.end());
        
        total_removed += (old_size - bucket.size());
    }
    
    if (total_removed > 0) {
        LOG_DHT_DEBUG("Cleaned up " << total_removed << " stale/failed nodes from routing table");
    }
}

void DhtClient::refresh_buckets() {
    // Find random nodes in each bucket to refresh
    std::lock_guard<std::mutex> lock(routing_table_mutex_);
    
    for (size_t i = 0; i < routing_table_.size(); ++i) {
        if (routing_table_[i].empty()) {
            // Generate a random node ID in this bucket's range
            NodeId random_id = generate_node_id();
            
            // Set the appropriate bits to place it in bucket i
            int byte_index = i / 8;
            int bit_index = i % 8;
            
            if (byte_index < NODE_ID_SIZE) {
                // Clear the target bit and higher bits
                for (int j = byte_index; j < NODE_ID_SIZE; ++j) {
                    random_id[j] = node_id_[j];
                }
                
                // Set the target bit
                random_id[byte_index] |= (1 << (7 - bit_index));
                
                // Find nodes to query
                auto closest_nodes = find_closest_nodes_unlocked(random_id, ALPHA);
                for (const auto& node : closest_nodes) {
                    send_find_node(node.peer, random_id);
                }
            }
        }
    }
}

void DhtClient::cleanup_stale_announces() {
    std::lock_guard<std::mutex> lock(pending_announces_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    auto stale_threshold = std::chrono::minutes(5);  // Remove announces older than 5 minutes
    
    auto it = pending_announces_.begin();
    while (it != pending_announces_.end()) {
        if (now - it->second.created_at > stale_threshold) {
            LOG_DHT_DEBUG("Removing stale pending announce for transaction " << it->first);
            it = pending_announces_.erase(it);
        } else {
            ++it;
        }
    }
}

void DhtClient::cleanup_stale_searches() {
    std::lock_guard<std::mutex> lock(pending_searches_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    auto stale_threshold = std::chrono::minutes(5);  // Remove searches older than 5 minutes
    
    // Clean up stale searches (by info_hash)
    auto search_it = pending_searches_.begin();
    while (search_it != pending_searches_.end()) {
        if (now - search_it->second.created_at > stale_threshold) {
            LOG_DHT_DEBUG("Removing stale pending search for info_hash " << search_it->first);
            search_it = pending_searches_.erase(search_it);
        } else {
            ++search_it;
        }
    }
    
    // Clean up stale transaction mappings (remove ones that point to non-existent searches)
    auto trans_it = transaction_to_search_.begin();
    while (trans_it != transaction_to_search_.end()) {
        if (pending_searches_.find(trans_it->second) == pending_searches_.end()) {
            LOG_DHT_DEBUG("Removing stale transaction mapping " << trans_it->first << " -> " << trans_it->second);
            trans_it = transaction_to_search_.erase(trans_it);
        } else {
            ++trans_it;
        }
    }
}

void DhtClient::handle_get_peers_response_for_announce(const std::string& transaction_id, const Peer& responder, const std::string& token) {
    std::lock_guard<std::mutex> lock(pending_announces_mutex_);
    
    auto it = pending_announces_.find(transaction_id);
    if (it != pending_announces_.end()) {
        const auto& pending_announce = it->second;
        LOG_DHT_DEBUG("Found pending announce for transaction " << transaction_id 
                      << " - sending announce_peer for info_hash " << node_id_to_hex(pending_announce.info_hash) 
                      << " to " << responder.ip << ":" << responder.port);
        
        // Send announce_peer with the received token using unified function
        send_announce_peer(responder, pending_announce.info_hash, pending_announce.port, token);
        
        // Remove the pending announce since we've handled it
        pending_announces_.erase(it);
    }
}


void DhtClient::handle_get_peers_response_for_search(const std::string& transaction_id, const Peer& responder, const std::vector<Peer>& peers) {
    std::lock_guard<std::mutex> lock(pending_searches_mutex_);
    
    auto trans_it = transaction_to_search_.find(transaction_id);
    if (trans_it != transaction_to_search_.end()) {
        std::string hash_key = trans_it->second;
        auto search_it = pending_searches_.find(hash_key);
        if (search_it != pending_searches_.end()) {
            auto& pending_search = search_it->second;
            LOG_DHT_DEBUG("Found pending search for KRPC transaction " << transaction_id 
                          << " - received " << peers.size() << " peers for info_hash " << hash_key 
                          << " from " << responder.ip << ":" << responder.port);
            
            if (!peers.empty()) {
                // We found actual peers - call callback and clean up search
                {
                    std::lock_guard<std::mutex> search_lock(active_searches_mutex_);
                    auto active_it = active_searches_.find(hash_key);
                    if (active_it != active_searches_.end() && active_it->second) {
                        active_it->second(peers, pending_search.info_hash);
                    }
                    // Remove from active searches since we found peers
                    active_searches_.erase(hash_key);
                }
                // Remove the completed search
                pending_searches_.erase(search_it);
            }
        }
        
        // Remove the transaction mapping since we've handled it
        transaction_to_search_.erase(trans_it);
    }
}


void DhtClient::handle_get_peers_response_with_nodes(const std::string& transaction_id, const Peer& responder, const std::vector<KrpcNode>& nodes) {
    std::lock_guard<std::mutex> lock(pending_searches_mutex_);
    
    auto trans_it = transaction_to_search_.find(transaction_id);
    if (trans_it != transaction_to_search_.end()) {
        std::string hash_key = trans_it->second;
        auto search_it = pending_searches_.find(hash_key);
        if (search_it != pending_searches_.end()) {
            auto& pending_search = search_it->second;
            LOG_DHT_DEBUG("Found pending search for KRPC transaction " << transaction_id 
                          << " - received " << nodes.size() << " nodes for info_hash " << hash_key 
                          << " from " << responder.ip << ":" << responder.port);

            // Continue search iteration
            bool should_remove_search = continue_search_iteration(pending_search);
            if (should_remove_search) {
                LOG_DHT_DEBUG("Removing completed search for info_hash " << hash_key);
                pending_searches_.erase(search_it);
                
                // Also clean up the active search callback
                {
                    std::lock_guard<std::mutex> search_lock(active_searches_mutex_);
                    active_searches_.erase(hash_key);
                }
            }
        }
        
        // Remove the transaction mapping since we've handled it
        transaction_to_search_.erase(trans_it);
    }
}



bool DhtClient::continue_search_iteration(PendingSearch& search) {
    std::string hash_key = node_id_to_hex(search.info_hash);
    
    LOG_DHT_DEBUG("Continuing search iteration for info_hash " << hash_key 
                  << " with " << search.queried_nodes.size() << " queried nodes, iteration " 
                  << search.iteration_count << "/" << search.iteration_max);
    
    // Stop if we've reached max iterations (simple limit)
    if (search.iteration_count >= search.iteration_max) {
        LOG_DHT_DEBUG("Stopping search for " << hash_key << " - reached max iterations (" << search.iteration_count << "/" << search.iteration_max << ")");
        return true;  // Return true to indicate the search should be removed
    }
    
    // Get closest nodes from routing table (already sorted by distance to target)
    std::vector<DhtNode> closest_nodes = find_closest_nodes(search.info_hash, ALPHA);
    
    // Query up to ALPHA closest unqueried nodes
    int nodes_queried = 0;
    int candidates_found = 0;
    for (const auto& node : closest_nodes) {
        if (nodes_queried >= ALPHA) {
            LOG_DHT_DEBUG("Reached ALPHA limit (" << ALPHA << ") for querying nodes in iteration " << (search.iteration_count + 1) << "/" << search.iteration_max);
            break;
        }
        
        std::string node_hex = node_id_to_hex(node.id);
        candidates_found++;
        
        if (search.queried_nodes.find(node_hex) == search.queried_nodes.end()) {
            LOG_DHT_DEBUG("Querying node " << node_hex << " at " << node.peer.ip << ":" << node.peer.port 
                          << " (protocol: BitTorrent" 
                          << ", iteration: " << (search.iteration_count + 1) << ")");
            
            // Mark node as queried in the shared search object
            search.queried_nodes.insert(node_hex);
            
            std::string transaction_id = KrpcProtocol::generate_transaction_id();
            transaction_to_search_[transaction_id] = hash_key;
            
            auto message = KrpcProtocol::create_get_peers_query(transaction_id, node_id_, search.info_hash);
            send_krpc_message(message, node.peer);
            
            nodes_queried++;
        } else {
            LOG_DHT_DEBUG("Skipping already queried node " << node_hex << " at " << node.peer.ip << ":" << node.peer.port);
        }
    }
    
    // Update iteration count
    search.iteration_count++;
    
    LOG_DHT_DEBUG("Search iteration summary for " << hash_key << ":");
    LOG_DHT_DEBUG("  - Candidates evaluated: " << candidates_found);
    LOG_DHT_DEBUG("  - Nodes queried: " << nodes_queried);
    LOG_DHT_DEBUG("  - Already queried nodes skipped: " << (candidates_found - nodes_queried));
    LOG_DHT_DEBUG("  - Current iteration: " << search.iteration_count << "/" << search.iteration_max);
    
    // If we are not making progress, or if we've hit the limit, stop the search.
    if (nodes_queried == 0) {
        LOG_DHT_DEBUG("Stopping search for " << hash_key << " - no new nodes to query");
        return true; // Signal to remove the search
    }
    
    return false;
}

// Peer announcement storage management
void DhtClient::store_announced_peer(const InfoHash& info_hash, const Peer& peer) {
    std::lock_guard<std::mutex> lock(announced_peers_mutex_);
    
    std::string hash_key = node_id_to_hex(info_hash);
    auto& peers = announced_peers_[hash_key];
    
    // Check if peer already exists
    auto it = std::find_if(peers.begin(), peers.end(),
                          [&peer](const AnnouncedPeer& announced) {
                              return announced.peer.ip == peer.ip && announced.peer.port == peer.port;
                          });
    
    if (it != peers.end()) {
        // Update existing peer's timestamp
        it->announced_at = std::chrono::steady_clock::now();
        LOG_DHT_DEBUG("Updated existing announced peer " << peer.ip << ":" << peer.port 
                      << " for info_hash " << hash_key);
    } else {
        // Add new peer
        peers.emplace_back(peer);
        LOG_DHT_DEBUG("Stored new announced peer " << peer.ip << ":" << peer.port 
                      << " for info_hash " << hash_key << " (total: " << peers.size() << ")");
    }
}

std::vector<Peer> DhtClient::get_announced_peers(const InfoHash& info_hash) {
    std::lock_guard<std::mutex> lock(announced_peers_mutex_);
    
    std::string hash_key = node_id_to_hex(info_hash);
    auto it = announced_peers_.find(hash_key);
    
    std::vector<Peer> peers;
    if (it != announced_peers_.end()) {
        peers.reserve(it->second.size());
        for (const auto& announced : it->second) {
            peers.push_back(announced.peer);
        }
        LOG_DHT_DEBUG("Retrieved " << peers.size() << " announced peers for info_hash " << hash_key);
    } else {
        LOG_DHT_DEBUG("No announced peers found for info_hash " << hash_key);
    }
    
    return peers;
}

void DhtClient::cleanup_stale_announced_peers() {
    std::lock_guard<std::mutex> lock(announced_peers_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    auto stale_threshold = std::chrono::minutes(30);  // BEP 5 standard: 30 minutes
    
    size_t total_before = 0;
    size_t total_after = 0;
    
    for (auto it = announced_peers_.begin(); it != announced_peers_.end(); ) {
        auto& peers = it->second;
        total_before += peers.size();
        
        // Remove stale peers
        peers.erase(std::remove_if(peers.begin(), peers.end(),
                                   [now, stale_threshold](const AnnouncedPeer& announced) {
                                       return now - announced.announced_at > stale_threshold;
                                   }), peers.end());
        
        total_after += peers.size();
        
        // Remove empty info_hash entries
        if (peers.empty()) {
            LOG_DHT_DEBUG("Removing empty announced peers entry for info_hash " << it->first);
            it = announced_peers_.erase(it);
        } else {
            ++it;
        }
    }
    
    if (total_before > total_after) {
        LOG_DHT_DEBUG("Cleaned up " << (total_before - total_after) << " stale announced peers "
                      << "(from " << total_before << " to " << total_after << ")");
    }
}

// Ping-before-replace eviction implementation
void DhtClient::initiate_ping_verification(const DhtNode& candidate_node, const DhtNode& old_node, int bucket_index) {
    {
        std::lock_guard<std::mutex> lock(pending_pings_mutex_);
        for (const auto& pending : pending_pings_) {
            if (pending.second.candidate_node.id == candidate_node.id) {
                LOG_DHT_DEBUG("Already pinging candidate node " << node_id_to_hex(candidate_node.id) 
                              << " - skipping duplicate ping verification");
                return;
            }
        }
    }
    
    std::string transaction_id = KrpcProtocol::generate_transaction_id();
    
    LOG_DHT_DEBUG("Initiating ping verification for candidate node " << node_id_to_hex(candidate_node.id) 
                  << " at " << candidate_node.peer.ip << ":" << candidate_node.peer.port 
                  << " to potentially replace old node " << node_id_to_hex(old_node.id) 
                  << " (transaction: " << transaction_id << ")");
    
    // Store ping verification state and mark old node as being replaced
    {
        std::lock_guard<std::mutex> ping_lock(pending_pings_mutex_);
        std::lock_guard<std::mutex> nodes_lock(nodes_being_replaced_mutex_);
        pending_pings_.emplace(transaction_id, PingVerification(candidate_node, old_node, bucket_index, transaction_id));
        nodes_being_replaced_.insert(old_node.id);
    }
    
    // Send ping to the CANDIDATE node to verify it's alive
    auto message = KrpcProtocol::create_ping_query(transaction_id, node_id_);
    send_krpc_message(message, candidate_node.peer);
}

void DhtClient::handle_ping_verification_response(const std::string& transaction_id, const NodeId& responder_id, const Peer& responder) {
    std::lock_guard<std::mutex> ping_lock(pending_pings_mutex_);
    std::lock_guard<std::mutex> nodes_lock(nodes_being_replaced_mutex_);
    
    auto it = pending_pings_.find(transaction_id);
    if (it != pending_pings_.end()) {
        const auto& verification = it->second;
        
        // Check if the responder node ID matches the candidate node we pinged
        if (responder_id == verification.candidate_node.id) {
            LOG_DHT_DEBUG("Ping verification successful for candidate node " << node_id_to_hex(verification.candidate_node.id) 
                          << " - proceeding with replacement of old node " << node_id_to_hex(verification.old_node.id));
            
            // The candidate node responded and is alive - perform the replacement
            // Create a copy of the candidate node with updated timestamp
            DhtNode updated_candidate = verification.candidate_node;
            updated_candidate.last_seen = std::chrono::steady_clock::now();
            perform_replacement(updated_candidate, verification.old_node, verification.bucket_index);
        } else {
            LOG_DHT_WARN("Ping verification response from unexpected node " << node_id_to_hex(responder_id) 
                         << " at " << responder.ip << ":" << responder.port 
                         << " (expected candidate node " << node_id_to_hex(verification.candidate_node.id) << ")");
        }
        
        // Remove the old node from nodes_being_replaced set
        nodes_being_replaced_.erase(verification.old_node.id);
        // Remove the pending ping verification
        pending_pings_.erase(it);
    }
}

void DhtClient::cleanup_stale_ping_verifications() {
    std::lock_guard<std::mutex> ping_lock(pending_pings_mutex_);
    std::lock_guard<std::mutex> nodes_lock(nodes_being_replaced_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    auto timeout_threshold = std::chrono::seconds(30);  // 30 second timeout for ping responses
    
    auto it = pending_pings_.begin();
    while (it != pending_pings_.end()) {
        if (now - it->second.ping_sent_at > timeout_threshold) {
            LOG_DHT_DEBUG("Ping verification timed out for candidate node " << node_id_to_hex(it->second.candidate_node.id) 
                          << " - candidate is unresponsive, keeping old node " << node_id_to_hex(it->second.old_node.id));
            
            // Remove the old node from nodes_being_replaced set since the ping verification failed
            nodes_being_replaced_.erase(it->second.old_node.id);
            it = pending_pings_.erase(it);
        } else {
            ++it;
        }
    }
}

void DhtClient::perform_replacement(const DhtNode& candidate_node, const DhtNode& node_to_replace, int bucket_index) {
    std::lock_guard<std::mutex> lock(routing_table_mutex_);
    
    auto& bucket = routing_table_[bucket_index];
    auto it = std::find_if(bucket.begin(), bucket.end(),
                          [&node_to_replace](const DhtNode& node) {
                              return node.id == node_to_replace.id;
                          });
    
    if (it != bucket.end()) {
        LOG_DHT_DEBUG("Replacing old node " << node_id_to_hex(node_to_replace.id) 
                      << " with " << node_id_to_hex(candidate_node.id) << " in bucket " << bucket_index);
        *it = candidate_node;
    } else {
        LOG_DHT_WARN("Could not find node " << node_id_to_hex(node_to_replace.id) 
                     << " to replace in bucket " << bucket_index);
    }
}

// Utility functions implementation
NodeId string_to_node_id(const std::string& str) {
    NodeId id;
    size_t copy_size = (std::min)(str.size(), NODE_ID_SIZE);
    std::copy(str.begin(), str.begin() + copy_size, id.begin());
    return id;
}

std::string node_id_to_string(const NodeId& id) {
    return std::string(id.begin(), id.end());
}

NodeId hex_to_node_id(const std::string& hex) {
    NodeId id;
    if (hex.size() != NODE_ID_SIZE * 2) {
        return id;  // Return zero-filled ID on error
    }
    
    for (size_t i = 0; i < NODE_ID_SIZE; ++i) {
        std::string byte_str = hex.substr(i * 2, 2);
        id[i] = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
    }
    
    return id;
}

std::string node_id_to_hex(const NodeId& id) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint8_t byte : id) {
        oss << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

} // namespace librats 