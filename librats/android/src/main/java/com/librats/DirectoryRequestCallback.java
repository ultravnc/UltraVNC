package com.librats;

/**
 * Callback interface for directory request events.
 */
public interface DirectoryRequestCallback {
    /**
     * Called when a peer requests a directory from this client.
     * 
     * @param peerId The ID of the requesting peer
     * @param transferId The unique transfer identifier
     * @param remotePath The requested directory path
     * @param directoryName The requested directory name
     */
    void onDirectoryRequest(String peerId, String transferId, String remotePath, String directoryName);
}
