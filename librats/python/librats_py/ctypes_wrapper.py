"""
Low-level ctypes wrapper for librats C API.
"""

import ctypes
import os
import platform
import sys
from ctypes import (
    CDLL, POINTER, Structure, c_void_p, c_char_p, c_int, c_size_t,
    c_uint32, c_uint16, c_uint8, byref, create_string_buffer
)
from typing import Optional

from .callbacks import *
from .enums import RatsError, ConnectionStrategy


class LibratsNotFoundError(Exception):
    """Raised when the librats shared library cannot be found."""
    pass


def find_librats_library() -> str:
    """Find the librats shared library."""
    system = platform.system().lower()
    
    # Common library names
    if system == 'windows':
        lib_names = ['librats.dll', 'rats.dll']
    elif system == 'darwin':
        lib_names = ['librats.dylib', 'librats.so']
    else:  # Linux and others
        lib_names = ['librats.so', 'librats.so.1']
    
    # Search paths
    search_paths = [
        '.',
        '../build',
        '../../build',
        '../../../build',
        '/usr/local/lib',
        '/usr/lib',
        os.path.join(os.path.dirname(__file__), '..', '..', 'build'),
    ]
    
    # Add system paths
    if 'LD_LIBRARY_PATH' in os.environ:
        search_paths.extend(os.environ['LD_LIBRARY_PATH'].split(':'))
    
    if system == 'windows' and 'PATH' in os.environ:
        search_paths.extend(os.environ['PATH'].split(';'))
    
    # Try to find the library
    for path in search_paths:
        for lib_name in lib_names:
            lib_path = os.path.join(path, lib_name)
            if os.path.exists(lib_path):
                return lib_path
    
    # If not found, try loading by name (system will search)
    for lib_name in lib_names:
        try:
            # Test if we can load it
            test_lib = CDLL(lib_name)
            return lib_name
        except OSError:
            continue
    
    raise LibratsNotFoundError(
        f"Could not find librats shared library. Searched for: {lib_names} "
        f"in paths: {search_paths}"
    )


