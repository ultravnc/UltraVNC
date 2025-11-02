package com.librats;

/**
 * Callback interface for file request events.
 */
public interface FileRequestCallback {
    /**
     * Called when a peer requests a file from this client.
     * 
     * @param peerId The ID of the requesting peer
     * @param transferId The unique transfer identifier
     * @param remotePath The requested file path
     * @param filename The requested filename
     */
    void onFileRequest(String peerId, String transferId, String remotePath, String filename);
}
