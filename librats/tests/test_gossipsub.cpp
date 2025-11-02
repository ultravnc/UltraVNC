#include <gtest/gtest.h>
#include "librats.h"
#include "gossipsub.h"
#include <thread>
#include <chrono>
#include <atomic>

using namespace librats;

class GossipSubTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create two RatsClient instances for testing
        client1_ = std::make_unique<RatsClient>(8081);
        client2_ = std::make_unique<RatsClient>(8082);
        
        // Start both clients
        ASSERT_TRUE(client1_->start());
        ASSERT_TRUE(client2_->start());
        
        // Connect the clients
        ASSERT_TRUE(client2_->connect_to_peer("127.0.0.1", 8081));
        
        // Wait for connection to establish
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    void TearDown() override {
        if (client1_) {
            client1_->stop();
        }
        if (client2_) {
            client2_->stop();
        }
    }
    
    std::unique_ptr<RatsClient> client1_;
    std::unique_ptr<RatsClient> client2_;
};

TEST_F(GossipSubTest, BasicSubscriptionAndPublishing) {
    // Verify GossipSub is available
    ASSERT_TRUE(client1_->is_gossipsub_available());
    ASSERT_TRUE(client2_->is_gossipsub_available());
    
    // Get GossipSub instances
    auto& gossipsub1 = client1_->get_gossipsub();
    auto& gossipsub2 = client2_->get_gossipsub();
    
    // Test subscription
    std::string test_topic = "test-topic";
    ASSERT_TRUE(gossipsub1.subscribe(test_topic));
    ASSERT_TRUE(gossipsub2.subscribe(test_topic));
    
    // Verify subscription status
    ASSERT_TRUE(gossipsub1.is_subscribed(test_topic));
    ASSERT_TRUE(gossipsub2.is_subscribed(test_topic));
    
    // Wait for mesh to stabilize
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Set up message handlers
    std::atomic<int> messages_received1{0};
    std::atomic<int> messages_received2{0};
    std::string last_message1, last_message2;
    
    gossipsub1.set_message_handler(test_topic, [&](const std::string& topic, const std::string& message, const std::string& sender) {
        last_message1 = message;
        messages_received1++;
    });
    
    gossipsub2.set_message_handler(test_topic, [&](const std::string& topic, const std::string& message, const std::string& sender) {
        last_message2 = message;
        messages_received2++;
    });
    
    // Publish a message from client1
    std::string test_message = "Hello GossipSub!";
    ASSERT_TRUE(gossipsub1.publish(test_topic, test_message));
    
    // Wait for message propagation
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Verify message was received
    EXPECT_EQ(messages_received2.load(), 1);
    EXPECT_EQ(last_message2, test_message);
    
    // Publish from client2
    std::string test_message2 = "Response from client2";
    ASSERT_TRUE(gossipsub2.publish(test_topic, test_message2));
    
    // Wait for message propagation
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Verify both clients received the message
    EXPECT_EQ(messages_received1.load(), 1);
    EXPECT_EQ(last_message1, test_message2);
}

TEST_F(GossipSubTest, MultipleTopics) {
    auto& gossipsub1 = client1_->get_gossipsub();
    auto& gossipsub2 = client2_->get_gossipsub();
    
    // Subscribe to different topics
    std::string topic1 = "topic1";
    std::string topic2 = "topic2";
    
    ASSERT_TRUE(gossipsub1.subscribe(topic1));
    ASSERT_TRUE(gossipsub1.subscribe(topic2));
    ASSERT_TRUE(gossipsub2.subscribe(topic1)); // Only subscribe to topic1
    
    // Wait for mesh to stabilize
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Set up message counters
    std::atomic<int> topic1_count{0};
    std::atomic<int> topic2_count{0};
    
    gossipsub2.set_message_handler(topic1, [&](const std::string& topic, const std::string& message, const std::string& sender) {
        topic1_count++;
    });
    
    gossipsub2.set_message_handler(topic2, [&](const std::string& topic, const std::string& message, const std::string& sender) {
        topic2_count++;
    });
    
    // Publish to both topics from client1
    ASSERT_TRUE(gossipsub1.publish(topic1, std::string("Message for topic1")));
    ASSERT_TRUE(gossipsub1.publish(topic2, std::string("Message for topic2")));
    
    // Wait for message propagation
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Verify client2 only received message from topic1
    EXPECT_EQ(topic1_count.load(), 1);
    EXPECT_EQ(topic2_count.load(), 0);
}

TEST_F(GossipSubTest, JSONMessages) {
    auto& gossipsub1 = client1_->get_gossipsub();
    auto& gossipsub2 = client2_->get_gossipsub();
    
    std::string topic = "json-topic";
    ASSERT_TRUE(gossipsub1.subscribe(topic));
    ASSERT_TRUE(gossipsub2.subscribe(topic));
    
    // Wait for mesh to stabilize
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Set up message handler for JSON
    std::string received_json;
    gossipsub2.set_message_handler(topic, [&](const std::string& topic, const std::string& message, const std::string& sender) {
        received_json = message;
    });
    
    // Create and publish JSON message
    nlohmann::json test_json;
    test_json["type"] = "test";
    test_json["data"] = "Hello World";
    test_json["timestamp"] = 12345;
    
    ASSERT_TRUE(gossipsub1.publish(topic, test_json));
    
    // Wait for message propagation
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Verify JSON message was received correctly
    EXPECT_FALSE(received_json.empty());
    
    nlohmann::json received = nlohmann::json::parse(received_json);
    EXPECT_EQ(received["type"], "test");
    EXPECT_EQ(received["data"], "Hello World");
    EXPECT_EQ(received["timestamp"], 12345);
}

