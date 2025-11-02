#include <gtest/gtest.h>
#include "librats.h"
#include "ice.h"
#include "stun.h"
#include <thread>
#include <chrono>

class IceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize socket library for network operations
        ASSERT_TRUE(librats::init_socket_library()) << "Failed to initialize socket library";
    }
    
    void TearDown() override {
        // Clean up socket library
        librats::cleanup_socket_library();
    }
};

TEST_F(IceTest, IceCandidateSerializationTest) {
    // Test ICE candidate JSON serialization/deserialization
    librats::IceCandidate candidate;
    candidate.foundation = "1";
    candidate.component_id = 1;
    candidate.transport = librats::IceTransport::UDP;
    candidate.priority = 65535;
    candidate.ip = "192.168.1.100";
    candidate.port = 8080;
    candidate.type = librats::IceCandidateType::HOST;
    candidate.ufrag = "test_ufrag";
    candidate.pwd = "test_password";
    
    // Test JSON conversion
    auto json = candidate.to_json();
    auto restored = librats::IceCandidate::from_json(json);
    
    EXPECT_EQ(candidate.foundation, restored.foundation);
    EXPECT_EQ(candidate.component_id, restored.component_id);
    EXPECT_EQ(candidate.transport, restored.transport);
    EXPECT_EQ(candidate.priority, restored.priority);
    EXPECT_EQ(candidate.ip, restored.ip);
    EXPECT_EQ(candidate.port, restored.port);
    EXPECT_EQ(candidate.type, restored.type);
    EXPECT_EQ(candidate.ufrag, restored.ufrag);
    EXPECT_EQ(candidate.pwd, restored.pwd);
}

TEST_F(IceTest, IceCandidateSdpTest) {
    // Test ICE candidate SDP format
    librats::IceCandidate candidate;
    candidate.foundation = "1";
    candidate.component_id = 1;
    candidate.transport = librats::IceTransport::UDP;
    candidate.priority = 65535;
    candidate.ip = "192.168.1.100";
    candidate.port = 8080;
    candidate.type = librats::IceCandidateType::HOST;
    
    std::string sdp = candidate.to_sdp();
    EXPECT_TRUE(sdp.find("candidate:1 1 udp 65535 192.168.1.100 8080 typ host") != std::string::npos);
    
    // Test parsing SDP back
    auto parsed = librats::IceCandidate::from_sdp(sdp);
    EXPECT_EQ(candidate.foundation, parsed.foundation);
    EXPECT_EQ(candidate.component_id, parsed.component_id);
    EXPECT_EQ(candidate.ip, parsed.ip);
    EXPECT_EQ(candidate.port, parsed.port);
}

TEST_F(IceTest, IceCandidatePairPriorityTest) {
    // Test candidate pair priority calculation
    librats::IceCandidate local;
    local.priority = 65535;
    
    librats::IceCandidate remote;
    remote.priority = 65534;
    
    librats::IceCandidatePair pair(local, remote);
    
    // Priority should be calculated based on the higher and lower priorities
    EXPECT_GT(pair.priority, 0);
    EXPECT_EQ(pair.calculate_priority(), pair.priority);
}

TEST_F(IceTest, IceConfigurationTest) {
    // Test ICE configuration with default values
    librats::IceConfig config;
    
    EXPECT_TRUE(config.enable_host_candidates);
    EXPECT_TRUE(config.enable_server_reflexive_candidates);
    EXPECT_TRUE(config.enable_relay_candidates);
    EXPECT_FALSE(config.enable_tcp_candidates);
    EXPECT_EQ(config.stun_timeout_ms, 5000);
    EXPECT_GT(config.stun_servers.size(), 0);
    
    // Test configuration modification
    config.enable_tcp_candidates = true;
    config.stun_timeout_ms = 10000;
    
    EXPECT_TRUE(config.enable_tcp_candidates);
    EXPECT_EQ(config.stun_timeout_ms, 10000);
}

