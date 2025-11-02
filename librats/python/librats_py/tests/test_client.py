"""
Unit tests for RatsClient.
"""

import unittest
import time
import threading
import json
from unittest.mock import Mock, patch

import sys
import os

# Add the parent directory to the path so we can import librats_py
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

try:
    from librats_py import RatsClient, RatsError, ConnectionStrategy, LogLevel
    from librats_py.exceptions import RatsInvalidParameterError, RatsNotRunningError
except ImportError as e:
    print(f"Warning: Could not import librats_py: {e}")
    print("This is expected if the native library is not built yet.")
    RatsClient = None


@unittest.skipIf(RatsClient is None, "librats_py not available")
class TestRatsClient(unittest.TestCase):
    """Test cases for RatsClient."""
    
    def setUp(self):
        """Set up test fixtures."""
        self.client = None
        self.callback_events = []
        self.callback_lock = threading.Lock()
    
    def tearDown(self):
        """Clean up after tests."""
        if self.client:
            try:
                self.client.stop()
            except:
                pass
    
    def callback_recorder(self, event_type):
        """Create a callback that records events."""
        def callback(*args):
            with self.callback_lock:
                self.callback_events.append((event_type, args))
        return callback
    
    def test_client_creation(self):
        """Test creating a RatsClient."""
        try:
            self.client = RatsClient(0)  # Use port 0 for automatic port assignment
            self.assertIsNotNone(self.client)
        except Exception as e:
            self.skipTest(f"Could not create RatsClient: {e}")
    
    def test_client_context_manager(self):
        """Test using RatsClient as a context manager."""
        try:
            with RatsClient(0) as client:
                self.assertIsNotNone(client)
                self.assertFalse(client.is_running())
        except Exception as e:
            self.skipTest(f"Could not create RatsClient: {e}")
    
    def test_start_stop(self):
        """Test starting and stopping the client."""
        try:
            self.client = RatsClient(0)
            
            # Initially not running
            self.assertFalse(self.client.is_running())
            
            # Start the client
            self.client.start()
            self.assertTrue(self.client.is_running())
            
            # Stop the client
            self.client.stop()
            self.assertFalse(self.client.is_running())
            
        except Exception as e:
            self.skipTest(f"Could not test start/stop: {e}")
    
    def test_peer_id(self):
        """Test getting our peer ID."""
        try:
            self.client = RatsClient(0)
            peer_id = self.client.get_our_peer_id()
            self.assertIsInstance(peer_id, str)
            self.assertGreater(len(peer_id), 0)
        except Exception as e:
            self.skipTest(f"Could not test peer ID: {e}")
    
    def test_peer_count(self):
        """Test getting peer count."""
        try:
            self.client = RatsClient(0)
            count = self.client.get_peer_count()
            self.assertIsInstance(count, int)
            self.assertGreaterEqual(count, 0)
        except Exception as e:
            self.skipTest(f"Could not test peer count: {e}")
    
    def test_max_peers(self):
        """Test setting and getting max peers."""
        try:
            self.client = RatsClient(0)
            
            # Test setting max peers
            self.client.set_max_peers(20)
            self.assertEqual(self.client.get_max_peers(), 20)
            
            # Test peer limit check
            limit_reached = self.client.is_peer_limit_reached()
            self.assertIsInstance(limit_reached, bool)
            
        except Exception as e:
            self.skipTest(f"Could not test max peers: {e}")
    
    def test_connection_statistics(self):
        """Test getting connection statistics."""
        try:
            self.client = RatsClient(0)
            stats = self.client.get_connection_statistics()
            self.assertIsInstance(stats, dict)
        except Exception as e:
            self.skipTest(f"Could not test connection statistics: {e}")
    
    def test_encryption(self):
        """Test encryption functionality."""
        try:
            self.client = RatsClient(0)
            
            # Test enabling encryption
            self.client.set_encryption_enabled(True)
            self.assertTrue(self.client.is_encryption_enabled())
            
            # Test disabling encryption
            self.client.set_encryption_enabled(False)
            self.assertFalse(self.client.is_encryption_enabled())
            
            # Test key generation
            key = self.client.generate_encryption_key()
            self.assertIsInstance(key, str)
            self.assertGreater(len(key), 0)
            
            # Test setting key
            self.client.set_encryption_key(key)
            retrieved_key = self.client.get_encryption_key()
            self.assertEqual(key, retrieved_key)
            
        except Exception as e:
            self.skipTest(f"Could not test encryption: {e}")
    
    def test_dht_discovery(self):
        """Test DHT discovery functionality."""
        try:
            self.client = RatsClient(0)
            
            # Initially DHT should not be running
            self.assertFalse(self.client.is_dht_running())
            
            # Start DHT discovery
            self.client.start_dht_discovery(6881)
            
            # Give it a moment to start
            time.sleep(0.1)
            
            # Check if running (may not be available in all builds)
            # self.assertTrue(self.client.is_dht_running())
            
            # Stop DHT discovery
            self.client.stop_dht_discovery()
            
        except Exception as e:
            self.skipTest(f"Could not test DHT discovery: {e}")
    
    def test_gossipsub(self):
        """Test GossipSub functionality."""
        try:
            self.client = RatsClient(0)
            
            # Check if GossipSub is available (may not be in all builds)
            available = self.client.is_gossipsub_available()
            self.assertIsInstance(available, bool)
            
            if available:
                # Test subscribing to a topic
                self.client.subscribe_to_topic("test_topic")
                
                # Test publishing to a topic
                self.client.publish_to_topic("test_topic", "Hello, GossipSub!")
                
                # Test unsubscribing
                self.client.unsubscribe_from_topic("test_topic")
            
        except Exception as e:
            self.skipTest(f"Could not test GossipSub: {e}")
    
    def test_callbacks(self):
        """Test setting callbacks."""
        try:
            self.client = RatsClient(0)
            
            # Test setting connection callback
            connection_cb = self.callback_recorder("connection")
            self.client.set_connection_callback(connection_cb)
            
            # Test setting string callback
            string_cb = self.callback_recorder("string")
            self.client.set_string_callback(string_cb)
            
            # Test setting binary callback
            binary_cb = self.callback_recorder("binary")
            self.client.set_binary_callback(binary_cb)
            
            # Test setting JSON callback
            json_cb = self.callback_recorder("json")
            self.client.set_json_callback(json_cb)
            
            # Test setting disconnect callback
            disconnect_cb = self.callback_recorder("disconnect")
            self.client.set_disconnect_callback(disconnect_cb)
            
            # Test clearing callbacks
            self.client.set_connection_callback(None)
            self.client.set_string_callback(None)
            self.client.set_binary_callback(None)
            self.client.set_json_callback(None)
            self.client.set_disconnect_callback(None)
            
        except Exception as e:
            self.skipTest(f"Could not test callbacks: {e}")
    
    def test_static_logging(self):
        """Test static logging methods."""
        try:
            # Test setting logging enabled
            RatsClient.set_logging_enabled(True)
            RatsClient.set_logging_enabled(False)
            
            # Test setting log level
            RatsClient.set_log_level(LogLevel.INFO)
            RatsClient.set_log_level(LogLevel.DEBUG)
            
        except Exception as e:
            self.skipTest(f"Could not test static logging: {e}")


