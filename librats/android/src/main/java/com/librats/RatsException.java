package com.librats;

/**
 * Exception class for LibRats-specific errors.
 */
public class RatsException extends Exception {
    private final int errorCode;
    
    public RatsException(String message) {
        super(message);
        this.errorCode = RatsClient.ERROR_OPERATION_FAILED;
    }
    
    public RatsException(String message, int errorCode) {
        super(message);
        this.errorCode = errorCode;
    }
    
    public RatsException(String message, Throwable cause) {
        super(message, cause);
        this.errorCode = RatsClient.ERROR_OPERATION_FAILED;
    }
    
    public RatsException(String message, int errorCode, Throwable cause) {
        super(message, cause);
        this.errorCode = errorCode;
    }
    
    public int getErrorCode() {
        return errorCode;
    }
    
    public static String getErrorMessage(int errorCode) {
        switch (errorCode) {
            case RatsClient.SUCCESS:
                return "Success";
            case RatsClient.ERROR_INVALID_HANDLE:
                return "Invalid handle";
            case RatsClient.ERROR_INVALID_PARAMETER:
                return "Invalid parameter";
            case RatsClient.ERROR_NOT_RUNNING:
                return "Client not running";
            case RatsClient.ERROR_OPERATION_FAILED:
                return "Operation failed";
            case RatsClient.ERROR_PEER_NOT_FOUND:
                return "Peer not found";
            case RatsClient.ERROR_MEMORY_ALLOCATION:
                return "Memory allocation failed";
            case RatsClient.ERROR_JSON_PARSE:
                return "JSON parse error";
            default:
                return "Unknown error";
        }
    }
}
