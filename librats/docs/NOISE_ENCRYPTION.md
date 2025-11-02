# 🔐 Noise Protocol Encryption in librats

This document describes the Noise Protocol encryption implementation in librats, which provides end-to-end encryption for all P2P communications.

## Overview

librats now includes built-in **Noise Protocol** encryption using the **Noise_XX** pattern with:
- **Curve25519** for ECDH key exchange
- **ChaCha20-Poly1305** for AEAD encryption
- **SHA256** for hashing and HKDF

This provides:
- ✅ **Mutual Authentication** - Both peers verify each other's identity
- ✅ **Forward Secrecy** - Past communications remain secure even if keys are compromised
- ✅ **Resistance to Key Compromise Impersonation** - Protects against advanced attacks
- ✅ **Transport Encryption** - All data is encrypted after handshake completion

## Features

### 🔑 **Automatic Key Management**
- Static keys are automatically generated and persisted
- Keys are stored securely in the configuration file
- Support for manual key import/export

### 🤝 **Seamless Integration**
- Encryption is enabled by default
- Automatic handshake during connection establishment
- Transparent encryption/decryption for all communications
- Backward compatibility with unencrypted mode

### 🛡️ **Security Properties**
- **Authentication**: Both peers verify each other's static keys
- **Confidentiality**: All data is encrypted with unique session keys
- **Integrity**: Messages are authenticated and tamper-proof
- **Perfect Forward Secrecy**: Session keys are ephemeral

## Usage

### Basic Usage (Encryption Enabled by Default)

```cpp
#include "librats.h"

int main() {
    // Create client with encryption enabled (default)
    librats::RatsClient client(8080);
    
    // Start the client - encryption is automatically initialized
    client.start();
    
    // Connect to peers - encryption handshake happens automatically
    client.connect_to_peer("192.168.1.100", 8081);
    
    // All communications are now encrypted
    client.broadcast_to_peers("This message is encrypted!");
    
    return 0;
}
```

### Advanced Encryption Management

```cpp
#include "librats.h"

int main() {
    librats::RatsClient client(8080);
    
    // Check encryption status
    bool encrypted = client.is_encryption_enabled();
    std::cout << "Encryption enabled: " << encrypted << std::endl;
    
    // Get the static encryption key
    std::string key = client.get_encryption_key();
    std::cout << "Static key: " << key << std::endl;
    
    // Generate a new encryption key
    std::string new_key = client.generate_new_encryption_key();
    std::cout << "New key generated: " << new_key << std::endl;
    
    // Set a specific encryption key
    client.set_encryption_key("your_64_character_hex_key_here...");
    
    // Start the client
    client.start();
    
    // Set up connection callback to check encryption status
    client.set_connection_callback([&](auto socket, const std::string& peer_id) {
        bool peer_encrypted = client.is_peer_encrypted(peer_id);
        std::cout << "Peer " << peer_id << " encryption: " 
                  << (peer_encrypted ? "ENCRYPTED" : "UNENCRYPTED") << std::endl;
    });
    
    return 0;
}
```

### Disabling Encryption (For Testing/Legacy Support)

```cpp
#include "librats.h"

int main() {
    librats::RatsClient client(8080);
    
    // Disable encryption
    client.set_encryption_enabled(false);
    
    // Start the client - no encryption will be used
    client.start();
    
    // All communications will be unencrypted
    client.connect_to_peer("192.168.1.100", 8081);
    
    return 0;
}
```

## Configuration

### Static Key Persistence

Encryption keys are automatically saved to `config.json`:

```json
{
    "peer_id": "your_peer_id_here",
    "encryption_enabled": true,
    "encryption_key": "64_character_hex_encoded_static_private_key",
    "version": "1.0",
    "listen_port": 8080,
    "created_at": 1234567890,
    "last_updated": 1234567890
}
```

### Environment Variables

You can control encryption behavior via environment variables:

