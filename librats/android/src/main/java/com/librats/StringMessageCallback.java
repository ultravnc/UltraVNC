package com.librats;

/**
 * Callback interface for string message events.
 */
public interface StringMessageCallback {
    /**
     * Called when a string message is received from a peer.
     * 
     * @param peerId The ID of the peer who sent the message
     * @param message The received message
     */
    void onStringMessage(String peerId, String message);
}