TEST_F(IceTest, IceAgentBasicTest) {
    // Test ICE agent creation and basic functionality
    librats::IceConfig config;
    librats::IceAgent agent(librats::IceRole::CONTROLLING, config);
    
    // Test initial state
    EXPECT_EQ(agent.get_role(), librats::IceRole::CONTROLLING);
    EXPECT_EQ(agent.get_connection_state(), librats::IceConnectionState::NEW);
    EXPECT_FALSE(agent.is_running());
    EXPECT_FALSE(agent.is_connected());
    
    // Test credential generation
    auto credentials = agent.get_local_credentials();
    EXPECT_FALSE(credentials.first.empty()); // ufrag
    EXPECT_FALSE(credentials.second.empty()); // pwd
}

TEST_F(IceTest, IceAgentStartStopTest) {
    // Test ICE agent lifecycle
    librats::IceConfig config;
    librats::IceAgent agent(librats::IceRole::CONTROLLING, config);
    
    // Test start
    bool started = agent.start();
    EXPECT_TRUE(started);
    EXPECT_TRUE(agent.is_running());
    
    // Give it a moment to initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Test stop
    agent.stop();
    EXPECT_FALSE(agent.is_running());
    EXPECT_EQ(agent.get_connection_state(), librats::IceConnectionState::CLOSED);
}

TEST_F(IceTest, NatTypeDetectorTest) {
    // Test NAT type detector
    librats::NatTypeDetector detector;
    
    // Test with empty STUN servers (should return UNKNOWN)
    std::vector<std::string> empty_servers;
    auto nat_type = detector.detect_nat_type(empty_servers, 1000);
    EXPECT_EQ(nat_type, librats::NatType::UNKNOWN);
    
    // Test string conversion
    std::string type_str = detector.nat_type_to_string(librats::NatType::OPEN_INTERNET);
    EXPECT_EQ(type_str, "Open Internet");
    
    type_str = detector.nat_type_to_string(librats::NatType::SYMMETRIC);
    EXPECT_EQ(type_str, "Symmetric NAT");
}

TEST_F(IceTest, UtilityFunctionsTest) {
    // Test utility functions
    EXPECT_EQ(librats::ice_candidate_type_to_string(librats::IceCandidateType::HOST), "host");
    EXPECT_EQ(librats::ice_candidate_type_to_string(librats::IceCandidateType::SERVER_REFLEXIVE), "srflx");
    EXPECT_EQ(librats::ice_candidate_type_to_string(librats::IceCandidateType::RELAY), "relay");
    
    EXPECT_EQ(librats::string_to_ice_candidate_type("host"), librats::IceCandidateType::HOST);
    EXPECT_EQ(librats::string_to_ice_candidate_type("srflx"), librats::IceCandidateType::SERVER_REFLEXIVE);
    EXPECT_EQ(librats::string_to_ice_candidate_type("relay"), librats::IceCandidateType::RELAY);
    
    EXPECT_EQ(librats::ice_transport_to_string(librats::IceTransport::UDP), "udp");
    EXPECT_EQ(librats::ice_transport_to_string(librats::IceTransport::TCP), "tcp");
    
    EXPECT_EQ(librats::string_to_ice_transport("udp"), librats::IceTransport::UDP);
    EXPECT_EQ(librats::string_to_ice_transport("tcp"), librats::IceTransport::TCP);
    
    EXPECT_EQ(librats::ice_connection_state_to_string(librats::IceConnectionState::NEW), "new");
    EXPECT_EQ(librats::ice_connection_state_to_string(librats::IceConnectionState::CHECKING), "checking");
    EXPECT_EQ(librats::ice_connection_state_to_string(librats::IceConnectionState::CONNECTED), "connected");
}

