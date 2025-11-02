# NAT Traversal in librats

## Overview

librats now includes comprehensive NAT (Network Address Translation) traversal capabilities, enabling peer-to-peer connections across NATs and firewalls. The implementation includes:

- **ICE (Interactive Connectivity Establishment)** - RFC 8445 compliant
- **STUN (Session Traversal Utilities for NAT)** - RFC 5389 compliant  
- **TURN (Traversal Using Relays around NAT)** - RFC 5766 compliant
- **UDP/TCP Hole Punching** - Coordinated NAT traversal
- **Advanced NAT Type Detection** - Detailed NAT behavior analysis
- **Automatic Strategy Selection** - Choose optimal connection method

## Features

### ✅ **ICE (Interactive Connectivity Establishment)**
- Full candidate gathering (host, server reflexive, relay)
- Connectivity checks with priority ordering
- Role negotiation (controlling/controlled)
- Candidate pair formation and nomination
- Real-time connection state tracking
- **Automatic candidate exchange via librats message protocol**

### ✅ **STUN Support**  
- Public IP address discovery
- NAT type detection (Open Internet, Full Cone, Restricted Cone, Port Restricted, Symmetric)
- Multiple STUN server support
- Message integrity with HMAC-SHA1
- ICE-specific STUN extensions

### ✅ **TURN Relay**
- TURN allocation for relay candidates
- Permission management
- Data relay through TURN servers
- Allocation refresh and management
- Authentication support

### ✅ **Hole Punching**
- UDP hole punching coordination
- TCP hole punching (where supported)
- Peer coordination through DHT/signaling
- Multiple attempt strategies

### ✅ **Advanced NAT Detection**
- Detailed NAT behavior analysis
- Filtering behavior detection
- Mapping behavior detection  
- Port preservation testing
- Hairpin support detection
- **Automatic NAT information exchange between peers**

## Quick Start

### Basic Usage with Automatic NAT Traversal

```cpp
#include "librats.h"

int main() {
    // Create NAT traversal configuration
    librats::NatTraversalConfig nat_config;
    nat_config.enable_ice = true;
    nat_config.enable_hole_punching = true;
    nat_config.enable_turn_relay = true;
    
    // Add TURN servers (optional)
    nat_config.turn_servers.push_back("turn.example.com:3478");
    nat_config.turn_usernames.push_back("username");
    nat_config.turn_passwords.push_back("password");
    
    // Create client with NAT traversal
    librats::RatsClient client(8080, 10, nat_config);
    
    // Set connection callback to track NAT traversal results
    client.set_advanced_connection_callback([](socket_t socket, const std::string& peer_id, 
                                              const librats::ConnectionAttemptResult& result) {
        std::cout << "✅ Connected via: " << result.method 
                  << " in " << result.duration.count() << "ms" << std::endl;
        std::cout << "📊 Local NAT: " << (int)result.local_nat_type 
                  << ", Remote NAT: " << (int)result.remote_nat_type << std::endl;
    });
    
    // Set NAT traversal progress callback
    client.set_nat_traversal_progress_callback([](const std::string& peer_id, const std::string& status) {
        std::cout << "🔄 NAT traversal for " << peer_id << ": " << status << std::endl;
    });
    
    // Set ICE candidate discovery callback
    client.set_ice_candidate_callback([](const std::string& peer_id, const librats::IceCandidate& candidate) {
        std::cout << "🧊 ICE candidate: " << candidate.ip << ":" << candidate.port 
                  << " (type: " << (int)candidate.type << ")" << std::endl;
    });
    
    // Start with all discovery methods
    client.start();
    client.start_dht_discovery();           // Wide-area discovery
    client.start_mdns_discovery();         // Local network discovery
    client.discover_and_ignore_public_ip(); // NAT traversal setup
    
    // Connect with automatic strategy selection
    client.connect_to_peer("peer.example.com", 8081, 
                          librats::ConnectionStrategy::AUTO_ADAPTIVE);
    
    return 0;
}
```

### ICE-Coordinated Connection

The new implementation automatically handles ICE coordination when peers connect:

```cpp
// Peer A (Initiator)
librats::NatTraversalConfig config;
config.enable_ice = true;
config.enable_hole_punching = true;

librats::RatsClient clientA(8080, 10, config);
clientA.start();

// ICE coordination happens automatically when connecting
clientA.connect_to_peer("peer.example.com", 8081, librats::ConnectionStrategy::ICE_FULL);

// The client will automatically:
// 1. Detect NAT type and gather ICE candidates
// 2. Exchange NAT information with the peer
// 3. Send ICE offer to the peer
// 4. Process ICE answer from the peer
// 5. Perform connectivity checks
// 6. Establish optimal connection
```

