# librats Python Bindings

Python bindings for the librats peer-to-peer networking library, providing high-level access to P2P networking, file transfers, NAT traversal, and GossipSub messaging.

### Prerequisites

1. **Build the librats C library** first (see main README)
2. **Python 3.7+** is required

### Installing the Python Package

```bash
cd python
pip install -e .
```

Or install from PyPI (when available):

```bash
pip install librats-py
```

### Development Installation

For development with additional tools:

```bash
cd python
pip install -e ".[dev]"
```

## Quick Start

### Basic Client

```python
from librats_py import RatsClient

# Create and start a client
with RatsClient(listen_port=8080) as client:
    client.start()
    
    # Set up callbacks
    client.set_connection_callback(lambda peer_id: print(f"Connected: {peer_id}"))
    client.set_string_callback(lambda peer_id, msg: print(f"Message: {msg}"))
    
    # Connect to another peer
    client.connect("192.168.1.100", 8081)
    
    # Send messages
    client.broadcast_string("Hello, P2P world!")
    
    # Keep running
    input("Press Enter to exit...")
```

### File Transfer

```python
from librats_py import RatsClient

with RatsClient(8080) as client:
    client.start()
    
    # Connect to a peer
    client.connect("192.168.1.100", 8081)
    
    # Send a file
    peer_id = "some_peer_id"
    transfer_id = client.send_file(peer_id, "/path/to/file.txt")
    print(f"File transfer started: {transfer_id}")
```

### GossipSub Messaging

```python
from librats_py import RatsClient

with RatsClient(8080) as client:
    client.start()
    
    # Subscribe to a topic
    client.subscribe_to_topic("chat")
    
    # Set up topic message handler
    def on_topic_message(peer_id, topic, message):
        print(f"[{topic}] {peer_id}: {message}")
    
    # Note: Topic callbacks need to be set up via the C API
    # This is a simplified example
    
    # Publish messages
    client.publish_to_topic("chat", "Hello everyone!")
```

## API Reference

### RatsClient

The main client class for P2P networking.

#### Constructor

```python
RatsClient(listen_port: int = 0)
```

- `listen_port`: Port to listen on (0 for automatic assignment)

#### Core Methods

##### Connection Management

```python
client.start()                    # Start the client
client.stop()                     # Stop the client
client.is_running() -> bool       # Check if running

client.connect(host: str, port: int, strategy: ConnectionStrategy = AUTO_ADAPTIVE)
client.disconnect_peer(peer_id: str)
```

##### Messaging

```python
# Send to specific peer
client.send_string(peer_id: str, message: str)
client.send_binary(peer_id: str, data: bytes)
client.send_json(peer_id: str, data: dict)

# Broadcast to all peers
client.broadcast_string(message: str) -> int
client.broadcast_binary(data: bytes) -> int
client.broadcast_json(data: dict) -> int
```

##### Information

```python
client.get_peer_count() -> int
client.get_our_peer_id() -> str
client.get_connection_statistics() -> dict
client.get_max_peers() -> int
client.set_max_peers(count: int)
```

#### Discovery Methods

```python
# DHT Discovery
client.start_dht_discovery(port: int = 6881)
client.stop_dht_discovery()
client.is_dht_running() -> bool

# GossipSub
client.subscribe_to_topic(topic: str)
client.unsubscribe_from_topic(topic: str)
client.publish_to_topic(topic: str, message: str)
client.is_gossipsub_available() -> bool
```

#### File Transfer

```python
# Send files
client.send_file(peer_id: str, file_path: str, remote_filename: str = None) -> str

# Handle incoming transfers
client.accept_file_transfer(transfer_id: str, local_path: str)
client.reject_file_transfer(transfer_id: str, reason: str = "")
client.cancel_file_transfer(transfer_id: str)
```

#### Encryption

```python
client.set_encryption_enabled(enabled: bool)
client.is_encryption_enabled() -> bool
client.generate_encryption_key() -> str
client.set_encryption_key(key_hex: str)
client.get_encryption_key() -> str
```

#### Callbacks

```python
# Set event handlers
client.set_connection_callback(callback: Callable[[str], None])
client.set_disconnect_callback(callback: Callable[[str], None])
client.set_string_callback(callback: Callable[[str, str], None])
client.set_binary_callback(callback: Callable[[str, bytes], None])
client.set_json_callback(callback: Callable[[str, dict], None])
```

