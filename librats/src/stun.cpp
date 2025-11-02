#include "stun.h"
#include "ice.h"
#include "network_utils.h"
#include "logger.h"
#include <random>
#include <cstring>
#include <chrono>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <arpa/inet.h>
    #include <netinet/in.h>
#endif

// STUN module logging macros
#define LOG_STUN_DEBUG(message) LOG_DEBUG("stun", message)
#define LOG_STUN_INFO(message)  LOG_INFO("stun", message)
#define LOG_STUN_WARN(message)  LOG_WARN("stun", message)
#define LOG_STUN_ERROR(message) LOG_ERROR("stun", message)

namespace librats {

StunClient::StunClient() {
    LOG_STUN_DEBUG("STUN client created");
}

StunClient::~StunClient() {
    LOG_STUN_DEBUG("STUN client destroyed");
}

bool StunClient::get_public_address_from_google(StunAddress& public_address, int timeout_ms) {
    return get_public_address("stun.l.google.com", 19302, public_address, timeout_ms);
}

bool StunClient::get_public_address(const std::string& stun_server, 
                                   int stun_port, 
                                   StunAddress& public_address,
                                   int timeout_ms) {
    LOG_STUN_INFO("Getting public address from STUN server: " << stun_server << ":" << stun_port);
    
    // Initialize socket library (safe to call multiple times)
    if (!init_socket_library()) {
        LOG_STUN_ERROR("Failed to initialize socket library");
        return false;
    }
    
    // Create UDP socket for STUN
    socket_t stun_socket = create_udp_socket_v4(0); // Use ephemeral port
    if (!is_valid_socket(stun_socket)) {
        LOG_STUN_ERROR("Failed to create UDP socket for STUN");
        return false;
    }
    
    // Set socket to non-blocking for timeout support
    if (!set_socket_nonblocking(stun_socket)) {
        LOG_STUN_WARN("Failed to set STUN socket to non-blocking mode");
    }
    
    // Create STUN binding request
    std::vector<uint8_t> request = create_binding_request();
    
    // Send request to STUN server
    if (!send_stun_request(stun_socket, stun_server, stun_port, request)) {
        LOG_STUN_ERROR("Failed to send STUN request to " << stun_server << ":" << stun_port);
        close_socket(stun_socket);
        return false;
    }
    
    // Receive response
    std::vector<uint8_t> response;
    if (!receive_stun_response(stun_socket, response, timeout_ms)) {
        LOG_STUN_ERROR("Failed to receive STUN response from " << stun_server << ":" << stun_port);
        close_socket(stun_socket);
        return false;
    }
    
    // Parse response to get mapped address
    if (!parse_binding_response(response, public_address)) {
        LOG_STUN_ERROR("Failed to parse STUN response from " << stun_server << ":" << stun_port);
        close_socket(stun_socket);
        return false;
    }
    
    close_socket(stun_socket);
    
    LOG_STUN_INFO("Successfully got public address: " << public_address.ip << ":" << public_address.port);
    return true;
}

std::vector<uint8_t> StunClient::create_binding_request() {
    std::vector<uint8_t> request(stun::HEADER_SIZE);
    
    // Message Type: Binding Request
    write_uint16(request.data(), stun::BINDING_REQUEST);
    
    // Message Length: 0 (no attributes for basic binding request)
    write_uint16(request.data() + 2, 0);
    
    // Magic Cookie
    write_uint32(request.data() + 4, stun::MAGIC_COOKIE);
    
    // Transaction ID (96 bits = 12 bytes)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (int i = 0; i < stun::TRANSACTION_ID_SIZE; ++i) {
        request[8 + i] = static_cast<uint8_t>(dis(gen));
    }
    
    LOG_STUN_DEBUG("Created STUN binding request (" << request.size() << " bytes)");
    return request;
}

