#ifndef LIBRATS_STUN_H
#define LIBRATS_STUN_H

#include "socket.h"
#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <memory>

namespace librats {

// STUN Protocol Constants
namespace stun {
    // STUN Message Types (RFC 5389)
    const uint16_t BINDING_REQUEST = 0x0001;
    const uint16_t BINDING_RESPONSE = 0x0101;
    const uint16_t BINDING_ERROR_RESPONSE = 0x0111;
    
    // TURN Message Types (RFC 5766)
    const uint16_t ALLOCATE_REQUEST = 0x0003;
    const uint16_t ALLOCATE_RESPONSE = 0x0103;
    const uint16_t ALLOCATE_ERROR_RESPONSE = 0x0113;
    const uint16_t REFRESH_REQUEST = 0x0004;
    const uint16_t REFRESH_RESPONSE = 0x0104;
    const uint16_t SEND_INDICATION = 0x0016;
    const uint16_t DATA_INDICATION = 0x0017;
    const uint16_t CREATE_PERMISSION_REQUEST = 0x0008;
    const uint16_t CREATE_PERMISSION_RESPONSE = 0x0108;
    const uint16_t CHANNEL_BIND_REQUEST = 0x0009;
    const uint16_t CHANNEL_BIND_RESPONSE = 0x0109;
    
    // STUN Magic Cookie (RFC 5389)
    const uint32_t MAGIC_COOKIE = 0x2112A442;
    
    // STUN Attribute Types (RFC 5389)
    const uint16_t ATTR_MAPPED_ADDRESS = 0x0001;
    const uint16_t ATTR_RESPONSE_ADDRESS = 0x0002;
    const uint16_t ATTR_CHANGE_REQUEST = 0x0003;
    const uint16_t ATTR_SOURCE_ADDRESS = 0x0004;
    const uint16_t ATTR_CHANGED_ADDRESS = 0x0005;
    const uint16_t ATTR_USERNAME = 0x0006;
    const uint16_t ATTR_PASSWORD = 0x0007;
    const uint16_t ATTR_MESSAGE_INTEGRITY = 0x0008;
    const uint16_t ATTR_ERROR_CODE = 0x0009;
    const uint16_t ATTR_UNKNOWN_ATTRIBUTES = 0x000A;
    const uint16_t ATTR_REFLECTED_FROM = 0x000B;
    const uint16_t ATTR_REALM = 0x0014;
    const uint16_t ATTR_NONCE = 0x0015;
    const uint16_t ATTR_XOR_MAPPED_ADDRESS = 0x0020;
    
    // TURN Attribute Types (RFC 5766)
    const uint16_t ATTR_CHANNEL_NUMBER = 0x000C;
    const uint16_t ATTR_LIFETIME = 0x000D;
    const uint16_t ATTR_BANDWIDTH = 0x0010;
    const uint16_t ATTR_XOR_PEER_ADDRESS = 0x0012;
    const uint16_t ATTR_DATA = 0x0013;
    const uint16_t ATTR_XOR_RELAYED_ADDRESS = 0x0016;
    const uint16_t ATTR_EVEN_PORT = 0x0018;
    const uint16_t ATTR_REQUESTED_TRANSPORT = 0x0019;
    const uint16_t ATTR_DONT_FRAGMENT = 0x001A;
    const uint16_t ATTR_RESERVATION_TOKEN = 0x0022;
    
    // ICE Attribute Types (RFC 8445)
    const uint16_t ATTR_PRIORITY = 0x0024;
    const uint16_t ATTR_USE_CANDIDATE = 0x0025;
    const uint16_t ATTR_ICE_CONTROLLED = 0x8029;
    const uint16_t ATTR_ICE_CONTROLLING = 0x802A;
    
    // Address families
    const uint8_t FAMILY_IPV4 = 0x01;
    const uint8_t FAMILY_IPV6 = 0x02;
    
    // STUN message header size
    const size_t HEADER_SIZE = 20;
    
    // Transaction ID size
    const size_t TRANSACTION_ID_SIZE = 12;
    
    // Error codes
    const uint16_t ERROR_TRY_ALTERNATE = 300;
    const uint16_t ERROR_BAD_REQUEST = 400;
    const uint16_t ERROR_UNAUTHORIZED = 401;
    const uint16_t ERROR_UNKNOWN_ATTRIBUTE = 420;
    const uint16_t ERROR_STALE_NONCE = 438;
    const uint16_t ERROR_SERVER_ERROR = 500;
    const uint16_t ERROR_ALLOCATION_MISMATCH = 437;
    const uint16_t ERROR_WRONG_CREDENTIALS = 441;
    const uint16_t ERROR_UNSUPPORTED_TRANSPORT = 442;
    const uint16_t ERROR_ALLOCATION_QUOTA_REACHED = 486;
    const uint16_t ERROR_INSUFFICIENT_CAPACITY = 508;
}

// STUN Message Header Structure
struct StunHeader {
    uint16_t message_type;
    uint16_t message_length;
    uint32_t magic_cookie;
    uint8_t transaction_id[stun::TRANSACTION_ID_SIZE];
};

// STUN Attribute Header
struct StunAttribute {
    uint16_t type;
    uint16_t length;
    // Value follows this header
};

// STUN Address Structure
struct StunAddress {
    uint8_t family;
    uint16_t port;
    std::string ip;
    