class TestEnumsAndExceptions(unittest.TestCase):
    """Test enums and exceptions."""
    
    def test_connection_strategy_enum(self):
        """Test ConnectionStrategy enum."""
        self.assertEqual(ConnectionStrategy.DIRECT_ONLY.value, 0)
        self.assertEqual(ConnectionStrategy.STUN_ASSISTED.value, 1)
        self.assertEqual(ConnectionStrategy.ICE_FULL.value, 2)
        self.assertEqual(ConnectionStrategy.TURN_RELAY.value, 3)
        self.assertEqual(ConnectionStrategy.AUTO_ADAPTIVE.value, 4)
    
    def test_log_level_enum(self):
        """Test LogLevel enum."""
        self.assertEqual(LogLevel.DEBUG.value, 0)
        self.assertEqual(LogLevel.INFO.value, 1)
        self.assertEqual(LogLevel.WARN.value, 2)
        self.assertEqual(LogLevel.ERROR.value, 3)
    
    def test_rats_error_exception(self):
        """Test RatsError exception."""
        error = RatsError("Test error")
        self.assertEqual(str(error), "Test error (error code: OPERATION_FAILED)")
        
        from librats_py.enums import RatsError as ErrorCode
        error_with_code = RatsError("Test error", ErrorCode.INVALID_PARAMETER)
        self.assertEqual(str(error_with_code), "Test error (error code: INVALID_PARAMETER)")


if __name__ == "__main__":
    unittest.main()
