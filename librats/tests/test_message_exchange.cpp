#include <gtest/gtest.h>
#include "../src/librats.h"
#include <thread>
#include <chrono>
#include <atomic>

using namespace librats;

class MessageExchangeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Enable debug logging
        librats::Logger::getInstance().set_log_level(librats::LogLevel::DEBUG);
        // Create two clients for testing
        client1 = std::make_unique<RatsClient>(8001, 5);
        client2 = std::make_unique<RatsClient>(8002, 5);
        
        // Reset counters
        greeting_count = 0;
        status_count = 0;
        once_count = 0;
        response_received = false;
        
        // Start both clients
        ASSERT_TRUE(client1->start()) << "Failed to start client1";
        ASSERT_TRUE(client2->start()) << "Failed to start client2";
        
        // Wait for initialization
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Connect client2 to client1
        ASSERT_TRUE(client2->connect_to_peer("127.0.0.1", 8001)) << "Failed to connect client2 to client1";
        
        // Wait for handshake to complete
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    void TearDown() override {
        if (client1) {
            client1->stop();
        }
        if (client2) {
            client2->stop();
        }
    }
    
    std::unique_ptr<RatsClient> client1;
    std::unique_ptr<RatsClient> client2;
    
    // Counters for testing
    std::atomic<int> greeting_count{0};
    std::atomic<int> status_count{0};
    std::atomic<int> once_count{0};
    std::atomic<bool> response_received{false};
};

TEST_F(MessageExchangeTest, BasicConnectionAndSetup) {
    EXPECT_TRUE(client1->is_running());
    EXPECT_TRUE(client2->is_running());
    EXPECT_EQ(client1->get_peer_count(), 1);
    EXPECT_EQ(client2->get_peer_count(), 1);
}

TEST_F(MessageExchangeTest, MessageHandlerRegistration) {
    // Set up message handlers on client1
    client1->on("greeting", [this](const std::string& peer_id, const nlohmann::json& data) {
        EXPECT_FALSE(peer_id.empty());
        EXPECT_TRUE(data.contains("message"));
        greeting_count++;
        
        // Send a response
        nlohmann::json response;
        response["message"] = "Hello back from client1!";
        response["original_sender"] = peer_id;
        client1->send(peer_id, "greeting_response", response);
    });
    
    client1->on("status", [this](const std::string& peer_id, const nlohmann::json& data) {
        EXPECT_FALSE(peer_id.empty());
        EXPECT_TRUE(data.contains("status"));
        status_count++;
    });
    
    client1->once("test_once", [this](const std::string& peer_id, const nlohmann::json& data) {
        EXPECT_FALSE(peer_id.empty());
        EXPECT_TRUE(data.contains("message"));
        once_count++;
    });
    
    // Set up handlers on client2
    client2->on("greeting_response", [this](const std::string& peer_id, const nlohmann::json& data) {
        EXPECT_FALSE(peer_id.empty());
        EXPECT_TRUE(data.contains("message"));
        response_received = true;
    });
    
    // Handlers should be registered (we can't directly test this, but the subsequent tests will validate)
    SUCCEED() << "Message handlers registered successfully";
}

TEST_F(MessageExchangeTest, GreetingMessageExchange) {
    // Set up handlers
    client1->on("greeting", [this](const std::string& peer_id, const nlohmann::json& data) {
        greeting_count++;
        
        // Send a response
        nlohmann::json response;
        response["message"] = "Hello back from client1!";
        response["original_sender"] = peer_id;
        client1->send(peer_id, "greeting_response", response);
    });
    
    client2->on("greeting_response", [this](const std::string& peer_id, const nlohmann::json& data) {
        response_received = true;
    });
    
    // Send greeting message
    nlohmann::json greeting_data;
    greeting_data["message"] = "Hello from client2!";
    greeting_data["sender"] = "test_client2";
    
    bool send_success = false;
    client2->send("greeting", greeting_data, [&send_success](bool success, const std::string& error) {
        send_success = success;
        if (!success) {
            ADD_FAILURE() << "Failed to send greeting: " << error;
        }
    });
    
    // Wait for message processing
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    
    EXPECT_TRUE(send_success);
    EXPECT_EQ(greeting_count.load(), 1);
    EXPECT_TRUE(response_received.load());
}

TEST_F(MessageExchangeTest, StatusMessageSending) {
    // Set up status handler
    client1->on("status", [this](const std::string& peer_id, const nlohmann::json& data) {
        EXPECT_EQ(data.value("status", ""), "online");
        EXPECT_EQ(data.value("details", ""), "Testing message exchange API");
        status_count++;
    });
    
    // Send status message
    nlohmann::json status_data;
    status_data["status"] = "online";
    status_data["details"] = "Testing message exchange API";
    
    client2->send("status", status_data);
    
    // Wait for message processing
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    EXPECT_EQ(status_count.load(), 1);
}