bool StunClient::parse_binding_response(const std::vector<uint8_t>& response, 
                                       StunAddress& mapped_address) {
    if (response.size() < stun::HEADER_SIZE) {
        LOG_STUN_ERROR("STUN response too short: " << response.size() << " bytes");
        return false;
    }
    
    // Parse header
    uint16_t message_type = parse_uint16(response.data());
    uint16_t message_length = parse_uint16(response.data() + 2);
    uint32_t magic_cookie = parse_uint32(response.data() + 4);
    
    // Verify this is a binding response
    if (message_type != stun::BINDING_RESPONSE) {
        LOG_STUN_ERROR("Invalid STUN message type: 0x" << std::hex << message_type);
        return false;
    }
    
    // Verify magic cookie
    if (magic_cookie != stun::MAGIC_COOKIE) {
        LOG_STUN_ERROR("Invalid STUN magic cookie: 0x" << std::hex << magic_cookie);
        return false;
    }
    
    // Check if response has the expected length
    if (response.size() != stun::HEADER_SIZE + message_length) {
        LOG_STUN_ERROR("STUN response length mismatch: expected " << 
                      (stun::HEADER_SIZE + message_length) << ", got " << response.size());
        return false;
    }
    
    // Parse attributes
    size_t offset = stun::HEADER_SIZE;
    bool found_mapped_address = false;
    
    while (offset < response.size()) {
        if (offset + 4 > response.size()) {
            LOG_STUN_ERROR("Truncated STUN attribute header");
            break;
        }
        
        uint16_t attr_type = parse_uint16(response.data() + offset);
        uint16_t attr_length = parse_uint16(response.data() + offset + 2);
        offset += 4;
        
        if (offset + attr_length > response.size()) {
            LOG_STUN_ERROR("Truncated STUN attribute value");
            break;
        }
        
        // Handle XOR-MAPPED-ADDRESS (preferred) or MAPPED-ADDRESS
        if (attr_type == stun::ATTR_XOR_MAPPED_ADDRESS || attr_type == stun::ATTR_MAPPED_ADDRESS) {
            if (attr_length < 8) {
                LOG_STUN_ERROR("Invalid STUN address attribute length: " << attr_length);
                offset += attr_length;
                continue;
            }
            
            const uint8_t* attr_data = response.data() + offset;
            
            // Skip reserved byte
            uint8_t family = attr_data[1];
            uint16_t port = parse_uint16(attr_data + 2);
            
            if (family == stun::FAMILY_IPV4 && attr_length >= 8) {
                // IPv4 address
                uint32_t ip_addr = parse_uint32(attr_data + 4);
                
                // Convert to string
                struct in_addr addr;
                addr.s_addr = htonl(ip_addr);
                char ip_str[INET_ADDRSTRLEN];
                if (inet_ntop(AF_INET, &addr, ip_str, INET_ADDRSTRLEN)) {
                    mapped_address.family = family;
                    mapped_address.port = port;
                    mapped_address.ip = ip_str;
                    
                    // If this is XOR-MAPPED-ADDRESS, we need to XOR with magic cookie and transaction ID
                    if (attr_type == stun::ATTR_XOR_MAPPED_ADDRESS) {
                        xor_address(mapped_address, response.data() + 8); // transaction ID starts at offset 8
                    }
                    
                    found_mapped_address = true;
                    LOG_STUN_DEBUG("Found " << (attr_type == stun::ATTR_XOR_MAPPED_ADDRESS ? "XOR-" : "") 
                                  << "MAPPED-ADDRESS: " << mapped_address.ip << ":" << mapped_address.port);
                    break;
                }
            } else if (family == stun::FAMILY_IPV6 && attr_length >= 20) {
                // IPv6 address
                LOG_STUN_DEBUG("IPv6 address found but not fully implemented");
                // TODO: Implement IPv6 support if needed
            }
        }
        
        // Move to next attribute (attributes are padded to 4-byte boundaries)
        size_t padded_length = (attr_length + 3) & ~3;
        offset += padded_length;
    }
    
    if (!found_mapped_address) {
        LOG_STUN_ERROR("No mapped address found in STUN response");
        return false;
    }
    
    return true;
}

void StunClient::generate_transaction_id(uint8_t* transaction_id) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (int i = 0; i < stun::TRANSACTION_ID_SIZE; ++i) {
        transaction_id[i] = static_cast<uint8_t>(dis(gen));
    }
}

bool StunClient::send_stun_request(socket_t sock, 
                                  const std::string& server, 
                                  int port,
                                  const std::vector<uint8_t>& request) {
    LOG_STUN_DEBUG("Sending STUN request to " << server << ":" << port << " (" << request.size() << " bytes)");
    
    // Use the socket library function to send UDP data
    int bytes_sent = send_udp_data_to(sock, request, server, port);
    
    if (bytes_sent < 0) {
        LOG_STUN_ERROR("Failed to send STUN request to " << server << ":" << port);
        return false;
    }
    
    if (bytes_sent != static_cast<int>(request.size())) {
        LOG_STUN_ERROR("Partial STUN request sent: " << bytes_sent << "/" << request.size() << " bytes");
        return false;
    }
    
    LOG_STUN_DEBUG("Successfully sent STUN request to " << server << ":" << port << " (" << bytes_sent << " bytes)");
    return true;
}