### Enums

#### ConnectionStrategy

```python
ConnectionStrategy.DIRECT_ONLY      # Direct connection only
ConnectionStrategy.STUN_ASSISTED    # Use STUN for NAT traversal
ConnectionStrategy.ICE_FULL         # Full ICE with candidate gathering
ConnectionStrategy.TURN_RELAY       # Force TURN relay usage
ConnectionStrategy.AUTO_ADAPTIVE    # Automatically choose best strategy
```

#### LogLevel

```python
LogLevel.DEBUG    # Debug messages
LogLevel.INFO     # Informational messages
LogLevel.WARN     # Warning messages
LogLevel.ERROR    # Error messages only
```

### Exceptions

All librats operations can raise `RatsError` or its subclasses:

- `RatsError` - Base exception
- `RatsConnectionError` - Connection-related errors
- `RatsInvalidParameterError` - Invalid parameters
- `RatsNotRunningError` - Client not running
- `RatsPeerNotFoundError` - Peer not found
- `RatsJsonParseError` - JSON parsing errors

## Examples

The `examples/` directory contains several demonstration scripts:

### Basic Client (`examples/basic_client.py`)

Interactive P2P client with command-line interface:

```bash
python -m librats_py.examples.basic_client 8080
```

### File Transfer (`examples/file_transfer.py`)

File transfer demonstration:

```bash
python -m librats_py.examples.file_transfer 8080
```

### GossipSub Chat (`examples/gossipsub_chat.py`)

Chat room using GossipSub:

```bash
python -m librats_py.examples.gossipsub_chat 8080 alice
```

## Testing

Run the test suite:

```bash
# Install test dependencies
pip install -e ".[dev]"

# Run tests
python -m pytest librats_py/tests/

# Run with coverage
python -m pytest --cov=librats_py librats_py/tests/
```

Note: Integration tests require the librats shared library to be built and available.

## Building from Source

### Requirements

- librats C library built and installed
- Python 3.7+
- ctypes (included with Python)

### Build Steps

1. **Build librats C library**:
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```

2. **Install Python bindings**:
   ```bash
   cd python
   pip install -e .
   ```

3. **Verify installation**:
   ```bash
   python -c "from librats_py import RatsClient; print('Success!')"
   ```

## Architecture

The Python bindings use ctypes to interface with the librats C library:

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   Python App    │    │   librats_py     │    │   librats.so    │
│                 │◄──►│                  │◄──►│                 │
│  (Your Code)    │    │  (Python API)    │    │   (C Library)   │
└─────────────────┘    └──────────────────┘    └─────────────────┘
```

- **High-level API**: Pythonic interface with proper error handling
- **ctypes layer**: Low-level binding to C functions
- **C library**: Core librats functionality

## Library Location

The Python bindings automatically search for the librats shared library in common locations:

- Current directory
- `../build/`, `../../build/`, etc.
- System library paths (`/usr/lib`, `/usr/local/lib`)
- Paths in `LD_LIBRARY_PATH` (Linux/macOS) or `PATH` (Windows)

You can also set the library path explicitly by placing the library file in the package directory.

## Troubleshooting

### "LibratsNotFoundError"

The shared library couldn't be found. Ensure:

1. The librats C library is built (`make` or `cmake --build build`)
2. The library is in your system path or the search paths
3. On Linux: Check `LD_LIBRARY_PATH`
4. On Windows: Check `PATH` or place DLL in Python package directory

### Import Errors

```python
ImportError: Could not import librats_py
```

Usually indicates the native library isn't available. The Python package will still import but functionality will be limited.

### Connection Issues

- Check firewall settings
- Ensure ports are not blocked
- Try different connection strategies
- Enable logging for detailed diagnostics

### Performance Tips

- Use binary data for large messages instead of JSON/strings
- Enable encryption only when needed
- Tune `max_peers` based on your use case
- Use GossipSub for efficient broadcast messaging

## Contributing

1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Ensure all tests pass
5. Submit a pull request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Links

- [Main librats repository](https://github.com/your-org/librats)
- [Documentation](https://your-org.github.io/librats)
- [Issue tracker](https://github.com/your-org/librats/issues)
