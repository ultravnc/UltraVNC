#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "socket.h"
#include <thread>
#include <chrono>

using namespace librats;

class SocketTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(init_socket_library());
    }
    
    void TearDown() override {
        cleanup_socket_library();
    }
};

// Test socket library initialization
TEST_F(SocketTest, InitializationTest) {
    // Should already be initialized in SetUp
    EXPECT_TRUE(true);
}

// Test Peer structure
TEST_F(SocketTest, PeerTest) {
    Peer peer1("127.0.0.1", 8080);
    EXPECT_EQ(peer1.ip, "127.0.0.1");
    EXPECT_EQ(peer1.port, 8080);
    
    Peer peer2("127.0.0.1", 8080);
    EXPECT_EQ(peer1, peer2);
    
    Peer peer3("127.0.0.1", 8081);
    EXPECT_NE(peer1, peer3);
}

// Test socket validity check
TEST_F(SocketTest, SocketValidityTest) {
    socket_t valid_socket = create_tcp_server(0);  // Use port 0 for automatic port assignment
    EXPECT_TRUE(is_valid_socket(valid_socket));
    
    close_socket(valid_socket);
    
    socket_t invalid_socket = INVALID_SOCKET_VALUE;
    EXPECT_FALSE(is_valid_socket(invalid_socket));
}

// Test TCP server creation
TEST_F(SocketTest, TCPServerCreationTest) {
    // Test dual-stack server (default)
    socket_t server = create_tcp_server(0);
    EXPECT_TRUE(is_valid_socket(server));
    close_socket(server);
    
    // Test IPv4 server
    socket_t server_v4 = create_tcp_server_v4(0);
    EXPECT_TRUE(is_valid_socket(server_v4));
    close_socket(server_v4);
    
    // Test IPv6 server
    socket_t server_v6 = create_tcp_server_v6(0);
    EXPECT_TRUE(is_valid_socket(server_v6));
    close_socket(server_v6);
}

// Test UDP socket creation
TEST_F(SocketTest, UDPSocketCreationTest) {
    // Test dual-stack UDP socket
    socket_t udp_socket = create_udp_socket(0);
    EXPECT_TRUE(is_valid_socket(udp_socket));
    close_socket(udp_socket);
    
    // Test IPv4 UDP socket
    socket_t udp_v4 = create_udp_socket_v4(0);
    EXPECT_TRUE(is_valid_socket(udp_v4));
    close_socket(udp_v4);
    
    // Test IPv6 UDP socket
    socket_t udp_v6 = create_udp_socket_v6(0);
    EXPECT_TRUE(is_valid_socket(udp_v6));
    close_socket(udp_v6);
}

// Test TCP client-server communication
TEST_F(SocketTest, TCPClientServerCommunicationTest) {
    // Create server
    socket_t server = create_tcp_server(0);
    ASSERT_TRUE(is_valid_socket(server));
    
    // Get the actual port the server is listening on
    sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    ASSERT_EQ(getsockname(server, (sockaddr*)&addr, &addr_len), 0);
    
    int port = 0;
    if (addr.ss_family == AF_INET) {
        port = ntohs(((sockaddr_in*)&addr)->sin_port);
    } else if (addr.ss_family == AF_INET6) {
        port = ntohs(((sockaddr_in6*)&addr)->sin6_port);
    }
    
    ASSERT_GT(port, 0);
    
    // Test connection in separate thread
    std::thread server_thread([&]() {
        socket_t client = accept_client(server);
        EXPECT_TRUE(is_valid_socket(client));
        
        if (is_valid_socket(client)) {
            std::string received = receive_tcp_string(client);
            EXPECT_EQ(received, "Hello Server!");
            
            int sent = send_tcp_string(client, "Hello Client!");
            EXPECT_GT(sent, 0);
            
            close_socket(client);
        }
    });
    
    // Give server time to start listening
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Create client and connect
    socket_t client = create_tcp_client("127.0.0.1", port);
    ASSERT_TRUE(is_valid_socket(client));
    
    // Send data
    int sent = send_tcp_string(client, "Hello Server!");
    EXPECT_GT(sent, 0);
    
    // Receive response
    std::string response = receive_tcp_string(client);
    EXPECT_EQ(response, "Hello Client!");
    
    close_socket(client);
    server_thread.join();
    close_socket(server);
}

// Test get peer address
TEST_F(SocketTest, GetPeerAddressTest) {
    socket_t server = create_tcp_server(0);
    ASSERT_TRUE(is_valid_socket(server));
    
    sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    ASSERT_EQ(getsockname(server, (sockaddr*)&addr, &addr_len), 0);
    
    int port = 0;
    if (addr.ss_family == AF_INET) {
        port = ntohs(((sockaddr_in*)&addr)->sin_port);
    } else if (addr.ss_family == AF_INET6) {
        port = ntohs(((sockaddr_in6*)&addr)->sin6_port);
    }
    
    std::thread server_thread([&]() {
        socket_t client = accept_client(server);
        if (is_valid_socket(client)) {
            std::string peer_addr = get_peer_address(client);
            EXPECT_FALSE(peer_addr.empty());
            EXPECT_THAT(peer_addr, testing::HasSubstr("127.0.0.1"));
            close_socket(client);
        }
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    socket_t client = create_tcp_client("127.0.0.1", port);
    ASSERT_TRUE(is_valid_socket(client));
    
    close_socket(client);
    server_thread.join();
    close_socket(server);
}

// Test non-blocking socket
TEST_F(SocketTest, NonBlockingSocketTest) {
    socket_t socket = create_tcp_server(0);
    ASSERT_TRUE(is_valid_socket(socket));
    
    bool result = set_socket_nonblocking(socket);
    EXPECT_TRUE(result);
    
    close_socket(socket);
}

// Test socket close
TEST_F(SocketTest, SocketCloseTest) {
    socket_t socket = create_tcp_server(0);
    ASSERT_TRUE(is_valid_socket(socket));
    
    close_socket(socket);
    
    // After closing, socket should be invalid
    // Note: We can't easily test this without platform-specific code
    // as the socket value might be reused
}

// Test invalid operations
TEST_F(SocketTest, InvalidOperationsTest) {
    socket_t invalid_socket = INVALID_SOCKET_VALUE;
    
    // Test sending to invalid socket
    int result = send_tcp_string(invalid_socket, "test");
    EXPECT_LE(result, 0);  // Should fail
    
    // Test receiving from invalid socket
    std::string data = receive_tcp_string(invalid_socket);
    EXPECT_TRUE(data.empty());  // Should return empty string
    
    // Test getting peer address from invalid socket
    std::string peer_addr = get_peer_address(invalid_socket);
    EXPECT_TRUE(peer_addr.empty());  // Should return empty string
}

// Test edge cases
TEST_F(SocketTest, EdgeCasesTest) {
    // Test creating server on invalid port
    socket_t server = create_tcp_server(-1);
    EXPECT_FALSE(is_valid_socket(server));
    
    // Test creating client with invalid host
    socket_t client = create_tcp_client("invalid.host.example", 80);
    EXPECT_FALSE(is_valid_socket(client));
    
    // Test creating client with invalid port
    socket_t client2 = create_tcp_client("127.0.0.1", -1);
    EXPECT_FALSE(is_valid_socket(client2));
} 