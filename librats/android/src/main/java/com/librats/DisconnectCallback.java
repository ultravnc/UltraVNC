package com.librats;

/**
 * Callback interface for peer disconnection events.
 */
public interface DisconnectCallback {
    /**
     * Called when a peer disconnects from this client.
     * 
     * @param peerId The ID of the disconnected peer
     */
    void onDisconnect(String peerId);
}