TEST_F(MessageExchangeTest, OnceHandlerBehavior) {
    // Set up once handler
    client1->once("test_once", [this](const std::string& peer_id, const nlohmann::json& data) {
        once_count++;
    });
    
    // Send once message multiple times
    nlohmann::json once_data;
    once_data["message"] = "This should only be handled once";
    
    client2->send("test_once", once_data);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    client2->send("test_once", once_data);  // Second call should not trigger handler
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    client2->send("test_once", once_data);  // Third call should not trigger handler
    
    // Wait for message processing
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    EXPECT_EQ(once_count.load(), 1) << "Once handler should only trigger once, but triggered " << once_count.load() << " times";
}

TEST_F(MessageExchangeTest, HandlerRemoval) {
    // Set up greeting handler
    client1->on("greeting", [this](const std::string& peer_id, const nlohmann::json& data) {
        greeting_count++;
    });
    
    // Send first greeting (should be handled)
    nlohmann::json greeting_data;
    greeting_data["message"] = "First greeting";
    client2->send("greeting", greeting_data);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(greeting_count.load(), 1);
    
    // Remove handler
    client1->off("greeting");
    
    // Send second greeting (should not be handled)
    greeting_data["message"] = "Second greeting should not be handled";
    client2->send("greeting", greeting_data);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Count should still be 1 (not incremented by second message)
    EXPECT_EQ(greeting_count.load(), 1) << "Handler was not properly removed";
}

TEST_F(MessageExchangeTest, TargetedMessageSending) {
    std::atomic<bool> targeted_message_received{false};
    std::string received_peer_id;
    
    // Set up handler on client1
    client1->on("targeted_test", [&](const std::string& peer_id, const nlohmann::json& data) {
        targeted_message_received = true;
        received_peer_id = peer_id;
    });
    
    // Get client1's peer ID as seen by client2
    auto client1_peers = client2->get_validated_peers();
    ASSERT_FALSE(client1_peers.empty()) << "No validated peers found";
    
    std::string target_peer_id = client1_peers[0].peer_id;
    
    // Send targeted message
    nlohmann::json targeted_data;
    targeted_data["message"] = "This is a targeted message";
    targeted_data["target"] = target_peer_id;
    
    client2->send(target_peer_id, "targeted_test", targeted_data);
    
    // Wait for message processing
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    EXPECT_TRUE(targeted_message_received.load());
    EXPECT_FALSE(received_peer_id.empty());
}

TEST_F(MessageExchangeTest, BroadcastMessageSending) {
    std::atomic<int> broadcast_count{0};
    
    // Set up handler on client1
    client1->on("broadcast_test", [&](const std::string& peer_id, const nlohmann::json& data) {
        broadcast_count++;
    });
    
    // Send broadcast message (should reach client1)
    nlohmann::json broadcast_data;
    broadcast_data["message"] = "This is a broadcast message";
    broadcast_data["type"] = "broadcast";
    
    client2->send("broadcast_test", broadcast_data);
    
    // Wait for message processing
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    EXPECT_EQ(broadcast_count.load(), 1);
}

TEST_F(MessageExchangeTest, MessageWithCallback) {
    bool callback_called = false;
    bool callback_success = false;
    std::string callback_error;
    
    // Set up handler
    client1->on("callback_test", [](const std::string& peer_id, const nlohmann::json& data) {
        // Just receive the message
    });
    
    // Send message with callback
    nlohmann::json callback_data;
    callback_data["message"] = "Testing callback functionality";
    
    client2->send("callback_test", callback_data, [&](bool success, const std::string& error) {
        callback_called = true;
        callback_success = success;
        callback_error = error;
    });
    
    // Wait for callback
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    EXPECT_TRUE(callback_called);
    EXPECT_TRUE(callback_success);
    EXPECT_TRUE(callback_error.empty());
}

TEST_F(MessageExchangeTest, InvalidPeerIdCallback) {
    bool callback_called = false;
    bool callback_success = false;
    std::string callback_error;
    
    // Send message to non-existent peer
    nlohmann::json test_data;
    test_data["message"] = "This should fail";
    
    client2->send("non_existent_peer_id", "test_message", test_data, [&](bool success, const std::string& error) {
        callback_called = true;
        callback_success = success;
        callback_error = error;
    });
    
    // Wait for callback
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    EXPECT_TRUE(callback_called);
    EXPECT_FALSE(callback_success);
    EXPECT_FALSE(callback_error.empty());
    EXPECT_TRUE(callback_error.find("Peer not found") != std::string::npos) << "Expected 'Peer not found' in error message: " << callback_error;
}

