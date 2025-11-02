# mDNS (Multicast DNS) Peer Discovery in librats

## Overview

The librats library includes comprehensive mDNS (multicast DNS) support for automatic peer discovery on local networks. This feature enables librats nodes to automatically find and connect to each other on the same local network segment without requiring external infrastructure like DHT bootstrap nodes or manual configuration.

## Features

- **Service Advertisement**: Announce librats services on the local network
- **Service Discovery**: Automatically discover other librats services
- **TXT Records**: Support for custom metadata in service announcements
- **Auto-Connection**: Automatically connect to discovered peers
- **Integration**: Seamless integration with existing DHT and STUN discovery
- **Cross-Platform**: Works on Windows, Linux, and macOS

## How It Works

mDNS uses multicast UDP packets on the local network to advertise and discover services. librats implements the following mDNS records:

- **PTR Records**: Point to service instances (`_librats._tcp.local.`)
- **SRV Records**: Provide hostname and port information
- **TXT Records**: Contain custom metadata about the service
- **A Records**: Provide IPv4 address resolution

### Network Details

- **Multicast Address**: 224.0.0.251 (IPv4)
- **Port**: 5353 (standard mDNS port)
- **Service Type**: `_librats._tcp.local.`
- **TTL**: 120 seconds (default)

## Usage

### Basic Usage with RatsClient

```cpp
#include "librats.h"

// Create a RatsClient
RatsClient client(8080, 10);

// Start the client
client.start();

// Start mDNS discovery with custom TXT records
std::map<std::string, std::string> txt_records;
txt_records["version"] = "1.0";
txt_records["features"] = "encryption,dht";
txt_records["max_peers"] = "10";

client.start_mdns_discovery("my-node", txt_records);

// Set callback for discovered services
client.set_mdns_callback([](const std::string& ip, int port, const std::string& service_name) {
    std::cout << "Discovered librats service: " << service_name 
              << " at " << ip << ":" << port << std::endl;
});

// Get list of discovered services
auto services = client.get_mdns_services();
for (const auto& service : services) {
    std::cout << "Service: " << service.service_name 
              << " at " << service.ip_address << ":" << service.port << std::endl;
}

// Stop mDNS discovery
client.stop_mdns_discovery();
```

### Direct mDNS Client Usage

```cpp
#include "mdns.h"

// Create mDNS client
MdnsClient mdns("my-service", 8080);

// Start the client
mdns.start();

// Announce service
std::map<std::string, std::string> txt_records;
txt_records["version"] = "1.0";
txt_records["protocol"] = "librats";

mdns.announce_service("my-service", 8080, txt_records);

// Set discovery callback
mdns.set_service_callback([](const MdnsService& service, bool is_new) {
    if (is_new) {
        std::cout << "New service: " << service.service_name 
                  << " at " << service.ip_address << ":" << service.port << std::endl;
        
        // Print TXT records
        for (const auto& pair : service.txt_records) {
            std::cout << "  " << pair.first << " = " << pair.second << std::endl;
        }
    }
});

// Start discovery
mdns.start_discovery();

// Manual query
mdns.query_services();

// Stop everything
mdns.stop();
```

## API Reference

### RatsClient mDNS Methods

#### `bool start_mdns_discovery(const std::string& service_instance_name, const std::map<std::string, std::string>& txt_records)`

Starts mDNS service discovery and announcement.

- **Parameters**:
  - `service_instance_name`: Name for this service instance (optional, auto-generated if empty)
  - `txt_records`: Key-value pairs for TXT record metadata
- **Returns**: `true` if successful, `false` otherwise

#### `void stop_mdns_discovery()`

Stops mDNS service discovery and announcement.

#### `bool is_mdns_running()`

Checks if mDNS discovery is currently running.

- **Returns**: `true` if mDNS is active, `false` otherwise

#### `void set_mdns_callback(std::function<void(const std::string&, int, const std::string&)> callback)`

Sets callback function for mDNS service discovery.

- **Parameters**:
  - `callback`: Function called when services are discovered (ip, port, service_name)

#### `std::vector<MdnsService> get_mdns_services()`

Gets list of recently discovered mDNS services.

- **Returns**: Vector of `MdnsService` objects

#### `bool query_mdns_services()`

Manually sends an mDNS query for librats services.

- **Returns**: `true` if query was sent successfully

### MdnsClient Direct API

#### Core Methods

- `MdnsClient(const std::string& service_instance_name, uint16_t service_port)`
- `bool start()` / `void stop()` / `bool is_running()`
- `bool announce_service(...)` / `void stop_announcing()` / `bool is_announcing()`
- `bool start_discovery()` / `void stop_discovery()` / `bool is_discovering()`

#### Configuration

- `void set_announcement_interval(std::chrono::seconds interval)` - Default: 60s
- `void set_query_interval(std::chrono::seconds interval)` - Default: 30s
- `void set_service_callback(MdnsServiceCallback callback)`

