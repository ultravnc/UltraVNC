"""
Core RatsClient implementation for Python bindings.
"""

import json
import threading
import weakref
from typing import Optional, List, Dict, Any, Callable
from ctypes import c_void_p, c_char_p, create_string_buffer, byref, cast, c_int, string_at

from .ctypes_wrapper import get_librats
from .enums import RatsError as ErrorCode, ConnectionStrategy, LogLevel, VersionInfo
from .exceptions import RatsError, check_error
from .callbacks import *


class RatsClient:
    """
    Python wrapper for the librats C client.
    
    This class provides a high-level Python interface to the librats P2P networking library.
    """
    
    def __init__(self, listen_port: int = 0):
        """
        Initialize a new RatsClient.
        
        Args:
            listen_port: Port to listen on for incoming connections (0 for random)
        """
        self._lib = get_librats()
        self._handle = self._lib.lib.rats_create(listen_port)
        if not self._handle:
            raise RatsError("Failed to create RatsClient")
        
        self._listen_port = listen_port
        self._running = False
        self._callbacks_lock = threading.Lock()
        
        # Store Python callbacks to prevent garbage collection
        self._callbacks = {}
        
        # Store C callback functions
        self._c_callbacks = {}
        
        # Weak reference for cleanup
        self._finalizer = weakref.finalize(self, self._cleanup, self._handle, self._lib)
    
    @staticmethod
    def _cleanup(handle, lib):
        """Cleanup function called when object is garbage collected."""
        if handle:
            lib.lib.rats_destroy(handle)
    
    def __enter__(self):
        """Context manager entry."""
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit."""
        self.stop()
    
    def start(self) -> None:
        """
        Start the RatsClient and begin listening for connections.
        
        Raises:
            RatsError: If starting the client fails
        """
        result = self._lib.lib.rats_start(self._handle)
        check_error(result, "Starting client")
        self._running = True
    
    def stop(self) -> None:
        """Stop the RatsClient and close all connections."""
        if self._handle:
            self._lib.lib.rats_stop(self._handle)
            self._running = False
    
    def is_running(self) -> bool:
        """Check if the client is currently running."""
        return self._running
    
    def connect(self, host: str, port: int, 
                strategy: ConnectionStrategy = ConnectionStrategy.AUTO_ADAPTIVE) -> None:
        """
        Connect to a peer.
        
        Args:
            host: Target host/IP address
            port: Target port
            strategy: Connection strategy to use
            
        Raises:
            RatsError: If connection fails
        """
        host_bytes = host.encode('utf-8')
        result = self._lib.lib.rats_connect_with_strategy(
            self._handle, host_bytes, port, strategy.value
        )
        check_error(result, f"Connecting to {host}:{port}")
    
    def disconnect_peer(self, peer_id: str) -> None:
        """
        Disconnect from a specific peer.
        
        Args:
            peer_id: Peer ID to disconnect
            
        Raises:
            RatsError: If disconnection fails
        """
        peer_id_bytes = peer_id.encode('utf-8')
        result = self._lib.lib.rats_disconnect_peer_by_id(self._handle, peer_id_bytes)
        check_error(result, f"Disconnecting peer {peer_id}")
    
    def send_string(self, peer_id: str, message: str) -> None:
        """
        Send a string message to a specific peer.
        
        Args:
            peer_id: Target peer ID
            message: String message to send
            
        Raises:
            RatsError: If sending fails
        """
        peer_id_bytes = peer_id.encode('utf-8')
        message_bytes = message.encode('utf-8')
        result = self._lib.lib.rats_send_string(self._handle, peer_id_bytes, message_bytes)
        check_error(result, f"Sending string to peer {peer_id}")
    
    def send_binary(self, peer_id: str, data: bytes) -> None:
        """
        Send binary data to a specific peer.
        
        Args:
            peer_id: Target peer ID
            data: Binary data to send
            
        Raises:
            RatsError: If sending fails
        """
        peer_id_bytes = peer_id.encode('utf-8')
        result = self._lib.lib.rats_send_binary(
            self._handle, peer_id_bytes, data, len(data)
        )
        check_error(result, f"Sending binary data to peer {peer_id}")
    
    def send_json(self, peer_id: str, data: Dict[str, Any]) -> None:
        """
        Send JSON data to a specific peer.
        
        Args:
            peer_id: Target peer ID
            data: Dictionary to send as JSON
            
        Raises:
            RatsError: If sending fails
        """
        peer_id_bytes = peer_id.encode('utf-8')
        json_bytes = json.dumps(data).encode('utf-8')
        result = self._lib.lib.rats_send_json(self._handle, peer_id_bytes, json_bytes)
        check_error(result, f"Sending JSON to peer {peer_id}")
    
    def broadcast_string(self, message: str) -> int:
        """
        Broadcast a string message to all connected peers.
        
        Args:
            message: String message to broadcast
            
        Returns:
            Number of peers the message was sent to
        """
        message_bytes = message.encode('utf-8')
        return self._lib.lib.rats_broadcast_string(self._handle, message_bytes)
    
    def broadcast_binary(self, data: bytes) -> int:
        """
        Broadcast binary data to all connected peers.
        
        Args:
            data: Binary data to broadcast
            
        Returns:
            Number of peers the data was sent to
        """
        return self._lib.lib.rats_broadcast_binary(self._handle, data, len(data))
    
    def broadcast_json(self, data: Dict[str, Any]) -> int:
        """
        Broadcast JSON data to all connected peers.
        
        Args:
            data: Dictionary to broadcast as JSON
            
        Returns:
            Number of peers the data was sent to
        """
        json_bytes = json.dumps(data).encode('utf-8')
        return self._lib.lib.rats_broadcast_json(self._handle, json_bytes)
    
    def get_peer_count(self) -> int:
        """Get the number of currently connected peers."""
        return self._lib.lib.rats_get_peer_count(self._handle)
    
    def get_listen_port(self) -> int:
        """Get the port the client is listening on."""
        return self._lib.lib.rats_get_listen_port(self._handle)
    
    def get_our_peer_id(self) -> str:
        """Get our own peer ID."""
        result = self._lib.lib.rats_get_our_peer_id(self._handle)
        if not result:
            return ""
        peer_id = string_at(result).decode('utf-8')
        self._lib.lib.rats_string_free(result)
        return peer_id
    
    def get_connection_statistics(self) -> Dict[str, Any]:
        """Get connection statistics as a dictionary."""
        result = self._lib.lib.rats_get_connection_statistics_json(self._handle)
        if not result:
            return {}
        
        json_str = string_at(result).decode('utf-8')
        self._lib.lib.rats_string_free(result)
        
        try:
            return json.loads(json_str)
        except json.JSONDecodeError:
            return {}
    
    def get_validated_peer_ids(self) -> List[str]:
        """Get list of validated peer IDs."""
        count = c_int()
        result = self._lib.lib.rats_get_validated_peer_ids(self._handle, byref(count))
        if not result or count.value == 0:
            return []
        
        peer_ids = []
        for i in range(count.value):
            if result[i]:
                peer_id = string_at(result[i]).decode('utf-8')
                peer_ids.append(peer_id)
                self._lib.lib.rats_string_free(result[i])
        
        # Free the array itself
        self._lib.lib.rats_string_free(cast(result, c_void_p))
        return peer_ids
    
    def get_peer_ids(self) -> List[str]:
        """Get list of all peer IDs."""
        count = c_int()
        result = self._lib.lib.rats_get_peer_ids(self._handle, byref(count))
        if not result or count.value == 0:
            return []
        
        peer_ids = []
        for i in range(count.value):
            if result[i]:
                peer_id = string_at(result[i]).decode('utf-8')
                peer_ids.append(peer_id)
                self._lib.lib.rats_string_free(result[i])
        
        # Free the array itself
        self._lib.lib.rats_string_free(cast(result, c_void_p))
        return peer_ids
    
    def get_peer_info(self, peer_id: str) -> Dict[str, Any]:
        """Get detailed information about a specific peer."""
        peer_id_bytes = peer_id.encode('utf-8')
        result = self._lib.lib.rats_get_peer_info_json(self._handle, peer_id_bytes)
        if not result:
            return {}
        
        json_str = string_at(result).decode('utf-8')
        self._lib.lib.rats_string_free(result)
        
        try:
            return json.loads(json_str)
        except json.JSONDecodeError:
            return {}
    
    # Peer configuration
    def set_max_peers(self, max_peers: int) -> None:
        """Set maximum number of peers."""
        result = self._lib.lib.rats_set_max_peers(self._handle, max_peers)
        check_error(result, "Setting max peers")
    
    def get_max_peers(self) -> int:
        """Get maximum number of peers."""
        return self._lib.lib.rats_get_max_peers(self._handle)
    
    def is_peer_limit_reached(self) -> bool:
        """Check if peer limit has been reached."""
        return bool(self._lib.lib.rats_is_peer_limit_reached(self._handle))
    
    # DHT Discovery
    def start_dht_discovery(self, dht_port: int = 6881) -> None:
        """Start DHT discovery."""
        result = self._lib.lib.rats_start_dht_discovery(self._handle, dht_port)
        check_error(result, "Starting DHT discovery")
    
    def stop_dht_discovery(self) -> None:
        """Stop DHT discovery."""
        self._lib.lib.rats_stop_dht_discovery(self._handle)
    
    def is_dht_running(self) -> bool:
        """Check if DHT is running."""
        return bool(self._lib.lib.rats_is_dht_running(self._handle))
    
    def announce_for_hash(self, content_hash: str, port: int) -> None:
        """Announce availability for a specific content hash."""
        content_hash_bytes = content_hash.encode('utf-8')
        result = self._lib.lib.rats_announce_for_hash(self._handle, content_hash_bytes, port)
        check_error(result, f"Announcing for hash {content_hash}")
    
    def get_dht_routing_table_size(self) -> int:
        """Get the size of the DHT routing table."""
        return self._lib.lib.rats_get_dht_routing_table_size(self._handle)
    
    # Automatic peer discovery
    def start_automatic_peer_discovery(self) -> None:
        """Start automatic peer discovery."""
        self._lib.lib.rats_start_automatic_peer_discovery(self._handle)
    
    def stop_automatic_peer_discovery(self) -> None:
        """Stop automatic peer discovery."""
        self._lib.lib.rats_stop_automatic_peer_discovery(self._handle)
    
    def is_automatic_discovery_running(self) -> bool:
        """Check if automatic discovery is running."""
        return bool(self._lib.lib.rats_is_automatic_discovery_running(self._handle))
    
    def get_discovery_hash(self) -> str:
        """Get the discovery hash for this client."""
        result = self._lib.lib.rats_get_discovery_hash(self._handle)
        if not result:
            return ""
        hash_str = string_at(result).decode('utf-8')
        self._lib.lib.rats_string_free(result)
        return hash_str
    
    @staticmethod
    def get_rats_peer_discovery_hash() -> str:
        """Get the default RATS peer discovery hash."""
        lib = get_librats()
        result = lib.lib.rats_get_rats_peer_discovery_hash()
        if not result:
            return ""
        hash_str = string_at(result).decode('utf-8')
        lib.lib.rats_string_free(result)
        return hash_str
    
    # mDNS Discovery
    def start_mdns_discovery(self, service_name: str = "rats-peer") -> None:
        """Start mDNS discovery with a service name."""
        service_name_bytes = service_name.encode('utf-8')
        result = self._lib.lib.rats_start_mdns_discovery(self._handle, service_name_bytes)
        check_error(result, f"Starting mDNS discovery with service {service_name}")
    
    def stop_mdns_discovery(self) -> None:
        """Stop mDNS discovery."""
        self._lib.lib.rats_stop_mdns_discovery(self._handle)
    
    def is_mdns_running(self) -> bool:
        """Check if mDNS discovery is running."""
        return bool(self._lib.lib.rats_is_mdns_running(self._handle))
    
    def query_mdns_services(self) -> None:
        """Query for mDNS services."""
        result = self._lib.lib.rats_query_mdns_services(self._handle)
        check_error(result, "Querying mDNS services")
    
    # Protocol configuration
    def set_protocol_name(self, protocol_name: str) -> None:
        """Set the protocol name."""
        protocol_name_bytes = protocol_name.encode('utf-8')
        result = self._lib.lib.rats_set_protocol_name(self._handle, protocol_name_bytes)
        check_error(result, f"Setting protocol name to {protocol_name}")
    
    def set_protocol_version(self, protocol_version: str) -> None:
        """Set the protocol version."""
        protocol_version_bytes = protocol_version.encode('utf-8')
        result = self._lib.lib.rats_set_protocol_version(self._handle, protocol_version_bytes)
        check_error(result, f"Setting protocol version to {protocol_version}")
    
    def get_protocol_name(self) -> str:
        """Get the current protocol name."""
        result = self._lib.lib.rats_get_protocol_name(self._handle)
        if not result:
            return ""
        protocol_name = string_at(result).decode('utf-8')
        self._lib.lib.rats_string_free(result)
        return protocol_name
    
    def get_protocol_version(self) -> str:
        """Get the current protocol version."""
        result = self._lib.lib.rats_get_protocol_version(self._handle)
        if not result:
            return ""
        protocol_version = string_at(result).decode('utf-8')
        self._lib.lib.rats_string_free(result)
        return protocol_version
    
    # Encryption
    def set_encryption_enabled(self, enabled: bool) -> None:
        """Enable or disable encryption."""
        result = self._lib.lib.rats_set_encryption_enabled(self._handle, int(enabled))
        check_error(result, "Setting encryption")
    
    def is_encryption_enabled(self) -> bool:
        """Check if encryption is enabled."""
        return bool(self._lib.lib.rats_is_encryption_enabled(self._handle))
    
    def get_encryption_key(self) -> str:
        """Get the encryption key as hex string."""
        result = self._lib.lib.rats_get_encryption_key(self._handle)
        if not result:
            return ""
        key = string_at(result).decode('utf-8')
        self._lib.lib.rats_string_free(result)
        return key
    
    def set_encryption_key(self, key_hex: str) -> None:
        """Set encryption key from hex string."""
        key_bytes = key_hex.encode('utf-8')
        result = self._lib.lib.rats_set_encryption_key(self._handle, key_bytes)
        check_error(result, "Setting encryption key")
    
    def generate_encryption_key(self) -> str:
        """Generate a new encryption key."""
        result = self._lib.lib.rats_generate_encryption_key(self._handle)
        if not result:
            return ""
        key = string_at(result).decode('utf-8')
        self._lib.lib.rats_string_free(result)
        return key
    
    # Message Exchange API
    def on_message(self, message_type: str, callback: MessageCallback) -> None:
        """Register a callback for a specific message type."""
        with self._callbacks_lock:
            callback_key = f"message_{message_type}"
            self._callbacks[callback_key] = callback
            
            if callback:
                c_callback = self._create_message_callback(callback)
                self._c_callbacks[callback_key] = c_callback
                message_type_bytes = message_type.encode('utf-8')
                result = self._lib.lib.rats_on_message(
                    self._handle, message_type_bytes, c_callback, None
                )
                check_error(result, f"Setting message callback for type {message_type}")
            else:
                message_type_bytes = message_type.encode('utf-8')
                result = self._lib.lib.rats_on_message(
                    self._handle, message_type_bytes, None, None
                )
                check_error(result, f"Clearing message callback for type {message_type}")
    
    def send_message(self, peer_id: str, message_type: str, data: str) -> None:
        """Send a typed message to a specific peer."""
        peer_id_bytes = peer_id.encode('utf-8')
        message_type_bytes = message_type.encode('utf-8')
        data_bytes = data.encode('utf-8')
        result = self._lib.lib.rats_send_message(
            self._handle, peer_id_bytes, message_type_bytes, data_bytes
        )
        check_error(result, f"Sending message type {message_type} to peer {peer_id}")
    
    def broadcast_message(self, message_type: str, data: str) -> None:
        """Broadcast a typed message to all connected peers."""
        message_type_bytes = message_type.encode('utf-8')
        data_bytes = data.encode('utf-8')
        result = self._lib.lib.rats_broadcast_message(
            self._handle, message_type_bytes, data_bytes
        )
        check_error(result, f"Broadcasting message type {message_type}")
    
    # File Transfer
    def send_file(self, peer_id: str, file_path: str, 
                  remote_filename: Optional[str] = None) -> str:
        """
        Send a file to a peer.
        
        Args:
            peer_id: Target peer ID
            file_path: Local file path to send
            remote_filename: Optional remote filename
            
        Returns:
            Transfer ID if successful
            
        Raises:
            RatsError: If sending fails
        """
        peer_id_bytes = peer_id.encode('utf-8')
        file_path_bytes = file_path.encode('utf-8')
        remote_filename_bytes = (remote_filename or "").encode('utf-8')
        
        result = self._lib.lib.rats_send_file(
            self._handle, peer_id_bytes, file_path_bytes, remote_filename_bytes
        )
        
        if not result:
            raise RatsError(f"Failed to send file {file_path} to peer {peer_id}")
        
        transfer_id = string_at(result).decode('utf-8')
        self._lib.lib.rats_string_free(result)
        return transfer_id
    
    def accept_file_transfer(self, transfer_id: str, local_path: str) -> None:
        """Accept an incoming file transfer."""
        transfer_id_bytes = transfer_id.encode('utf-8')
        local_path_bytes = local_path.encode('utf-8')
        result = self._lib.lib.rats_accept_file_transfer(
            self._handle, transfer_id_bytes, local_path_bytes
        )
        check_error(result, f"Accepting file transfer {transfer_id}")
    
    def reject_file_transfer(self, transfer_id: str, reason: str = "") -> None:
        """Reject an incoming file transfer."""
        transfer_id_bytes = transfer_id.encode('utf-8')
        reason_bytes = reason.encode('utf-8')
        result = self._lib.lib.rats_reject_file_transfer(
            self._handle, transfer_id_bytes, reason_bytes
        )
        check_error(result, f"Rejecting file transfer {transfer_id}")
    
    def cancel_file_transfer(self, transfer_id: str) -> None:
        """Cancel an active file transfer."""
        transfer_id_bytes = transfer_id.encode('utf-8')
        result = self._lib.lib.rats_cancel_file_transfer(self._handle, transfer_id_bytes)
        check_error(result, f"Cancelling file transfer {transfer_id}")
    
    def send_directory(self, peer_id: str, directory_path: str, 
                      remote_directory_name: str, recursive: bool = True) -> str:
        """
        Send a directory to a peer.
        
        Args:
            peer_id: Target peer ID
            directory_path: Local directory path to send
            remote_directory_name: Remote directory name
            recursive: Whether to send recursively
            
        Returns:
            Transfer ID if successful
            
        Raises:
            RatsError: If sending fails
        """
        peer_id_bytes = peer_id.encode('utf-8')
        directory_path_bytes = directory_path.encode('utf-8')
        remote_directory_name_bytes = remote_directory_name.encode('utf-8')
        
        result = self._lib.lib.rats_send_directory(
            self._handle, peer_id_bytes, directory_path_bytes, 
            remote_directory_name_bytes, int(recursive)
        )
        
        if not result:
            raise RatsError(f"Failed to send directory {directory_path} to peer {peer_id}")
        
        transfer_id = string_at(result).decode('utf-8')
        self._lib.lib.rats_string_free(result)
        return transfer_id
    
    def request_file(self, peer_id: str, remote_file_path: str, local_path: str) -> str:
        """
        Request a file from a peer.
        
        Args:
            peer_id: Target peer ID
            remote_file_path: Remote file path to request
            local_path: Local path to save the file
            
        Returns:
            Transfer ID if successful
            
        Raises:
            RatsError: If request fails
        """
        peer_id_bytes = peer_id.encode('utf-8')
        remote_file_path_bytes = remote_file_path.encode('utf-8')
        local_path_bytes = local_path.encode('utf-8')
        
        result = self._lib.lib.rats_request_file(
            self._handle, peer_id_bytes, remote_file_path_bytes, local_path_bytes
        )
        
        if not result:
            raise RatsError(f"Failed to request file {remote_file_path} from peer {peer_id}")
        
        transfer_id = string_at(result).decode('utf-8')
        self._lib.lib.rats_string_free(result)
        return transfer_id
    
    def request_directory(self, peer_id: str, remote_directory_path: str, 
                         local_directory_path: str, recursive: bool = True) -> str:
        """
        Request a directory from a peer.
        
        Args:
            peer_id: Target peer ID
            remote_directory_path: Remote directory path to request
            local_directory_path: Local directory path to save to
            recursive: Whether to request recursively
            
        Returns:
            Transfer ID if successful
            
        Raises:
            RatsError: If request fails
        """
        peer_id_bytes = peer_id.encode('utf-8')
        remote_directory_path_bytes = remote_directory_path.encode('utf-8')
        local_directory_path_bytes = local_directory_path.encode('utf-8')
        
        result = self._lib.lib.rats_request_directory(
            self._handle, peer_id_bytes, remote_directory_path_bytes, 
            local_directory_path_bytes, int(recursive)
        )
        
        if not result:
            raise RatsError(f"Failed to request directory {remote_directory_path} from peer {peer_id}")
        
        transfer_id = string_at(result).decode('utf-8')
        self._lib.lib.rats_string_free(result)
        return transfer_id
    
    def accept_directory_transfer(self, transfer_id: str, local_path: str) -> None:
        """Accept an incoming directory transfer."""
        transfer_id_bytes = transfer_id.encode('utf-8')
        local_path_bytes = local_path.encode('utf-8')
        result = self._lib.lib.rats_accept_directory_transfer(
            self._handle, transfer_id_bytes, local_path_bytes
        )
        check_error(result, f"Accepting directory transfer {transfer_id}")
    
    def reject_directory_transfer(self, transfer_id: str, reason: str = "") -> None:
        """Reject an incoming directory transfer."""
        transfer_id_bytes = transfer_id.encode('utf-8')
        reason_bytes = reason.encode('utf-8')
        result = self._lib.lib.rats_reject_directory_transfer(
            self._handle, transfer_id_bytes, reason_bytes
        )
        check_error(result, f"Rejecting directory transfer {transfer_id}")
    
    def pause_file_transfer(self, transfer_id: str) -> None:
        """Pause an active file transfer."""
        transfer_id_bytes = transfer_id.encode('utf-8')
        result = self._lib.lib.rats_pause_file_transfer(self._handle, transfer_id_bytes)
        check_error(result, f"Pausing file transfer {transfer_id}")
    
    def resume_file_transfer(self, transfer_id: str) -> None:
        """Resume a paused file transfer."""
        transfer_id_bytes = transfer_id.encode('utf-8')
        result = self._lib.lib.rats_resume_file_transfer(self._handle, transfer_id_bytes)
        check_error(result, f"Resuming file transfer {transfer_id}")
    
    def get_file_transfer_progress(self, transfer_id: str) -> Dict[str, Any]:
        """Get progress information for a file transfer."""
        transfer_id_bytes = transfer_id.encode('utf-8')
        result = self._lib.lib.rats_get_file_transfer_progress_json(
            self._handle, transfer_id_bytes
        )
        if not result:
            return {}
        
        json_str = string_at(result).decode('utf-8')
        self._lib.lib.rats_string_free(result)
        
        try:
            return json.loads(json_str)
        except json.JSONDecodeError:
            return {}
    
    def get_file_transfer_statistics(self) -> Dict[str, Any]:
        """Get overall file transfer statistics."""
        result = self._lib.lib.rats_get_file_transfer_statistics_json(self._handle)
        if not result:
            return {}
        
        json_str = string_at(result).decode('utf-8')
        self._lib.lib.rats_string_free(result)
        
        try:
            return json.loads(json_str)
        except json.JSONDecodeError:
            return {}
    
    # GossipSub
    def is_gossipsub_available(self) -> bool:
        """Check if GossipSub is available."""
        return bool(self._lib.lib.rats_is_gossipsub_available(self._handle))
    
    def subscribe_to_topic(self, topic: str) -> None:
        """Subscribe to a GossipSub topic."""
        topic_bytes = topic.encode('utf-8')
        result = self._lib.lib.rats_subscribe_to_topic(self._handle, topic_bytes)
        check_error(result, f"Subscribing to topic {topic}")
    
    def unsubscribe_from_topic(self, topic: str) -> None:
        """Unsubscribe from a GossipSub topic."""
        topic_bytes = topic.encode('utf-8')
        result = self._lib.lib.rats_unsubscribe_from_topic(self._handle, topic_bytes)
        check_error(result, f"Unsubscribing from topic {topic}")
    
    def publish_to_topic(self, topic: str, message: str) -> None:
        """Publish a message to a GossipSub topic."""
        topic_bytes = topic.encode('utf-8')
        message_bytes = message.encode('utf-8')
        result = self._lib.lib.rats_publish_to_topic(self._handle, topic_bytes, message_bytes)
        check_error(result, f"Publishing to topic {topic}")
    
    def is_gossipsub_running(self) -> bool:
        """Check if GossipSub is running."""
        return bool(self._lib.lib.rats_is_gossipsub_running(self._handle))
    
    def is_subscribed_to_topic(self, topic: str) -> bool:
        """Check if we are subscribed to a specific topic."""
        topic_bytes = topic.encode('utf-8')
        return bool(self._lib.lib.rats_is_subscribed_to_topic(self._handle, topic_bytes))
    
    def get_subscribed_topics(self) -> List[str]:
        """Get list of topics we are subscribed to."""
        count = c_int()
        result = self._lib.lib.rats_get_subscribed_topics(self._handle, byref(count))
        if not result or count.value == 0:
            return []
        
        topics = []
        for i in range(count.value):
            if result[i]:
                topic = string_at(result[i]).decode('utf-8')
                topics.append(topic)
                self._lib.lib.rats_string_free(result[i])
        
        # Free the array itself
        self._lib.lib.rats_string_free(cast(result, c_void_p))
        return topics
    
    def publish_json_to_topic(self, topic: str, data: Dict[str, Any]) -> None:
        """Publish JSON data to a GossipSub topic."""
        topic_bytes = topic.encode('utf-8')
        json_bytes = json.dumps(data).encode('utf-8')
        result = self._lib.lib.rats_publish_json_to_topic(self._handle, topic_bytes, json_bytes)
        check_error(result, f"Publishing JSON to topic {topic}")
    
    def get_topic_peers(self, topic: str) -> List[str]:
        """Get list of peers subscribed to a topic."""
        topic_bytes = topic.encode('utf-8')
        count = c_int()
        result = self._lib.lib.rats_get_topic_peers(self._handle, topic_bytes, byref(count))
        if not result or count.value == 0:
            return []
        
        peers = []
        for i in range(count.value):
            if result[i]:
                peer_id = string_at(result[i]).decode('utf-8')
                peers.append(peer_id)
                self._lib.lib.rats_string_free(result[i])
        
        # Free the array itself
        self._lib.lib.rats_string_free(cast(result, c_void_p))
        return peers
    
    def get_topic_mesh_peers(self, topic: str) -> List[str]:
        """Get list of mesh peers for a topic."""
        topic_bytes = topic.encode('utf-8')
        count = c_int()
        result = self._lib.lib.rats_get_topic_mesh_peers(self._handle, topic_bytes, byref(count))
        if not result or count.value == 0:
            return []
        
        peers = []
        for i in range(count.value):
            if result[i]:
                peer_id = string_at(result[i]).decode('utf-8')
                peers.append(peer_id)
                self._lib.lib.rats_string_free(result[i])
        
        # Free the array itself
        self._lib.lib.rats_string_free(cast(result, c_void_p))
        return peers
    
    def get_gossipsub_statistics(self) -> Dict[str, Any]:
        """Get GossipSub statistics."""
        result = self._lib.lib.rats_get_gossipsub_statistics_json(self._handle)
        if not result:
            return {}
        
        json_str = string_at(result).decode('utf-8')
        self._lib.lib.rats_string_free(result)
        
        try:
            return json.loads(json_str)
        except json.JSONDecodeError:
            return {}
    
    # NAT Traversal and STUN
    def discover_and_ignore_public_ip(self, stun_server: str = "stun.l.google.com", 
                                     stun_port: int = 19302) -> None:
        """Discover public IP via STUN and add it to ignore list."""
        stun_server_bytes = stun_server.encode('utf-8')
        result = self._lib.lib.rats_discover_and_ignore_public_ip(
            self._handle, stun_server_bytes, stun_port
        )
        check_error(result, f"Discovering public IP via {stun_server}:{stun_port}")
    
    def get_public_ip(self) -> str:
        """Get the discovered public IP address."""
        result = self._lib.lib.rats_get_public_ip(self._handle)
        if not result:
            return ""
        ip = string_at(result).decode('utf-8')
        self._lib.lib.rats_string_free(result)
        return ip
    
    def detect_nat_type(self) -> int:
        """Detect NAT type and return as integer."""
        return self._lib.lib.rats_detect_nat_type(self._handle)
    
    def get_nat_characteristics(self) -> Dict[str, Any]:
        """Get NAT characteristics information."""
        result = self._lib.lib.rats_get_nat_characteristics_json(self._handle)
        if not result:
            return {}
        
        json_str = string_at(result).decode('utf-8')
        self._lib.lib.rats_string_free(result)
        
        try:
            return json.loads(json_str)
        except json.JSONDecodeError:
            return {}
    
    def add_ignored_address(self, ip_address: str) -> None:
        """Add an IP address to the ignore list."""
        ip_address_bytes = ip_address.encode('utf-8')
        self._lib.lib.rats_add_ignored_address(self._handle, ip_address_bytes)
    
    def get_nat_traversal_statistics(self) -> Dict[str, Any]:
        """Get NAT traversal statistics."""
        result = self._lib.lib.rats_get_nat_traversal_statistics_json(self._handle)
        if not result:
            return {}
        
        json_str = string_at(result).decode('utf-8')
        self._lib.lib.rats_string_free(result)
        
        try:
            return json.loads(json_str)
        except json.JSONDecodeError:
            return {}
    
    # ICE coordination
    def create_ice_offer(self, peer_id: str) -> Dict[str, Any]:
        """Create an ICE offer for a peer."""
        peer_id_bytes = peer_id.encode('utf-8')
        result = self._lib.lib.rats_create_ice_offer(self._handle, peer_id_bytes)
        if not result:
            return {}
        
        json_str = string_at(result).decode('utf-8')
        self._lib.lib.rats_string_free(result)
        
        try:
            return json.loads(json_str)
        except json.JSONDecodeError:
            return {}
    
    def connect_with_ice(self, peer_id: str, ice_offer: Dict[str, Any]) -> None:
        """Connect to a peer using ICE."""
        peer_id_bytes = peer_id.encode('utf-8')
        ice_offer_json = json.dumps(ice_offer).encode('utf-8')
        result = self._lib.lib.rats_connect_with_ice(
            self._handle, peer_id_bytes, ice_offer_json
        )
        check_error(result, f"Connecting with ICE to peer {peer_id}")
    
    def handle_ice_answer(self, peer_id: str, ice_answer: Dict[str, Any]) -> None:
        """Handle an ICE answer from a peer."""
        peer_id_bytes = peer_id.encode('utf-8')
        ice_answer_json = json.dumps(ice_answer).encode('utf-8')
        result = self._lib.lib.rats_handle_ice_answer(
            self._handle, peer_id_bytes, ice_answer_json
        )
        check_error(result, f"Handling ICE answer from peer {peer_id}")
    
    # Configuration persistence
    def load_configuration(self) -> None:
        """Load configuration from persistent storage."""
        result = self._lib.lib.rats_load_configuration(self._handle)
        check_error(result, "Loading configuration")
    
    def save_configuration(self) -> None:
        """Save configuration to persistent storage."""
        result = self._lib.lib.rats_save_configuration(self._handle)
        check_error(result, "Saving configuration")
    
    def set_data_directory(self, directory_path: str) -> None:
        """Set the data directory path."""
        directory_path_bytes = directory_path.encode('utf-8')
        result = self._lib.lib.rats_set_data_directory(self._handle, directory_path_bytes)
        check_error(result, f"Setting data directory to {directory_path}")
    
    def get_data_directory(self) -> str:
        """Get the current data directory path."""
        result = self._lib.lib.rats_get_data_directory(self._handle)
        if not result:
            return ""
        directory = string_at(result).decode('utf-8')
        self._lib.lib.rats_string_free(result)
        return directory
    
    def load_and_reconnect_peers(self) -> int:
        """Load and reconnect to historical peers."""
        return self._lib.lib.rats_load_and_reconnect_peers(self._handle)
    
    def load_historical_peers(self) -> int:
        """Load historical peers list."""
        return self._lib.lib.rats_load_historical_peers(self._handle)
    
    def save_historical_peers(self) -> int:
        """Save current peers to historical list."""
        return self._lib.lib.rats_save_historical_peers(self._handle)
    
    def clear_historical_peers(self) -> None:
        """Clear the historical peers list."""
        self._lib.lib.rats_clear_historical_peers(self._handle)
    
    def get_historical_peer_ids(self) -> List[str]:
        """Get list of historical peer IDs."""
        count = c_int()
        result = self._lib.lib.rats_get_historical_peer_ids(self._handle, byref(count))
        if not result or count.value == 0:
            return []
        
        peer_ids = []
        for i in range(count.value):
            if result[i]:
                peer_id = string_at(result[i]).decode('utf-8')
                peer_ids.append(peer_id)
                self._lib.lib.rats_string_free(result[i])
        
        # Free the array itself
        self._lib.lib.rats_string_free(cast(result, c_void_p))
        return peer_ids
    
    # Logging instance methods
    def set_log_file_path(self, file_path: str) -> None:
        """Set log file path for this client."""
        file_path_bytes = file_path.encode('utf-8')
        self._lib.lib.rats_set_log_file_path(self._handle, file_path_bytes)
    
    def get_log_file_path(self) -> str:
        """Get the current log file path."""
        result = self._lib.lib.rats_get_log_file_path(self._handle)
        if not result:
            return ""
        path = string_at(result).decode('utf-8')
        self._lib.lib.rats_string_free(result)
        return path
    
    def set_log_colors_enabled(self, enabled: bool) -> None:
        """Enable or disable log colors."""
        self._lib.lib.rats_set_log_colors_enabled(self._handle, int(enabled))
    
    def is_log_colors_enabled(self) -> bool:
        """Check if log colors are enabled."""
        return bool(self._lib.lib.rats_is_log_colors_enabled(self._handle))
    
    def set_log_timestamps_enabled(self, enabled: bool) -> None:
        """Enable or disable log timestamps."""
        self._lib.lib.rats_set_log_timestamps_enabled(self._handle, int(enabled))
    
    def is_log_timestamps_enabled(self) -> bool:
        """Check if log timestamps are enabled."""
        return bool(self._lib.lib.rats_is_log_timestamps_enabled(self._handle))
    
    def set_log_rotation_size(self, max_size_bytes: int) -> None:
        """Set log rotation size in bytes."""
        self._lib.lib.rats_set_log_rotation_size(self._handle, max_size_bytes)
    
    def set_log_retention_count(self, count: int) -> None:
        """Set log retention count."""
        self._lib.lib.rats_set_log_retention_count(self._handle, count)
    
    def clear_log_file(self) -> None:
        """Clear the log file."""
        self._lib.lib.rats_clear_log_file(self._handle)
    
    # Callback management
    def _create_connection_callback(self, callback: ConnectionCallback):
        """Create a C callback wrapper for connection events."""
        def c_callback(user_data, peer_id_ptr):
            if callback and peer_id_ptr:
                peer_id = peer_id_ptr.decode('utf-8')
                try:
                    callback(peer_id)
                except Exception as e:
                    print(f"Error in connection callback: {e}")
        return ConnectionCallbackType(c_callback)
    
    def _create_string_callback(self, callback: StringCallback):
        """Create a C callback wrapper for string messages."""
        def c_callback(user_data, peer_id_ptr, message_ptr):
            if callback and peer_id_ptr and message_ptr:
                peer_id = peer_id_ptr.decode('utf-8')
                message = message_ptr.decode('utf-8')
                try:
                    callback(peer_id, message)
                except Exception as e:
                    print(f"Error in string callback: {e}")
        return StringCallbackType(c_callback)
    
    def _create_binary_callback(self, callback: BinaryCallback):
        """Create a C callback wrapper for binary data."""
        def c_callback(user_data, peer_id_ptr, data_ptr, size):
            if callback and peer_id_ptr and data_ptr and size:
                peer_id = peer_id_ptr.decode('utf-8')
                data_bytes = string_at(data_ptr, size)
                try:
                    callback(peer_id, data_bytes)
                except Exception as e:
                    print(f"Error in binary callback: {e}")
        return BinaryCallbackType(c_callback)
    
    def _create_json_callback(self, callback: JsonCallback):
        """Create a C callback wrapper for JSON messages."""
        def c_callback(user_data, peer_id_ptr, json_ptr):
            if callback and peer_id_ptr and json_ptr:
                peer_id = peer_id_ptr.decode('utf-8')
                json_str = json_ptr.decode('utf-8')
                try:
                    data = json.loads(json_str)
                    callback(peer_id, data)
                except (json.JSONDecodeError, Exception) as e:
                    print(f"Error in JSON callback: {e}")
        return JsonCallbackType(c_callback)
    
    def _create_disconnect_callback(self, callback: DisconnectCallback):
        """Create a C callback wrapper for disconnect events."""
        def c_callback(user_data, peer_id_ptr):
            if callback and peer_id_ptr:
                peer_id = peer_id_ptr.decode('utf-8')
                try:
                    callback(peer_id)
                except Exception as e:
                    print(f"Error in disconnect callback: {e}")
        return DisconnectCallbackType(c_callback)
    
    def set_connection_callback(self, callback: ConnectionCallback) -> None:
        """Set callback for new peer connections."""
        with self._callbacks_lock:
            self._callbacks['connection'] = callback
            if callback:
                c_callback = self._create_connection_callback(callback)
                self._c_callbacks['connection'] = c_callback
                self._lib.lib.rats_set_connection_callback(self._handle, c_callback, None)
            else:
                self._lib.lib.rats_set_connection_callback(self._handle, None, None)
    
    def set_string_callback(self, callback: StringCallback) -> None:
        """Set callback for string messages."""
        with self._callbacks_lock:
            self._callbacks['string'] = callback
            if callback:
                c_callback = self._create_string_callback(callback)
                self._c_callbacks['string'] = c_callback
                self._lib.lib.rats_set_string_callback(self._handle, c_callback, None)
            else:
                self._lib.lib.rats_set_string_callback(self._handle, None, None)
    
    def set_binary_callback(self, callback: BinaryCallback) -> None:
        """Set callback for binary data."""
        with self._callbacks_lock:
            self._callbacks['binary'] = callback
            if callback:
                c_callback = self._create_binary_callback(callback)
                self._c_callbacks['binary'] = c_callback
                self._lib.lib.rats_set_binary_callback(self._handle, c_callback, None)
            else:
                self._lib.lib.rats_set_binary_callback(self._handle, None, None)
    
    def set_json_callback(self, callback: JsonCallback) -> None:
        """Set callback for JSON messages."""
        with self._callbacks_lock:
            self._callbacks['json'] = callback
            if callback:
                c_callback = self._create_json_callback(callback)
                self._c_callbacks['json'] = c_callback
                self._lib.lib.rats_set_json_callback(self._handle, c_callback, None)
            else:
                self._lib.lib.rats_set_json_callback(self._handle, None, None)
    
    def set_disconnect_callback(self, callback: DisconnectCallback) -> None:
        """Set callback for peer disconnections."""
        with self._callbacks_lock:
            self._callbacks['disconnect'] = callback
            if callback:
                c_callback = self._create_disconnect_callback(callback)
                self._c_callbacks['disconnect'] = c_callback
                self._lib.lib.rats_set_disconnect_callback(self._handle, c_callback, None)
            else:
                self._lib.lib.rats_set_disconnect_callback(self._handle, None, None)
    
    # Additional callback creators and setters
    def _create_message_callback(self, callback: MessageCallback):
        """Create a C callback wrapper for message events."""
        def c_callback(user_data, peer_id_ptr, message_data_ptr):
            if callback and peer_id_ptr and message_data_ptr:
                peer_id = peer_id_ptr.decode('utf-8')
                message_data = message_data_ptr.decode('utf-8')
                try:
                    callback(peer_id, message_data)
                except Exception as e:
                    print(f"Error in message callback: {e}")
        return MessageCallbackType(c_callback)
    
    def _create_file_request_callback(self, callback: FileRequestCallback):
        """Create a C callback wrapper for file request events."""
        def c_callback(user_data, peer_id_ptr, transfer_id_ptr, remote_path_ptr, filename_ptr):
            if callback and peer_id_ptr and transfer_id_ptr and remote_path_ptr and filename_ptr:
                peer_id = peer_id_ptr.decode('utf-8')
                transfer_id = transfer_id_ptr.decode('utf-8')
                remote_path = remote_path_ptr.decode('utf-8')
                filename = filename_ptr.decode('utf-8')
                try:
                    callback(peer_id, transfer_id, remote_path, filename)
                except Exception as e:
                    print(f"Error in file request callback: {e}")
        return FileRequestCallbackType(c_callback)
    
    def _create_directory_request_callback(self, callback: DirectoryRequestCallback):
        """Create a C callback wrapper for directory request events."""
        def c_callback(user_data, peer_id_ptr, transfer_id_ptr, remote_path_ptr, directory_name_ptr):
            if callback and peer_id_ptr and transfer_id_ptr and remote_path_ptr and directory_name_ptr:
                peer_id = peer_id_ptr.decode('utf-8')
                transfer_id = transfer_id_ptr.decode('utf-8')
                remote_path = remote_path_ptr.decode('utf-8')
                directory_name = directory_name_ptr.decode('utf-8')
                try:
                    callback(peer_id, transfer_id, remote_path, directory_name)
                except Exception as e:
                    print(f"Error in directory request callback: {e}")
        return DirectoryRequestCallbackType(c_callback)
    
    def _create_file_progress_callback(self, callback: FileProgressCallback):
        """Create a C callback wrapper for file progress events."""
        def c_callback(user_data, transfer_id_ptr, progress_percent, status_ptr):
            if callback and transfer_id_ptr and status_ptr:
                transfer_id = transfer_id_ptr.decode('utf-8')
                status = status_ptr.decode('utf-8')
                try:
                    callback(transfer_id, progress_percent, status)
                except Exception as e:
                    print(f"Error in file progress callback: {e}")
        return FileProgressCallbackType(c_callback)
    
    def _create_directory_progress_callback(self, callback: DirectoryProgressCallback):
        """Create a C callback wrapper for directory progress events."""
        def c_callback(user_data, transfer_id_ptr, files_completed, total_files, current_file_ptr):
            if callback and transfer_id_ptr and current_file_ptr:
                transfer_id = transfer_id_ptr.decode('utf-8')
                current_file = current_file_ptr.decode('utf-8')
                try:
                    callback(transfer_id, files_completed, total_files, current_file)
                except Exception as e:
                    print(f"Error in directory progress callback: {e}")
        return DirectoryProgressCallbackType(c_callback)
    
    def _create_peer_discovered_callback(self, callback: PeerDiscoveredCallback):
        """Create a C callback wrapper for peer discovery events."""
        def c_callback(user_data, host_ptr, port, service_name_ptr):
            if callback and host_ptr and service_name_ptr:
                host = host_ptr.decode('utf-8')
                service_name = service_name_ptr.decode('utf-8')
                try:
                    callback(host, port, service_name)
                except Exception as e:
                    print(f"Error in peer discovered callback: {e}")
        return PeerDiscoveredCallbackType(c_callback)
    
    def _create_topic_message_callback(self, callback: TopicMessageCallback):
        """Create a C callback wrapper for topic message events."""
        def c_callback(user_data, peer_id_ptr, topic_ptr, message_ptr):
            if callback and peer_id_ptr and topic_ptr and message_ptr:
                peer_id = peer_id_ptr.decode('utf-8')
                topic = topic_ptr.decode('utf-8')
                message = message_ptr.decode('utf-8')
                try:
                    callback(peer_id, topic, message)
                except Exception as e:
                    print(f"Error in topic message callback: {e}")
        return TopicMessageCallbackType(c_callback)
    
    def _create_topic_json_message_callback(self, callback: TopicJsonMessageCallback):
        """Create a C callback wrapper for topic JSON message events."""
        def c_callback(user_data, peer_id_ptr, topic_ptr, json_ptr):
            if callback and peer_id_ptr and topic_ptr and json_ptr:
                peer_id = peer_id_ptr.decode('utf-8')
                topic = topic_ptr.decode('utf-8')
                json_str = json_ptr.decode('utf-8')
                try:
                    data = json.loads(json_str)
                    callback(peer_id, topic, data)
                except (json.JSONDecodeError, Exception) as e:
                    print(f"Error in topic JSON message callback: {e}")
        return TopicJsonMessageCallbackType(c_callback)
    
    def _create_topic_peer_joined_callback(self, callback: TopicPeerJoinedCallback):
        """Create a C callback wrapper for topic peer joined events."""
        def c_callback(user_data, peer_id_ptr, topic_ptr):
            if callback and peer_id_ptr and topic_ptr:
                peer_id = peer_id_ptr.decode('utf-8')
                topic = topic_ptr.decode('utf-8')
                try:
                    callback(peer_id, topic)
                except Exception as e:
                    print(f"Error in topic peer joined callback: {e}")
        return TopicPeerJoinedCallbackType(c_callback)
    
    def _create_topic_peer_left_callback(self, callback: TopicPeerLeftCallback):
        """Create a C callback wrapper for topic peer left events."""
        def c_callback(user_data, peer_id_ptr, topic_ptr):
            if callback and peer_id_ptr and topic_ptr:
                peer_id = peer_id_ptr.decode('utf-8')
                topic = topic_ptr.decode('utf-8')
                try:
                    callback(peer_id, topic)
                except Exception as e:
                    print(f"Error in topic peer left callback: {e}")
        return TopicPeerLeftCallbackType(c_callback)
    
    # Additional callback setters
    def set_file_request_callback(self, callback: FileRequestCallback) -> None:
        """Set callback for file transfer requests."""
        with self._callbacks_lock:
            self._callbacks['file_request'] = callback
            if callback:
                c_callback = self._create_file_request_callback(callback)
                self._c_callbacks['file_request'] = c_callback
                self._lib.lib.rats_set_file_request_callback(self._handle, c_callback, None)
            else:
                self._lib.lib.rats_set_file_request_callback(self._handle, None, None)
    
    def set_directory_request_callback(self, callback: DirectoryRequestCallback) -> None:
        """Set callback for directory transfer requests."""
        with self._callbacks_lock:
            self._callbacks['directory_request'] = callback
            if callback:
                c_callback = self._create_directory_request_callback(callback)
                self._c_callbacks['directory_request'] = c_callback
                self._lib.lib.rats_set_directory_request_callback(self._handle, c_callback, None)
            else:
                self._lib.lib.rats_set_directory_request_callback(self._handle, None, None)
    
    def set_file_progress_callback(self, callback: FileProgressCallback) -> None:
        """Set callback for file transfer progress."""
        with self._callbacks_lock:
            self._callbacks['file_progress'] = callback
            if callback:
                c_callback = self._create_file_progress_callback(callback)
                self._c_callbacks['file_progress'] = c_callback
                self._lib.lib.rats_set_file_progress_callback(self._handle, c_callback, None)
            else:
                self._lib.lib.rats_set_file_progress_callback(self._handle, None, None)
    
    def set_directory_progress_callback(self, callback: DirectoryProgressCallback) -> None:
        """Set callback for directory transfer progress."""
        with self._callbacks_lock:
            self._callbacks['directory_progress'] = callback
            if callback:
                c_callback = self._create_directory_progress_callback(callback)
                self._c_callbacks['directory_progress'] = c_callback
                self._lib.lib.rats_set_directory_progress_callback(self._handle, c_callback, None)
            else:
                self._lib.lib.rats_set_directory_progress_callback(self._handle, None, None)
    
    def set_peer_discovered_callback(self, callback: PeerDiscoveredCallback) -> None:
        """Set callback for peer discovery events."""
        with self._callbacks_lock:
            self._callbacks['peer_discovered'] = callback
            if callback:
                c_callback = self._create_peer_discovered_callback(callback)
                self._c_callbacks['peer_discovered'] = c_callback
                self._lib.lib.rats_set_peer_discovered_callback(self._handle, c_callback, None)
            else:
                self._lib.lib.rats_set_peer_discovered_callback(self._handle, None, None)
    
    def set_topic_message_callback(self, topic: str, callback: TopicMessageCallback) -> None:
        """Set callback for topic messages."""
        with self._callbacks_lock:
            callback_key = f"topic_message_{topic}"
            self._callbacks[callback_key] = callback
            
            if callback:
                c_callback = self._create_topic_message_callback(callback)
                self._c_callbacks[callback_key] = c_callback
                topic_bytes = topic.encode('utf-8')
                self._lib.lib.rats_set_topic_message_callback(
                    self._handle, topic_bytes, c_callback, None
                )
            else:
                topic_bytes = topic.encode('utf-8')
                self._lib.lib.rats_set_topic_message_callback(
                    self._handle, topic_bytes, None, None
                )
    
    def set_topic_json_message_callback(self, topic: str, callback: TopicJsonMessageCallback) -> None:
        """Set callback for topic JSON messages."""
        with self._callbacks_lock:
            callback_key = f"topic_json_message_{topic}"
            self._callbacks[callback_key] = callback
            
            if callback:
                c_callback = self._create_topic_json_message_callback(callback)
                self._c_callbacks[callback_key] = c_callback
                topic_bytes = topic.encode('utf-8')
                self._lib.lib.rats_set_topic_json_message_callback(
                    self._handle, topic_bytes, c_callback, None
                )
            else:
                topic_bytes = topic.encode('utf-8')
                self._lib.lib.rats_set_topic_json_message_callback(
                    self._handle, topic_bytes, None, None
                )
    
    def set_topic_peer_joined_callback(self, topic: str, callback: TopicPeerJoinedCallback) -> None:
        """Set callback for topic peer joined events."""
        with self._callbacks_lock:
            callback_key = f"topic_peer_joined_{topic}"
            self._callbacks[callback_key] = callback
            
            if callback:
                c_callback = self._create_topic_peer_joined_callback(callback)
                self._c_callbacks[callback_key] = c_callback
                topic_bytes = topic.encode('utf-8')
                self._lib.lib.rats_set_topic_peer_joined_callback(
                    self._handle, topic_bytes, c_callback, None
                )
            else:
                topic_bytes = topic.encode('utf-8')
                self._lib.lib.rats_set_topic_peer_joined_callback(
                    self._handle, topic_bytes, None, None
                )
    
    def set_topic_peer_left_callback(self, topic: str, callback: TopicPeerLeftCallback) -> None:
        """Set callback for topic peer left events."""
        with self._callbacks_lock:
            callback_key = f"topic_peer_left_{topic}"
            self._callbacks[callback_key] = callback
            
            if callback:
                c_callback = self._create_topic_peer_left_callback(callback)
                self._c_callbacks[callback_key] = c_callback
                topic_bytes = topic.encode('utf-8')
                self._lib.lib.rats_set_topic_peer_left_callback(
                    self._handle, topic_bytes, c_callback, None
                )
            else:
                topic_bytes = topic.encode('utf-8')
                self._lib.lib.rats_set_topic_peer_left_callback(
                    self._handle, topic_bytes, None, None
                )
    
    def clear_topic_callbacks(self, topic: str) -> None:
        """Clear all callbacks for a specific topic."""
        topic_bytes = topic.encode('utf-8')
        self._lib.lib.rats_clear_topic_callbacks(self._handle, topic_bytes)
        
        # Also clear from our Python callback storage
        with self._callbacks_lock:
            keys_to_remove = [k for k in self._callbacks.keys() if k.endswith(f"_{topic}")]
            for key in keys_to_remove:
                if key in self._callbacks:
                    del self._callbacks[key]
                if key in self._c_callbacks:
                    del self._c_callbacks[key]
    
    # Static logging methods
    @staticmethod
    def set_logging_enabled(enabled: bool) -> None:
        """Enable or disable global logging."""
        lib = get_librats()
        lib.lib.rats_set_logging_enabled(int(enabled))
    
    @staticmethod
    def set_log_level(level: LogLevel) -> None:
        """Set global log level."""
        lib = get_librats()
        level_str = level.name.encode('utf-8')
        lib.lib.rats_set_log_level(level_str)
    
    # Static version methods
    @staticmethod
    def get_version_string() -> str:
        """Get version string."""
        lib = get_librats()
        result = lib.lib.rats_get_version_string()
        if not result:
            return ""
        version = string_at(result).decode('utf-8')
        lib.lib.rats_string_free(result)
        return version
    
    @staticmethod
    def get_version() -> VersionInfo:
        """Get detailed version information."""
        lib = get_librats()
        major = c_int()
        minor = c_int()
        patch = c_int()
        build = c_int()
        lib.lib.rats_get_version(byref(major), byref(minor), byref(patch), byref(build))
        return VersionInfo(major.value, minor.value, patch.value, build.value)
    
    @staticmethod
    def get_git_describe() -> str:
        """Get git describe string."""
        lib = get_librats()
        result = lib.lib.rats_get_git_describe()
        if not result:
            return ""
        git_desc = string_at(result).decode('utf-8')
        lib.lib.rats_string_free(result)
        return git_desc
    
    @staticmethod
    def get_abi() -> int:
        """Get ABI version."""
        lib = get_librats()
        return lib.lib.rats_get_abi()


# The RatsError exception is imported from exceptions module in __init__.py
