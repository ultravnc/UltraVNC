"""
Exception classes for librats Python bindings.
"""

from .enums import RatsError as ErrorCode


class RatsError(Exception):
    """Base exception class for librats errors."""
    
    def __init__(self, message: str, error_code: ErrorCode = ErrorCode.OPERATION_FAILED):
        super().__init__(message)
        self.error_code = error_code
        self.message = message
    
    def __str__(self):
        return f"{self.message} (error code: {self.error_code.name})"


class RatsConnectionError(RatsError):
    """Exception raised for connection-related errors."""
    
    def __init__(self, message: str = "Connection failed"):
        super().__init__(message, ErrorCode.OPERATION_FAILED)


class RatsInvalidParameterError(RatsError):
    """Exception raised for invalid parameter errors."""
    
    def __init__(self, message: str = "Invalid parameter"):
        super().__init__(message, ErrorCode.INVALID_PARAMETER)


class RatsNotRunningError(RatsError):
    """Exception raised when operations are attempted on a stopped client."""
    
    def __init__(self, message: str = "Client is not running"):
        super().__init__(message, ErrorCode.NOT_RUNNING)


class RatsPeerNotFoundError(RatsError):
    """Exception raised when a peer cannot be found."""
    
    def __init__(self, message: str = "Peer not found"):
        super().__init__(message, ErrorCode.PEER_NOT_FOUND)


class RatsJsonParseError(RatsError):
    """Exception raised for JSON parsing errors."""
    
    def __init__(self, message: str = "JSON parse error"):
        super().__init__(message, ErrorCode.JSON_PARSE)


def check_error(error_code: int, operation: str = "Operation"):
    """Check error code and raise appropriate exception if needed."""
    if error_code == ErrorCode.SUCCESS:
        return
    
    error_enum = ErrorCode(error_code)
    message = f"{operation} failed"
    
    if error_enum == ErrorCode.INVALID_PARAMETER:
        raise RatsInvalidParameterError(message)
    elif error_enum == ErrorCode.NOT_RUNNING:
        raise RatsNotRunningError(message)
    elif error_enum == ErrorCode.PEER_NOT_FOUND:
        raise RatsPeerNotFoundError(message)
    elif error_enum == ErrorCode.JSON_PARSE:
        raise RatsJsonParseError(message)
    else:
        raise RatsError(message, error_enum)
