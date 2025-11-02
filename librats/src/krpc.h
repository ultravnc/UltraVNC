#pragma once

#include "bencode.h"
#include "socket.h"
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <atomic>

namespace librats {

using NodeId = std::array<uint8_t, 20>;
using InfoHash = std::array<uint8_t, 20>;

/**
 * KRPC Message types
 */
enum class KrpcMessageType {
    Query,
    Response,
    Error
};

/**
 * KRPC Query types
 */
enum class KrpcQueryType {
    Ping,
    FindNode,
    GetPeers,
    AnnouncePeer
};

/**
 * KRPC Error codes
 */
enum class KrpcErrorCode {
    GenericError = 201,
    ServerError = 202,
    ProtocolError = 203,
    MethodUnknown = 204
};

/**
 * KRPC Node information
 */
struct KrpcNode {
    NodeId id;
    std::string ip;
    uint16_t port;
    
    KrpcNode() : port(0) {}
    KrpcNode(const NodeId& node_id, const std::string& ip_addr, uint16_t port_num)
        : id(node_id), ip(ip_addr), port(port_num) {}
};

/**
 * KRPC Message structure
 */
struct KrpcMessage {
    KrpcMessageType type;
    std::string transaction_id;
    
    // For queries
    KrpcQueryType query_type;
    NodeId sender_id;
    NodeId target_id;
    InfoHash info_hash;
    uint16_t port;
    std::string token;
    
    // For responses
    NodeId response_id;
    std::vector<KrpcNode> nodes;
    std::vector<Peer> peers;
    
    // For errors
    KrpcErrorCode error_code;
    std::string error_message;
    
    KrpcMessage() : type(KrpcMessageType::Query), query_type(KrpcQueryType::Ping), port(0), error_code(KrpcErrorCode::GenericError) {}
};

/**
 * KRPC Protocol implementation
 */
class KrpcProtocol {
public:
    KrpcProtocol();
    ~KrpcProtocol();
    
    /**
     * Create KRPC messages
     */
    static KrpcMessage create_ping_query(const std::string& transaction_id, const NodeId& sender_id);
    static KrpcMessage create_find_node_query(const std::string& transaction_id, const NodeId& sender_id, const NodeId& target_id);
    static KrpcMessage create_get_peers_query(const std::string& transaction_id, const NodeId& sender_id, const InfoHash& info_hash);
    static KrpcMessage create_announce_peer_query(const std::string& transaction_id, const NodeId& sender_id, const InfoHash& info_hash, uint16_t port, const std::string& token);
    
    static KrpcMessage create_ping_response(const std::string& transaction_id, const NodeId& response_id);
    static KrpcMessage create_find_node_response(const std::string& transaction_id, const NodeId& response_id, const std::vector<KrpcNode>& nodes);
    static KrpcMessage create_get_peers_response(const std::string& transaction_id, const NodeId& response_id, const std::vector<Peer>& peers, const std::string& token);
    static KrpcMessage create_get_peers_response_with_nodes(const std::string& transaction_id, const NodeId& response_id, const std::vector<KrpcNode>& nodes, const std::string& token);
    static KrpcMessage create_announce_peer_response(const std::string& transaction_id, const NodeId& response_id);
    
    static KrpcMessage create_error(const std::string& transaction_id, KrpcErrorCode error_code, const std::string& error_message);
    
    /**
     * Encode/decode KRPC messages
     */
    static std::vector<uint8_t> encode_message(const KrpcMessage& message);
    static std::unique_ptr<KrpcMessage> decode_message(const std::vector<uint8_t>& data);
    
    /**
     * Generate transaction ID
     */
    static std::string generate_transaction_id();
    
    /**
     * Utility functions
     */
    static std::string node_id_to_string(const NodeId& id);
    static NodeId string_to_node_id(const std::string& str);
    static std::string compact_peer_info(const Peer& peer);
    static std::string compact_node_info(const KrpcNode& node);
    static std::vector<Peer> parse_compact_peer_info(const std::string& compact_info);
    static std::vector<KrpcNode> parse_compact_node_info(const std::string& compact_info);

private:
    static BencodeValue encode_query(const KrpcMessage& message);
    static BencodeValue encode_response(const KrpcMessage& message);
    static BencodeValue encode_error(const KrpcMessage& message);
    
    static std::unique_ptr<KrpcMessage> decode_query(const BencodeValue& data);
    static std::unique_ptr<KrpcMessage> decode_response(const BencodeValue& data);
    static std::unique_ptr<KrpcMessage> decode_error(const BencodeValue& data);
    
    static KrpcQueryType string_to_query_type(const std::string& str);
    static std::string query_type_to_string(KrpcQueryType type);
    
    static std::atomic<uint32_t> transaction_counter_;
};

} // namespace librats 