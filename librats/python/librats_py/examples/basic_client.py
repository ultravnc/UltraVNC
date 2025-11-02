#!/usr/bin/env python3
"""
Basic librats client example.

This example demonstrates basic peer-to-peer communication using librats.
"""

import time
import sys
import threading
from librats_py import RatsClient, RatsError


def on_peer_connected(peer_id: str):
    """Called when a new peer connects."""
    print(f"✓ Peer connected: {peer_id}")


def on_peer_disconnected(peer_id: str):
    """Called when a peer disconnects."""
    print(f"✗ Peer disconnected: {peer_id}")


def on_string_message(peer_id: str, message: str):
    """Called when a string message is received."""
    print(f"📨 Message from {peer_id}: {message}")


def on_json_message(peer_id: str, data: dict):
    """Called when a JSON message is received."""
    print(f"📨 JSON from {peer_id}: {data}")


def main():
    if len(sys.argv) != 2:
        print("Usage: python basic_client.py <listen_port>")
        print("Example: python basic_client.py 8080")
        sys.exit(1)
    
    try:
        listen_port = int(sys.argv[1])
    except ValueError:
        print("Error: Port must be a number")
        sys.exit(1)
    
    # Create and configure the client
    try:
        with RatsClient(listen_port) as client:
            print(f"🚀 Starting librats client on port {listen_port}")
            
            # Set up callbacks
            client.set_connection_callback(on_peer_connected)
            client.set_disconnect_callback(on_peer_disconnected)
            client.set_string_callback(on_string_message)
            client.set_json_callback(on_json_message)
            
            # Enable logging
            RatsClient.set_logging_enabled(True)
            
            # Start the client
            client.start()
            print(f"✓ Client started successfully")
            print(f"🆔 Our peer ID: {client.get_our_peer_id()}")
            
            # Interactive command loop
            print("\nCommands:")
            print("  connect <host> <port>  - Connect to a peer")
            print("  send <peer_id> <msg>   - Send a message to a peer")
            print("  broadcast <message>    - Broadcast message to all peers")
            print("  peers                  - Show connected peers")
            print("  stats                  - Show connection statistics")
            print("  quit                   - Exit the program")
            print()
            
            while True:
                try:
                    command = input("librats> ").strip().split()
                    if not command:
                        continue
                    
                    cmd = command[0].lower()
                    
                    if cmd == "quit" or cmd == "exit":
                        break
                    
                    elif cmd == "connect":
                        if len(command) != 3:
                            print("Usage: connect <host> <port>")
                            continue
                        
                        try:
                            host = command[1]
                            port = int(command[2])
                            client.connect(host, port)
                            print(f"🔗 Connecting to {host}:{port}...")
                        except ValueError:
                            print("Error: Port must be a number")
                        except RatsError as e:
                            print(f"Connection failed: {e}")
                    
                    elif cmd == "send":
                        if len(command) < 3:
                            print("Usage: send <peer_id> <message>")
                            continue
                        
                        peer_id = command[1]
                        message = " ".join(command[2:])
                        
                        try:
                            client.send_string(peer_id, message)
                            print(f"📤 Sent message to {peer_id}")
                        except RatsError as e:
                            print(f"Send failed: {e}")
                    
                    elif cmd == "broadcast":
                        if len(command) < 2:
                            print("Usage: broadcast <message>")
                            continue
                        
                        message = " ".join(command[1:])
                        count = client.broadcast_string(message)
                        print(f"📡 Broadcasted message to {count} peers")
                    
                    elif cmd == "peers":
                        count = client.get_peer_count()
                        print(f"👥 Connected peers: {count}")
                        
                        # Show connection statistics
                        stats = client.get_connection_statistics()
                        if stats and 'peers' in stats:
                            for peer_info in stats['peers']:
                                print(f"  - {peer_info}")
                    
                    elif cmd == "stats":
                        stats = client.get_connection_statistics()
                        print("📊 Connection Statistics:")
                        for key, value in stats.items():
                            print(f"  {key}: {value}")
                    
                    else:
                        print(f"Unknown command: {cmd}")
                
                except KeyboardInterrupt:
                    break
                except EOFError:
                    break
                except Exception as e:
                    print(f"Error: {e}")
            
            print("\n👋 Shutting down...")
    
    except RatsError as e:
        print(f"❌ Error: {e}")
        sys.exit(1)
    except KeyboardInterrupt:
        print("\n👋 Interrupted by user")
    except Exception as e:
        print(f"❌ Unexpected error: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
