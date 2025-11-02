#!/usr/bin/env python3
"""
File transfer example using librats.

This example demonstrates peer-to-peer file transfer capabilities.
"""

import os
import sys
import time
import threading
from pathlib import Path
from librats_py import RatsClient, RatsError


class FileTransferExample:
    def __init__(self, listen_port: int):
        self.client = RatsClient(listen_port)
        self.listen_port = listen_port
        self.active_transfers = {}
        
        # Set up callbacks
        self.client.set_connection_callback(self.on_peer_connected)
        self.client.set_disconnect_callback(self.on_peer_disconnected)
    
    def on_peer_connected(self, peer_id: str):
        """Called when a new peer connects."""
        print(f"✓ Peer connected: {peer_id}")
    
    def on_peer_disconnected(self, peer_id: str):
        """Called when a peer disconnects."""
        print(f"✗ Peer disconnected: {peer_id}")
        # Clean up any active transfers with this peer
        transfers_to_remove = [tid for tid, info in self.active_transfers.items() 
                              if info.get('peer_id') == peer_id]
        for tid in transfers_to_remove:
            del self.active_transfers[tid]
    
    def start(self):
        """Start the client."""
        try:
            # Enable logging
            RatsClient.set_logging_enabled(True)
            
            # Start the client
            self.client.start()
            print(f"✓ File transfer client started on port {self.listen_port}")
            print(f"🆔 Our peer ID: {self.client.get_our_peer_id()}")
            return True
        except RatsError as e:
            print(f"❌ Failed to start client: {e}")
            return False
    
    def stop(self):
        """Stop the client."""
        self.client.stop()
        print("👋 Client stopped")
    
    def connect_to_peer(self, host: str, port: int):
        """Connect to a peer."""
        try:
            self.client.connect(host, port)
            print(f"🔗 Connecting to {host}:{port}...")
        except RatsError as e:
            print(f"❌ Connection failed: {e}")
    
    def send_file(self, peer_id: str, file_path: str, remote_filename: str = None):
        """Send a file to a peer."""
        if not os.path.exists(file_path):
            print(f"❌ File not found: {file_path}")
            return
        
        try:
            transfer_id = self.client.send_file(peer_id, file_path, remote_filename)
            self.active_transfers[transfer_id] = {
                'type': 'send',
                'peer_id': peer_id,
                'file_path': file_path,
                'remote_filename': remote_filename or os.path.basename(file_path)
            }
            print(f"📤 Sending file {file_path} to {peer_id} (transfer: {transfer_id})")
        except RatsError as e:
            print(f"❌ Failed to send file: {e}")
    
    def list_transfers(self):
        """List active file transfers."""
        if not self.active_transfers:
            print("📁 No active transfers")
            return
        
        print("📁 Active transfers:")
        for transfer_id, info in self.active_transfers.items():
            print(f"  {transfer_id}: {info['type']} {info['file_path']} "
                  f"({'to' if info['type'] == 'send' else 'from'} {info['peer_id']})")
    
    def show_peers(self):
        """Show connected peers."""
        count = client.get_peer_count()
        print(f"👥 Connected peers: {count}")
        
        stats = client.get_connection_statistics()
        if stats and 'peers' in stats:
            for peer_info in stats['peers']:
                print(f"  - {peer_info}")
    
    def run_interactive(self):
        """Run interactive command loop."""
        print("\nFile Transfer Commands:")
        print("  connect <host> <port>           - Connect to a peer")
        print("  send <peer_id> <file_path>      - Send a file to a peer")
        print("  send <peer_id> <file_path> <remote_name> - Send file with custom name")
        print("  transfers                       - Show active transfers")
        print("  peers                          - Show connected peers")
        print("  stats                          - Show connection statistics")
        print("  quit                           - Exit the program")
        print()
        
        while True:
            try:
                command = input("file-transfer> ").strip().split()
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
                        self.connect_to_peer(host, port)
                    except ValueError:
                        print("Error: Port must be a number")
                
                elif cmd == "send":
                    if len(command) < 3:
                        print("Usage: send <peer_id> <file_path> [remote_name]")
                        continue
                    
                    peer_id = command[1]
                    file_path = command[2]
                    remote_name = command[3] if len(command) > 3 else None
                    
                    self.send_file(peer_id, file_path, remote_name)
                
                elif cmd == "transfers":
                    self.list_transfers()
                
                elif cmd == "peers":
                    self.show_peers()
                
                elif cmd == "stats":
                    stats = self.client.get_connection_statistics()
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


def main():
    if len(sys.argv) != 2:
        print("Usage: python file_transfer.py <listen_port>")
        print("Example: python file_transfer.py 8080")
        sys.exit(1)
    
    try:
        listen_port = int(sys.argv[1])
    except ValueError:
        print("Error: Port must be a number")
        sys.exit(1)
    
    example = FileTransferExample(listen_port)
    
    try:
        if not example.start():
            sys.exit(1)
        
        example.run_interactive()
        
    except KeyboardInterrupt:
        print("\n👋 Interrupted by user")
    except Exception as e:
        print(f"❌ Unexpected error: {e}")
    finally:
        example.stop()


if __name__ == "__main__":
    main()
