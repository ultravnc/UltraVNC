# LibRats Android Library

This directory contains the Android JNI bindings for LibRats, enabling peer-to-peer networking capabilities in Android applications.

## Features

- **Peer-to-peer networking**: Direct connections between Android devices
- **NAT traversal**: Support for STUN, ICE, and TURN protocols
- **Multiple connection strategies**: Direct, STUN-assisted, ICE, TURN relay, and auto-adaptive
- **Message types**: String, binary, and JSON message support
- **File transfer**: Send and receive files between peers
- **Service discovery**: mDNS and DHT-based peer discovery
- **Encryption**: Optional end-to-end encryption using Noise protocol
- **Gossip protocol**: Support for gossip-based messaging

## Directory Structure

```
android/
├── src/main/
│   ├── cpp/                    # JNI C++ wrapper code
│   │   ├── librats_jni.cpp    # Main JNI implementation
│   │   └── CMakeLists.txt     # CMake build configuration
│   ├── java/com/librats/      # Java API classes
│   │   ├── RatsClient.java    # Main client class
│   │   ├── *Callback.java     # Callback interfaces
│   │   └── RatsException.java # Exception handling
│   └── AndroidManifest.xml    # Required permissions
├── examples/                   # Example Android app
├── build.gradle              # Library build configuration
├── proguard-rules.pro        # ProGuard rules
└── README.md                 # This file
```

## Integration

### Add to your Android project

1. **Copy the library**:
   ```bash
   cp -r android/ your-project/librats/
   ```

2. **Add to settings.gradle**:
   ```gradle
   include ':librats'
   project(':librats').projectDir = new File('librats')
   ```

3. **Add dependency in app/build.gradle**:
   ```gradle
   dependencies {
       implementation project(':librats')
   }
   ```

### Required Permissions

Add these permissions to your app's `AndroidManifest.xml`:

```xml
<uses-permission android:name="android.permission.INTERNET" />
<uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
<uses-permission android:name="android.permission.ACCESS_WIFI_STATE" />
<uses-permission android:name="android.permission.CHANGE_WIFI_MULTICAST_STATE" />
<uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE" />
<uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" />
<uses-permission android:name="android.permission.WAKE_LOCK" />
```

## Quick Start

### Basic Usage

```java
import com.librats.RatsClient;
import com.librats.ConnectionCallback;
import com.librats.StringMessageCallback;

public class MainActivity extends AppCompatActivity {
    private RatsClient ratsClient;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // Create client
        ratsClient = new RatsClient(8080); // Listen on port 8080
        
        // Set up callbacks
        ratsClient.setConnectionCallback(new ConnectionCallback() {
            @Override
            public void onConnection(String peerId) {
                Log.d("LibRats", "Connected to peer: " + peerId);
            }
        });
        
        ratsClient.setStringCallback(new StringMessageCallback() {
            @Override
            public void onStringMessage(String peerId, String message) {
                Log.d("LibRats", "Received: " + message);
            }
        });
        
        // Start the client
        ratsClient.start();
    }
    
    private void connectToPeer() {
        // Connect to another peer
        ratsClient.connectWithStrategy("192.168.1.100", 8080, 
                                     RatsClient.STRATEGY_AUTO_ADAPTIVE);
    }
    
    private void sendMessage() {
        // Send a message to all connected peers
        ratsClient.broadcastString("Hello from Android!");
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (ratsClient != null) {
            ratsClient.stop();
            ratsClient.destroy();
        }
    }
}
```

### Connection Strategies

```java
// Direct connection only
ratsClient.connectWithStrategy(host, port, RatsClient.STRATEGY_DIRECT_ONLY);

// STUN-assisted connection
ratsClient.connectWithStrategy(host, port, RatsClient.STRATEGY_STUN_ASSISTED);

// Full ICE with STUN/TURN
ratsClient.connectWithStrategy(host, port, RatsClient.STRATEGY_ICE_FULL);

// TURN relay only
ratsClient.connectWithStrategy(host, port, RatsClient.STRATEGY_TURN_RELAY);

// Auto-adaptive (recommended)
ratsClient.connectWithStrategy(host, port, RatsClient.STRATEGY_AUTO_ADAPTIVE);
```

### Message Types

```java
// String messages
ratsClient.sendString(peerId, "Hello!");
ratsClient.broadcastString("Hello everyone!");

// Binary data
byte[] data = {0x01, 0x02, 0x03};
ratsClient.sendBinary(peerId, data);
ratsClient.broadcastBinary(data);

// JSON messages
ratsClient.sendJson(peerId, "{\"type\":\"ping\"}");
ratsClient.broadcastJson("{\"type\":\"announcement\"}");
```

### File Transfer

```java
// Send a file
String transferId = ratsClient.sendFile(peerId, "/path/to/file.txt", "remote_file.txt");

// Handle incoming file transfers (set up callback first)
ratsClient.setFileProgressCallback(new FileProgressCallback() {
    @Override
    public void onFileProgress(String transferId, int progress, String status) {
        // Handle progress updates
    }
});

// Accept or reject file transfers
ratsClient.acceptFileTransfer(transferId, "/path/to/save/file.txt");
ratsClient.rejectFileTransfer(transferId, "Not accepting files");
```

### Service Discovery