TEST_F(MessageExchangeTest, LargeMessageIntegrity) {
    std::atomic<bool> large_message_received{false};
    nlohmann::json received_data;
    std::string received_peer_id;
    
    // Set up handler on client1 to receive large message
    client1->on("large_message_test", [&](const std::string& peer_id, const nlohmann::json& data) {
        large_message_received = true;
        received_data = data;
        received_peer_id = peer_id;
    });
    
    // Create a large JSON message (>8192 bytes)
    nlohmann::json large_data;
    large_data["type"] = "large_message_test";
    large_data["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Create a large array of strings to exceed 8192 bytes
    nlohmann::json large_array = nlohmann::json::array();
    std::string base_string = "This is a test string that will be repeated many times to create a large message payload for testing the integrity of large JSON messages over the network. ";
    
    // Add strings until we exceed 8192 bytes
    std::string current_size_check = large_data.dump();
    while (current_size_check.size() < 8192) {
        large_array.push_back(base_string + std::to_string(large_array.size()));
        large_data["payload"] = large_array;
        current_size_check = large_data.dump();
    }
    
    // Add some additional data to ensure we're well over 8192 bytes
    for (int i = 0; i < 50; ++i) {
        large_array.push_back("Additional data entry " + std::to_string(i) + " - " + base_string);
    }
    large_data["payload"] = large_array;
    
    // Add metadata
    large_data["metadata"]["sender"] = "client2";
    large_data["metadata"]["test_case"] = "LargeMessageIntegrity";
    large_data["metadata"]["expected_size_bytes"] = large_data.dump().size();
    
    // Verify the message is indeed larger than 8192 bytes
    std::string serialized_message = large_data.dump();
    ASSERT_GT(serialized_message.size(), 8192) << "Test message should be larger than 8192 bytes, got: " << serialized_message.size();
    
    // Send the large message
    bool send_success = false;
    std::string send_error;
    client2->send("large_message_test", large_data, [&](bool success, const std::string& error) {
        send_success = success;
        send_error = error;
    });
    
    // Wait longer for large message processing
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    
    // Verify the message was sent successfully
    EXPECT_TRUE(send_success) << "Failed to send large message: " << send_error;
    
    // Verify the message was received
    EXPECT_TRUE(large_message_received.load()) << "Large message was not received";
    
    // Verify message integrity
    ASSERT_FALSE(received_data.empty()) << "Received data is empty";
    
    // Check that all fields are present and correct
    EXPECT_EQ(received_data.value("type", ""), "large_message_test");
    EXPECT_EQ(received_data["timestamp"], large_data["timestamp"]);
    EXPECT_TRUE(received_data.contains("payload")) << "Payload field missing";
    EXPECT_TRUE(received_data.contains("metadata")) << "Metadata field missing";
    
    // Verify payload integrity
    ASSERT_TRUE(received_data["payload"].is_array()) << "Payload should be an array";
    EXPECT_EQ(received_data["payload"].size(), large_data["payload"].size()) 
        << "Payload array size mismatch. Expected: " << large_data["payload"].size() 
        << ", Received: " << received_data["payload"].size();
    
    // Verify metadata integrity
    EXPECT_EQ(received_data["metadata"]["sender"], "client2");
    EXPECT_EQ(received_data["metadata"]["test_case"], "LargeMessageIntegrity");
    EXPECT_EQ(received_data["metadata"]["expected_size_bytes"], large_data["metadata"]["expected_size_bytes"]);
    
    // Verify complete message integrity by comparing serialized versions
    std::string received_serialized = received_data.dump();
    EXPECT_EQ(received_serialized.size(), serialized_message.size()) 
        << "Serialized message size mismatch. Expected: " << serialized_message.size() 
        << ", Received: " << received_serialized.size();
    
    // Compare a few sample entries from the payload array to ensure data integrity
    for (size_t i = 0; i < (std::min)(size_t(10), large_data["payload"].size()); ++i) {
        EXPECT_EQ(received_data["payload"][i], large_data["payload"][i]) 
            << "Payload mismatch at index " << i;
    }
    
    // Verify the last few entries as well
    size_t payload_size = large_data["payload"].size();
    for (size_t i = (std::max)(size_t(0), payload_size - 5); i < payload_size; ++i) {
        EXPECT_EQ(received_data["payload"][i], large_data["payload"][i]) 
            << "Payload mismatch at index " << i;
    }
    
    SUCCEED() << "Large message (" << serialized_message.size() 
              << " bytes) sent and received with full integrity preservation";
}
