# Message Exchange API Documentation

The librats library now includes a powerful message exchange API that provides an event-driven interface for handling peer-to-peer communication. This API allows you to easily register message handlers and send messages between peers.

## Overview

The message exchange API consists of:

- **Handler Registration**: `on()`, `once()`, `off()` methods for registering and removing message handlers
- **Message Sending**: `send()` methods for sending messages to specific peers or broadcasting to all peers
- **Callback Support**: Optional callbacks for send operations and automatic peer ID resolution

## Core Concepts

### Message Types
Messages are identified by a string type (e.g., "greeting", "status", "file_request"). You can create any custom message types you need.

### Message Data
Message data is passed as JSON objects using `nlohmann::json`, providing flexibility for complex data structures.

### Peer IDs
Handlers receive the actual peer ID (not the temporary connection hash), ensuring consistent identification across the network.

## API Reference

### Handler Registration

#### `on(message_type, callback)`
Registers a persistent message handler that will be called for every message of the specified type.

```cpp
client.on("chat", [](const std::string& peer_id, const nlohmann::json& data) {
    std::cout << "Chat from " << peer_id << ": " << data.value("message", "") << std::endl;
});
```

#### `once(message_type, callback)`
Registers a one-time message handler that will be removed after the first call.

```cpp
client.once("init_sync", [](const std::string& peer_id, const nlohmann::json& data) {
    std::cout << "Received initial sync from " << peer_id << std::endl;
    // Process initial synchronization data...
});
```

#### `off(message_type)`
Removes all handlers for a specific message type.

```cpp
client.off("temporary_handler");
```

### Message Sending

#### `send(message_type, data, callback)`
Broadcasts a message to all validated peers.

```cpp
nlohmann::json announcement;
announcement["message"] = "Server maintenance in 5 minutes";
announcement["priority"] = "high";

client.send("announcement", announcement, [](bool success, const std::string& error) {
    if (success) {
        std::cout << "Announcement sent successfully" << std::endl;
    } else {
        std::cout << "Failed to send announcement: " << error << std::endl;
    }
});
```

#### `send(peer_id, message_type, data, callback)`
Sends a message to a specific peer.

```cpp
nlohmann::json private_msg;
private_msg["message"] = "Hello there!";
private_msg["sender"] = "Admin";

client.send("target_peer_id", "private_message", private_msg, [](bool success, const std::string& error) {
    if (!success) {
        std::cout << "Failed to send private message: " << error << std::endl;
    }
});
```

## Usage Examples

### Basic Chat System

```cpp
#include "librats.h"
using namespace librats;

int main() {
    RatsClient client(8080);
    
    // Handle incoming chat messages
    client.on("chat", [&client](const std::string& peer_id, const nlohmann::json& data) {
        std::string message = data.value("message", "");
        std::string sender = data.value("sender", "Anonymous");
        
        std::cout << "[" << sender << " @ " << peer_id << "]: " << message << std::endl;
        
        // Echo the message back
        nlohmann::json echo;
        echo["message"] = "Echo: " + message;
        echo["sender"] = "EchoBot";
        client.send(peer_id, "chat", echo);
    });
    
    client.start();
    
    // Send a chat message to all peers
    nlohmann::json chat_msg;
    chat_msg["message"] = "Hello everyone!";
    chat_msg["sender"] = "User";
    client.send("chat", chat_msg);
    
    // Keep running...
    std::cin.get();
    return 0;
}
```

### Request-Response Pattern

```cpp
// Server side - handle file requests
client.on("file_request", [&client](const std::string& peer_id, const nlohmann::json& data) {
    std::string filename = data.value("filename", "");
    
    nlohmann::json response;
    response["filename"] = filename;
    
    if (file_or_directory_exists(filename)) {
        response["status"] = "found";
        response["size"] = get_file_size(filename);
        response["content"] = read_file_content(filename);
    } else {
        response["status"] = "not_found";
    }
    
    client.send(peer_id, "file_response", response);
});

// Client side - request a file
client.once("file_response", [](const std::string& peer_id, const nlohmann::json& data) {
    std::string status = data.value("status", "");
    if (status == "found") {
        std::cout << "File received from " << peer_id << std::endl;
        // Process file content...
    } else {
        std::cout << "File not found on " << peer_id << std::endl;
    }
});

nlohmann::json request;
request["filename"] = "example.txt";
client.send("file_request", request);
```

### Status Broadcasting

```cpp
// Broadcast status updates
void broadcast_status(RatsClient& client, const std::string& status) {
    nlohmann::json status_msg;
    status_msg["status"] = status;
    status_msg["timestamp"] = std::time(nullptr);
    status_msg["node_id"] = client.get_our_peer_id();
    
    client.send("status", status_msg);
}

// Handle status updates from other peers
client.on("status", [](const std::string& peer_id, const nlohmann::json& data) {
    std::string status = data.value("status", "unknown");
    time_t timestamp = data.value("timestamp", 0);
    
    std::cout << "Peer " << peer_id << " is " << status 
              << " (updated: " << std::ctime(&timestamp) << ")" << std::endl;
});

// Usage
broadcast_status(client, "online");
broadcast_status(client, "busy");
broadcast_status(client, "offline");
```

## Built-in Message Types

The following message types are handled internally by librats and are also forwarded to registered handlers:

- **"peer"**: Peer exchange messages (automatic peer discovery)
- **"peers_request"**: Request for peer list
- **"peers_response"**: Response with peer list

You can register handlers for these to monitor internal operations:

```cpp
client.on("peer", [](const std::string& peer_id, const nlohmann::json& data) {
    std::cout << "New peer announced: " << data.value("ip", "") 
              << ":" << data.value("port", 0) << std::endl;
});
```

## Thread Safety

- All message handlers are called asynchronously and should be thread-safe
- Handler registration/removal is thread-safe
- Message sending is thread-safe
- Handlers should avoid blocking operations to prevent message processing delays

## Best Practices

1. **Keep handlers fast**: Message handlers should complete quickly to avoid blocking message processing
2. **Use meaningful message types**: Choose descriptive names like "chat_message", "file_request", "status_update"
3. **Handle errors gracefully**: Always include error handling in message handlers
4. **Use callbacks for important sends**: Use send callbacks for critical messages to ensure delivery
5. **Clean up handlers**: Remove handlers you no longer need with `off()`

## Error Handling

Send callbacks provide error information:

```cpp
client.send("important_message", data, [](bool success, const std::string& error) {
    if (!success) {
        // Handle the error
        std::cerr << "Send failed: " << error << std::endl;
        // Possible errors:
        // - "Client is not running"
        // - "Peer not found: [peer_id]"
        // - "Peer handshake not completed: [peer_id]"
        // - "No peers to send message to"
        // - "Failed to send message to peer: [peer_id]"
    }
});
```
