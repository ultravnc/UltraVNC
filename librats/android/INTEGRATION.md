# LibRats Android Integration Guide

## Overview

This Android library provides JNI bindings for the LibRats C++ peer-to-peer networking library. It enables Android applications to:

- Connect to peers using various NAT traversal strategies
- Send/receive string, binary, and JSON messages
- Transfer files between devices
- Discover peers using mDNS and DHT
- Use end-to-end encryption

## Quick Integration

### 1. Add to Your Project

Copy the entire `android/` directory to your project:

```bash
cp -r /path/to/librats/android /path/to/your/project/librats-android
```

### 2. Update settings.gradle

```gradle
include ':librats-android'
project(':librats-android').projectDir = new File('librats-android')
```

### 3. Add Dependency

In your app's `build.gradle`:

```gradle
dependencies {
    implementation project(':librats-android')
    // ... other dependencies
}
```

### 4. Update AndroidManifest.xml

Add required permissions:

```xml
<uses-permission android:name="android.permission.INTERNET" />
<uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
<uses-permission android:name="android.permission.ACCESS_WIFI_STATE" />
<uses-permission android:name="android.permission.CHANGE_WIFI_MULTICAST_STATE" />
```

## Usage Example

```java
import com.librats.RatsClient;
import com.librats.ConnectionCallback;
import com.librats.StringMessageCallback;

public class P2PService {
    private RatsClient client;
    
    public void initializeP2P() {
        // Create client listening on port 8080
        client = new RatsClient(8080);
        
        // Set up callbacks
        client.setConnectionCallback(peerId -> {
            Log.d("P2P", "Peer connected: " + peerId);
        });
        
        client.setStringCallback((peerId, message) -> {
            Log.d("P2P", "Message from " + peerId + ": " + message);
        });
        
        // Start the client
        client.start();
    }
    
    public void connectToPeer(String host, int port) {
        client.connectWithStrategy(host, port, RatsClient.STRATEGY_AUTO_ADAPTIVE);
    }
    
    public void sendMessage(String message) {
        client.broadcastString(message);
    }
    
    public void cleanup() {
        if (client != null) {
            client.stop();
            client.destroy();
        }
    }
}
```

## Architecture

```
┌─────────────────────────────────────────┐
│           Android Application           │
├─────────────────────────────────────────┤
│          Java API (com.librats)         │
├─────────────────────────────────────────┤
│         JNI Layer (librats_jni.cpp)     │
├─────────────────────────────────────────┤
│        LibRats C API (librats_c.h)      │
├─────────────────────────────────────────┤
│       LibRats Core (C++ Implementation) │
└─────────────────────────────────────────┘
```

## File Structure

- **`src/main/cpp/`**: JNI C++ wrapper and CMake build config
- **`src/main/java/com/librats/`**: Java API classes and interfaces  
- **`examples/`**: Complete Android app demonstrating usage
- **`build.gradle`**: Library build configuration
- **`README.md`**: Comprehensive documentation

## Key Components

### RatsClient
Main class providing all LibRats functionality:
- Client lifecycle management
- Connection methods with different strategies
- Message sending (string, binary, JSON)
- File transfer capabilities
- Service discovery (mDNS, DHT)
- Encryption support

### Callback Interfaces
- `ConnectionCallback`: Peer connection events
- `StringMessageCallback`: String message reception
- `BinaryMessageCallback`: Binary data reception
- `JsonMessageCallback`: JSON message reception
- `DisconnectCallback`: Peer disconnection events

### Connection Strategies
- `STRATEGY_DIRECT_ONLY`: Direct connections only
- `STRATEGY_STUN_ASSISTED`: STUN-assisted NAT traversal
- `STRATEGY_ICE_FULL`: Full ICE with STUN/TURN support
- `STRATEGY_TURN_RELAY`: TURN relay connections
- `STRATEGY_AUTO_ADAPTIVE`: Automatic strategy selection

## Building

The library uses CMake to build the native components:

1. LibRats C++ source files are compiled into `librats.so`
2. JNI wrapper is compiled into `librats_jni.so`
3. Both libraries are packaged with the Android AAR

Supported ABIs:
- arm64-v8a (64-bit ARM)
- armeabi-v7a (32-bit ARM)
- x86_64 (64-bit x86)
- x86 (32-bit x86)

## Testing

Use the example app to test the integration:

1. Build and install on two Android devices
2. Connect both to the same WiFi network
3. Start the client on both devices
4. Connect one to the other using IP address
5. Send messages and verify reception

## Requirements

- **Minimum SDK**: Android 5.0 (API 21)
- **Target SDK**: Android 15 (API 35)
- **NDK**: 21 or higher
- **CMake**: 3.18.1 or higher

## Performance Notes

- Callbacks run on background threads
- Use `runOnUiThread()` for UI updates
- Call `destroy()` to properly release native resources
- Consider using services for background P2P operations

## Troubleshooting

### Build Issues
- Ensure NDK and CMake are properly installed
- Check that all LibRats source files are accessible
- Verify CMake version compatibility

### Runtime Issues
- Check all required permissions are granted
- Enable logging for debugging: `RatsClient.setLoggingEnabled(true)`
- Monitor logcat for native errors: `adb logcat -s LibRatsJNI`

### Network Issues
- Verify devices are on same network
- Check firewall/router settings
- Test with direct connection strategy first
- Ensure ports are not blocked

## License

This Android library follows the same license as the main LibRats project.
