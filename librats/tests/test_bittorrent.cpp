#include <gtest/gtest.h>
#include "bittorrent.h"
#include "librats.h"
#include <memory>
#include <thread>
#include <chrono>

class BitTorrentTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize socket library for testing
        librats::init_socket_library();
    }
    
    void TearDown() override {
        librats::cleanup_socket_library();
    }
};

// Test TorrentInfo creation and basic functionality
TEST_F(BitTorrentTest, TorrentInfoCreation) {
    librats::TorrentInfo torrent_info;
    
    // Initially should be invalid
    EXPECT_FALSE(torrent_info.is_valid());
    EXPECT_EQ(torrent_info.get_name(), "");
    EXPECT_EQ(torrent_info.get_total_length(), 0);
    EXPECT_EQ(torrent_info.get_num_pieces(), 0);
}

// Test BitTorrent client creation
TEST_F(BitTorrentTest, BitTorrentClientCreation) {
    auto bt_client = std::make_unique<librats::BitTorrentClient>();
    
    EXPECT_FALSE(bt_client->is_running());
    EXPECT_EQ(bt_client->get_active_torrents_count(), 0);
    EXPECT_EQ(bt_client->get_total_downloaded(), 0);
    EXPECT_EQ(bt_client->get_total_uploaded(), 0);
}

// Test BitTorrent client start/stop
TEST_F(BitTorrentTest, BitTorrentClientStartStop) {
    auto bt_client = std::make_unique<librats::BitTorrentClient>();
    
    // Start on a high port to avoid conflicts
    EXPECT_TRUE(bt_client->start(58881));
    EXPECT_TRUE(bt_client->is_running());
    
    bt_client->stop();
    EXPECT_FALSE(bt_client->is_running());
}

// Test RatsClient BitTorrent integration
TEST_F(BitTorrentTest, RatsClientBitTorrentIntegration) {
    librats::RatsClient client(58080);  // Use high port to avoid conflicts
    
    // BitTorrent should be disabled by default
    EXPECT_FALSE(client.is_bittorrent_enabled());
    EXPECT_EQ(client.get_active_torrents_count(), 0);
    
    // Start the client
    EXPECT_TRUE(client.start());
    
    // Enable BitTorrent
    EXPECT_TRUE(client.enable_bittorrent(58882));
    EXPECT_TRUE(client.is_bittorrent_enabled());
    
    // Get stats (should be zero)
    auto stats = client.get_bittorrent_stats();
    EXPECT_EQ(stats.first, 0);   // downloaded
    EXPECT_EQ(stats.second, 0);  // uploaded
    
    // Disable BitTorrent
    client.disable_bittorrent();
    EXPECT_FALSE(client.is_bittorrent_enabled());
    
    client.stop();
}

// Test PeerMessage creation and serialization
TEST_F(BitTorrentTest, PeerMessageCreation) {
    // Test simple messages
    auto choke_msg = librats::PeerMessage::create_choke();
    EXPECT_EQ(choke_msg.type, librats::MessageType::CHOKE);
    EXPECT_TRUE(choke_msg.payload.empty());
    
    auto unchoke_msg = librats::PeerMessage::create_unchoke();
    EXPECT_EQ(unchoke_msg.type, librats::MessageType::UNCHOKE);
    EXPECT_TRUE(unchoke_msg.payload.empty());
    
    // Test have message
    auto have_msg = librats::PeerMessage::create_have(42);
    EXPECT_EQ(have_msg.type, librats::MessageType::HAVE);
    EXPECT_EQ(have_msg.payload.size(), 4);
    
    // Test serialization
    auto serialized = choke_msg.serialize();
    EXPECT_EQ(serialized.size(), 5);  // 4 bytes length + 1 byte message type
}

// Test utility functions
TEST_F(BitTorrentTest, UtilityFunctions) {
    // Test peer ID generation
    auto peer_id = librats::generate_peer_id();
    EXPECT_EQ(peer_id.size(), 20);
    
    // Should start with our client identifier
    std::string prefix(peer_id.begin(), peer_id.begin() + 8);
    EXPECT_EQ(prefix, "-LR0001-");
    
    // Test info hash conversion
    librats::InfoHash test_hash;
    test_hash.fill(0xAB);  // Fill with test pattern
    
    std::string hex = librats::info_hash_to_hex(test_hash);
    EXPECT_EQ(hex.length(), 40);  // 20 bytes * 2 hex chars
    
    auto converted_back = librats::hex_to_info_hash(hex);
    EXPECT_EQ(test_hash, converted_back);
}

// Test handshake message creation and parsing
TEST_F(BitTorrentTest, HandshakeMessages) {
    librats::InfoHash info_hash;
    info_hash.fill(0x12);  // Test pattern
    
    librats::PeerID peer_id = librats::generate_peer_id();
    
    // Create handshake
    auto handshake = librats::create_handshake_message(info_hash, peer_id);
    EXPECT_EQ(handshake.size(), 68);  // Fixed handshake size
    
    // Parse handshake
    librats::InfoHash parsed_hash;
    librats::PeerID parsed_peer_id;
    
    EXPECT_TRUE(librats::parse_handshake_message(handshake, parsed_hash, parsed_peer_id));
    EXPECT_EQ(info_hash, parsed_hash);
    EXPECT_EQ(peer_id, parsed_peer_id);
}

// Test multiple BitTorrent clients (no conflicts)
TEST_F(BitTorrentTest, MultipleBitTorrentClients) {
    auto client1 = std::make_unique<librats::BitTorrentClient>();
    auto client2 = std::make_unique<librats::BitTorrentClient>();
    
    // Start on different ports
    EXPECT_TRUE(client1->start(58883));
    EXPECT_TRUE(client2->start(58884));
    
    EXPECT_TRUE(client1->is_running());
    EXPECT_TRUE(client2->is_running());
    
    // Stop them
    client1->stop();
    client2->stop();
    
    EXPECT_FALSE(client1->is_running());
    EXPECT_FALSE(client2->is_running());
}

// Test RatsClient with BitTorrent disabled operations
TEST_F(BitTorrentTest, BitTorrentDisabledOperations) {
    librats::RatsClient client(58085);
    
    EXPECT_TRUE(client.start());
    
    // All BitTorrent operations should fail gracefully when disabled
    EXPECT_FALSE(client.is_bittorrent_enabled());
    
    // These should return empty/null results
    auto torrents = client.get_all_torrents();
    EXPECT_TRUE(torrents.empty());
    
    auto stats = client.get_bittorrent_stats();
    EXPECT_EQ(stats.first, 0);
    EXPECT_EQ(stats.second, 0);
    
    EXPECT_EQ(client.get_active_torrents_count(), 0);
    
    // These should return null/false
    librats::InfoHash dummy_hash;
    dummy_hash.fill(0);
    
    auto torrent = client.get_torrent(dummy_hash);
    EXPECT_EQ(torrent, nullptr);
    
    EXPECT_FALSE(client.remove_torrent(dummy_hash));
    
    client.stop();
} 