    StunAddress() : family(stun::FAMILY_IPV4), port(0) {}
    StunAddress(const std::string& ip_addr, uint16_t port_num) 
        : family(stun::FAMILY_IPV4), port(port_num), ip(ip_addr) {}
};

// STUN Error Information
struct StunError {
    uint16_t code;
    std::string reason;
    
    StunError() : code(0) {}
    StunError(uint16_t error_code, const std::string& reason_phrase) 
        : code(error_code), reason(reason_phrase) {}
};

// STUN Message for parsing responses
struct StunMessage {
    uint16_t message_type;
    uint16_t message_length;
    uint8_t transaction_id[stun::TRANSACTION_ID_SIZE];
    std::vector<StunAttribute*> attributes;
    
    // Parsed attribute data
    StunAddress mapped_address;
    StunAddress xor_mapped_address;
    StunAddress source_address;
    StunAddress changed_address;
    StunAddress xor_relayed_address;
    StunAddress xor_peer_address;
    StunError error;
    std::string username;
    std::string realm;
    std::string nonce;
    std::vector<uint8_t> data;
    uint32_t lifetime;
    uint32_t priority;
    bool use_candidate;
    uint64_t ice_controlled;
    uint64_t ice_controlling;
    
    StunMessage();
    ~StunMessage();
    void clear();
};

// STUN Client Class with enhanced capabilities
class StunClient {
public:
    StunClient();
    ~StunClient();
    
    // Basic STUN functionality
    bool get_public_address(const std::string& stun_server, 
                           int stun_port, 
                           StunAddress& public_address,
                           int timeout_ms = 5000);
    
    // Get public IP from Google STUN server
    bool get_public_address_from_google(StunAddress& public_address, 
                                       int timeout_ms = 5000);
    
    // Advanced STUN functionality for NAT detection
    bool test_stun_binding(const std::string& stun_server, int stun_port,
                          StunAddress& mapped_addr, StunAddress& source_addr,
                          int timeout_ms = 5000);
    
    bool test_change_request(const std::string& stun_server, int stun_port,
                            bool change_ip, bool change_port,
                            StunAddress& response_addr, int timeout_ms = 5000);
    
    // ICE STUN functionality
    bool send_binding_request_ice(socket_t socket, const std::string& remote_ip, uint16_t remote_port,
                                 const std::string& username, const std::string& password,
                                 uint32_t priority, bool controlling, uint64_t tie_breaker,
                                 bool use_candidate = false);
    
    bool handle_binding_request_ice(const std::vector<uint8_t>& request, 
                                   const std::string& from_ip, uint16_t from_port,
                                   const std::string& local_username, const std::string& local_password,
                                   std::vector<uint8_t>& response);
    
    // Static helper functions
    static std::vector<uint8_t> create_binding_request();
    static std::vector<uint8_t> create_binding_request_ice(const std::string& username,
                                                          const std::string& password,
                                                          uint32_t priority,
                                                          bool controlling,
                                                          uint64_t tie_breaker,
                                                          bool use_candidate = false);
    
    static std::vector<uint8_t> create_change_request(bool change_ip, bool change_port);
    static std::vector<uint8_t> create_binding_response(const uint8_t* transaction_id,
                                                       const StunAddress& mapped_addr);
    static std::vector<uint8_t> create_error_response(const uint8_t* transaction_id,
                                                     uint16_t error_code,
                                                     const std::string& reason);
    
    static bool parse_stun_message(const std::vector<uint8_t>& data, StunMessage& message);
    static bool parse_binding_response(const std::vector<uint8_t>& response, 
                                      StunAddress& mapped_address);
    
    // TURN client functionality
    bool allocate_turn_relay(const std::string& turn_server, uint16_t turn_port,
                            const std::string& username, const std::string& password,
                            StunAddress& relayed_address, uint32_t& lifetime,
                            int timeout_ms = 10000);
    
    bool refresh_turn_allocation(socket_t turn_socket, uint32_t lifetime,
                                const std::string& username, const std::string& password,
                                int timeout_ms = 5000);
    
    bool create_turn_permission(socket_t turn_socket, const StunAddress& peer_address,
                               const std::string& username, const std::string& password,
                               int timeout_ms = 5000);
    
    bool send_turn_data(socket_t turn_socket, const std::vector<uint8_t>& data,
                       const StunAddress& peer_address);
    