TEST_F(IceTest, IceDescriptionTest) {
    // Test ICE local/remote description creation
    librats::IceConfig config;
    librats::IceAgent agent(librats::IceRole::CONTROLLING, config);
    
    EXPECT_TRUE(agent.start());
    
    // Get local description
    auto local_desc = agent.get_local_description();
    EXPECT_TRUE(local_desc.contains("ufrag"));
    EXPECT_TRUE(local_desc.contains("pwd"));
    EXPECT_TRUE(local_desc.contains("candidates"));
    
    // Test connection offer/answer
    auto offer = agent.create_connection_offer();
    EXPECT_TRUE(offer.contains("type"));
    EXPECT_TRUE(offer.contains("ice"));
    EXPECT_EQ(offer["type"], "offer");
    
    auto answer = agent.create_connection_answer(offer);
    EXPECT_TRUE(answer.contains("type"));
    EXPECT_TRUE(answer.contains("ice"));
    EXPECT_EQ(answer["type"], "answer");
    
    agent.stop();
}

// Test with minimal STUN functionality
TEST_F(IceTest, StunIntegrationTest) {
    // Test STUN client integration with ICE
    librats::StunClient stun_client;
    
    // Test STUN message creation
    auto binding_request = librats::StunClient::create_binding_request();
    EXPECT_GE(binding_request.size(), 20); // At least header size
    
    // Test ICE-specific STUN message creation
    auto ice_request = librats::StunClient::create_binding_request_ice(
        "test_user", "test_pass", 65535, true, 12345);
    EXPECT_GT(ice_request.size(), binding_request.size()); // Should have additional attributes
}

TEST_F(IceTest, TurnClientBasicTest) {
    // Test TURN client basic functionality (without actual server)
    librats::TurnClient turn_client("turn.example.com", 3478, "username", "password");
    
    // Test basic state
    EXPECT_FALSE(turn_client.is_allocated());
    EXPECT_EQ(turn_client.get_allocated_ip(), "");
    EXPECT_EQ(turn_client.get_allocated_port(), 0);
    
    // Test data send without allocation (should fail gracefully)
    std::vector<uint8_t> test_data = {1, 2, 3, 4, 5};
    EXPECT_FALSE(turn_client.send_data(test_data, "192.168.1.1", 8080));
}

TEST_F(IceTest, ConnectionStrategyTest) {
    // Test connection strategy enumeration
    auto strategies = {
        librats::ConnectionStrategy::DIRECT_ONLY,
        librats::ConnectionStrategy::STUN_ASSISTED,
        librats::ConnectionStrategy::ICE_FULL,
        librats::ConnectionStrategy::TURN_RELAY,
        librats::ConnectionStrategy::AUTO_ADAPTIVE
    };
    
    // Just ensure all strategies are defined and accessible
    EXPECT_EQ(strategies.size(), 5);
}

TEST_F(IceTest, NatTraversalConfigTest) {
    // Test NAT traversal configuration
    librats::NatTraversalConfig config;
    
    // Test default values
    EXPECT_TRUE(config.enable_ice);
    EXPECT_FALSE(config.enable_upnp);
    EXPECT_TRUE(config.enable_hole_punching);
    EXPECT_TRUE(config.enable_turn_relay);
    EXPECT_FALSE(config.prefer_ipv6);
    
    // Test that default STUN servers are configured
    EXPECT_GT(config.stun_servers.size(), 0);
    EXPECT_TRUE(config.stun_servers[0].find("stun.l.google.com") != std::string::npos);
    
    // Test timeout defaults
    EXPECT_EQ(config.ice_gathering_timeout_ms, 10000);
    EXPECT_EQ(config.ice_connectivity_timeout_ms, 30000);
    EXPECT_EQ(config.hole_punch_attempts, 5);
    
    // Test priority defaults
    EXPECT_EQ(config.host_candidate_priority, 65535);
    EXPECT_EQ(config.server_reflexive_priority, 65534);
    EXPECT_EQ(config.relay_candidate_priority, 65533);
} 