```java
// Start mDNS discovery
ratsClient.startMdnsDiscovery("my-app-service");

// Start DHT discovery
ratsClient.startDhtDiscovery(6881);

// Set discovery callback
ratsClient.setPeerDiscoveredCallback(new PeerDiscoveredCallback() {
    @Override
    public void onPeerDiscovered(String host, int port, String serviceName) {
        // Auto-connect to discovered peer
        ratsClient.connect(host, port);
    }
});
```

### Encryption

```java
// Enable encryption
ratsClient.setEncryptionEnabled(true);

// Generate a new key
String key = ratsClient.generateEncryptionKey();

// Set a specific key
ratsClient.setEncryptionKey("your-hex-key-here");
```

## API Reference

### RatsClient Class

#### Lifecycle Methods
- `RatsClient(int listenPort)` - Create client
- `int start()` - Start the client
- `void stop()` - Stop the client
- `void destroy()` - Destroy and cleanup

#### Connection Methods
- `int connect(String host, int port)` - Simple connect
- `int connectWithStrategy(String host, int port, int strategy)` - Connect with strategy
- `int getPeerCount()` - Get number of connected peers
- `String[] getPeerIds()` - Get all peer IDs

#### Messaging Methods
- `int sendString(String peerId, String message)` - Send string to peer
- `int broadcastString(String message)` - Broadcast string to all
- `int sendBinary(String peerId, byte[] data)` - Send binary to peer
- `int broadcastBinary(byte[] data)` - Broadcast binary to all
- `int sendJson(String peerId, String json)` - Send JSON to peer
- `int broadcastJson(String json)` - Broadcast JSON to all

#### File Transfer Methods
- `String sendFile(String peerId, String path, String filename)` - Send file
- `int acceptFileTransfer(String transferId, String path)` - Accept transfer
- `int rejectFileTransfer(String transferId, String reason)` - Reject transfer

#### Discovery Methods
- `int startMdnsDiscovery(String serviceName)` - Start mDNS
- `void stopMdnsDiscovery()` - Stop mDNS
- `int startDhtDiscovery(int port)` - Start DHT
- `void stopDhtDiscovery()` - Stop DHT

#### Encryption Methods
- `int setEncryptionEnabled(boolean enabled)` - Enable/disable encryption
- `String generateEncryptionKey()` - Generate new key
- `int setEncryptionKey(String key)` - Set encryption key

#### Static Methods
- `String getVersionString()` - Get library version
- `void setLoggingEnabled(boolean enabled)` - Enable/disable logging
- `void setLogLevel(String level)` - Set log level

### Callback Interfaces

- `ConnectionCallback` - Peer connections
- `StringMessageCallback` - String messages
- `BinaryMessageCallback` - Binary messages
- `JsonMessageCallback` - JSON messages
- `DisconnectCallback` - Peer disconnections
- `PeerDiscoveredCallback` - Service discovery
- `FileProgressCallback` - File transfer progress

### Constants

#### Connection Strategies
- `STRATEGY_DIRECT_ONLY` - Direct connection only
- `STRATEGY_STUN_ASSISTED` - STUN-assisted connection
- `STRATEGY_ICE_FULL` - Full ICE with STUN/TURN
- `STRATEGY_TURN_RELAY` - TURN relay only
- `STRATEGY_AUTO_ADAPTIVE` - Auto-adaptive (recommended)

#### Error Codes
- `SUCCESS` - Operation successful
- `ERROR_INVALID_HANDLE` - Invalid client handle
- `ERROR_INVALID_PARAMETER` - Invalid parameter
- `ERROR_NOT_RUNNING` - Client not running
- `ERROR_OPERATION_FAILED` - Operation failed
- `ERROR_PEER_NOT_FOUND` - Peer not found
- `ERROR_MEMORY_ALLOCATION` - Memory allocation failed
- `ERROR_JSON_PARSE` - JSON parse error

## Building

### Prerequisites

- Android Studio 4.0+
- Android NDK 21+
- CMake 3.18.1+
- MinSDK 21 (Android 5.0)

### Build Steps

1. Open the project in Android Studio
2. The library will build automatically with the app
3. Native libraries will be built for: arm64-v8a, armeabi-v7a, x86_64, x86

### Manual Build

```bash
cd android
./gradlew assembleRelease
```

## Example App

The `examples/` directory contains a complete Android app demonstrating LibRats usage:

- Basic peer-to-peer messaging
- Connection management
- Real-time message display
- Error handling

To run the example:

1. Open `examples/` in Android Studio
2. Build and run on two devices
3. Start both clients
4. Connect one to the other using IP address
5. Send messages between devices

## Troubleshooting

### Common Issues

1. **Library not found**: Ensure NDK is properly installed and CMake version matches
2. **Permission denied**: Check that all required permissions are granted
3. **Connection fails**: Verify both devices are on same network and ports are open
4. **Native crash**: Check logcat for JNI errors and ensure proper lifecycle management

### Debugging

Enable verbose logging:
```java
RatsClient.setLoggingEnabled(true);
RatsClient.setLogLevel("DEBUG");
```

Check native logs:
```bash
adb logcat -s LibRatsJNI
```

## Performance Considerations

- **Threading**: Callbacks are called on background threads, use `runOnUiThread()` for UI updates
- **Memory**: Call `destroy()` to properly cleanup native resources
- **Battery**: Consider using wake locks for background operations
- **Network**: Monitor network state changes and reconnect as needed

## License

This library follows the same license as the main LibRats project.
