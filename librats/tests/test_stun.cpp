#include <gtest/gtest.h>
#include "../src/stun.h"
#include "../src/socket.h"
#include "../src/librats.h"
#include <iostream>

using namespace librats;

class StunTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize socket library for tests
        init_socket_library();
    }
    
    void TearDown() override {
        // Cleanup socket library after tests
        cleanup_socket_library();
    }
};

TEST_F(StunTest, CreateStunClient) {
    StunClient client;
    // Test that we can create a STUN client without errors
    EXPECT_TRUE(true); // Basic creation test
}

TEST_F(StunTest, CreateBindingRequest) {
    auto request = StunClient::create_binding_request();
    
    // Verify basic structure
    EXPECT_EQ(request.size(), 20); // STUN header is 20 bytes
    
    // Verify message type (first 2 bytes, big endian)
    uint16_t message_type = (static_cast<uint16_t>(request[0]) << 8) | static_cast<uint16_t>(request[1]);
    EXPECT_EQ(message_type, 0x0001); // BINDING_REQUEST
    
    // Verify message length (next 2 bytes, big endian) - should be 0 for basic request
    uint16_t message_length = (static_cast<uint16_t>(request[2]) << 8) | static_cast<uint16_t>(request[3]);
    EXPECT_EQ(message_length, 0);
    
    // Verify magic cookie (next 4 bytes, big endian)
    uint32_t magic_cookie = (static_cast<uint32_t>(request[4]) << 24) |
                           (static_cast<uint32_t>(request[5]) << 16) |
                           (static_cast<uint32_t>(request[6]) << 8) |
                           static_cast<uint32_t>(request[7]);
    EXPECT_EQ(magic_cookie, 0x2112A442);
}

TEST_F(StunTest, GoogleStunDiscovery) {
    StunClient client;
    StunAddress public_address;
    
    // Test Google STUN server (this is a real network test)
    // Note: This test may fail if there's no internet connection
    bool result = client.get_public_address_from_google(public_address, 10000); // 10 second timeout
    
    if (result) {
        // If successful, verify we got valid data
        EXPECT_FALSE(public_address.ip.empty());
        EXPECT_GT(public_address.port, 0);
        EXPECT_EQ(public_address.family, 1); // IPv4
        
        std::cout << "Discovered public IP: " << public_address.ip << ":" << public_address.port << std::endl;
    } else {
        // If failed, log it but don't fail the test (could be network issues)
        std::cout << "STUN discovery failed (this is normal if no internet connection)" << std::endl;
        GTEST_SKIP() << "Skipping STUN test due to network connectivity issues";
    }
}

TEST_F(StunTest, ParseValidStunResponse) {
    // Create a mock STUN response with XOR-MAPPED-ADDRESS
    std::vector<uint8_t> mock_response = {
        // STUN Header
        0x01, 0x01,             // Message Type: Binding Response
        0x00, 0x0C,             // Message Length: 12 bytes (one attribute)
        0x21, 0x12, 0xA4, 0x42, // Magic Cookie
        0x12, 0x34, 0x56, 0x78, // Transaction ID (12 bytes)
        0x9A, 0xBC, 0xDE, 0xF0,
        0x11, 0x22, 0x33, 0x44,
        
        // XOR-MAPPED-ADDRESS Attribute
        0x00, 0x20,             // Attribute Type: XOR-MAPPED-ADDRESS
        0x00, 0x08,             // Attribute Length: 8 bytes
        0x00, 0x01,             // Reserved + Family (IPv4)
        0x12, 0x34,             // Port (XORed with magic cookie high bits)
        0x12, 0x34, 0x56, 0x78  // IPv4 Address (XORed with magic cookie)
    };
    
    StunAddress mapped_address;
    bool result = StunClient::parse_binding_response(mock_response, mapped_address);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(mapped_address.family, 1); // IPv4
    // The actual values will be XORed, so we mainly test that parsing succeeds
}

TEST_F(StunTest, ParseInvalidStunResponse) {
    // Test with too short response
    std::vector<uint8_t> short_response = {0x01, 0x01}; // Only 2 bytes
    
    StunAddress mapped_address;
    bool result = StunClient::parse_binding_response(short_response, mapped_address);
    
    EXPECT_FALSE(result);
}

TEST_F(StunTest, SocketLibraryIntegration) {
    // Test that STUN works with the socket library functions
    StunClient client;
    
    // Create a UDP socket using the socket library
    socket_t test_socket = create_udp_socket_v4(0);
    EXPECT_TRUE(is_valid_socket(test_socket));
    
    if (is_valid_socket(test_socket)) {
        // Test sending data to a STUN server using socket library
        std::vector<uint8_t> test_request = StunClient::create_binding_request();
        
        // Try to send to Google STUN server
        int result = send_udp_data_to(test_socket, test_request, "stun.l.google.com", 19302);
        
        if (result > 0) {
            std::cout << "Successfully sent STUN request using socket library" << std::endl;
            
            // Try to receive response with timeout
            std::string sender_ip;
            int sender_port;
            auto response = receive_udp_data_with_timeout(test_socket, 1024, 5000, &sender_ip, &sender_port);
            
            if (!response.empty()) {
                std::cout << "Received STUN response from " << sender_ip << ":" << sender_port 
                         << " (" << response.size() << " bytes)" << std::endl;
                
                // Try to parse the response
                StunAddress mapped_address;
                bool parse_result = StunClient::parse_binding_response(response, mapped_address);
                if (parse_result) {
                    std::cout << "Parsed public address: " << mapped_address.ip << ":" << mapped_address.port << std::endl;
                }
            } else {
                std::cout << "No STUN response received (timeout or network issue)" << std::endl;
            }
        } else {
            std::cout << "Failed to send STUN request (network issue)" << std::endl;
        }
        
        close_socket(test_socket);
    }
}

TEST_F(StunTest, RatsClientStunIntegration) {
    // Test that RatsClient can discover public IP
    RatsClient client(12345); // Use a test port
    
    // Test manual STUN discovery
    bool result = client.discover_and_ignore_public_ip();
    
    if (result) {
        std::string public_ip = client.get_public_ip();
        EXPECT_FALSE(public_ip.empty());
        std::cout << "RatsClient discovered public IP: " << public_ip << std::endl;
    } else {
        std::cout << "RatsClient STUN discovery failed (this is normal if no internet connection)" << std::endl;
        GTEST_SKIP() << "Skipping RatsClient STUN test due to network connectivity issues";
    }
} 