    // NAT traversal coordination
    bool coordinate_nat_traversal(const std::string& peer_ip, uint16_t peer_port,
                                 const std::string& coordination_data,
                                 std::function<void(bool, const std::string&)> callback);
    
private:
    // Helper functions
    void generate_transaction_id(uint8_t* transaction_id);
    bool send_stun_request(socket_t sock, 
                          const std::string& server, 
                          int port,
                          const std::vector<uint8_t>& request);
    bool receive_stun_response(socket_t sock, 
                              std::vector<uint8_t>& response,
                              int timeout_ms);
    
    // Parsing helpers
    static uint16_t parse_uint16(const uint8_t* data);
    static uint32_t parse_uint32(const uint8_t* data);
    static uint64_t parse_uint64(const uint8_t* data);
    static void write_uint16(uint8_t* data, uint16_t value);
    static void write_uint32(uint8_t* data, uint32_t value);
    static void write_uint64(uint8_t* data, uint64_t value);
    
    // Attribute parsing
    static bool parse_address_attribute(const uint8_t* data, size_t length, 
                                       StunAddress& address, bool xor_mapped = false,
                                       const uint8_t* transaction_id = nullptr);
    static bool parse_error_attribute(const uint8_t* data, size_t length, StunError& error);
    static bool parse_string_attribute(const uint8_t* data, size_t length, std::string& result);
    
    // Attribute creation
    static void add_mapped_address_attribute(std::vector<uint8_t>& message, 
                                           const StunAddress& address);
    static void add_xor_mapped_address_attribute(std::vector<uint8_t>& message, 
                                               const StunAddress& address,
                                               const uint8_t* transaction_id);
    static void add_username_attribute(std::vector<uint8_t>& message, const std::string& username);
    static void add_message_integrity_attribute(std::vector<uint8_t>& message, 
                                              const std::string& password);
    static void add_priority_attribute(std::vector<uint8_t>& message, uint32_t priority);
    static void add_ice_controlling_attribute(std::vector<uint8_t>& message, uint64_t tie_breaker);
    static void add_ice_controlled_attribute(std::vector<uint8_t>& message, uint64_t tie_breaker);
    static void add_use_candidate_attribute(std::vector<uint8_t>& message);
    static void add_error_code_attribute(std::vector<uint8_t>& message, 
                                        uint16_t error_code, const std::string& reason);
    
    // XOR operations for XOR-MAPPED-ADDRESS
    static void xor_address(StunAddress& address, const uint8_t* transaction_id);
    static void xor_address_data(uint8_t* addr_data, size_t length, const uint8_t* transaction_id);
    
    // Message integrity calculation
    static std::vector<uint8_t> calculate_hmac_sha1(const std::vector<uint8_t>& message, 
                                                   const std::string& key);
    static bool verify_message_integrity(const std::vector<uint8_t>& message, 
                                        const std::string& password);
    
    // Authentication helpers
    bool authenticate_stun_message(std::vector<uint8_t>& message, 
                                  const std::string& username, 
                                  const std::string& password,
                                  const std::string& realm = "",
                                  const std::string& nonce = "");
    
    // TURN specific helpers
    static std::vector<uint8_t> create_allocate_request(const std::string& username,
                                                       const std::string& password);
    static std::vector<uint8_t> create_refresh_request(uint32_t lifetime,
                                                      const std::string& username,
                                                      const std::string& password);
    static std::vector<uint8_t> create_permission_request(const StunAddress& peer_address,
                                                         const std::string& username,
                                                         const std::string& password);
    static std::vector<uint8_t> create_send_indication(const std::vector<uint8_t>& data,
                                                      const StunAddress& peer_address);
    
    bool handle_turn_error_response(const StunMessage& message, 
                                   std::string& realm, std::string& nonce);
};

// Enhanced NAT type detection
enum class NatBehavior {
    UNKNOWN,
    ENDPOINT_INDEPENDENT,
    ADDRESS_DEPENDENT,
    ADDRESS_PORT_DEPENDENT
};

struct NatTypeInfo {
    bool has_nat;
    NatBehavior filtering_behavior;
    NatBehavior mapping_behavior;
    bool preserves_port;
    bool hairpin_support;
    std::string description;
};

class AdvancedNatDetector {
public:
    AdvancedNatDetector();
    ~AdvancedNatDetector();
    
    NatTypeInfo detect_nat_characteristics(const std::vector<std::string>& stun_servers,
                                          int timeout_ms = 5000);
    
    bool test_hairpin_support(const std::string& stun_server, int timeout_ms);
    bool test_port_preservation(const std::vector<std::string>& stun_servers, int timeout_ms);
    NatBehavior test_filtering_behavior(const std::vector<std::string>& stun_servers, int timeout_ms);
    NatBehavior test_mapping_behavior(const std::vector<std::string>& stun_servers, int timeout_ms);
    
private:
    std::unique_ptr<StunClient> stun_client_;
};

} // namespace librats

#endif // LIBRATS_STUN_H 