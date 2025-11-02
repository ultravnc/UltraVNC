package com.librats;

/**
 * Callback interface for directory transfer progress events.
 */
public interface DirectoryProgressCallback {
    /**
     * Called when directory transfer progress is updated.
     * 
     * @param transferId The unique transfer identifier
     * @param filesCompleted Number of files completed
     * @param totalFiles Total number of files
     * @param currentFile Currently transferring file
     */
    void onDirectoryProgress(String transferId, int filesCompleted, int totalFiles, String currentFile);
}
