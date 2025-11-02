# Custom Protocol Configuration

The librats library now supports customizable protocol names and versions for enhanced flexibility and network segregation.

## Overview

By default, librats uses:
- Protocol name: `"rats"`
- Protocol version: `"1.0"`
- DHT discovery hash: SHA1 of `"rats_peer_discovery_v1.0"`

With the new functionality, you can customize these values to create isolated networks or versioned protocols.

## API Methods

### Setting Custom Protocol Configuration

```cpp
// Set custom protocol name (affects handshakes and DHT discovery)
void set_protocol_name(const std::string& protocol_name);

// Set custom protocol version (affects handshakes)
void set_protocol_version(const std::string& protocol_version);
```

### Getting Current Configuration

```cpp
// Get current protocol name
std::string get_protocol_name() const;

// Get current protocol version
std::string get_protocol_version() const;

// Get discovery hash based on current protocol configuration
std::string get_discovery_hash() const;

// Get the default RATS discovery hash (static method)
static std::string get_rats_peer_discovery_hash();
```

## Usage Examples

### Basic Custom Protocol Setup

```cpp
#include "librats.h"

librats::RatsClient client(8080);

// Set custom protocol
client.set_protocol_name("myapp");
client.set_protocol_version("3.2");

// Start client and discovery
client.start();
client.start_dht_discovery();
client.start_automatic_peer_discovery();
```

### Network Segregation

Different protocol names create isolated networks:

```cpp
// Network A - only connects to other "network_a" peers
librats::RatsClient clientA(8080);
clientA.set_protocol_name("network_a");
clientA.set_protocol_version("1.0");

// Network B - only connects to other "network_b" peers  
librats::RatsClient clientB(8081);
clientB.set_protocol_name("network_b");
clientB.set_protocol_version("1.0");
```

### Version Compatibility

Peers with different protocol versions will reject each other during handshake:

```cpp
// Client with version 2.0
librats::RatsClient client_v2(8080);
client_v2.set_protocol_version("2.0");

// Client with version 1.0 - these won't connect to each other
librats::RatsClient client_v1(8081);
client_v1.set_protocol_version("1.0");
```

## DHT Discovery Hash Generation

The discovery hash is generated as:
```
SHA1(protocol_name + "_peer_discovery_v" + protocol_version)
```

Examples:
- `"rats"` + `"1.0"` → SHA1(`"rats_peer_discovery_v1.0"`)
- `"myapp"` + `"2.5"` → SHA1(`"myapp_peer_discovery_v2.5"`)
- `"game"` + `"beta"` → SHA1(`"game_peer_discovery_vbeta"`)
