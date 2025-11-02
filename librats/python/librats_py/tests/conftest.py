"""
Pytest configuration for librats tests.
"""

import pytest
import sys
import os

# Add the parent directory to the path so we can import librats_py
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

try:
    from librats_py import RatsClient
    LIBRATS_AVAILABLE = True
except ImportError:
    LIBRATS_AVAILABLE = False


def pytest_configure(config):
    """Configure pytest."""
    config.addinivalue_line(
        "markers", "integration: mark test as an integration test"
    )
    config.addinivalue_line(
        "markers", "slow: mark test as slow running"
    )


def pytest_collection_modifyitems(config, items):
    """Modify test collection."""
    if not LIBRATS_AVAILABLE:
        skip_native = pytest.mark.skip(reason="librats native library not available")
        for item in items:
            if "test_client" in item.nodeid or "test_integration" in item.nodeid:
                item.add_marker(skip_native)


@pytest.fixture
def rats_client():
    """Fixture providing a RatsClient instance."""
    if not LIBRATS_AVAILABLE:
        pytest.skip("librats native library not available")
    
    client = RatsClient(0)
    yield client
    try:
        client.stop()
    except:
        pass


@pytest.fixture
def started_rats_client():
    """Fixture providing a started RatsClient instance."""
    if not LIBRATS_AVAILABLE:
        pytest.skip("librats native library not available")
    
    client = RatsClient(0)
    try:
        client.start()
        yield client
    finally:
        try:
            client.stop()
        except:
            pass