TEST_F(GossipSubTest, MessageValidation) {
    auto& gossipsub1 = client1_->get_gossipsub();
    auto& gossipsub2 = client2_->get_gossipsub();
    
    std::string topic = "validation-topic";
    ASSERT_TRUE(gossipsub1.subscribe(topic));
    ASSERT_TRUE(gossipsub2.subscribe(topic));
    
    // Set up validator that rejects messages containing "bad"
    gossipsub2.set_message_validator(topic, [](const std::string& topic, const std::string& message, const std::string& sender) {
        if (message.find("bad") != std::string::npos) {
            return ValidationResult::REJECT;
        }
        return ValidationResult::ACCEPT;
    });
    
    // Wait for mesh to stabilize
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Set up message counter
    std::atomic<int> messages_received{0};
    gossipsub2.set_message_handler(topic, [&](const std::string& topic, const std::string& message, const std::string& sender) {
        messages_received++;
    });
    
    // Publish a good message
    ASSERT_TRUE(gossipsub1.publish(topic, std::string("This is a good message")));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(messages_received.load(), 1);
    
    // Publish a bad message
    ASSERT_TRUE(gossipsub1.publish(topic, std::string("This is a bad message")));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(messages_received.load(), 1); // Should still be 1, bad message rejected
}

TEST_F(GossipSubTest, Unsubscription) {
    auto& gossipsub1 = client1_->get_gossipsub();
    auto& gossipsub2 = client2_->get_gossipsub();
    
    std::string topic = "unsub-topic";
    ASSERT_TRUE(gossipsub1.subscribe(topic));
    ASSERT_TRUE(gossipsub2.subscribe(topic));
    
    // Verify subscriptions
    ASSERT_TRUE(gossipsub1.is_subscribed(topic));
    ASSERT_TRUE(gossipsub2.is_subscribed(topic));
    
    // Wait for mesh to stabilize
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Unsubscribe client2
    ASSERT_TRUE(gossipsub2.unsubscribe(topic));
    ASSERT_FALSE(gossipsub2.is_subscribed(topic));
    
    // Wait for unsubscription to propagate
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Set up message counter
    std::atomic<int> messages_received{0};
    gossipsub2.set_message_handler(topic, [&](const std::string& topic, const std::string& message, const std::string& sender) {
        messages_received++;
    });
    
    // Publish message from client1
    ASSERT_TRUE(gossipsub1.publish(topic, std::string("Test message after unsubscribe")));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Client2 should not receive the message since it unsubscribed
    EXPECT_EQ(messages_received.load(), 0);
}

TEST_F(GossipSubTest, PeerJoinedLeftHandlers) {
    auto& gossipsub1 = client1_->get_gossipsub();
    auto& gossipsub2 = client2_->get_gossipsub();
    
    std::string topic = "peer-events-topic";
    
    // Set up peer event handlers
    std::atomic<int> peer_joined_count{0};
    std::atomic<int> peer_left_count{0};
    
    gossipsub1.set_peer_joined_handler(topic, [&](const std::string& topic, const std::string& peer_id) {
        peer_joined_count++;
    });
    
    gossipsub1.set_peer_left_handler(topic, [&](const std::string& topic, const std::string& peer_id) {
        peer_left_count++;
    });
    
    // Subscribe to topic from client1 first
    ASSERT_TRUE(gossipsub1.subscribe(topic));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Then subscribe from client2 (should trigger peer joined event)
    ASSERT_TRUE(gossipsub2.subscribe(topic));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    EXPECT_GE(peer_joined_count.load(), 1);
    
    // Unsubscribe client2 (should trigger peer left event)
    ASSERT_TRUE(gossipsub2.unsubscribe(topic));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    EXPECT_GE(peer_left_count.load(), 1);
}

TEST_F(GossipSubTest, Statistics) {
    auto& gossipsub1 = client1_->get_gossipsub();
    
    // Get initial statistics
    auto stats = gossipsub1.get_statistics();
    EXPECT_TRUE(stats.contains("running"));
    EXPECT_TRUE(stats.contains("subscribed_topics_count"));
    EXPECT_TRUE(stats.contains("total_topics_count"));
    
    // Subscribe to a topic
    ASSERT_TRUE(gossipsub1.subscribe("stats-topic"));
    
    // Get updated statistics
    stats = gossipsub1.get_statistics();
    EXPECT_EQ(stats["subscribed_topics_count"], 1);
    
    // Check cache statistics
    auto cache_stats = gossipsub1.get_cache_statistics();
    EXPECT_TRUE(cache_stats.contains("cached_messages_count"));
    EXPECT_TRUE(cache_stats.contains("seen_message_ids_count"));
}

// Performance test for high-frequency publishing
TEST_F(GossipSubTest, HighFrequencyPublishing) {
    auto& gossipsub1 = client1_->get_gossipsub();
    auto& gossipsub2 = client2_->get_gossipsub();
    
    std::string topic = "perf-topic";
    ASSERT_TRUE(gossipsub1.subscribe(topic));
    ASSERT_TRUE(gossipsub2.subscribe(topic));
    
    // Wait for mesh to stabilize
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Set up message counter
    std::atomic<int> messages_received{0};
    gossipsub2.set_message_handler(topic, [&](const std::string& topic, const std::string& message, const std::string& sender) {
        messages_received++;
    });
    
    // Publish multiple messages rapidly
    const int num_messages = 50;
    for (int i = 0; i < num_messages; i++) {
        ASSERT_TRUE(gossipsub1.publish(topic, "Message " + std::to_string(i)));
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Small delay to avoid overwhelming
    }
    
    // Wait for all messages to propagate
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Should receive most or all messages (allowing for some message loss in heavy load)
    EXPECT_GE(messages_received.load(), num_messages * 0.8); // At least 80% delivery rate
}

 