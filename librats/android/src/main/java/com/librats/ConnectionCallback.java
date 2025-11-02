package com.librats;

/**
 * Callback interface for peer connection events.
 */
public interface ConnectionCallback {
    /**
     * Called when a new peer connects to this client.
     * 
     * @param peerId The ID of the connected peer
     */
    void onConnection(String peerId);
}