#### Service Management

- `std::vector<MdnsService> get_discovered_services()`
- `std::vector<MdnsService> get_recent_services(std::chrono::seconds max_age)`
- `void clear_old_services(std::chrono::seconds max_age)` - Default: 600s

## Data Structures

### MdnsService

```cpp
struct MdnsService {
    std::string service_name;      // e.g., "my-node._librats._tcp.local."
    std::string host_name;         // e.g., "MyComputer.local."
    std::string ip_address;        // IPv4 or IPv6 address
    uint16_t port;                 // Service port
    std::map<std::string, std::string> txt_records;  // TXT record key-value pairs
    std::chrono::steady_clock::time_point last_seen; // Last discovery time
};
```

## Integration with Other Discovery Methods

The mDNS implementation works alongside existing discovery methods:

1. **DHT Discovery**: For wide-area peer discovery
2. **STUN**: For NAT traversal and public IP discovery
3. **mDNS**: For local network discovery
4. **Manual Connection**: Direct peer connections

### Recommended Usage Pattern

```cpp
RatsClient client(8080, 10);
client.start();

// Start all discovery methods
client.start_dht_discovery(6881);           // DHT for wide-area discovery
client.discover_and_ignore_public_ip();     // STUN for NAT traversal
client.start_mdns_discovery("my-node");     // mDNS for local discovery

// Set callbacks for each discovery method
client.set_mdns_callback([](const std::string& ip, int port, const std::string& name) {
    std::cout << "Local peer discovered via mDNS: " << name << std::endl;
});

// The client will automatically use all discovery methods
```

## Network Requirements

### Firewall Configuration

- **UDP Port 5353**: Must be open for mDNS multicast traffic
- **Multicast**: Must allow multicast traffic on 224.0.0.251

### Network Topology

- Works on **any local network segment** where multicast is supported
- Does **not work across routers** (local network only)
- Ideal for **LAN environments**, **home networks**, and **office networks**

## Security Considerations

### Network Exposure

- Services are **advertised on the local network**
- Consider **TXT record content** carefully (avoid sensitive information)
- Use **encryption** for actual peer communication

### Best Practices

```cpp
// Good: Generic information
std::map<std::string, std::string> txt_records;
txt_records["version"] = "1.0";
txt_records["protocol"] = "librats";
txt_records["features"] = "encryption";

// Avoid: Sensitive information
txt_records["api_key"] = "secret123";      // DON'T DO THIS
txt_records["username"] = "admin";         // DON'T DO THIS
```

## Troubleshooting

### Common Issues

1. **No services discovered**:
   - Check firewall allows UDP 5353
   - Verify multicast is enabled on network interface
   - Ensure peers are on same network segment

2. **Services discovered but connections fail**:
   - Check if peer ports are accessible
   - Verify no IP address conflicts
   - Check peer limit configuration

3. **High network traffic**:
   - Adjust announcement/query intervals
   - Reduce TXT record size
   - Implement proper service filtering

### Debug Logging

Enable debug logging to troubleshoot issues:

```cpp
// Check mDNS-specific logs
LOG_MDNS_DEBUG("Debug message");
LOG_MDNS_INFO("Info message");
LOG_MDNS_WARN("Warning message");
LOG_MDNS_ERROR("Error message");
```

### Network Analysis

Use network tools to analyze mDNS traffic:

```bash
# Capture mDNS traffic
tcpdump -i any -n port 5353

# Monitor multicast traffic
tcpdump -i any -n host 224.0.0.251

# Use mDNS debugging tools
dns-sd -B _librats._tcp.     # Browse for librats services (macOS)
avahi-browse _librats._tcp   # Browse for librats services (Linux)
```

## Examples

### Simple Peer Discovery

```cpp
#include "librats.h"

int main() {
    RatsClient client(8080);
    
    client.set_connection_callback([](socket_t socket, const std::string& peer_id) {
        std::cout << "Connected to peer: " << peer_id << std::endl;
    });
    
    client.start();
    client.start_mdns_discovery("example-node");
    
    std::cout << "Press Enter to stop..." << std::endl;
    std::cin.get();
    
    client.stop();
    return 0;
}
```

### Custom Service Metadata

```cpp
std::map<std::string, std::string> metadata;
metadata["node_type"] = "storage";
metadata["capacity"] = "1TB";
metadata["location"] = "datacenter-1";
metadata["availability"] = "99.9%";

client.start_mdns_discovery("storage-node-001", metadata);
```

### Service Filtering

```cpp
client.set_mdns_callback([](const std::string& ip, int port, const std::string& service_name) {
    // Only connect to storage nodes
    if (service_name.find("storage-node") != std::string::npos) {
        std::cout << "Found storage node: " << service_name << std::endl;
        // Custom connection logic here
    }
});
```
