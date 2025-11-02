"""
Enumerations and constants for librats Python bindings.
"""

from enum import IntEnum


class RatsError(IntEnum):
    """Error codes returned by librats functions."""
    SUCCESS = 0
    INVALID_HANDLE = -1
    INVALID_PARAMETER = -2
    NOT_RUNNING = -3
    OPERATION_FAILED = -4
    PEER_NOT_FOUND = -5
    MEMORY_ALLOCATION = -6
    JSON_PARSE = -7


class ConnectionStrategy(IntEnum):
    """Connection establishment strategies."""
    DIRECT_ONLY = 0
    STUN_ASSISTED = 1
    ICE_FULL = 2
    TURN_RELAY = 3
    AUTO_ADAPTIVE = 4


class MessageDataType(IntEnum):
    """Message data types for librats messages."""
    BINARY = 1
    STRING = 2
    JSON = 3


class FileTransferStatus(IntEnum):
    """File transfer status values."""
    PENDING = 0
    STARTING = 1
    IN_PROGRESS = 2
    PAUSED = 3
    COMPLETED = 4
    FAILED = 5
    CANCELLED = 6
    RESUMING = 7


class LogLevel(IntEnum):
    """Logging levels."""
    DEBUG = 0
    INFO = 1
    WARN = 2
    ERROR = 3


class NatType(IntEnum):
    """NAT type detection results."""
    UNKNOWN = 0
    OPEN_INTERNET = 1
    FULL_CONE = 2
    RESTRICTED_CONE = 3
    PORT_RESTRICTED_CONE = 4
    SYMMETRIC = 5
    UDP_BLOCKED = 6


# Version information helper
class VersionInfo:
    """Version information container."""
    def __init__(self, major: int, minor: int, patch: int, build: int):
        self.major = major
        self.minor = minor
        self.patch = patch
        self.build = build
    
    def __str__(self) -> str:
        return f"{self.major}.{self.minor}.{self.patch}.{self.build}"
    
    def __repr__(self) -> str:
        return f"VersionInfo(major={self.major}, minor={self.minor}, patch={self.patch}, build={self.build})"