bool StunClient::receive_stun_response(socket_t sock, 
                                      std::vector<uint8_t>& response,
                                      int timeout_ms) {
    LOG_STUN_DEBUG("Waiting for STUN response with timeout " << timeout_ms << "ms");
    
    std::string sender_ip;
    int sender_port;
    
    // Use the socket library function to receive UDP data with timeout
    response = receive_udp_data_with_timeout(sock, 1024, timeout_ms, &sender_ip, &sender_port);
    
    if (response.empty()) {
        LOG_STUN_ERROR("Failed to receive STUN response or timeout occurred");
        return false;
    }
    
    LOG_STUN_DEBUG("Received STUN response from " << sender_ip << ":" << sender_port 
                  << " (" << response.size() << " bytes)");
    
    return true;
}

// Helper functions for parsing binary data
uint16_t StunClient::parse_uint16(const uint8_t* data) {
    return (static_cast<uint16_t>(data[0]) << 8) | static_cast<uint16_t>(data[1]);
}

uint32_t StunClient::parse_uint32(const uint8_t* data) {
    return (static_cast<uint32_t>(data[0]) << 24) |
           (static_cast<uint32_t>(data[1]) << 16) |
           (static_cast<uint32_t>(data[2]) << 8) |
           static_cast<uint32_t>(data[3]);
}

void StunClient::write_uint16(uint8_t* data, uint16_t value) {
    data[0] = static_cast<uint8_t>((value >> 8) & 0xFF);
    data[1] = static_cast<uint8_t>(value & 0xFF);
}

void StunClient::write_uint32(uint8_t* data, uint32_t value) {
    data[0] = static_cast<uint8_t>((value >> 24) & 0xFF);
    data[1] = static_cast<uint8_t>((value >> 16) & 0xFF);
    data[2] = static_cast<uint8_t>((value >> 8) & 0xFF);
    data[3] = static_cast<uint8_t>(value & 0xFF);
}

void StunClient::xor_address(StunAddress& address, const uint8_t* transaction_id) {
    // For XOR-MAPPED-ADDRESS, we need to XOR the port with the first 16 bits of magic cookie
    // and the IP address with magic cookie
    
    // XOR port with first 16 bits of magic cookie
    uint16_t magic_cookie_high = (stun::MAGIC_COOKIE >> 16) & 0xFFFF;
    address.port ^= magic_cookie_high;
    
    // XOR IPv4 address with magic cookie
    if (address.family == stun::FAMILY_IPV4) {
        struct in_addr addr;
        if (inet_pton(AF_INET, address.ip.c_str(), &addr) == 1) {
            uint32_t ip_addr = ntohl(addr.s_addr);
            ip_addr ^= stun::MAGIC_COOKIE;
            addr.s_addr = htonl(ip_addr);
            
            char ip_str[INET_ADDRSTRLEN];
            if (inet_ntop(AF_INET, &addr, ip_str, INET_ADDRSTRLEN)) {
                address.ip = ip_str;
            }
        }
    }
    // TODO: Implement IPv6 XOR if needed (requires XOR with magic cookie + transaction ID)
}

// ICE-specific static helper function implementation
std::vector<uint8_t> StunClient::create_binding_request_ice(const std::string& username,
                                                          const std::string& password,
                                                          uint32_t priority,
                                                          bool controlling,
                                                          uint64_t tie_breaker,
                                                          bool use_candidate) {
    // Start with basic binding request
    std::vector<uint8_t> message = create_binding_request();
    
    // Add ICE-specific attributes
    size_t original_size = message.size();
    
    // USERNAME attribute (0x0006)
    if (!username.empty()) {
        uint16_t attr_type = 0x0006;
        uint16_t attr_length = username.length();
        
        message.push_back((attr_type >> 8) & 0xFF);
        message.push_back(attr_type & 0xFF);
        message.push_back((attr_length >> 8) & 0xFF);
        message.push_back(attr_length & 0xFF);
        
        for (char c : username) {
            message.push_back(static_cast<uint8_t>(c));
        }
        
        // Pad to 4-byte boundary
        while ((message.size() - original_size) % 4 != 0) {
            message.push_back(0);
        }
    }
    
    // PRIORITY attribute (0x0024)
    {
        uint16_t attr_type = 0x0024;
        uint16_t attr_length = 4;
        
        message.push_back((attr_type >> 8) & 0xFF);
        message.push_back(attr_type & 0xFF);
        message.push_back((attr_length >> 8) & 0xFF);
        message.push_back(attr_length & 0xFF);
        
        message.push_back((priority >> 24) & 0xFF);
        message.push_back((priority >> 16) & 0xFF);
        message.push_back((priority >> 8) & 0xFF);
        message.push_back(priority & 0xFF);
    }
    
    // ICE-CONTROLLING or ICE-CONTROLLED attribute
    {
        uint16_t attr_type = controlling ? 0x802A : 0x8029; // ICE-CONTROLLING : ICE-CONTROLLED
        uint16_t attr_length = 8;
        
        message.push_back((attr_type >> 8) & 0xFF);
        message.push_back(attr_type & 0xFF);
        message.push_back((attr_length >> 8) & 0xFF);
        message.push_back(attr_length & 0xFF);
        
        // Tie breaker value
        for (int i = 7; i >= 0; i--) {
            message.push_back((tie_breaker >> (i * 8)) & 0xFF);
        }
    }
    
    // USE-CANDIDATE attribute (0x0025) - only for controlling agent
    if (use_candidate && controlling) {
        uint16_t attr_type = 0x0025;
        uint16_t attr_length = 0; // No value for this attribute
        
        message.push_back((attr_type >> 8) & 0xFF);
        message.push_back(attr_type & 0xFF);
        message.push_back((attr_length >> 8) & 0xFF);
        message.push_back(attr_length & 0xFF);
    }
    
    // Update message length
    uint16_t total_length = message.size() - 20; // Exclude header
    message[2] = (total_length >> 8) & 0xFF;
    message[3] = total_length & 0xFF;
    
    return message;
}