class LibratsCtypes:
    """Low-level ctypes wrapper for librats C API."""
    
    def __init__(self):
        lib_path = find_librats_library()
        try:
            self.lib = CDLL(lib_path)
        except OSError as e:
            raise LibratsNotFoundError(f"Failed to load librats library at {lib_path}: {e}")
        
        self._setup_function_signatures()
    
    def _setup_function_signatures(self):
        """Set up function signatures for type safety."""
        
        # Memory management
        self.lib.rats_string_free.argtypes = [c_void_p]
        self.lib.rats_string_free.restype = None
        
        # Version functions
        self.lib.rats_get_version_string.argtypes = []
        self.lib.rats_get_version_string.restype = c_void_p
        
        self.lib.rats_get_version.argtypes = [POINTER(c_int), POINTER(c_int), POINTER(c_int), POINTER(c_int)]
        self.lib.rats_get_version.restype = None
        
        self.lib.rats_get_git_describe.argtypes = []
        self.lib.rats_get_git_describe.restype = c_void_p
        
        self.lib.rats_get_abi.argtypes = []
        self.lib.rats_get_abi.restype = c_uint32
        
        # Client lifecycle
        self.lib.rats_create.argtypes = [c_int]
        self.lib.rats_create.restype = c_void_p
        
        self.lib.rats_destroy.argtypes = [c_void_p]
        self.lib.rats_destroy.restype = None
        
        self.lib.rats_start.argtypes = [c_void_p]
        self.lib.rats_start.restype = c_int
        
        self.lib.rats_stop.argtypes = [c_void_p]
        self.lib.rats_stop.restype = None
        
        # Basic operations
        self.lib.rats_connect.argtypes = [c_void_p, c_char_p, c_int]
        self.lib.rats_connect.restype = c_int
        
        self.lib.rats_get_listen_port.argtypes = [c_void_p]
        self.lib.rats_get_listen_port.restype = c_int
        
        self.lib.rats_broadcast_string.argtypes = [c_void_p, c_char_p]
        self.lib.rats_broadcast_string.restype = c_int
        
        self.lib.rats_send_string.argtypes = [c_void_p, c_char_p, c_char_p]
        self.lib.rats_send_string.restype = c_int
        
        # Info functions
        self.lib.rats_get_peer_count.argtypes = [c_void_p]
        self.lib.rats_get_peer_count.restype = c_int
        
        self.lib.rats_get_our_peer_id.argtypes = [c_void_p]
        self.lib.rats_get_our_peer_id.restype = c_void_p
        
        self.lib.rats_get_connection_statistics_json.argtypes = [c_void_p]
        self.lib.rats_get_connection_statistics_json.restype = c_void_p
        
        # Peer configuration
        self.lib.rats_set_max_peers.argtypes = [c_void_p, c_int]
        self.lib.rats_set_max_peers.restype = c_int
        
        self.lib.rats_get_max_peers.argtypes = [c_void_p]
        self.lib.rats_get_max_peers.restype = c_int
        
        self.lib.rats_is_peer_limit_reached.argtypes = [c_void_p]
        self.lib.rats_is_peer_limit_reached.restype = c_int
        
        # Advanced connection methods
        self.lib.rats_connect_with_strategy.argtypes = [c_void_p, c_char_p, c_int, c_int]
        self.lib.rats_connect_with_strategy.restype = c_int
        
        self.lib.rats_disconnect_peer_by_id.argtypes = [c_void_p, c_char_p]
        self.lib.rats_disconnect_peer_by_id.restype = c_int
        
        # Binary data operations
        self.lib.rats_send_binary.argtypes = [c_void_p, c_char_p, c_void_p, c_size_t]
        self.lib.rats_send_binary.restype = c_int
        
        self.lib.rats_broadcast_binary.argtypes = [c_void_p, c_void_p, c_size_t]
        self.lib.rats_broadcast_binary.restype = c_int
        
        # JSON operations
        self.lib.rats_send_json.argtypes = [c_void_p, c_char_p, c_char_p]
        self.lib.rats_send_json.restype = c_int
        
        self.lib.rats_broadcast_json.argtypes = [c_void_p, c_char_p]
        self.lib.rats_broadcast_json.restype = c_int
        
        # DHT Discovery
        self.lib.rats_start_dht_discovery.argtypes = [c_void_p, c_int]
        self.lib.rats_start_dht_discovery.restype = c_int
        
        self.lib.rats_stop_dht_discovery.argtypes = [c_void_p]
        self.lib.rats_stop_dht_discovery.restype = None
        
        self.lib.rats_is_dht_running.argtypes = [c_void_p]
        self.lib.rats_is_dht_running.restype = c_int
        
        # Encryption
        self.lib.rats_set_encryption_enabled.argtypes = [c_void_p, c_int]
        self.lib.rats_set_encryption_enabled.restype = c_int
        
        self.lib.rats_is_encryption_enabled.argtypes = [c_void_p]
        self.lib.rats_is_encryption_enabled.restype = c_int
        
        self.lib.rats_get_encryption_key.argtypes = [c_void_p]
        self.lib.rats_get_encryption_key.restype = c_void_p
        
        self.lib.rats_set_encryption_key.argtypes = [c_void_p, c_char_p]
        self.lib.rats_set_encryption_key.restype = c_int
        
        self.lib.rats_generate_encryption_key.argtypes = [c_void_p]
        self.lib.rats_generate_encryption_key.restype = c_void_p
        
        # File Transfer
        self.lib.rats_send_file.argtypes = [c_void_p, c_char_p, c_char_p, c_char_p]
        self.lib.rats_send_file.restype = c_void_p
        
        self.lib.rats_accept_file_transfer.argtypes = [c_void_p, c_char_p, c_char_p]
        self.lib.rats_accept_file_transfer.restype = c_int
        
        self.lib.rats_reject_file_transfer.argtypes = [c_void_p, c_char_p, c_char_p]
        self.lib.rats_reject_file_transfer.restype = c_int
        
        self.lib.rats_cancel_file_transfer.argtypes = [c_void_p, c_char_p]
        self.lib.rats_cancel_file_transfer.restype = c_int
        
        # GossipSub
        self.lib.rats_is_gossipsub_available.argtypes = [c_void_p]
        self.lib.rats_is_gossipsub_available.restype = c_int
        
        self.lib.rats_subscribe_to_topic.argtypes = [c_void_p, c_char_p]
        self.lib.rats_subscribe_to_topic.restype = c_int
        
        self.lib.rats_unsubscribe_from_topic.argtypes = [c_void_p, c_char_p]
        self.lib.rats_unsubscribe_from_topic.restype = c_int
        
        self.lib.rats_publish_to_topic.argtypes = [c_void_p, c_char_p, c_char_p]
        self.lib.rats_publish_to_topic.restype = c_int
        
        # Callbacks
        self.lib.rats_set_connection_callback.argtypes = [c_void_p, ConnectionCallbackType, c_void_p]
        self.lib.rats_set_connection_callback.restype = None
        
        self.lib.rats_set_string_callback.argtypes = [c_void_p, StringCallbackType, c_void_p]
        self.lib.rats_set_string_callback.restype = None
        
        self.lib.rats_set_binary_callback.argtypes = [c_void_p, BinaryCallbackType, c_void_p]
        self.lib.rats_set_binary_callback.restype = None
        
        self.lib.rats_set_json_callback.argtypes = [c_void_p, JsonCallbackType, c_void_p]
        self.lib.rats_set_json_callback.restype = None
        
        self.lib.rats_set_disconnect_callback.argtypes = [c_void_p, DisconnectCallbackType, c_void_p]
        self.lib.rats_set_disconnect_callback.restype = None
        
        # Additional info functions  
        self.lib.rats_get_validated_peer_ids.argtypes = [c_void_p, POINTER(c_int)]
        self.lib.rats_get_validated_peer_ids.restype = POINTER(c_char_p)
        
        self.lib.rats_get_peer_ids.argtypes = [c_void_p, POINTER(c_int)]
        self.lib.rats_get_peer_ids.restype = POINTER(c_char_p)
        
        self.lib.rats_get_peer_info_json.argtypes = [c_void_p, c_char_p]
        self.lib.rats_get_peer_info_json.restype = c_void_p
        
        # Enhanced DHT Discovery
        self.lib.rats_announce_for_hash.argtypes = [c_void_p, c_char_p, c_int]
        self.lib.rats_announce_for_hash.restype = c_int
        
        self.lib.rats_get_dht_routing_table_size.argtypes = [c_void_p]
        self.lib.rats_get_dht_routing_table_size.restype = c_size_t
        
        # Automatic peer discovery
        self.lib.rats_start_automatic_peer_discovery.argtypes = [c_void_p]
        self.lib.rats_start_automatic_peer_discovery.restype = None
        
        self.lib.rats_stop_automatic_peer_discovery.argtypes = [c_void_p]
        self.lib.rats_stop_automatic_peer_discovery.restype = None
        
        self.lib.rats_is_automatic_discovery_running.argtypes = [c_void_p]
        self.lib.rats_is_automatic_discovery_running.restype = c_int
        
        self.lib.rats_get_discovery_hash.argtypes = [c_void_p]
        self.lib.rats_get_discovery_hash.restype = c_void_p
        
        self.lib.rats_get_rats_peer_discovery_hash.argtypes = []
        self.lib.rats_get_rats_peer_discovery_hash.restype = c_void_p
        
        # mDNS Discovery
        self.lib.rats_start_mdns_discovery.argtypes = [c_void_p, c_char_p]
        self.lib.rats_start_mdns_discovery.restype = c_int
        
        self.lib.rats_stop_mdns_discovery.argtypes = [c_void_p]
        self.lib.rats_stop_mdns_discovery.restype = None
        
        self.lib.rats_is_mdns_running.argtypes = [c_void_p]
        self.lib.rats_is_mdns_running.restype = c_int
        
        self.lib.rats_query_mdns_services.argtypes = [c_void_p]
        self.lib.rats_query_mdns_services.restype = c_int
        
        # Protocol configuration
        self.lib.rats_set_protocol_name.argtypes = [c_void_p, c_char_p]
        self.lib.rats_set_protocol_name.restype = c_int
        
        self.lib.rats_set_protocol_version.argtypes = [c_void_p, c_char_p]
        self.lib.rats_set_protocol_version.restype = c_int
        
        self.lib.rats_get_protocol_name.argtypes = [c_void_p]
        self.lib.rats_get_protocol_name.restype = c_void_p
        
        self.lib.rats_get_protocol_version.argtypes = [c_void_p]
        self.lib.rats_get_protocol_version.restype = c_void_p
        
        # Message Exchange API
        self.lib.rats_on_message.argtypes = [c_void_p, c_char_p, MessageCallbackType, c_void_p]
        self.lib.rats_on_message.restype = c_int
        
        self.lib.rats_send_message.argtypes = [c_void_p, c_char_p, c_char_p, c_char_p]
        self.lib.rats_send_message.restype = c_int
        
        self.lib.rats_broadcast_message.argtypes = [c_void_p, c_char_p, c_char_p]
        self.lib.rats_broadcast_message.restype = c_int
        
        # Advanced File Transfer
        self.lib.rats_send_directory.argtypes = [c_void_p, c_char_p, c_char_p, c_char_p, c_int]
        self.lib.rats_send_directory.restype = c_void_p
        
        self.lib.rats_request_file.argtypes = [c_void_p, c_char_p, c_char_p, c_char_p]
        self.lib.rats_request_file.restype = c_void_p
        
        self.lib.rats_request_directory.argtypes = [c_void_p, c_char_p, c_char_p, c_char_p, c_int]
        self.lib.rats_request_directory.restype = c_void_p
        
        self.lib.rats_accept_directory_transfer.argtypes = [c_void_p, c_char_p, c_char_p]
        self.lib.rats_accept_directory_transfer.restype = c_int
        
        self.lib.rats_reject_directory_transfer.argtypes = [c_void_p, c_char_p, c_char_p]
        self.lib.rats_reject_directory_transfer.restype = c_int
        
        self.lib.rats_pause_file_transfer.argtypes = [c_void_p, c_char_p]
        self.lib.rats_pause_file_transfer.restype = c_int
        
        self.lib.rats_resume_file_transfer.argtypes = [c_void_p, c_char_p]
        self.lib.rats_resume_file_transfer.restype = c_int
        
        self.lib.rats_get_file_transfer_progress_json.argtypes = [c_void_p, c_char_p]
        self.lib.rats_get_file_transfer_progress_json.restype = c_void_p
        
        self.lib.rats_get_file_transfer_statistics_json.argtypes = [c_void_p]
        self.lib.rats_get_file_transfer_statistics_json.restype = c_void_p
        
        # File transfer callbacks
        self.lib.rats_set_file_request_callback.argtypes = [c_void_p, FileRequestCallbackType, c_void_p]
        self.lib.rats_set_file_request_callback.restype = None
        
        self.lib.rats_set_directory_request_callback.argtypes = [c_void_p, DirectoryRequestCallbackType, c_void_p]
        self.lib.rats_set_directory_request_callback.restype = None
        
        self.lib.rats_set_file_progress_callback.argtypes = [c_void_p, FileProgressCallbackType, c_void_p]
        self.lib.rats_set_file_progress_callback.restype = None
        
        self.lib.rats_set_directory_progress_callback.argtypes = [c_void_p, DirectoryProgressCallbackType, c_void_p]
        self.lib.rats_set_directory_progress_callback.restype = None
        
        # Enhanced GossipSub
        self.lib.rats_is_gossipsub_running.argtypes = [c_void_p]
        self.lib.rats_is_gossipsub_running.restype = c_int
        
        self.lib.rats_is_subscribed_to_topic.argtypes = [c_void_p, c_char_p]
        self.lib.rats_is_subscribed_to_topic.restype = c_int
        
        self.lib.rats_get_subscribed_topics.argtypes = [c_void_p, POINTER(c_int)]
        self.lib.rats_get_subscribed_topics.restype = POINTER(c_char_p)
        
        self.lib.rats_publish_json_to_topic.argtypes = [c_void_p, c_char_p, c_char_p]
        self.lib.rats_publish_json_to_topic.restype = c_int
        
        self.lib.rats_get_topic_peers.argtypes = [c_void_p, c_char_p, POINTER(c_int)]
        self.lib.rats_get_topic_peers.restype = POINTER(c_char_p)
        
        self.lib.rats_get_topic_mesh_peers.argtypes = [c_void_p, c_char_p, POINTER(c_int)]
        self.lib.rats_get_topic_mesh_peers.restype = POINTER(c_char_p)
        
        self.lib.rats_get_gossipsub_statistics_json.argtypes = [c_void_p]
        self.lib.rats_get_gossipsub_statistics_json.restype = c_void_p
        
        # GossipSub topic callbacks
        self.lib.rats_set_topic_message_callback.argtypes = [c_void_p, c_char_p, TopicMessageCallbackType, c_void_p]
        self.lib.rats_set_topic_message_callback.restype = None
        
        self.lib.rats_set_topic_json_message_callback.argtypes = [c_void_p, c_char_p, TopicJsonMessageCallbackType, c_void_p]
        self.lib.rats_set_topic_json_message_callback.restype = None
        
        self.lib.rats_set_topic_peer_joined_callback.argtypes = [c_void_p, c_char_p, TopicPeerJoinedCallbackType, c_void_p]
        self.lib.rats_set_topic_peer_joined_callback.restype = None
        
        self.lib.rats_set_topic_peer_left_callback.argtypes = [c_void_p, c_char_p, TopicPeerLeftCallbackType, c_void_p]
        self.lib.rats_set_topic_peer_left_callback.restype = None
        
        self.lib.rats_clear_topic_callbacks.argtypes = [c_void_p, c_char_p]
        self.lib.rats_clear_topic_callbacks.restype = None
        
        # Peer discovery callback
        self.lib.rats_set_peer_discovered_callback.argtypes = [c_void_p, PeerDiscoveredCallbackType, c_void_p]
        self.lib.rats_set_peer_discovered_callback.restype = None
        
        # NAT Traversal and STUN
        self.lib.rats_discover_and_ignore_public_ip.argtypes = [c_void_p, c_char_p, c_int]
        self.lib.rats_discover_and_ignore_public_ip.restype = c_int
        
        self.lib.rats_get_public_ip.argtypes = [c_void_p]
        self.lib.rats_get_public_ip.restype = c_void_p
        
        self.lib.rats_detect_nat_type.argtypes = [c_void_p]
        self.lib.rats_detect_nat_type.restype = c_int
        
        self.lib.rats_get_nat_characteristics_json.argtypes = [c_void_p]
        self.lib.rats_get_nat_characteristics_json.restype = c_void_p
        
        self.lib.rats_add_ignored_address.argtypes = [c_void_p, c_char_p]
        self.lib.rats_add_ignored_address.restype = None
        
        self.lib.rats_get_nat_traversal_statistics_json.argtypes = [c_void_p]
        self.lib.rats_get_nat_traversal_statistics_json.restype = c_void_p
        
        # ICE coordination
        self.lib.rats_create_ice_offer.argtypes = [c_void_p, c_char_p]
        self.lib.rats_create_ice_offer.restype = c_void_p
        
        self.lib.rats_connect_with_ice.argtypes = [c_void_p, c_char_p, c_char_p]
        self.lib.rats_connect_with_ice.restype = c_int
        
        self.lib.rats_handle_ice_answer.argtypes = [c_void_p, c_char_p, c_char_p]
        self.lib.rats_handle_ice_answer.restype = c_int
        
        # Configuration persistence
        self.lib.rats_load_configuration.argtypes = [c_void_p]
        self.lib.rats_load_configuration.restype = c_int
        
        self.lib.rats_save_configuration.argtypes = [c_void_p]
        self.lib.rats_save_configuration.restype = c_int
        
        self.lib.rats_set_data_directory.argtypes = [c_void_p, c_char_p]
        self.lib.rats_set_data_directory.restype = c_int
        
        self.lib.rats_get_data_directory.argtypes = [c_void_p]
        self.lib.rats_get_data_directory.restype = c_void_p
        
        self.lib.rats_load_and_reconnect_peers.argtypes = [c_void_p]
        self.lib.rats_load_and_reconnect_peers.restype = c_int
        
        self.lib.rats_load_historical_peers.argtypes = [c_void_p]
        self.lib.rats_load_historical_peers.restype = c_int
        
        self.lib.rats_save_historical_peers.argtypes = [c_void_p]
        self.lib.rats_save_historical_peers.restype = c_int
        
        self.lib.rats_clear_historical_peers.argtypes = [c_void_p]
        self.lib.rats_clear_historical_peers.restype = None
        
        self.lib.rats_get_historical_peer_ids.argtypes = [c_void_p, POINTER(c_int)]
        self.lib.rats_get_historical_peer_ids.restype = POINTER(c_char_p)

        # Logging functions
        self.lib.rats_set_logging_enabled.argtypes = [c_int]
        self.lib.rats_set_logging_enabled.restype = None
        
        self.lib.rats_set_log_level.argtypes = [c_char_p]
        self.lib.rats_set_log_level.restype = None
        
        self.lib.rats_set_log_file_path.argtypes = [c_void_p, c_char_p]
        self.lib.rats_set_log_file_path.restype = None
        
        self.lib.rats_get_log_file_path.argtypes = [c_void_p]
        self.lib.rats_get_log_file_path.restype = c_void_p
        
        self.lib.rats_set_log_colors_enabled.argtypes = [c_void_p, c_int]
        self.lib.rats_set_log_colors_enabled.restype = None
        
        self.lib.rats_is_log_colors_enabled.argtypes = [c_void_p]
        self.lib.rats_is_log_colors_enabled.restype = c_int
        
        self.lib.rats_set_log_timestamps_enabled.argtypes = [c_void_p, c_int]
        self.lib.rats_set_log_timestamps_enabled.restype = None
        
        self.lib.rats_is_log_timestamps_enabled.argtypes = [c_void_p]
        self.lib.rats_is_log_timestamps_enabled.restype = c_int
        
        self.lib.rats_set_log_rotation_size.argtypes = [c_void_p, c_size_t]
        self.lib.rats_set_log_rotation_size.restype = None
        
        self.lib.rats_set_log_retention_count.argtypes = [c_void_p, c_int]
        self.lib.rats_set_log_retention_count.restype = None
        
        self.lib.rats_clear_log_file.argtypes = [c_void_p]
        self.lib.rats_clear_log_file.restype = None


# Global instance
_librats = None

def get_librats() -> LibratsCtypes:
    """Get the global librats ctypes instance."""
    global _librats
    if _librats is None:
        _librats = LibratsCtypes()
    return _librats
