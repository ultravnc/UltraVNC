# LibRats Node.js Bindings

Node.js bindings for librats - A high-performance peer-to-peer networking library with support for DHT, GossipSub, file transfer, NAT traversal, and more.

## Features

- **Peer-to-peer networking** - Direct connections between peers
- **DHT (Distributed Hash Table)** - Decentralized peer discovery
- **GossipSub** - Topic-based publish/subscribe messaging
- **File Transfer** - Send and receive files and directories
- **mDNS Discovery** - Local network peer discovery
- **NAT Traversal** - STUN/ICE support for connecting through firewalls
- **Encryption** - End-to-end encryption using Noise protocol
- **Message Exchange** - String, binary, and JSON message types
- **Configuration Persistence** - Save and restore client settings

## Installation

### Prerequisites

- Node.js 12.0.0 or higher
- Build tools for your platform:
  - **Windows**: Visual Studio Build Tools or Visual Studio Community
  - **Linux**: `build-essential` package (`sudo apt install build-essential`)
  - **macOS**: Xcode Command Line Tools (`xcode-select --install`)

### Building librats

First, build the native librats library:

```bash
# From the project root
mkdir build && cd build
cmake ..
cmake --build .
```

### Installing Node.js Dependencies

```bash
cd nodejs
npm install
```

This will automatically build the native addon using `node-gyp`.

## Quick Start

### Basic Example

```javascript
const { RatsClient, ConnectionStrategy } = require('librats');

// Create a client listening on port 8080
const client = new RatsClient(8080);

// Set up event handlers
client.onConnection((peerId) => {
  console.log(`Peer connected: ${peerId}`);
  client.sendString(peerId, 'Hello from Node.js!');
});

client.onString((peerId, message) => {
  console.log(`Received: ${message} from ${peerId}`);
});

// Start the client
client.start();

// Connect to another peer
client.connect('127.0.0.1', 8081);
```

### File Transfer Example

```javascript
const { RatsClient } = require('librats');

const client = new RatsClient(8080);

// Set up file transfer progress monitoring
client.onFileProgress((transferId, progressPercent, status) => {
  console.log(`Transfer ${transferId}: ${progressPercent}% - ${status}`);
});

client.start();

// Send a file to a peer
const transferId = client.sendFile('peer_id_here', './myfile.txt', 'remote_file.txt');
console.log(`File transfer started: ${transferId}`);
```

### GossipSub Chat Example

```javascript
const { RatsClient } = require('librats');

const client = new RatsClient(8080);
client.start();

// Subscribe to a topic
client.subscribeToTopic('general-chat');

// Publish a message
const message = {
  type: 'chat',
  username: 'Alice',
  message: 'Hello everyone!',
  timestamp: Date.now()
};

client.publishJsonToTopic('general-chat', JSON.stringify(message));
```

## API Reference

### RatsClient

#### Constructor

- `new RatsClient(listenPort: number)` - Create a new client instance

#### Basic Operations

- `start(): boolean` - Start the client
- `stop(): void` - Stop the client
- `connect(host: string, port: number): boolean` - Connect to a peer
- `connectWithStrategy(host: string, port: number, strategy: number): boolean` - Connect with specific strategy
- `disconnect(peerId: string): void` - Disconnect from a peer

#### Messaging

- `sendString(peerId: string, message: string): boolean` - Send string message
- `sendBinary(peerId: string, data: Buffer): boolean` - Send binary data
- `sendJson(peerId: string, jsonStr: string): boolean` - Send JSON data
- `broadcastString(message: string): number` - Broadcast string to all peers
- `broadcastBinary(data: Buffer): number` - Broadcast binary to all peers
- `broadcastJson(jsonStr: string): number` - Broadcast JSON to all peers

#### Information

- `getPeerCount(): number` - Get number of connected peers
- `getOurPeerId(): string | null` - Get our peer ID
- `getPeerIds(): string[]` - Get list of connected peer IDs
- `getConnectionStatistics(): string | null` - Get connection statistics as JSON

#### File Transfer

- `sendFile(peerId: string, filePath: string, remoteFilename?: string): string | null` - Send file
- `sendDirectory(peerId: string, dirPath: string, remoteDirName?: string, recursive?: boolean): string | null` - Send directory
- `requestFile(peerId: string, remoteFilePath: string, localPath: string): string | null` - Request file
- `requestDirectory(peerId: string, remoteDirPath: string, localDirPath: string, recursive?: boolean): string | null` - Request directory
- `acceptFileTransfer(transferId: string, localPath: string): boolean` - Accept file transfer
- `rejectFileTransfer(transferId: string, reason?: string): boolean` - Reject file transfer
- `cancelFileTransfer(transferId: string): boolean` - Cancel file transfer
- `pauseFileTransfer(transferId: string): boolean` - Pause file transfer
- `resumeFileTransfer(transferId: string): boolean` - Resume file transfer

#### GossipSub

- `isGossipsubAvailable(): boolean` - Check if GossipSub is available
- `subscribeToTopic(topic: string): boolean` - Subscribe to topic
- `unsubscribeFromTopic(topic: string): boolean` - Unsubscribe from topic
- `publishToTopic(topic: string, message: string): boolean` - Publish message
- `publishJsonToTopic(topic: string, jsonStr: string): boolean` - Publish JSON
- `getSubscribedTopics(): string[]` - Get subscribed topics
- `getTopicPeers(topic: string): string[]` - Get peers in topic