### Manual Connection Strategy Testing

```cpp
librats::RatsClient client(8080);
client.start();

// Test different connection strategies
std::vector<librats::ConnectionStrategy> strategies = {
    librats::ConnectionStrategy::DIRECT_ONLY,
    librats::ConnectionStrategy::STUN_ASSISTED,
    librats::ConnectionStrategy::ICE_FULL,
    librats::ConnectionStrategy::TURN_RELAY
};

auto results = client.test_connection_strategies("target.example.com", 8081, strategies);

for (const auto& result : results) {
    std::cout << "📈 Strategy " << result.method << ": " 
              << (result.success ? "✅ SUCCESS" : "❌ FAILED") 
              << " (" << result.duration.count() << "ms)" << std::endl;
              
    if (!result.error_message.empty()) {
        std::cout << "   Error: " << result.error_message << std::endl;
    }
}
```

### Monitoring NAT Traversal and Connection Statistics

```cpp
librats::RatsClient client(8080);
client.start();

// Get detailed connection statistics
auto stats = client.get_connection_statistics();
std::cout << "Connection Statistics: " << stats.dump(2) << std::endl;

// Get NAT traversal specific statistics  
auto nat_stats = client.get_nat_traversal_statistics();
std::cout << "NAT Statistics: " << nat_stats.dump(2) << std::endl;

// Example output:
// {
//   "detected_nat_type": 2,
//   "has_nat": true,
//   "filtering_behavior": 1,
//   "mapping_behavior": 0,
//   "preserves_port": false,
//   "hairpin_support": false,
//   "ice_available": true,
//   "ice_running": true,
//   "ice_state": 3,
//   "local_ice_candidates": 4
// }
```

## Configuration

### NAT Traversal Configuration

```cpp
librats::NatTraversalConfig config;

// Enable/disable features
config.enable_ice = true;              // ICE for full NAT traversal
config.enable_upnp = false;            // UPnP port mapping (future)
config.enable_hole_punching = true;    // UDP/TCP hole punching
config.enable_turn_relay = true;       // TURN relay as fallback
config.prefer_ipv6 = false;            // Prefer IPv6 when available

// STUN servers for public IP discovery
config.stun_servers = {
    "stun.l.google.com:19302",
    "stun1.l.google.com:19302", 
    "stun.stunprotocol.org:3478"
};

// TURN servers for relay (optional)
config.turn_servers = {"turn.example.com:3478"};
config.turn_usernames = {"username"};
config.turn_passwords = {"password"};

// Timeouts and limits
config.ice_gathering_timeout_ms = 10000;      // 10 seconds
config.ice_connectivity_timeout_ms = 30000;   // 30 seconds
config.hole_punch_attempts = 5;               // 5 attempts
config.turn_allocation_timeout_ms = 10000;    // 10 seconds

// Candidate priorities (higher = preferred)
config.host_candidate_priority = 65535;       // Direct local connection
config.server_reflexive_priority = 65534;     // STUN-discovered public IP
config.relay_candidate_priority = 65533;      // TURN relay
```

### Connection Strategies

```cpp
enum class ConnectionStrategy {
    DIRECT_ONLY,        // Try direct connection only (no NAT traversal)
    STUN_ASSISTED,      // Use STUN for public IP discovery
    ICE_FULL,           // Full ICE with candidate gathering  
    TURN_RELAY,         // Force TURN relay usage
    AUTO_ADAPTIVE       // Automatically choose best strategy (recommended)
};
```

## Advanced Features

### NAT Type Detection

```cpp
librats::RatsClient client(8080);
client.start();

// Detect NAT type
auto nat_type = client.detect_nat_type();
std::cout << "NAT Type: " << (int)nat_type << std::endl;

// Get detailed NAT characteristics
auto nat_info = client.get_nat_characteristics();
std::cout << "Has NAT: " << nat_info.has_nat << std::endl;
std::cout << "Filtering: " << (int)nat_info.filtering_behavior << std::endl;
std::cout << "Mapping: " << (int)nat_info.mapping_behavior << std::endl;
std::cout << "Port Preservation: " << nat_info.preserves_port << std::endl;
std::cout << "Hairpin Support: " << nat_info.hairpin_support << std::endl;
```

### Connection Statistics

