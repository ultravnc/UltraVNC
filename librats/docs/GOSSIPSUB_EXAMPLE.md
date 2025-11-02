# GossipSub Example

This example demonstrates how to use the GossipSub publish-subscribe messaging system in librats.

## Two API Approaches

librats now provides **two ways** to interact with GossipSub:

### 1. Direct GossipSub API (Advanced Users)
```cpp
#include "librats.h"

// Access GossipSub directly for fine-grained control
auto& gossipsub = client.get_gossipsub();
gossipsub.subscribe("chat");
gossipsub.set_message_handler("chat", [](const std::string& topic, const std::string& message, const std::string& sender_peer_id) {
    std::cout << "Direct API - Message on " << topic << " from " << sender_peer_id << ": " << message << std::endl;
});
gossipsub.publish("chat", "Hello from direct API!");
```

### 2. Unified Convenience API (Recommended for Most Users)
```cpp
#include "librats.h"

// Use RatsClient's unified convenience methods - consistent with the rest of the API
client.subscribe_to_topic("chat");
client.on_topic_message("chat", [](const std::string& peer_id, const std::string& topic, const std::string& message) {
    std::cout << "Unified API - Message on " << topic << " from " << peer_id << ": " << message << std::endl;
});
client.publish_to_topic("chat", "Hello from unified API!");
```

## Complete Working Example

```cpp
#include "librats.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    // Create two clients
    librats::RatsClient client1(8080);
    librats::RatsClient client2(8081);
    
    // Start both clients
    if (!client1.start() || !client2.start()) {
        std::cerr << "Failed to start clients" << std::endl;
        return 1;
    }
    
    // Connect client2 to client1
    client2.connect_to_peer("127.0.0.1", 8080);
    
    // Wait for connection
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // === UNIFIED CONVENIENCE API USAGE ===
    
    // Subscribe to topics using the unified API
    client1.subscribe_to_topic("news");
    client1.subscribe_to_topic("chat");
    client2.subscribe_to_topic("news");
    client2.subscribe_to_topic("chat");
    
    // Set up message handlers using unified API (consistent with client.on() pattern)
    client1.on_topic_message("news", [](const std::string& peer_id, const std::string& topic, const std::string& message) {
        std::cout << "[Client1] News from " << peer_id << ": " << message << std::endl;
    });
    
    client1.on_topic_json_message("chat", [](const std::string& peer_id, const std::string& topic, const nlohmann::json& message) {
        std::cout << "[Client1] Chat JSON from " << peer_id << ": " << message.dump() << std::endl;
    });
    
    client2.on_topic_message("news", [](const std::string& peer_id, const std::string& topic, const std::string& message) {
        std::cout << "[Client2] News from " << peer_id << ": " << message << std::endl;
    });
    
    // Set up peer event handlers
    client1.on_topic_peer_joined("news", [](const std::string& peer_id, const std::string& topic) {
        std::cout << "[Client1] Peer " << peer_id << " joined topic " << topic << std::endl;
    });
    
    client2.on_topic_peer_left("news", [](const std::string& peer_id, const std::string& topic) {
        std::cout << "[Client2] Peer " << peer_id << " left topic " << topic << std::endl;
    });
    
    // Set up message validation
    client1.set_topic_message_validator("chat", [](const std::string& topic, const std::string& message, const std::string& sender_peer_id) {
        // Only allow messages shorter than 100 characters
        return message.length() <= 100 ? librats::ValidationResult::ACCEPT : librats::ValidationResult::REJECT;
    });
    
    // Wait for subscriptions to propagate
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Publish messages using unified API
    std::cout << "\n=== Publishing Messages ===" << std::endl;
    
    // Publish string messages
    client1.publish_to_topic("news", "Breaking: librats GossipSub is working!");
    client2.publish_to_topic("news", "Update: Unified API makes it easy to use!");
    
    // Publish JSON messages
    nlohmann::json chat_message;
    chat_message["user"] = "alice";
    chat_message["text"] = "Hello everyone!";
    chat_message["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    client2.publish_json_to_topic("chat", chat_message);
    
    // Wait for messages to propagate
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // === INFORMATION QUERIES ===
    
    std::cout << "\n=== Topic Information ===" << std::endl;
    
    // Check subscriptions
    auto client1_topics = client1.get_subscribed_topics();
    std::cout << "Client1 subscribed topics: ";
    for (const auto& topic : client1_topics) {
        std::cout << topic << " ";
    }
    std::cout << std::endl;
    
    // Get topic peers
    auto news_peers = client1.get_topic_peers("news");
    std::cout << "Peers subscribed to 'news': " << news_peers.size() << std::endl;
    
    auto mesh_peers = client1.get_topic_mesh_peers("news");
    std::cout << "Mesh peers for 'news': " << mesh_peers.size() << std::endl;
    
    // Get statistics
    auto stats = client1.get_gossipsub_statistics();
    std::cout << "GossipSub running: " << client1.is_gossipsub_running() << std::endl;
    std::cout << "GossipSub stats: " << stats.dump(2) << std::endl;
    
    // === CLEANUP ===
    
    std::cout << "\n=== Cleanup ===" << std::endl;
    
    // Unsubscribe using unified API
    client1.unsubscribe_from_topic("news");
    client2.off_topic("chat");  // Remove all handlers for topic
    
    // Check final subscription status
    std::cout << "Client1 still subscribed to news: " << client1.is_subscribed_to_topic("news") << std::endl;
    
    // Stop clients
    client1.stop();
    client2.stop();
    
    return 0;
}
```

## Compilation

```bash
g++ -std=c++17 -I./src gossipsub_example.cpp -L./build -lrats -pthread -o gossipsub_example
./gossipsub_example
```

## API Comparison

| Feature | Direct GossipSub API | Unified Convenience API |
|---------|---------------------|-------------------------|
| **Consistency** | Different from RatsClient patterns | Matches `client.on()` pattern |
| **Error Handling** | Manual exception handling required | Automatic logging and error handling |
| **Callback Signature** | `(topic, message, sender_peer_id)` | `(peer_id, topic, message)` - consistent with RatsClient |
| **JSON Support** | Manual parsing required | Built-in JSON parsing with `on_topic_json_message()` |
| **Null Safety** | Direct access to internals | Safe null checks and graceful degradation |
| **Learning Curve** | Requires GossipSub knowledge | Uses familiar RatsClient patterns |

## When to Use Each API

**Use the Unified Convenience API when:**
- You want consistent patterns across your codebase
- You prefer automatic error handling and logging
- You're building typical publish-subscribe applications
- You want built-in JSON message support

**Use the Direct GossipSub API when:**
- You need fine-grained control over GossipSub behavior
- You want to customize advanced features like peer scoring
- You're building specialized mesh networking applications
- You need direct access to GossipSub statistics and internals

Both APIs work together seamlessly - you can mix and match based on your needs! 