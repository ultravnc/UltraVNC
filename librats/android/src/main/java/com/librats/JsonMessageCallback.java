package com.librats;

/**
 * Callback interface for JSON message events.
 */
public interface JsonMessageCallback {
    /**
     * Called when a JSON message is received from a peer.
     * 
     * @param peerId The ID of the peer who sent the message
     * @param jsonStr The received JSON string
     */
    void onJsonMessage(String peerId, String jsonStr);
}
