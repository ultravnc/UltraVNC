package com.librats;

/**
 * Callback interface for binary message events.
 */
public interface BinaryMessageCallback {
    /**
     * Called when binary data is received from a peer.
     * 
     * @param peerId The ID of the peer who sent the data
     * @param data The received binary data
     */
    void onBinaryMessage(String peerId, byte[] data);
}
