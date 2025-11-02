#include "../src/librats.h"
#include "../src/fs.h"
#include "../src/json.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>

using namespace librats;

class ConfigPersistenceTest : public ::testing::Test {
protected:
    void clean_test_files() {
        // Old files
        if (file_or_directory_exists("config.json")) delete_file("config.json");
        if (file_or_directory_exists("peers.rats")) delete_file("peers.rats");

        // New port-specific files used in tests
        std::vector<int> ports = {8888, 8889, 8890, 8891, 8892};
        for (int port : ports) {
            std::string config_file = "config_" + std::to_string(port) + ".json";
            std::string peers_file = "peers_" + std::to_string(port) + ".json";
            if (file_or_directory_exists(config_file)) delete_file(config_file.c_str());
            if (file_or_directory_exists(peers_file)) delete_file(peers_file.c_str());
        }
    }

    void SetUp() override {
        // Clean up any existing config files before each test
        clean_test_files();
    }
    
    void TearDown() override {
        // Clean up after each test
        clean_test_files();
    }
};

TEST_F(ConfigPersistenceTest, PeerIdGenerationAndPersistence) {
    const int port = 8888;
    const std::string config_file = "config_" + std::to_string(port) + ".json";

    // Create first client instance
    std::string first_peer_id;
    {
        RatsClient client1(port, 5);
        first_peer_id = client1.get_our_peer_id();
        EXPECT_FALSE(first_peer_id.empty());
        EXPECT_EQ(first_peer_id.length(), 40); // SHA1 hash length
    }
    
    // Create second client instance - should load the same peer ID
    {
        RatsClient client2(port, 5);
        std::string second_peer_id = client2.get_our_peer_id();
        EXPECT_EQ(second_peer_id, first_peer_id);
    }
    
    // Verify config file was created
    EXPECT_TRUE(file_or_directory_exists(config_file));
    
    // Parse and verify config file content
    std::string config_data = read_file_text_cpp(config_file);
    EXPECT_FALSE(config_data.empty());
    
    nlohmann::json config = nlohmann::json::parse(config_data);
    EXPECT_TRUE(config.contains("peer_id"));
    EXPECT_EQ(config["peer_id"], first_peer_id);
    EXPECT_TRUE(config.contains("version"));
    EXPECT_TRUE(config.contains("listen_port"));
    EXPECT_EQ(config["listen_port"], port);
}

/*
TEST_F(ConfigPersistenceTest, PeerSerialization) {
    // Create two clients on different ports
    RatsClient client1(8889, 5);
    RatsClient client2(8890, 5);
    
    // Start both clients
    ASSERT_TRUE(client1.start());
    ASSERT_TRUE(client2.start());
    
    // Give them time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Connect client1 to client2
    bool connected = client1.connect_to_peer("127.0.0.1", 8890);
    EXPECT_TRUE(connected);
    
    // Give time for handshake to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // Check that they are connected
    EXPECT_GE(client1.get_peer_count(), 1);
    EXPECT_GE(client2.get_peer_count(), 1);
    
    // Stop the clients (this should save peers)
    client1.stop();
    client2.stop();
    
    // Give time for cleanup
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Verify peers.rats file was created
    EXPECT_TRUE(file_or_directory_exists("peers_8889.json") || file_or_directory_exists("peers_8890.json"));
    
    // For this test, we just check one of them.
    // In a real scenario, both clients would save their peer lists.
    std::string peers_file_to_check;
    if (file_or_directory_exists("peers_8889.json")) {
        peers_file_to_check = "peers_8889.json";
    } else {
        peers_file_to_check = "peers_8890.json";
    }

    // Parse and verify peers file content
    std::string peers_data = read_file_text_cpp(peers_file_to_check);
    EXPECT_FALSE(peers_data.empty());
    
    nlohmann::json peers = nlohmann::json::parse(peers_data);
    EXPECT_TRUE(peers.is_array());
    
    if (peers.size() > 0) {
        // Check first peer has required fields
        const auto& peer = peers[0];
        EXPECT_TRUE(peer.contains("ip"));
        EXPECT_TRUE(peer.contains("port"));
        EXPECT_TRUE(peer.contains("peer_id"));
        EXPECT_TRUE(peer.contains("normalized_address"));
        EXPECT_TRUE(peer.contains("last_seen"));
    }
}
*/

TEST_F(ConfigPersistenceTest, PeerReconnectionAttempt) {
    const int client_port = 8892;
    const int peer_port = 8891;
    const std::string peers_file = "peers_" + std::to_string(client_port) + ".json";

    // Create a mock peers.rats file with a test peer
    nlohmann::json peers = nlohmann::json::array();
    nlohmann::json test_peer;
    test_peer["ip"] = "127.0.0.1";
    test_peer["port"] = peer_port;
    test_peer["peer_id"] = "test_peer_id_for_reconnection_test";
    test_peer["normalized_address"] = "127.0.0.1:" + std::to_string(peer_port);
    test_peer["is_outgoing"] = true;
    test_peer["version"] = "1.0";
    
    auto now = std::chrono::high_resolution_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    test_peer["last_seen"] = timestamp;
    
    peers.push_back(test_peer);
    
    std::string peers_data = peers.dump(4);
    ASSERT_TRUE(create_file(peers_file, peers_data));
    
    // Create a client that should attempt to reconnect
    RatsClient client(client_port, 5);
    ASSERT_TRUE(client.start());
    
    // Give time for reconnection attempts
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    client.stop();
    
    // Test passes if no crashes occur (reconnection will fail since no server on 8891, but that's expected)
    SUCCEED();
} 