# NAT Traversal Example - librats

This example demonstrates the comprehensive NAT traversal capabilities in librats, including automatic ICE coordination, NAT detection, and intelligent connection strategy selection.

## Complete NAT Traversal Example

```cpp
#include "librats.h"
#include <iostream>
#include <thread>
#include <chrono>

class NatTraversalDemo {
private:
    std::unique_ptr<librats::RatsClient> client_;
    int listen_port_;
    
public:
    NatTraversalDemo(int port) : listen_port_(port) {
        // Configure comprehensive NAT traversal
        librats::NatTraversalConfig nat_config;
        
        // Enable all NAT traversal features
        nat_config.enable_ice = true;
        nat_config.enable_hole_punching = true;
        nat_config.enable_turn_relay = true;
        nat_config.prefer_ipv6 = false;
        
        // Configure STUN servers for public IP discovery and ICE
        nat_config.stun_servers = {
            "stun.l.google.com:19302",
            "stun1.l.google.com:19302",
            "stun.stunprotocol.org:3478"
        };
        
        // Configure TURN servers (optional - add your own)
        // nat_config.turn_servers = {"turn.example.com:3478"};
        // nat_config.turn_usernames = {"username"};
        // nat_config.turn_passwords = {"password"};
        
        // Optimize timeouts for faster connections
        nat_config.ice_gathering_timeout_ms = 8000;
        nat_config.ice_connectivity_timeout_ms = 20000;
        nat_config.hole_punch_attempts = 3;
        
        // Create client with NAT traversal configuration
        client_ = std::make_unique<librats::RatsClient>(listen_port_, 10, nat_config);
        
        setup_callbacks();
    }
    
    void setup_callbacks() {
        // Enhanced connection callback with NAT traversal information
        client_->set_advanced_connection_callback([this](socket_t socket, const std::string& peer_id, 
                                                        const librats::ConnectionAttemptResult& result) {
            std::cout << "\n🎉 PEER CONNECTED!" << std::endl;
            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
            std::cout << "📋 Peer ID       : " << peer_id << std::endl;
            std::cout << "🔗 Method Used   : " << result.method << std::endl;
            std::cout << "⏱️  Duration      : " << result.duration.count() << " ms" << std::endl;
            std::cout << "🏠 Local NAT     : " << nat_type_to_string(result.local_nat_type) << std::endl;
            std::cout << "🌐 Remote NAT    : " << nat_type_to_string(result.remote_nat_type) << std::endl;
            std::cout << "🔌 Socket        : " << socket << std::endl;
            
            if (result.used_candidates.size() > 0) {
                std::cout << "🧊 ICE Candidates Used:" << std::endl;
                for (const auto& candidate : result.used_candidates) {
                    std::cout << "   • " << candidate.ip << ":" << candidate.port 
                             << " (type: " << ice_candidate_type_to_string(candidate.type) << ")" << std::endl;
                }
            }
            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
        });
        
        // NAT traversal progress monitoring
        client_->set_nat_traversal_progress_callback([](const std::string& peer_id, const std::string& status) {
            std::cout << "🔄 NAT Progress: " << peer_id << " - " << status << std::endl;
        });
        
        // ICE candidate discovery monitoring
        client_->set_ice_candidate_callback([](const std::string& peer_id, const librats::IceCandidate& candidate) {
            std::cout << "🧊 ICE Candidate: " << candidate.ip << ":" << candidate.port 
                     << " (type: " << ice_candidate_type_to_string(candidate.type) 
                     << ", priority: " << candidate.priority << ")" << std::endl;
        });
        
        // Connection callback (legacy)
        client_->set_connection_callback([this](socket_t socket, const std::string& peer_id) {
            std::cout << "✅ Handshake completed with peer " << peer_id << std::endl;
            
            // Send test message using new message exchange API
            nlohmann::json greeting;
            greeting["message"] = "Hello from NAT traversal demo!";
            greeting["port"] = listen_port_;
            greeting["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            
            client_->send(peer_id, "demo_greeting", greeting);
        });
        
        // Data callback for received messages
        client_->set_string_data_callback([](socket_t socket, const std::string& peer_id, const std::string& data) {
            std::cout << "📨 Received data from " << peer_id << ": " << data.substr(0, 100) 
                     << (data.length() > 100 ? "..." : "") << std::endl;
        });
        
        // Disconnect callback
        client_->set_disconnect_callback([](socket_t socket, const std::string& peer_id) {
            std::cout << "👋 Peer disconnected: " << peer_id << std::endl;
        });
        
        // Register message handlers
        client_->on("demo_greeting", [this](const std::string& peer_id, const nlohmann::json& data) {
            std::cout << "🎤 Greeting from " << peer_id << ": " << data.value("message", "") << std::endl;
            
            // Send response
            nlohmann::json response;
            response["message"] = "Hello back!";
            response["your_port"] = data.value("port", 0);
            response["my_port"] = listen_port_;
            
            client_->send(peer_id, "demo_response", response);
        });
        
        client_->on("demo_response", [](const std::string& peer_id, const nlohmann::json& data) {
            std::cout << "💬 Response from " << peer_id << ": " << data.value("message", "") << std::endl;
        });
    }
    
    bool start() {
        std::cout << "🚀 Starting NAT Traversal Demo on port " << listen_port_ << std::endl;
        
        if (!client_->start()) {
            std::cout << "❌ Failed to start client" << std::endl;
            return false;
        }
        
        // Start discovery services
        std::cout << "🔍 Starting discovery services..." << std::endl;
        
        // Start DHT for wide-area peer discovery
        if (client_->start_dht_discovery()) {
            std::cout << "✅ DHT discovery started" << std::endl;
        } else {
            std::cout << "⚠️  DHT discovery failed" << std::endl;
        }
        
        // Start mDNS for local network discovery
        std::map<std::string, std::string> mdns_records;
        mdns_records["version"] = "1.0";
        mdns_records["nat_demo"] = "true";
        
        if (client_->start_mdns_discovery("", mdns_records)) {
            std::cout << "✅ mDNS discovery started" << std::endl;
        } else {
            std::cout << "⚠️  mDNS discovery failed" << std::endl;
        }
        
        // Discover public IP via STUN
        if (client_->discover_and_ignore_public_ip()) {
            std::cout << "✅ Public IP discovery completed" << std::endl;
        } else {
            std::cout << "⚠️  Public IP discovery failed" << std::endl;
        }
        
        return true;
    }
    
    void print_status() {
        std::cout << "\n📊 NETWORK STATUS" << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        
        // Basic client info
        auto stats = client_->get_connection_statistics();
        std::cout << "🏠 Listen Port   : " << listen_port_ << std::endl;
        std::cout << "🆔 Our Peer ID  : " << client_->get_our_peer_id().substr(0, 16) << "..." << std::endl;
        std::cout << "👥 Connected Peers: " << stats.value("validated_peers", 0) 
                 << "/" << stats.value("max_peers", 0) << std::endl;
        std::cout << "🔐 Encryption   : " << (stats.value("encryption_enabled", false) ? "ON" : "OFF") << std::endl;
        
        // Public IP
        std::string public_ip = client_->get_public_ip();
        if (!public_ip.empty()) {
            std::cout << "🌐 Public IP    : " << public_ip << std::endl;
        }
        
        // NAT information
        auto nat_stats = client_->get_nat_traversal_statistics();
        auto nat_type = static_cast<librats::NatType>(nat_stats.value("detected_nat_type", 0));
        std::cout << "🔀 NAT Type     : " << nat_type_to_string(nat_type) << std::endl;
        std::cout << "🚧 Has NAT      : " << (nat_stats.value("has_nat", false) ? "YES" : "NO") << std::endl;
        
        // ICE information
        if (nat_stats.value("ice_available", false)) {
            std::cout << "🧊 ICE Status   : " << (nat_stats.value("ice_running", false) ? "RUNNING" : "STOPPED") << std::endl;
            std::cout << "🧊 ICE State    : " << nat_stats.value("ice_state", 0) << std::endl;
            if (nat_stats.contains("local_ice_candidates")) {
                std::cout << "🧊 ICE Candidates: " << nat_stats.value("local_ice_candidates", 0) << std::endl;
            }
        }
        
        // Discovery services
        std::cout << "🔍 DHT Running  : " << (stats.value("dht_running", false) ? "YES" : "NO") << std::endl;
        std::cout << "🔍 mDNS Running : " << (stats.value("mdns_running", false) ? "YES" : "NO") << std::endl;
        
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
    }
    
    bool connect_to_peer(const std::string& host, int port, 
                        librats::ConnectionStrategy strategy = librats::ConnectionStrategy::AUTO_ADAPTIVE) {
        std::cout << "🔗 Attempting connection to " << host << ":" << port 
                 << " using strategy: " << connection_strategy_to_string(strategy) << std::endl;
        
        return client_->connect_to_peer(host, port, strategy);
    }
    
    void test_all_strategies(const std::string& host, int port) {
        std::cout << "\n🧪 TESTING ALL CONNECTION STRATEGIES" << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        
        std::vector<librats::ConnectionStrategy> strategies = {
            librats::ConnectionStrategy::DIRECT_ONLY,
            librats::ConnectionStrategy::STUN_ASSISTED,
            librats::ConnectionStrategy::ICE_FULL,
            librats::ConnectionStrategy::TURN_RELAY
        };
        
        auto results = client_->test_connection_strategies(host, port, strategies);
        
        for (const auto& result : results) {
            std::string status = result.success ? "✅ SUCCESS" : "❌ FAILED";
            std::cout << "📈 " << std::setw(15) << result.method << " : " 
                     << status << " (" << result.duration.count() << "ms)";
            
            if (!result.error_message.empty()) {
                std::cout << " - " << result.error_message;
            }
            std::cout << std::endl;
        }
        
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
    }
    
    void stop() {
        std::cout << "🛑 Stopping NAT Traversal Demo..." << std::endl;
        client_->stop();
        std::cout << "✅ Demo stopped" << std::endl;
    }
    
    librats::RatsClient* get_client() { return client_.get(); }
    
private:
    std::string nat_type_to_string(librats::NatType type) {
        switch (type) {
            case librats::NatType::OPEN_INTERNET: return "Open Internet";
            case librats::NatType::FULL_CONE: return "Full Cone NAT";
            case librats::NatType::RESTRICTED_CONE: return "Restricted Cone NAT";
            case librats::NatType::PORT_RESTRICTED: return "Port Restricted NAT";
            case librats::NatType::SYMMETRIC: return "Symmetric NAT";
            case librats::NatType::BLOCKED: return "UDP Blocked";
            default: return "Unknown";
        }
    }
    
    std::string ice_candidate_type_to_string(librats::IceCandidateType type) {
        switch (type) {
            case librats::IceCandidateType::HOST: return "host";
            case librats::IceCandidateType::SERVER_REFLEXIVE: return "srflx";
            case librats::IceCandidateType::RELAY: return "relay";
            case librats::IceCandidateType::PEER_REFLEXIVE: return "prflx";
            default: return "unknown";
        }
    }
    
    std::string connection_strategy_to_string(librats::ConnectionStrategy strategy) {
        switch (strategy) {
            case librats::ConnectionStrategy::DIRECT_ONLY: return "Direct Only";
            case librats::ConnectionStrategy::STUN_ASSISTED: return "STUN Assisted";
            case librats::ConnectionStrategy::ICE_FULL: return "ICE Full";
            case librats::ConnectionStrategy::TURN_RELAY: return "TURN Relay";
            case librats::ConnectionStrategy::AUTO_ADAPTIVE: return "Auto Adaptive";
            default: return "Unknown";
        }
    }
};

// Interactive demo interface
class InteractiveDemo {
private:
    std::unique_ptr<NatTraversalDemo> demo_;
    
public:
    InteractiveDemo(int port) : demo_(std::make_unique<NatTraversalDemo>(port)) {}
    
    void run() {
        if (!demo_->start()) {
            std::cout << "❌ Failed to start demo" << std::endl;
            return;
        }
        
        // Initial status
        std::this_thread::sleep_for(std::chrono::seconds(2));
        demo_->print_status();
        
        // Interactive menu
        while (true) {
            std::cout << "\n📋 NAT TRAVERSAL DEMO MENU" << std::endl;
            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
            std::cout << "1. 📊 Print Status" << std::endl;
            std::cout << "2. 🔗 Connect to Peer (Auto)" << std::endl;
            std::cout << "3. 🔗 Connect to Peer (Direct)" << std::endl;
            std::cout << "4. 🔗 Connect to Peer (STUN)" << std::endl;
            std::cout << "5. 🔗 Connect to Peer (ICE)" << std::endl;
            std::cout << "6. 🧪 Test All Strategies" << std::endl;
            std::cout << "7. 🧊 Create ICE Offer" << std::endl;
            std::cout << "8. 📊 Show Statistics" << std::endl;
            std::cout << "9. 👥 List Peers" << std::endl;
            std::cout << "0. 🚪 Exit" << std::endl;
            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
            std::cout << "Enter choice: ";
            
            int choice;
            std::cin >> choice;
            
            switch (choice) {
                case 1:
                    demo_->print_status();
                    break;
                    
                case 2: case 3: case 4: case 5: {
                    std::string host;
                    int port;
                    std::cout << "Enter host: ";
                    std::cin >> host;
                    std::cout << "Enter port: ";
                    std::cin >> port;
                    
                    librats::ConnectionStrategy strategy;
                    switch (choice) {
                        case 2: strategy = librats::ConnectionStrategy::AUTO_ADAPTIVE; break;
                        case 3: strategy = librats::ConnectionStrategy::DIRECT_ONLY; break;
                        case 4: strategy = librats::ConnectionStrategy::STUN_ASSISTED; break;
                        case 5: strategy = librats::ConnectionStrategy::ICE_FULL; break;
                    }
                    
                    demo_->connect_to_peer(host, port, strategy);
                    break;
                }
                
                case 6: {
                    std::string host;
                    int port;
                    std::cout << "Enter host to test: ";
                    std::cin >> host;
                    std::cout << "Enter port: ";
                    std::cin >> port;
                    
                    demo_->test_all_strategies(host, port);
                    break;
                }
                
                case 7: {
                    std::string peer_id;
                    std::cout << "Enter peer ID for ICE offer: ";
                    std::cin >> peer_id;
                    
                    auto offer = demo_->get_client()->create_ice_offer(peer_id);
                    std::cout << "ICE Offer created:" << std::endl;
                    std::cout << offer.dump(2) << std::endl;
                    break;
                }
                
                case 8: {
                    auto conn_stats = demo_->get_client()->get_connection_statistics();
                    auto nat_stats = demo_->get_client()->get_nat_traversal_statistics();
                    
                    std::cout << "\n📊 Connection Statistics:" << std::endl;
                    std::cout << conn_stats.dump(2) << std::endl;
                    
                    std::cout << "\n🔀 NAT Traversal Statistics:" << std::endl;
                    std::cout << nat_stats.dump(2) << std::endl;
                    break;
                }
                
                case 9: {
                    auto peers = demo_->get_client()->get_validated_peers();
                    std::cout << "\n👥 Connected Peers (" << peers.size() << "):" << std::endl;
                    
                    for (const auto& peer : peers) {
                        std::cout << "  • ID: " << peer.peer_id.substr(0, 16) << "..." << std::endl;
                        std::cout << "    Address: " << peer.ip << ":" << peer.port << std::endl;
                        std::cout << "    Method: " << peer.connection_method << std::endl;
                        std::cout << "    Encrypted: " << (peer.encryption_enabled ? "Yes" : "No") << std::endl;
                        std::cout << "    ICE: " << (peer.ice_enabled ? "Yes" : "No") << std::endl;
                        std::cout << std::endl;
                    }
                    break;
                }
                
                case 0:
                    demo_->stop();
                    return;
                    
                default:
                    std::cout << "❌ Invalid choice" << std::endl;
                    break;
            }
        }
    }
};

int main(int argc, char* argv[]) {
    int port = 8080;
    
    if (argc > 1) {
        port = std::atoi(argv[1]);
    }
    
    std::cout << "🐀 librats NAT Traversal Demo" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "This demo showcases comprehensive NAT traversal capabilities:" << std::endl;
    std::cout << "• Automatic NAT type detection" << std::endl;
    std::cout << "• ICE candidate gathering and exchange" << std::endl;
    std::cout << "• Multiple connection strategies" << std::endl;
    std::cout << "• Real-time progress monitoring" << std::endl;
    std::cout << "• Connection statistics and analysis" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    try {
        InteractiveDemo demo(port);
        demo.run();
    } catch (const std::exception& e) {
        std::cout << "❌ Demo failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
```

