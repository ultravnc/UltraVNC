"""
Callback type definitions for librats Python bindings.
"""

from typing import Callable, Optional, Any
from ctypes import CFUNCTYPE, c_void_p, c_char_p, c_int, c_size_t

# C callback function types
ConnectionCallbackType = CFUNCTYPE(None, c_void_p, c_char_p)
StringCallbackType = CFUNCTYPE(None, c_void_p, c_char_p, c_char_p)
BinaryCallbackType = CFUNCTYPE(None, c_void_p, c_char_p, c_void_p, c_size_t)
JsonCallbackType = CFUNCTYPE(None, c_void_p, c_char_p, c_char_p)
DisconnectCallbackType = CFUNCTYPE(None, c_void_p, c_char_p)
PeerDiscoveredCallbackType = CFUNCTYPE(None, c_void_p, c_char_p, c_int, c_char_p)
MessageCallbackType = CFUNCTYPE(None, c_void_p, c_char_p, c_char_p)
FileProgressCallbackType = CFUNCTYPE(None, c_void_p, c_char_p, c_int, c_char_p)
FileRequestCallbackType = CFUNCTYPE(None, c_void_p, c_char_p, c_char_p, c_char_p, c_char_p)
DirectoryRequestCallbackType = CFUNCTYPE(None, c_void_p, c_char_p, c_char_p, c_char_p, c_char_p)
DirectoryProgressCallbackType = CFUNCTYPE(None, c_void_p, c_char_p, c_int, c_int, c_char_p)
TopicMessageCallbackType = CFUNCTYPE(None, c_void_p, c_char_p, c_char_p, c_char_p)
TopicJsonMessageCallbackType = CFUNCTYPE(None, c_void_p, c_char_p, c_char_p, c_char_p)
TopicPeerJoinedCallbackType = CFUNCTYPE(None, c_void_p, c_char_p, c_char_p)
TopicPeerLeftCallbackType = CFUNCTYPE(None, c_void_p, c_char_p, c_char_p)

# Python callback type hints
ConnectionCallback = Optional[Callable[[str], None]]
StringCallback = Optional[Callable[[str, str], None]]
BinaryCallback = Optional[Callable[[str, bytes], None]]
JsonCallback = Optional[Callable[[str, dict], None]]
DisconnectCallback = Optional[Callable[[str], None]]
PeerDiscoveredCallback = Optional[Callable[[str, int, str], None]]
MessageCallback = Optional[Callable[[str, Any], None]]
FileProgressCallback = Optional[Callable[[str, int, str], None]]
FileRequestCallback = Optional[Callable[[str, str, str, str], None]]
DirectoryRequestCallback = Optional[Callable[[str, str, str, str], None]]
DirectoryProgressCallback = Optional[Callable[[str, int, int, str], None]]
TopicMessageCallback = Optional[Callable[[str, str, str], None]]
TopicJsonMessageCallback = Optional[Callable[[str, str, dict], None]]
TopicPeerJoinedCallback = Optional[Callable[[str, str], None]]
TopicPeerLeftCallback = Optional[Callable[[str, str], None]]