```cpp
// Get detailed connection statistics
auto stats = client.get_connection_statistics();
std::cout << "Connection Statistics: " << stats.dump(2) << std::endl;

// Get NAT traversal specific statistics  
auto nat_stats = client.get_nat_traversal_statistics();
std::cout << "NAT Statistics: " << nat_stats.dump(2) << std::endl;
```

### ICE Candidate Monitoring

```cpp
client.set_ice_candidate_callback([](const std::string& peer_id, 
                                   const librats::IceCandidate& candidate) {
    
    std::cout << "ICE Candidate Discovered:" << std::endl;
    std::cout << "  Peer ID: " << peer_id << std::endl;
    std::cout << "  Type: " << (int)candidate.type << std::endl;
    std::cout << "  Address: " << candidate.ip << ":" << candidate.port << std::endl;
    std::cout << "  Priority: " << candidate.priority << std::endl;
    std::cout << "  Foundation: " << candidate.foundation << std::endl;
});

client.set_nat_traversal_progress_callback([](const std::string& peer_id, 
                                             const std::string& status) {
    std::cout << "NAT traversal progress for " << peer_id << ": " << status << std::endl;
});
```

### Coordinated Hole Punching

```cpp
// Coordinate hole punching with peer
nlohmann::json coordination_data;
coordination_data["method"] = "udp_hole_punch";
coordination_data["local_candidates"] = client.get_local_ice_candidates();
coordination_data["timing"] = "synchronized";

bool success = client.coordinate_hole_punching("peer.example.com", 8081, coordination_data);
```

## Automatic Features

### NAT Information Exchange

The new implementation automatically exchanges NAT information between peers when they connect. This includes:

- Detected NAT type
- Public IP address  
- NAT behavior characteristics
- Port preservation capabilities
- Hairpin support

### ICE Coordination

For outgoing connections with ICE enabled, the client automatically:

1. **Detects local NAT type and characteristics**
2. **Gathers ICE candidates** (host, server reflexive, relay)
3. **Exchanges candidates with peer** via librats messaging protocol
4. **Performs connectivity checks** to find best connection path
5. **Establishes optimal connection** based on results

### Automatic Strategy Selection

When using `ConnectionStrategy::AUTO_ADAPTIVE`, the client automatically selects the best strategy based on detected NAT type:

- **Open Internet**: Direct connection
- **Full/Restricted Cone NAT**: ICE (if enabled) or STUN
- **Port Restricted NAT**: ICE or hole punching
- **Symmetric NAT**: TURN relay or ICE
- **Unknown NAT**: ICE (if available) or STUN

## Protocol Messages

The NAT traversal implementation uses the existing librats message protocol for coordination:

### ICE Messages

```json
// ICE Offer
{
  "rats_protocol": true,
  "type": "ice_offer",
  "payload": {
    "peer_id": "sender_peer_id",
    "target_peer_id": "target_peer_id", 
    "ice_ufrag": "username_fragment",
    "ice_pwd": "password",
    "candidates": [
      {
        "ip": "192.168.1.100",
        "port": 54321,
        "type": 0,
        "priority": 65535,
        "foundation": "1"
      }
    ]
  }
}

// ICE Answer (same format as offer)
{
  "rats_protocol": true,
  "type": "ice_answer", 
  "payload": { /* same as offer */ }
}

// ICE Candidate
{
  "rats_protocol": true,
  "type": "ice_candidate",
  "payload": {
    "ip": "203.0.113.1",
    "port": 12345,
    "type": 1,
    "priority": 65534,
    "foundation": "2"
  }
}
```

### NAT Information Exchange

```json
{
  "rats_protocol": true,
  "type": "nat_info_exchange",
  "payload": {
    "nat_type": 2,
    "has_nat": true,
    "public_ip": "203.0.113.1",
    "filtering_behavior": 1,
    "mapping_behavior": 0,
    "preserves_port": false,
    "hairpin_support": false,
    "response": false
  }
}
```

### Hole Punching Coordination

```json
{
  "rats_protocol": true,
  "type": "hole_punch_coordination",
  "payload": {
    "method": "udp_hole_punch",
    "peer_ip": "peer.example.com",
    "peer_port": 8081,
    "timing": "synchronized",
    "attempts": 5
  }
}
```

## Network Requirements

### Firewall Configuration

**Outbound (Required):**
- UDP port 3478 (STUN)
- UDP port 5349 (STUN over TLS)
- TCP/UDP ports for TURN servers
- UDP/TCP ports for peer communication

**Inbound (Recommended):**
- Your application's listen port
- UDP port range for ICE candidates

### NAT Compatibility