```bash
# Disable encryption globally
export LIBRATS_ENCRYPTION_DISABLED=1

# Set a specific encryption key
export LIBRATS_ENCRYPTION_KEY="your_64_character_hex_key"
```

## Security Considerations

### 🔐 **Key Management**
- **Static keys are generated automatically** using cryptographically secure random number generation
- **Keys are stored in hex format** in the configuration file
- **Backup your keys** - losing them means you can't authenticate with peers who know your old key
- **Rotate keys periodically** for enhanced security

### 🛡️ **Network Security**
- **Man-in-the-middle protection**: The Noise_XX pattern provides mutual authentication
- **Forward secrecy**: Session keys are ephemeral and not stored
- **Replay protection**: Each message has a unique nonce
- **Length hiding**: Message framing obscures payload lengths

### ⚠️ **Important Notes**
- **First connection security**: The first time two peers connect, they exchange static public keys. Ensure this happens over a secure channel for maximum security.
- **Key verification**: Consider implementing out-of-band key verification for high-security applications
- **Performance impact**: Encryption adds ~16 bytes overhead per message plus CPU overhead for crypto operations

## Protocol Details

### Handshake Flow (Noise_XX Pattern)

```
Initiator                    Responder
    |                            |
    |  -> e                      |  (Message 1: Send ephemeral key)
    |                            |
    |  <- e, ee, s, es           |  (Message 2: Send ephemeral key, do ECDH, send static key, do ECDH)
    |                            |
    |  -> s, se                  |  (Message 3: Send static key, do ECDH)
    |                            |
    |  [Transport encryption]    |  (All subsequent messages encrypted)
```

### Message Format

```
Handshake Messages:
[4 bytes: HANDSHAKE_MAGIC][4 bytes: NOISE_MAGIC][4 bytes: length][encrypted handshake data]

Transport Messages:
[4 bytes: NOISE_MAGIC][4 bytes: length][encrypted data + 16 byte auth tag]
```

### Cryptographic Primitives

- **Key Exchange**: Curve25519 ECDH
- **Encryption**: ChaCha20-Poly1305 AEAD
- **Hashing**: SHA256
- **Key Derivation**: HKDF-SHA256

## Performance

### Benchmarks (Approximate)

- **Handshake time**: ~1-5ms (depending on hardware)
- **Encryption overhead**: ~16 bytes per message + ~0.1ms CPU time
- **Memory overhead**: ~1KB per encrypted connection
- **Throughput impact**: ~5-10% reduction compared to unencrypted

### Optimization Tips

- **Batch messages** when possible to amortize encryption overhead
- **Use binary protocols** instead of JSON for high-throughput applications
- **Consider message size** - very small messages have higher relative overhead

## Troubleshooting

### Common Issues

**Problem**: Connection fails with "handshake timeout"
**Solution**: Check that both peers have compatible encryption settings

**Problem**: "Cannot send data - encryption handshake not completed"
**Solution**: Wait for handshake completion or check network connectivity

**Problem**: High CPU usage
**Solution**: Consider disabling encryption for high-throughput local connections

### Debug Logging

Enable debug logging to see encryption details:

```cpp
// Enable detailed encryption logging
LOG_SET_LEVEL("encrypt", LOG_LEVEL_DEBUG);
LOG_SET_LEVEL("noise", LOG_LEVEL_DEBUG);
```

### Testing Encryption

Run the noise protocol tests:

```bash
cd build
./bin/librats_tests --gtest_filter="NoiseTest.*"
```

## Migration Guide

### From Unencrypted to Encrypted

1. **Update all nodes** to a version with encryption support
2. **Enable encryption** on one node at a time
3. **Verify connectivity** between encrypted and unencrypted nodes (should fail as expected)
4. **Enable encryption** on all nodes
5. **Verify all connections** are now encrypted

### Key Rotation

```cpp
// Generate new key
std::string new_key = client.generate_new_encryption_key();

// Save the key securely
// Restart the client to use the new key
// Update peer configurations with the new public key
```