#### DHT

- `startDhtDiscovery(dhtPort: number): boolean` - Start DHT discovery
- `stopDhtDiscovery(): void` - Stop DHT discovery
- `isDhtRunning(): boolean` - Check if DHT is running
- `announceForHash(contentHash: string, port: number): boolean` - Announce for hash

#### Encryption

- `setEncryptionEnabled(enabled: boolean): boolean` - Enable/disable encryption
- `isEncryptionEnabled(): boolean` - Check if encryption is enabled
- `generateEncryptionKey(): string | null` - Generate new encryption key
- `setEncryptionKey(keyHex: string): boolean` - Set encryption key
- `getEncryptionKey(): string | null` - Get current encryption key

#### Event Handlers

- `onConnection(callback: (peerId: string) => void): void` - Set connection callback
- `onString(callback: (peerId: string, message: string) => void): void` - Set string message callback
- `onBinary(callback: (peerId: string, data: Buffer) => void): void` - Set binary message callback
- `onJson(callback: (peerId: string, jsonStr: string) => void): void` - Set JSON message callback
- `onDisconnect(callback: (peerId: string) => void): void` - Set disconnect callback
- `onFileProgress(callback: (transferId: string, progressPercent: number, status: string) => void): void` - Set file progress callback

### Utility Functions

- `getVersionString(): string` - Get librats version string
- `getVersion(): VersionInfo` - Get version components
- `getGitDescribe(): string` - Get git describe string
- `getAbi(): number` - Get ABI version

### Constants

#### ConnectionStrategy

- `ConnectionStrategy.DIRECT_ONLY` - Direct connection only
- `ConnectionStrategy.STUN_ASSISTED` - STUN-assisted connection
- `ConnectionStrategy.ICE_FULL` - Full ICE negotiation
- `ConnectionStrategy.TURN_RELAY` - TURN relay connection
- `ConnectionStrategy.AUTO_ADAPTIVE` - Automatic strategy selection

#### ErrorCodes

- `ErrorCodes.SUCCESS` - Operation successful
- `ErrorCodes.INVALID_HANDLE` - Invalid client handle
- `ErrorCodes.INVALID_PARAMETER` - Invalid parameter
- `ErrorCodes.NOT_RUNNING` - Client not running
- `ErrorCodes.OPERATION_FAILED` - Operation failed
- `ErrorCodes.PEER_NOT_FOUND` - Peer not found
- `ErrorCodes.MEMORY_ALLOCATION` - Memory allocation error
- `ErrorCodes.JSON_PARSE` - JSON parsing error

## Examples

The `examples/` directory contains comprehensive examples:

- **`basic_client.js`** - Basic peer-to-peer networking and messaging
- **`file_transfer.js`** - File and directory transfer with interactive CLI
- **`gossipsub_chat.js`** - Topic-based chat using GossipSub

### Running Examples

```bash
# Basic client (listens on port 8080)
node examples/basic_client.js

# Basic client connecting to another peer
node examples/basic_client.js 8081 127.0.0.1 8080

# File transfer client
node examples/file_transfer.js 8080

# GossipSub chat
node examples/gossipsub_chat.js 8080 Alice
```

## Testing

Run the test suite:

```bash
npm test
```

Or run the simple test script:

```bash
node test/test.js
```

## Building from Source

### Debug Build

```bash
npm run build
```

### Clean Build

```bash
npm run clean
npm run build
```

## TypeScript Support

Full TypeScript definitions are included. Import types:

```typescript
import { RatsClient, ConnectionStrategy, ErrorCodes, VersionInfo } from 'librats';

const client: RatsClient = new RatsClient(8080);
client.start();
```

## Error Handling

Most operations return boolean values indicating success/failure. For detailed error information, check the console output or use the statistics functions:

```javascript
const stats = client.getConnectionStatistics();
if (stats) {
  const parsed = JSON.parse(stats);
  console.log('Connection stats:', parsed);
}
```

## Performance Considerations

- Use binary messages for large data transfers
- Enable encryption only when needed (adds overhead)
- Monitor file transfer progress for large files
- Use appropriate connection strategies for your network environment

## Platform Support

- **Windows** - Requires Visual Studio Build Tools
- **Linux** - Requires build-essential
- **macOS** - Requires Xcode Command Line Tools
- **Android** - Supported via native Android bindings (separate package)

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Submit a pull request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Troubleshooting

### Build Issues

**Windows**: Ensure Visual Studio Build Tools are installed
```bash
npm install --global windows-build-tools
```

**Linux**: Install build dependencies
```bash
sudo apt install build-essential python3
```

**macOS**: Install Xcode Command Line Tools
```bash
xcode-select --install
```

### Runtime Issues

**Port already in use**: Choose a different port or kill the process using the port
```bash
# Find process using port
netstat -tulpn | grep :8080

# Kill process
kill -9 <pid>
```

**Permission denied**: Run with appropriate permissions or use ports > 1024

**Connection timeout**: Check firewall settings and ensure peers are reachable

### Debug Mode

Enable debug logging by setting environment variable:
```bash
export LIBRATS_DEBUG=1
node examples/basic_client.js
```

## Support

- [GitHub Issues](https://github.com/librats/librats/issues)
- [Documentation](https://librats.github.io/)
- [Examples Repository](https://github.com/librats/librats/tree/main/nodejs/examples)

For more advanced usage, see the [C API documentation](../docs/) and the [main project README](../README.md).
