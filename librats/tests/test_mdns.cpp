#include <gtest/gtest.h>
#include "mdns.h"
#include "librats.h"
#include "logger.h"
#include "socket.h"  // Add socket header for init/cleanup functions
#include <iostream>
#include <thread>
#include <chrono>

using namespace librats;

class MdnsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize socket library before any socket operations
        ASSERT_TRUE(init_socket_library()) << "Failed to initialize socket library";
    }
    
    void TearDown() override {
        // Cleanup socket library after tests
        cleanup_socket_library();
    }
};

TEST_F(MdnsTest, BasicMdnsFunctionality) {
    std::cout << "=== Testing Basic mDNS Functionality ===" << std::endl;
    
    // Create mDNS client
    MdnsClient mdns("test-node", 8080);
    
    // Test starting the client
    EXPECT_TRUE(mdns.start()) << "Failed to start mDNS client";
    EXPECT_TRUE(mdns.is_running()) << "mDNS client should be running after start";
    
    std::cout << "✓ mDNS client started successfully" << std::endl;
    
    // Test service announcement
    std::map<std::string, std::string> txt_records;
    txt_records["version"] = "1.0";
    txt_records["protocol"] = "test";
    
    EXPECT_TRUE(mdns.announce_service("test-node", 8080, txt_records)) 
        << "Failed to announce service";
    EXPECT_TRUE(mdns.is_announcing()) << "mDNS client should be announcing after announce_service";
    
    std::cout << "✓ Service announcement started" << std::endl;
    
    // Test service discovery
    bool service_discovered = false;
    mdns.set_service_callback([&service_discovered](const MdnsService& service, bool is_new) {
        if (is_new) {
            std::cout << "✓ Discovered service: " << service.service_name 
                     << " at " << service.ip_address << ":" << service.port << std::endl;
            service_discovered = true;
        }
    });
    
    EXPECT_TRUE(mdns.start_discovery()) << "Failed to start service discovery";
    EXPECT_TRUE(mdns.is_discovering()) << "mDNS client should be discovering after start_discovery";
    
    std::cout << "✓ Service discovery started" << std::endl;
    
    // Query for services
    EXPECT_TRUE(mdns.query_services()) << "Failed to send service query";
    std::cout << "✓ Service query sent" << std::endl;
    
    // Wait a bit for network activity
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Check discovered services
    auto services = mdns.get_discovered_services();
    std::cout << "✓ Retrieved " << services.size() << " discovered services" << std::endl;
    
    // Stop the client
    mdns.stop();
    EXPECT_FALSE(mdns.is_running()) << "mDNS client should not be running after stop";
    
    std::cout << "✓ mDNS client stopped successfully" << std::endl;
    std::cout << "=== Basic mDNS Test Completed ===" << std::endl << std::endl;
}

