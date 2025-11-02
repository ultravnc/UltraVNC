#!/usr/bin/env python3
"""
GossipSub chat room example using librats.

This example demonstrates publish-subscribe messaging using GossipSub.
"""

import sys
import time
import json
import threading
from datetime import datetime
from librats_py import RatsClient, RatsError


class GossipSubChat:
    def __init__(self, listen_port: int, username: str):
        self.client = RatsClient(listen_port)
        self.username = username
        self.listen_port = listen_port
        self.current_topic = None
        
        # Set up callbacks
        self.client.set_connection_callback(self.on_peer_connected)
        self.client.set_disconnect_callback(self.on_peer_disconnected)
    
    def on_peer_connected(self, peer_id: str):
        """Called when a new peer connects."""
        print(f"\n✓ Peer connected: {peer_id}")
        if self.current_topic:
            print(f"gossip:{self.current_topic}> ", end="", flush=True)
    
    def on_peer_disconnected(self, peer_id: str):
        """Called when a peer disconnects."""
        print(f"\n✗ Peer disconnected: {peer_id}")
        if self.current_topic:
            print(f"gossip:{self.current_topic}> ", end="", flush=True)
    
    def on_topic_message(self, peer_id: str, topic: str, message: str):
        """Called when a topic message is received."""
        try:
            # Try to parse as JSON for rich messages
            data = json.loads(message)
            if data.get('type') == 'chat':
                timestamp = datetime.fromisoformat(data.get('timestamp', ''))
                username = data.get('username', 'Unknown')
                content = data.get('content', '')
                
                print(f"\n[{timestamp.strftime('%H:%M:%S')}] {username}: {content}")
                if self.current_topic:
                    print(f"gossip:{self.current_topic}> ", end="", flush=True)
        except (json.JSONDecodeError, KeyError):
            # Plain text message
            print(f"\n{peer_id}: {message}")
            if self.current_topic:
                print(f"gossip:{self.current_topic}> ", end="", flush=True)
    
    def start(self):
        """Start the client."""
        try:
            # Enable logging
            RatsClient.set_logging_enabled(True)
            
            # Start the client
            self.client.start()
            print(f"✓ GossipSub chat client started on port {self.listen_port}")
            print(f"🆔 Our peer ID: {self.client.get_our_peer_id()}")
            print(f"👤 Username: {self.username}")
            
            # Check if GossipSub is available
            if not self.client.is_gossipsub_available():
                print("❌ GossipSub is not available in this build")
                return False
            
            return True
        except RatsError as e:
            print(f"❌ Failed to start client: {e}")
            return False
    
    def stop(self):
        """Stop the client."""
        if self.current_topic:
            try:
                self.leave_topic(self.current_topic)
            except:
                pass
        self.client.stop()
        print("👋 Chat client stopped")
    
    def connect_to_peer(self, host: str, port: int):
        """Connect to a peer."""
        try:
            self.client.connect(host, port)
            print(f"🔗 Connecting to {host}:{port}...")
        except RatsError as e:
            print(f"❌ Connection failed: {e}")
    
    def join_topic(self, topic: str):
        """Join a chat topic."""
        try:
            # Leave current topic if any
            if self.current_topic:
                self.leave_topic(self.current_topic)
            
            # Subscribe to the new topic
            self.client.subscribe_to_topic(topic)
            
            # Set up topic message callback
            from librats_py.callbacks import TopicMessageCallbackType
            # Note: This would need to be implemented in the ctypes wrapper
            # For now, we'll use a simplified approach
            
            self.current_topic = topic
            print(f"📺 Joined topic: {topic}")
            
            # Send join message
            join_message = {
                'type': 'join',
                'username': self.username,
                'timestamp': datetime.now().isoformat()
            }
            self.client.publish_to_topic(topic, json.dumps(join_message))
            
        except RatsError as e:
            print(f"❌ Failed to join topic: {e}")
    
    def leave_topic(self, topic: str):
        """Leave a chat topic."""
        try:
            # Send leave message
            leave_message = {
                'type': 'leave',
                'username': self.username,
                'timestamp': datetime.now().isoformat()
            }
            self.client.publish_to_topic(topic, json.dumps(leave_message))
            
            # Unsubscribe from topic
            self.client.unsubscribe_from_topic(topic)
            
            if self.current_topic == topic:
                self.current_topic = None
            
            print(f"👋 Left topic: {topic}")
            
        except RatsError as e:
            print(f"❌ Failed to leave topic: {e}")
    
    def send_message(self, content: str):
        """Send a chat message to the current topic."""
        if not self.current_topic:
            print("❌ No topic joined. Use 'join <topic>' first.")
            return
        
        try:
            message = {
                'type': 'chat',
                'username': self.username,
                'content': content,
                'timestamp': datetime.now().isoformat()
            }
            
            self.client.publish_to_topic(self.current_topic, json.dumps(message))
            
        except RatsError as e:
            print(f"❌ Failed to send message: {e}")
    
    def show_peers(self):
        """Show connected peers."""
        count = self.client.get_peer_count()
        print(f"👥 Connected peers: {count}")
        
        stats = self.client.get_connection_statistics()
        if stats and 'peers' in stats:
            for peer_info in stats['peers']:
                print(f"  - {peer_info}")
    
    def run_interactive(self):
        """Run interactive chat loop."""
        print("\nGossipSub Chat Commands:")
        print("  connect <host> <port>  - Connect to a peer")
        print("  join <topic>           - Join a chat topic")
        print("  leave                  - Leave current topic")
        print("  peers                  - Show connected peers")
        print("  stats                  - Show connection statistics")
        print("  quit                   - Exit the program")
        print("  <message>              - Send message to current topic")
        print()
        
        while True:
            try:
                if self.current_topic:
                    command = input(f"gossip:{self.current_topic}> ").strip()
                else:
                    command = input("gossip> ").strip()
                
                if not command:
                    continue
                
                # Check if it's a command (starts with a known command word)
                parts = command.split()
                cmd = parts[0].lower()
                
                if cmd == "quit" or cmd == "exit":
                    break
                
                elif cmd == "connect":
                    if len(parts) != 3:
                        print("Usage: connect <host> <port>")
                        continue
                    
                    try:
                        host = parts[1]
                        port = int(parts[2])
                        self.connect_to_peer(host, port)
                    except ValueError:
                        print("Error: Port must be a number")
                
                elif cmd == "join":
                    if len(parts) != 2:
                        print("Usage: join <topic>")
                        continue
                    
                    topic = parts[1]
                    self.join_topic(topic)
                
                elif cmd == "leave":
                    if self.current_topic:
                        self.leave_topic(self.current_topic)
                    else:
                        print("❌ No topic to leave")
                
                elif cmd == "peers":
                    self.show_peers()
                
                elif cmd == "stats":
                    stats = self.client.get_connection_statistics()
                    print("📊 Connection Statistics:")
                    for key, value in stats.items():
                        print(f"  {key}: {value}")
                
                else:
                    # Treat as a chat message
                    if self.current_topic:
                        self.send_message(command)
                    else:
                        print("❌ No topic joined. Use 'join <topic>' first.")
            
            except KeyboardInterrupt:
                break
            except EOFError:
                break
            except Exception as e:
                print(f"Error: {e}")


def main():
    if len(sys.argv) != 3:
        print("Usage: python gossipsub_chat.py <listen_port> <username>")
        print("Example: python gossipsub_chat.py 8080 alice")
        sys.exit(1)
    
    try:
        listen_port = int(sys.argv[1])
        username = sys.argv[2]
    except ValueError:
        print("Error: Port must be a number")
        sys.exit(1)
    
    if not username or len(username.strip()) == 0:
        print("Error: Username cannot be empty")
        sys.exit(1)
    
    chat = GossipSubChat(listen_port, username.strip())
    
    try:
        if not chat.start():
            sys.exit(1)
        
        chat.run_interactive()
        
    except KeyboardInterrupt:
        print("\n👋 Interrupted by user")
    except Exception as e:
        print(f"❌ Unexpected error: {e}")
    finally:
        chat.stop()


if __name__ == "__main__":
    main()