## Compilation

```bash
# Compile the example
g++ -std=c++17 -O2 -o nat_demo nat_traversal_example.cpp -L. -lrats -lpthread

# Run peer 1
./nat_demo 8080

# Run peer 2 (in another terminal)
./nat_demo 8081
```

## Expected Output

```
🐀 librats NAT Traversal Demo
=========================================
This demo showcases comprehensive NAT traversal capabilities:
• Automatic NAT type detection
• ICE candidate gathering and exchange
• Multiple connection strategies
• Real-time progress monitoring
• Connection statistics and analysis
=========================================

🚀 Starting NAT Traversal Demo on port 8080
🔍 Starting discovery services...
✅ DHT discovery started
✅ mDNS discovery started
✅ Public IP discovery completed
🧊 ICE Candidate: 192.168.1.100:8080 (type: host, priority: 65535)
🧊 ICE Candidate: 203.0.113.1:54321 (type: srflx, priority: 65534)

📊 NETWORK STATUS
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
🏠 Listen Port   : 8080
🆔 Our Peer ID  : a1b2c3d4e5f6789a...
👥 Connected Peers: 0/10
🔐 Encryption   : ON
🌐 Public IP    : 203.0.113.1
🔀 NAT Type     : Full Cone NAT
🚧 Has NAT      : YES
🧊 ICE Status   : RUNNING
🧊 ICE State    : 1
🧊 ICE Candidates: 2
🔍 DHT Running  : YES
🔍 mDNS Running : YES
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

📋 NAT TRAVERSAL DEMO MENU
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
1. 📊 Print Status
2. 🔗 Connect to Peer (Auto)
3. 🔗 Connect to Peer (Direct)
4. 🔗 Connect to Peer (STUN)
5. 🔗 Connect to Peer (ICE)
6. 🧪 Test All Strategies
7. 🧊 Create ICE Offer
8. 📊 Show Statistics
9. 👥 List Peers
0. 🚪 Exit
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Enter choice: 2
Enter host: 192.168.1.101
Enter port: 8081

🔗 Attempting connection to 192.168.1.101:8081 using strategy: Auto Adaptive
🔄 NAT Progress:  - ICE state: 2
🧊 ICE Candidate: 192.168.1.101:8081 (type: host, priority: 65535)
🔄 NAT Progress: xyz123 - Exchanging NAT information
🔄 NAT Progress: xyz123 - Starting ICE connectivity checks

🎉 PEER CONNECTED!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
📋 Peer ID       : xyz123def456ghi789
🔗 Method Used   : ice
⏱️  Duration      : 1250 ms
🏠 Local NAT     : Full Cone NAT
🌐 Remote NAT    : Restricted Cone NAT
🔌 Socket        : 15
🧊 ICE Candidates Used:
   • 192.168.1.100:8080 (type: host)
   • 192.168.1.101:8081 (type: host)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

✅ Handshake completed with peer xyz123def456ghi789
🎤 Greeting from xyz123def456ghi789: Hello from NAT traversal demo!
💬 Response from xyz123def456ghi789: Hello back!
```