| NAT Type | Direct | STUN | ICE | TURN | Success Rate |
|----------|--------|------|-----|------|--------------|
| Open Internet | ✅ | ✅ | ✅ | ✅ | 100% |
| Full Cone | ❌ | ✅ | ✅ | ✅ | 95% |
| Restricted Cone | ❌ | ✅ | ✅ | ✅ | 90% |
| Port Restricted | ❌ | ✅ | ✅ | ✅ | 85% |
| Symmetric | ❌ | ❌ | ⚠️ | ✅ | 70% |
| Symmetric + Symmetric | ❌ | ❌ | ❌ | ✅ | 99% |

## Troubleshooting

### Common Issues

**ICE gathering fails:**
```cpp
// Check STUN servers are reachable
auto public_ip = client.get_public_ip();
if (public_ip.empty()) {
    std::cout << "STUN servers unreachable" << std::endl;
}

// Verify network connectivity
auto nat_stats = client.get_nat_traversal_statistics();
if (!nat_stats.value("ice_available", false)) {
    std::cout << "ICE agent not available" << std::endl;
}
```

**Connection attempts fail:**
```cpp
// Test individual strategies
auto results = client.test_connection_strategies(host, port, {
    librats::ConnectionStrategy::DIRECT_ONLY,
    librats::ConnectionStrategy::STUN_ASSISTED,
    librats::ConnectionStrategy::ICE_FULL
});

for (const auto& result : results) {
    if (!result.success) {
        std::cout << "Strategy " << result.method << " failed: " 
                  << result.error_message << std::endl;
    }
}
```

**NAT detection issues:**
```cpp
// Force NAT detection refresh
auto nat_characteristics = client.get_nat_characteristics();
if (nat_characteristics.description.empty()) {
    // NAT detection may have failed
    std::cout << "NAT detection incomplete" << std::endl;
}
```

### Performance Tuning

**Optimize ICE timeouts:**
```cpp
librats::NatTraversalConfig config;
config.ice_gathering_timeout_ms = 5000;    // Faster gathering
config.ice_connectivity_timeout_ms = 15000; // Shorter connectivity checks
```

**Reduce STUN server load:**
```cpp
config.stun_servers = {"stun.l.google.com:19302"}; // Use fewer servers
```

**Priority optimization:**
```cpp
config.host_candidate_priority = 65535;      // Prefer direct connections
config.server_reflexive_priority = 65534;    // Then STUN
config.relay_candidate_priority = 65533;     // TURN as last resort
```

## Security Considerations

### Network Security
- Use secure STUN/TURN servers with TLS when possible
- Monitor for excessive candidate gathering (potential DDoS)
- Validate ICE candidates before processing
- Rate limit connection attempts

### TURN Security
- Use authenticated TURN servers
- Rotate TURN credentials regularly
- Monitor TURN usage for abuse

### ICE Security
- ICE credentials are generated randomly and rotated per session
- Consider additional authentication after ICE completes
- Monitor for ICE flooding attacks
- Validate candidate addresses

### Application Security
```cpp
// Validate peer after successful ICE connection
client.set_advanced_connection_callback([](socket_t socket, const std::string& peer_id,
                                          const librats::ConnectionAttemptResult& result) {
    // Additional peer validation
    if (!validate_peer_certificate(peer_id)) {
        client.disconnect_peer(socket);
        return;
    }
    
    // Connection established and validated
    std::cout << "Secure connection via " << result.method << std::endl;
});
```

## API Reference

### Core Classes

- **`RatsClient`** - Main client with enhanced NAT traversal
- **`IceAgent`** - ICE implementation for connectivity establishment  
- **`StunClient`** - Enhanced STUN client with ICE support
- **`TurnClient`** - TURN client for relay functionality
- **`NatTypeDetector`** - Advanced NAT type detection
- **`AdvancedNatDetector`** - Detailed NAT behavior analysis

### Key Enums

- **`ConnectionStrategy`** - Connection establishment strategies
- **`NatType`** - Detected NAT types
- **`IceConnectionState`** - ICE connection states
- **`IceCandidateType`** - ICE candidate types

### Configuration Structures

- **`NatTraversalConfig`** - NAT traversal configuration
- **`IceConfig`** - ICE-specific configuration
- **`ConnectionAttemptResult`** - Connection attempt results

### Callback Types

- **`AdvancedConnectionCallback`** - Enhanced connection callback with NAT info
- **`NatTraversalProgressCallback`** - NAT traversal progress updates
- **`IceCandidateDiscoveredCallback`** - ICE candidate discovery notifications
