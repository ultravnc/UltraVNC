#include "krpc.h"
#include "network_utils.h"
#include "logger.h"
#include <random>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <arpa/inet.h>
    #include <netinet/in.h>
#endif

// KRPC module logging macros
#define LOG_KRPC_DEBUG(message) LOG_DEBUG("krpc", message)
#define LOG_KRPC_INFO(message)  LOG_INFO("krpc", message)
#define LOG_KRPC_WARN(message)  LOG_WARN("krpc", message)
#define LOG_KRPC_ERROR(message) LOG_ERROR("krpc", message)

namespace librats {

std::atomic<uint32_t> KrpcProtocol::transaction_counter_ = 0;

KrpcProtocol::KrpcProtocol() {}

KrpcProtocol::~KrpcProtocol() {}

// Create query messages
KrpcMessage KrpcProtocol::create_ping_query(const std::string& transaction_id, const NodeId& sender_id) {
    KrpcMessage message;
    message.type = KrpcMessageType::Query;
    message.transaction_id = transaction_id;
    message.query_type = KrpcQueryType::Ping;
    message.sender_id = sender_id;
    return message;
}

KrpcMessage KrpcProtocol::create_find_node_query(const std::string& transaction_id, const NodeId& sender_id, const NodeId& target_id) {
    KrpcMessage message;
    message.type = KrpcMessageType::Query;
    message.transaction_id = transaction_id;
    message.query_type = KrpcQueryType::FindNode;
    message.sender_id = sender_id;
    message.target_id = target_id;
    return message;
}

KrpcMessage KrpcProtocol::create_get_peers_query(const std::string& transaction_id, const NodeId& sender_id, const InfoHash& info_hash) {
    KrpcMessage message;
    message.type = KrpcMessageType::Query;
    message.transaction_id = transaction_id;
    message.query_type = KrpcQueryType::GetPeers;
    message.sender_id = sender_id;
    message.info_hash = info_hash;
    return message;
}

KrpcMessage KrpcProtocol::create_announce_peer_query(const std::string& transaction_id, const NodeId& sender_id, const InfoHash& info_hash, uint16_t port, const std::string& token) {
    KrpcMessage message;
    message.type = KrpcMessageType::Query;
    message.transaction_id = transaction_id;
    message.query_type = KrpcQueryType::AnnouncePeer;
    message.sender_id = sender_id;
    message.info_hash = info_hash;
    message.port = port;
    message.token = token;
    return message;
}

// Create response messages
KrpcMessage KrpcProtocol::create_ping_response(const std::string& transaction_id, const NodeId& response_id) {
    KrpcMessage message;
    message.type = KrpcMessageType::Response;
    message.transaction_id = transaction_id;
    message.response_id = response_id;
    return message;
}

KrpcMessage KrpcProtocol::create_find_node_response(const std::string& transaction_id, const NodeId& response_id, const std::vector<KrpcNode>& nodes) {
    KrpcMessage message;
    message.type = KrpcMessageType::Response;
    message.transaction_id = transaction_id;
    message.response_id = response_id;
    message.nodes = nodes;
    return message;
}

KrpcMessage KrpcProtocol::create_get_peers_response(const std::string& transaction_id, const NodeId& response_id, const std::vector<Peer>& peers, const std::string& token) {
    KrpcMessage message;
    message.type = KrpcMessageType::Response;
    message.transaction_id = transaction_id;
    message.response_id = response_id;
    message.peers = peers;
    message.token = token;
    return message;
}

KrpcMessage KrpcProtocol::create_get_peers_response_with_nodes(const std::string& transaction_id, const NodeId& response_id, const std::vector<KrpcNode>& nodes, const std::string& token) {
    KrpcMessage message;
    message.type = KrpcMessageType::Response;
    message.transaction_id = transaction_id;
    message.response_id = response_id;
    message.nodes = nodes;
    message.token = token;
    return message;
}

KrpcMessage KrpcProtocol::create_announce_peer_response(const std::string& transaction_id, const NodeId& response_id) {
    KrpcMessage message;
    message.type = KrpcMessageType::Response;
    message.transaction_id = transaction_id;
    message.response_id = response_id;
    return message;
}

// Create error message
KrpcMessage KrpcProtocol::create_error(const std::string& transaction_id, KrpcErrorCode error_code, const std::string& error_message) {
    KrpcMessage message;
    message.type = KrpcMessageType::Error;
    message.transaction_id = transaction_id;
    message.error_code = error_code;
    message.error_message = error_message;
    return message;
}

// Encode messages
std::vector<uint8_t> KrpcProtocol::encode_message(const KrpcMessage& message) {
    BencodeValue root;
    
    switch (message.type) {
        case KrpcMessageType::Query:
            root = encode_query(message);
            break;
        case KrpcMessageType::Response:
            root = encode_response(message);
            break;
        case KrpcMessageType::Error:
            root = encode_error(message);
            break;
        default:
            LOG_KRPC_ERROR("Unknown message type: " << static_cast<int>(message.type));
            return {};
    }
    
    return root.encode();
}

BencodeValue KrpcProtocol::encode_query(const KrpcMessage& message) {
    BencodeValue root = BencodeValue::create_dict();
    
    // Common fields
    root["t"] = BencodeValue(message.transaction_id);
    root["y"] = BencodeValue("q");
    root["q"] = BencodeValue(query_type_to_string(message.query_type));
    
    // Arguments
    BencodeValue args = BencodeValue::create_dict();
    args["id"] = BencodeValue(node_id_to_string(message.sender_id));
    
    switch (message.query_type) {
        case KrpcQueryType::Ping:
            // No additional arguments for ping
            break;
        case KrpcQueryType::FindNode:
            args["target"] = BencodeValue(node_id_to_string(message.target_id));
            break;
        case KrpcQueryType::GetPeers:
            args["info_hash"] = BencodeValue(node_id_to_string(message.info_hash));
            break;
        case KrpcQueryType::AnnouncePeer:
            args["info_hash"] = BencodeValue(node_id_to_string(message.info_hash));
            args["port"] = BencodeValue(static_cast<int64_t>(message.port));
            args["token"] = BencodeValue(message.token);
            break;
    }
    
    root["a"] = args;
    return root;
}

BencodeValue KrpcProtocol::encode_response(const KrpcMessage& message) {
    BencodeValue root = BencodeValue::create_dict();
    
    // Common fields
    root["t"] = BencodeValue(message.transaction_id);
    root["y"] = BencodeValue("r");
    
    // Response data
    BencodeValue response = BencodeValue::create_dict();
    response["id"] = BencodeValue(node_id_to_string(message.response_id));
    
    // Add nodes if present
    if (!message.nodes.empty()) {
        std::string compact_nodes;
        for (const auto& node : message.nodes) {
            compact_nodes += compact_node_info(node);
        }
        response["nodes"] = BencodeValue(compact_nodes);
    }
    
    // Add peers if present
    if (!message.peers.empty()) {
        BencodeValue values = BencodeValue::create_list();
        for (const auto& peer : message.peers) {
            values.push_back(BencodeValue(compact_peer_info(peer)));
        }
        response["values"] = values;
    }
    
    // Add token if present
    if (!message.token.empty()) {
        response["token"] = BencodeValue(message.token);
    }
    
    root["r"] = response;
    return root;
}

BencodeValue KrpcProtocol::encode_error(const KrpcMessage& message) {
    BencodeValue root = BencodeValue::create_dict();
    
    // Common fields
    root["t"] = BencodeValue(message.transaction_id);
    root["y"] = BencodeValue("e");
    
    // Error data
    BencodeValue error = BencodeValue::create_list();
    error.push_back(BencodeValue(static_cast<int64_t>(message.error_code)));
    error.push_back(BencodeValue(message.error_message));
    
    root["e"] = error;
    return root;
}

// Decode messages
std::unique_ptr<KrpcMessage> KrpcProtocol::decode_message(const std::vector<uint8_t>& data) {
    try {
        BencodeValue root = bencode::decode(data);
        
        if (!root.is_dict()) {
            LOG_KRPC_ERROR("Root is not a dictionary");
            return nullptr;
        }
        
        if (!root.has_key("t") || !root.has_key("y")) {
            LOG_KRPC_ERROR("Missing required fields 't' or 'y'");
            return nullptr;
        }
        
        std::string transaction_id = root["t"].as_string();
        std::string message_type = root["y"].as_string();
        
        if (message_type == "q") {
            return decode_query(root);
        } else if (message_type == "r") {
            return decode_response(root);
        } else if (message_type == "e") {
            return decode_error(root);
        } else {
            LOG_KRPC_ERROR("Unknown message type: " << message_type);
            return nullptr;
        }
    } catch (const std::exception& e) {
        LOG_KRPC_ERROR("Failed to decode KRPC message: " << e.what());
        return nullptr;
    }
}

std::unique_ptr<KrpcMessage> KrpcProtocol::decode_query(const BencodeValue& data) {
    if (!data.has_key("q") || !data.has_key("a")) {
        LOG_KRPC_ERROR("Missing required query fields 'q' or 'a'");
        return nullptr;
    }
    
    auto message = std::make_unique<KrpcMessage>();
    message->type = KrpcMessageType::Query;
    message->transaction_id = data["t"].as_string();
    
    std::string query_method = data["q"].as_string();
    message->query_type = string_to_query_type(query_method);
    
    const BencodeValue& args = data["a"];
    if (!args.is_dict() || !args.has_key("id")) {
        LOG_KRPC_ERROR("Invalid arguments in query");
        return nullptr;
    }
    
    message->sender_id = string_to_node_id(args["id"].as_string());
    
    switch (message->query_type) {
        case KrpcQueryType::Ping:
            // No additional arguments
            break;
        case KrpcQueryType::FindNode:
            if (args.has_key("target")) {
                message->target_id = string_to_node_id(args["target"].as_string());
            }
            break;
        case KrpcQueryType::GetPeers:
            if (args.has_key("info_hash")) {
                message->info_hash = string_to_node_id(args["info_hash"].as_string());
            }
            break;
        case KrpcQueryType::AnnouncePeer:
            if (args.has_key("info_hash")) {
                message->info_hash = string_to_node_id(args["info_hash"].as_string());
            }
            if (args.has_key("port")) {
                message->port = static_cast<uint16_t>(args["port"].as_integer());
            }
            if (args.has_key("token")) {
                message->token = args["token"].as_string();
            }
            break;
    }
    
    return message;
}

std::unique_ptr<KrpcMessage> KrpcProtocol::decode_response(const BencodeValue& data) {
    if (!data.has_key("r")) {
        LOG_KRPC_ERROR("Missing required response field 'r'");
        return nullptr;
    }
    
    auto message = std::make_unique<KrpcMessage>();
    message->type = KrpcMessageType::Response;
    message->transaction_id = data["t"].as_string();
    
    const BencodeValue& response = data["r"];
    if (!response.is_dict() || !response.has_key("id")) {
        LOG_KRPC_ERROR("Invalid response data");
        return nullptr;
    }
    
    message->response_id = string_to_node_id(response["id"].as_string());
    
    // Parse nodes if present
    if (response.has_key("nodes")) {
        std::string compact_nodes = response["nodes"].as_string();
        message->nodes = parse_compact_node_info(compact_nodes);
    }
    
    // Parse peers if present
    if (response.has_key("values")) {
        const BencodeValue& values = response["values"];
        if (values.is_list()) {
            for (size_t i = 0; i < values.size(); ++i) {
                std::string compact_peers = values[i].as_string();
                auto peers = parse_compact_peer_info(compact_peers);
                message->peers.insert(message->peers.end(), peers.begin(), peers.end());
            }
        }
    }
    
    // Parse token if present
    if (response.has_key("token")) {
        message->token = response["token"].as_string();
    }
    
    return message;
}

std::unique_ptr<KrpcMessage> KrpcProtocol::decode_error(const BencodeValue& data) {
    if (!data.has_key("e")) {
        LOG_KRPC_ERROR("Missing required error field 'e'");
        return nullptr;
    }
    
    auto message = std::make_unique<KrpcMessage>();
    message->type = KrpcMessageType::Error;
    message->transaction_id = data["t"].as_string();
    
    const BencodeValue& error = data["e"];
    if (!error.is_list() || error.size() < 2) {
        LOG_KRPC_ERROR("Invalid error data");
        return nullptr;
    }
    
    message->error_code = static_cast<KrpcErrorCode>(error[0].as_integer());
    message->error_message = error[1].as_string();
    
    return message;
}

// Utility functions
std::string KrpcProtocol::generate_transaction_id() {
    return std::to_string(++transaction_counter_);
}

std::string KrpcProtocol::node_id_to_string(const NodeId& id) {
    return std::string(id.begin(), id.end());
}

NodeId KrpcProtocol::string_to_node_id(const std::string& str) {
    NodeId id;
    if (str.size() >= 20) {
        std::copy_n(str.begin(), 20, id.begin());
    } else {
        std::fill(id.begin(), id.end(), 0);
        std::copy(str.begin(), str.end(), id.begin());
    }
    return id;
}

std::string KrpcProtocol::compact_peer_info(const Peer& peer) {
    std::string result;
    result.reserve(6);
    
    // Convert IP address to 4 bytes
    if (network_utils::is_valid_ipv4(peer.ip)) {
        struct in_addr addr;
        if (inet_pton(AF_INET, peer.ip.c_str(), &addr) == 1) {
            uint32_t ip = ntohl(addr.s_addr);
            result += static_cast<char>((ip >> 24) & 0xFF);
            result += static_cast<char>((ip >> 16) & 0xFF);
            result += static_cast<char>((ip >> 8) & 0xFF);
            result += static_cast<char>(ip & 0xFF);
        } else {
            // Invalid IP, use 0.0.0.0
            result += "\x00\x00\x00\x00";
        }
    } else {
        // Not IPv4, use 0.0.0.0
        result += "\x00\x00\x00\x00";
    }
    
    // Convert port to 2 bytes (network byte order)
    result += static_cast<char>((peer.port >> 8) & 0xFF);
    result += static_cast<char>(peer.port & 0xFF);
    
    return result;
}

std::string KrpcProtocol::compact_node_info(const KrpcNode& node) {
    std::string result;
    result.reserve(26);
    
    // Node ID (20 bytes)
    result += node_id_to_string(node.id);
    
    // IP address (4 bytes)
    if (network_utils::is_valid_ipv4(node.ip)) {
        struct in_addr addr;
        if (inet_pton(AF_INET, node.ip.c_str(), &addr) == 1) {
            uint32_t ip = ntohl(addr.s_addr);
            result += static_cast<char>((ip >> 24) & 0xFF);
            result += static_cast<char>((ip >> 16) & 0xFF);
            result += static_cast<char>((ip >> 8) & 0xFF);
            result += static_cast<char>(ip & 0xFF);
        } else {
            // Invalid IP, use 0.0.0.0
            result += "\x00\x00\x00\x00";
        }
    } else {
        // Not IPv4, use 0.0.0.0
        result += "\x00\x00\x00\x00";
    }
    
    // Port (2 bytes, network byte order)
    result += static_cast<char>((node.port >> 8) & 0xFF);
    result += static_cast<char>(node.port & 0xFF);
    
    return result;
}

std::vector<Peer> KrpcProtocol::parse_compact_peer_info(const std::string& compact_info) {
    std::vector<Peer> peers;
    
    if (compact_info.size() % 6 != 0) {
        LOG_KRPC_WARN("Invalid compact peer info size: " << compact_info.size());
        return peers;
    }
    
    for (size_t i = 0; i < compact_info.size(); i += 6) {
        // Extract IP address (4 bytes)
        uint32_t ip = 0;
        ip |= (static_cast<uint8_t>(compact_info[i]) << 24);
        ip |= (static_cast<uint8_t>(compact_info[i + 1]) << 16);
        ip |= (static_cast<uint8_t>(compact_info[i + 2]) << 8);
        ip |= static_cast<uint8_t>(compact_info[i + 3]);
        
        struct in_addr addr;
        addr.s_addr = htonl(ip);
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr, ip_str, INET_ADDRSTRLEN);
        
        // Extract port (2 bytes)
        uint16_t port = 0;
        port |= (static_cast<uint8_t>(compact_info[i + 4]) << 8);
        port |= static_cast<uint8_t>(compact_info[i + 5]);
        
        peers.emplace_back(ip_str, port);
    }
    