// AdvancedNatDetector implementation
AdvancedNatDetector::AdvancedNatDetector() {
    stun_client_ = std::make_unique<StunClient>();
    LOG_STUN_DEBUG("AdvancedNatDetector created");
}

AdvancedNatDetector::~AdvancedNatDetector() {
    LOG_STUN_DEBUG("AdvancedNatDetector destroyed");
}

NatTypeInfo AdvancedNatDetector::detect_nat_characteristics(const std::vector<std::string>& stun_servers,
                                                           int timeout_ms) {
    NatTypeInfo info;
    
    if (stun_servers.empty()) {
        LOG_STUN_WARN("No STUN servers provided for NAT detection");
        info.has_nat = true; // Assume NAT if we can't test
        info.filtering_behavior = NatBehavior::UNKNOWN;
        info.mapping_behavior = NatBehavior::UNKNOWN;
        info.preserves_port = false;
        info.hairpin_support = false;
        info.description = "No STUN servers available";
        return info;
    }
    
    // Basic implementation - test if we can reach STUN server
    StunAddress public_address;
    
    if (stun_client_->get_public_address(stun_servers[0], 3478, public_address, timeout_ms)) {
        // Simple NAT type detection - this is a minimal implementation
        info.has_nat = true; // Assume NAT for now
        info.filtering_behavior = NatBehavior::ENDPOINT_INDEPENDENT;
        info.mapping_behavior = NatBehavior::ENDPOINT_INDEPENDENT;
        info.preserves_port = false;
        info.hairpin_support = false;
        info.description = "NAT detected via STUN";
    } else {
        info.has_nat = false;
        info.filtering_behavior = NatBehavior::ENDPOINT_INDEPENDENT;
        info.mapping_behavior = NatBehavior::ENDPOINT_INDEPENDENT;
        info.preserves_port = true;
        info.hairpin_support = true;
        info.description = "Open internet connection or STUN failure";
    }
    
    return info;
}

bool AdvancedNatDetector::test_hairpin_support(const std::string& stun_server, int timeout_ms) {
    // Simplified implementation - return false for now
    LOG_STUN_DEBUG("Testing hairpin support (not fully implemented)");
    return false;
}

bool AdvancedNatDetector::test_port_preservation(const std::vector<std::string>& stun_servers, int timeout_ms) {
    // Simplified implementation - return false for now
    LOG_STUN_DEBUG("Testing port preservation (not fully implemented)");
    return false;
}

NatBehavior AdvancedNatDetector::test_filtering_behavior(const std::vector<std::string>& stun_servers, int timeout_ms) {
    // Simplified implementation - return endpoint independent for now
    LOG_STUN_DEBUG("Testing filtering behavior (not fully implemented)");
    return NatBehavior::ENDPOINT_INDEPENDENT;
}

NatBehavior AdvancedNatDetector::test_mapping_behavior(const std::vector<std::string>& stun_servers, int timeout_ms) {
    // Simplified implementation - return endpoint independent for now
    LOG_STUN_DEBUG("Testing mapping behavior (not fully implemented)");
    return NatBehavior::ENDPOINT_INDEPENDENT;
}

} // namespace librats 