TEST_F(MdnsTest, MdnsIntegrationWithRatsClient) {
    std::cout << "=== Testing mDNS Integration with RatsClient ===" << std::endl;
    
    // Create two RatsClient instances on different ports
    RatsClient client1(9001, 5);
    RatsClient client2(9002, 5);
    
    bool client1_discovered_service = false;
    bool client2_discovered_service = false;
    
    // Set mDNS callbacks
    client1.set_mdns_callback([&client1_discovered_service](const std::string& ip, int port, const std::string& service_name) {
        std::cout << "Client1 discovered mDNS service: " << service_name << " at " << ip << ":" << port << std::endl;
        client1_discovered_service = true;
    });
    
    client2.set_mdns_callback([&client2_discovered_service](const std::string& ip, int port, const std::string& service_name) {
        std::cout << "Client2 discovered mDNS service: " << service_name << " at " << ip << ":" << port << std::endl;
        client2_discovered_service = true;
    });
    
    // Start both clients
    EXPECT_TRUE(client1.start()) << "Failed to start client1";
    EXPECT_TRUE(client2.start()) << "Failed to start client2";
    
    std::cout << "✓ Both RatsClients started" << std::endl;
    
    // Start mDNS discovery on both clients
    std::map<std::string, std::string> txt_records1;
    txt_records1["node"] = "client1";
    txt_records1["version"] = "1.0";
    
    std::map<std::string, std::string> txt_records2;
    txt_records2["node"] = "client2";
    txt_records2["version"] = "1.0";
    
    EXPECT_TRUE(client1.start_mdns_discovery("test-client1", txt_records1)) 
        << "Failed to start mDNS discovery on client1";
    EXPECT_TRUE(client2.start_mdns_discovery("test-client2", txt_records2)) 
        << "Failed to start mDNS discovery on client2";
    
    EXPECT_TRUE(client1.is_mdns_running()) << "mDNS should be running on client1";
    EXPECT_TRUE(client2.is_mdns_running()) << "mDNS should be running on client2";
    
    std::cout << "✓ mDNS discovery started on both clients" << std::endl;
    
    // Wait for discovery and potential connections
    std::cout << "Waiting for mDNS discovery and auto-connection..." << std::endl;
    
    bool clients_connected = false;
    for (int i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        std::cout << "Client1 peers: " << client1.get_peer_count() 
                 << ", Client2 peers: " << client2.get_peer_count() << std::endl;
        
        // Check if clients have connected to each other
        if (client1.get_peer_count() > 0 && client2.get_peer_count() > 0) {
            std::cout << "✓ Clients successfully connected via mDNS discovery!" << std::endl;
            clients_connected = true;
            break;
        }
    }
    
    // Get mDNS services
    auto services1 = client1.get_mdns_services();
    auto services2 = client2.get_mdns_services();
    
    std::cout << "Client1 discovered " << services1.size() << " mDNS services" << std::endl;
    std::cout << "Client2 discovered " << services2.size() << " mDNS services" << std::endl;
    
    // Test manual mDNS query
    EXPECT_TRUE(client1.query_mdns_services()) << "Failed to send mDNS query from client1";
    EXPECT_TRUE(client2.query_mdns_services()) << "Failed to send mDNS query from client2";
    
    std::cout << "✓ Manual mDNS queries sent" << std::endl;
    
    // Stop mDNS discovery
    client1.stop_mdns_discovery();
    client2.stop_mdns_discovery();
    
    EXPECT_FALSE(client1.is_mdns_running()) << "mDNS should not be running on client1 after stop";
    EXPECT_FALSE(client2.is_mdns_running()) << "mDNS should not be running on client2 after stop";
    
    std::cout << "✓ mDNS discovery stopped on both clients" << std::endl;
    
    // Stop clients
    client1.stop();
    client2.stop();
    
    std::cout << "✓ Both RatsClients stopped" << std::endl;
    std::cout << "=== mDNS Integration Test Completed ===" << std::endl << std::endl;
}

TEST_F(MdnsTest, MdnsTxtRecords) {
    std::cout << "=== Testing mDNS TXT Records ===" << std::endl;
    
    MdnsClient mdns("txt-test-node", 8090);
    
    // Start mDNS client
    EXPECT_TRUE(mdns.start()) << "Failed to start mDNS client for TXT records test";
    
    // Create TXT records with various data
    std::map<std::string, std::string> txt_records;
    txt_records["version"] = "2.1.0";
    txt_records["protocol"] = "librats";
    txt_records["features"] = "encryption,dht,mdns";
    txt_records["max_peers"] = "10";
    txt_records["node_id"] = "test-node-12345";
    
    // Announce service with TXT records
    EXPECT_TRUE(mdns.announce_service("txt-test-node", 8090, txt_records)) 
        << "Failed to announce service with TXT records";
    
    std::cout << "✓ Service announced with TXT records" << std::endl;
    
    // Set up discovery callback to check TXT records
    bool txt_records_verified = false;
    mdns.set_service_callback([&txt_records_verified, &txt_records](const MdnsService& service, bool is_new) {
        if (is_new && service.service_name.find("txt-test-node") != std::string::npos) {
            std::cout << "Verifying TXT records for service: " << service.service_name << std::endl;
            
            for (const auto& pair : service.txt_records) {
                std::cout << "  " << pair.first << " = " << pair.second << std::endl;
                
                // Verify some key TXT records
                auto it = txt_records.find(pair.first);
                if (it != txt_records.end() && it->second == pair.second) {
                    std::cout << "  ✓ TXT record verified: " << pair.first << std::endl;
                }
            }
            
            txt_records_verified = true;
        }
    });
    
    // Start discovery
    EXPECT_TRUE(mdns.start_discovery()) << "Failed to start discovery for TXT records test";
    
    // Wait for self-discovery (may not happen due to loopback filtering)
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    std::cout << "✓ TXT records test completed" << std::endl;
    
    // Stop client
    mdns.stop();
    
    std::cout << "=== mDNS TXT Records Test Completed ===" << std::endl << std::endl;
}