    return peers;
}

std::vector<KrpcNode> KrpcProtocol::parse_compact_node_info(const std::string& compact_info) {
    std::vector<KrpcNode> nodes;
    
    if (compact_info.size() % 26 != 0) {
        LOG_KRPC_WARN("Invalid compact node info size: " << compact_info.size());
        return nodes;
    }
    
    for (size_t i = 0; i < compact_info.size(); i += 26) {
        // Extract node ID (20 bytes)
        NodeId node_id;
        std::copy_n(compact_info.begin() + i, 20, node_id.begin());
        
        // Extract IP address (4 bytes)
        uint32_t ip = 0;
        ip |= (static_cast<uint8_t>(compact_info[i + 20]) << 24);
        ip |= (static_cast<uint8_t>(compact_info[i + 21]) << 16);
        ip |= (static_cast<uint8_t>(compact_info[i + 22]) << 8);
        ip |= static_cast<uint8_t>(compact_info[i + 23]);
        
        struct in_addr addr;
        addr.s_addr = htonl(ip);
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr, ip_str, INET_ADDRSTRLEN);
        
        // Extract port (2 bytes)
        uint16_t port = 0;
        port |= (static_cast<uint8_t>(compact_info[i + 24]) << 8);
        port |= static_cast<uint8_t>(compact_info[i + 25]);
        
        nodes.emplace_back(node_id, ip_str, port);
    }
    
    return nodes;
}

KrpcQueryType KrpcProtocol::string_to_query_type(const std::string& str) {
    if (str == "ping") return KrpcQueryType::Ping;
    if (str == "find_node") return KrpcQueryType::FindNode;
    if (str == "get_peers") return KrpcQueryType::GetPeers;
    if (str == "announce_peer") return KrpcQueryType::AnnouncePeer;
    return KrpcQueryType::Ping; // Default
}

std::string KrpcProtocol::query_type_to_string(KrpcQueryType type) {
    switch (type) {
        case KrpcQueryType::Ping: return "ping";
        case KrpcQueryType::FindNode: return "find_node";
        case KrpcQueryType::GetPeers: return "get_peers";
        case KrpcQueryType::AnnouncePeer: return "announce_peer";
        default: return "ping";
    }
}

} // namespace librats 