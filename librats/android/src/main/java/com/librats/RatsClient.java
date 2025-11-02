package com.librats;

import android.util.Log;

/**
 * LibRats Android client wrapper providing peer-to-peer networking capabilities.
 * 
 * This class provides a Java interface to the native LibRats C library,
 * enabling Android applications to participate in peer-to-peer networks
 * with features like direct connections, NAT traversal, encryption, 
 * file transfer, and service discovery.
 */
public class RatsClient {
    private static final String TAG = "RatsClient";
    
    static {
        try {
            System.loadLibrary("rats_jni");
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Failed to load native library", e);
            throw e;
        }
    }
    
    private long nativeClientPtr = 0;
    
    // Connection strategies
    public static final int STRATEGY_DIRECT_ONLY = 0;
    public static final int STRATEGY_STUN_ASSISTED = 1;
    public static final int STRATEGY_ICE_FULL = 2;
    public static final int STRATEGY_TURN_RELAY = 3;
    public static final int STRATEGY_AUTO_ADAPTIVE = 4;
    
    // Error codes
    public static final int SUCCESS = 0;
    public static final int ERROR_INVALID_HANDLE = -1;
    public static final int ERROR_INVALID_PARAMETER = -2;
    public static final int ERROR_NOT_RUNNING = -3;
    public static final int ERROR_OPERATION_FAILED = -4;
    public static final int ERROR_PEER_NOT_FOUND = -5;
    public static final int ERROR_MEMORY_ALLOCATION = -6;
    public static final int ERROR_JSON_PARSE = -7;
    
    /**
     * Creates a new RatsClient instance.
     * 
     * @param listenPort The port to listen on for incoming connections (0 for automatic)
     */
    public RatsClient(int listenPort) {
        nativeClientPtr = nativeCreate(listenPort);
        if (nativeClientPtr == 0) {
            throw new RuntimeException("Failed to create native RatsClient");
        }
    }
    
    /**
     * Starts the client and begins listening for connections.
     * 
     * @return SUCCESS on success, error code on failure
     */
    public int start() {
        return nativeStart(nativeClientPtr);
    }
    
    /**
     * Stops the client and closes all connections.
     */
    public void stop() {
        if (nativeClientPtr != 0) {
            nativeStop(nativeClientPtr);
        }
    }
    
    /**
     * Destroys the client and releases all resources.
     * This should be called when the client is no longer needed.
     */
    public void destroy() {
        if (nativeClientPtr != 0) {
            nativeDestroy(nativeClientPtr);
            nativeClientPtr = 0;
        }
    }
    
    @Override
    protected void finalize() throws Throwable {
        destroy();
        super.finalize();
    }
    
    /**
     * Connects to a peer using the default strategy.
     * 
     * @param host The hostname or IP address of the peer
     * @param port The port number of the peer
     * @return 1 on success, 0 on failure
     */
    public int connect(String host, int port) {
        return nativeConnect(nativeClientPtr, host, port);
    }
    
    /**
     * Connects to a peer using a specific connection strategy.
     * 
     * @param host The hostname or IP address of the peer
     * @param port The port number of the peer
     * @param strategy The connection strategy to use (STRATEGY_*)
     * @return SUCCESS on success, error code on failure
     */
    public int connectWithStrategy(String host, int port, int strategy) {
        return nativeConnectWithStrategy(nativeClientPtr, host, port, strategy);
    }
    
    /**
     * Sends a string message to a specific peer.
     * 
     * @param peerId The ID of the target peer
     * @param message The message to send
     * @return SUCCESS on success, error code on failure
     */
    public int sendString(String peerId, String message) {
        return nativeSendString(nativeClientPtr, peerId, message);
    }
    
    /**
     * Broadcasts a string message to all connected peers.
     * 
     * @param message The message to broadcast
     * @return Number of peers the message was sent to
     */
    public int broadcastString(String message) {
        return nativeBroadcastString(nativeClientPtr, message);
    }
    
    /**
     * Sends binary data to a specific peer.
     * 
     * @param peerId The ID of the target peer
     * @param data The binary data to send
     * @return SUCCESS on success, error code on failure
     */
    public int sendBinary(String peerId, byte[] data) {
        return nativeSendBinary(nativeClientPtr, peerId, data);
    }
    
    /**
     * Broadcasts binary data to all connected peers.
     * 
     * @param data The binary data to broadcast
     * @return Number of peers the data was sent to
     */
    public int broadcastBinary(byte[] data) {
        return nativeBroadcastBinary(nativeClientPtr, data);
    }
    
    /**
     * Sends a JSON message to a specific peer.
     * 
     * @param peerId The ID of the target peer
     * @param jsonStr The JSON string to send
     * @return SUCCESS on success, error code on failure
     */
    public int sendJson(String peerId, String jsonStr) {
        return nativeSendJson(nativeClientPtr, peerId, jsonStr);
    }
    
    /**
     * Broadcasts a JSON message to all connected peers.
     * 
     * @param jsonStr The JSON string to broadcast
     * @return Number of peers the message was sent to
     */
    public int broadcastJson(String jsonStr) {
        return nativeBroadcastJson(nativeClientPtr, jsonStr);
    }
    
    /**
     * Gets the number of currently connected peers.
     * 
     * @return The number of connected peers
     */
    public int getPeerCount() {
        return nativeGetPeerCount(nativeClientPtr);
    }
    
    /**
     * Gets the port this client is listening on.
     * 
     * @return The listen port number
     */
    public int getListenPort() {
        return nativeGetListenPort(nativeClientPtr);
    }
    
    /**
     * Gets this client's peer ID.
     * 
     * @return The peer ID string
     */
    public String getOurPeerId() {
        return nativeGetOurPeerId(nativeClientPtr);
    }
    
    /**
     * Gets connection statistics as a JSON string.
     * 
     * @return JSON string containing connection statistics
     */
    public String getConnectionStatisticsJson() {
        return nativeGetConnectionStatisticsJson(nativeClientPtr);
    }
    
    /**
     * Gets the IDs of all connected peers.
     * 
     * @return Array of peer ID strings
     */
    public String[] getPeerIds() {
        return nativeGetPeerIds(nativeClientPtr);
    }
    
    /**
     * Enables or disables encryption for this client.
     * 
     * @param enabled true to enable encryption, false to disable
     * @return SUCCESS on success, error code on failure
     */
    public int setEncryptionEnabled(boolean enabled) {
        return nativeSetEncryptionEnabled(nativeClientPtr, enabled);
    }
    
    /**
     * Checks if encryption is enabled for this client.
     * 
     * @return true if encryption is enabled, false otherwise
     */
    public boolean isEncryptionEnabled() {
        return nativeIsEncryptionEnabled(nativeClientPtr);
    }
    
    /**
     * Generates a new encryption key for this client.
     * 
     * @return The generated encryption key as a hex string
     */
    public String generateEncryptionKey() {
        return nativeGenerateEncryptionKey(nativeClientPtr);
    }
    
    /**
     * Sets the encryption key for this client.
     * 
     * @param key The encryption key as a hex string
     * @return SUCCESS on success, error code on failure
     */
    public int setEncryptionKey(String key) {
        return nativeSetEncryptionKey(nativeClientPtr, key);
    }
    
    /**
     * Starts DHT discovery on the specified port.
     * 
     * @param dhtPort The port to use for DHT discovery
     * @return SUCCESS on success, error code on failure
     */
    public int startDhtDiscovery(int dhtPort) {
        return nativeStartDhtDiscovery(nativeClientPtr, dhtPort);
    }
    
    /**
     * Stops DHT discovery.
     */
    public void stopDhtDiscovery() {
        nativeStopDhtDiscovery(nativeClientPtr);
    }
    
    /**
     * Checks if DHT discovery is running.
     * 
     * @return true if DHT is running, false otherwise
     */
    public boolean isDhtRunning() {
        return nativeIsDhtRunning(nativeClientPtr);
    }
    
    /**
     * Starts mDNS discovery with the specified service name.
     * 
     * @param serviceName The service name to advertise/discover
     * @return SUCCESS on success, error code on failure
     */
    public int startMdnsDiscovery(String serviceName) {
        return nativeStartMdnsDiscovery(nativeClientPtr, serviceName);
    }
    
    /**
     * Stops mDNS discovery.
     */
    public void stopMdnsDiscovery() {
        nativeStopMdnsDiscovery(nativeClientPtr);
    }
    
    /**
     * Checks if mDNS discovery is running.
     * 
     * @return true if mDNS is running, false otherwise
     */
    public boolean isMdnsRunning() {
        return nativeIsMdnsRunning(nativeClientPtr);
    }
    
    /**
     * Sends a file to a peer.
     * 
     * @param peerId The ID of the target peer
     * @param filePath The local path of the file to send
     * @param remoteFilename The filename as it should appear on the remote peer
     * @return The transfer ID on success, null on failure
     */
    public String sendFile(String peerId, String filePath, String remoteFilename) {
        return nativeSendFile(nativeClientPtr, peerId, filePath, remoteFilename);
    }
    
    /**
     * Accepts an incoming file transfer.
     * 
     * @param transferId The transfer ID of the incoming file
     * @param localPath The local path where the file should be saved
     * @return SUCCESS on success, error code on failure
     */
    public int acceptFileTransfer(String transferId, String localPath) {
        return nativeAcceptFileTransfer(nativeClientPtr, transferId, localPath);
    }
    
    /**
     * Rejects an incoming file transfer.
     * 
     * @param transferId The transfer ID of the incoming file
     * @param reason The reason for rejection
     * @return SUCCESS on success, error code on failure
     */
    public int rejectFileTransfer(String transferId, String reason) {
        return nativeRejectFileTransfer(nativeClientPtr, transferId, reason);
    }

    // ===================== GOSSIPSUB FUNCTIONALITY =====================
    
    /**
     * Checks if GossipSub is available.
     * 
     * @return true if GossipSub is available, false otherwise
     */
    public boolean isGossipsubAvailable() {
        return nativeIsGossipsubAvailable(nativeClientPtr);
    }
    
    /**
     * Checks if GossipSub is running.
     * 
     * @return true if GossipSub is running, false otherwise
     */
    public boolean isGossipsubRunning() {
        return nativeIsGossipsubRunning(nativeClientPtr);
    }
    
    /**
     * Subscribes to a GossipSub topic.
     * 
     * @param topic The topic name to subscribe to
     * @return SUCCESS on success, error code on failure
     */
    public int subscribeToTopic(String topic) {
        return nativeSubscribeToTopic(nativeClientPtr, topic);
    }
    
    /**
     * Unsubscribes from a GossipSub topic.
     * 
     * @param topic The topic name to unsubscribe from
     * @return SUCCESS on success, error code on failure
     */
    public int unsubscribeFromTopic(String topic) {
        return nativeUnsubscribeFromTopic(nativeClientPtr, topic);
    }
    
    /**
     * Checks if subscribed to a GossipSub topic.
     * 
     * @param topic The topic name to check
     * @return true if subscribed, false otherwise
     */
    public boolean isSubscribedToTopic(String topic) {
        return nativeIsSubscribedToTopic(nativeClientPtr, topic);
    }
    
    /**
     * Gets all subscribed topics.
     * 
     * @return Array of subscribed topic names
     */
    public String[] getSubscribedTopics() {
        return nativeGetSubscribedTopics(nativeClientPtr);
    }
    
    /**
     * Publishes a message to a GossipSub topic.
     * 
     * @param topic The topic to publish to
     * @param message The message to publish
     * @return SUCCESS on success, error code on failure
     */
    public int publishToTopic(String topic, String message) {
        return nativePublishToTopic(nativeClientPtr, topic, message);
    }
    
    /**
     * Publishes a JSON message to a GossipSub topic.
     * 
     * @param topic The topic to publish to
     * @param jsonStr The JSON message to publish
     * @return SUCCESS on success, error code on failure
     */
    public int publishJsonToTopic(String topic, String jsonStr) {
        return nativePublishJsonToTopic(nativeClientPtr, topic, jsonStr);
    }
    
    /**
     * Gets peers subscribed to a topic.
     * 
     * @param topic The topic name
     * @return Array of peer IDs subscribed to the topic
     */
    public String[] getTopicPeers(String topic) {
        return nativeGetTopicPeers(nativeClientPtr, topic);
    }
    
    /**
     * Gets mesh peers for a topic.
     * 
     * @param topic The topic name
     * @return Array of peer IDs in the topic mesh
     */
    public String[] getTopicMeshPeers(String topic) {
        return nativeGetTopicMeshPeers(nativeClientPtr, topic);
    }
    
    /**
     * Gets GossipSub statistics as JSON.
     * 
     * @return JSON string containing GossipSub statistics
     */
    public String getGossipsubStatisticsJson() {
        return nativeGetGossipsubStatisticsJson(nativeClientPtr);
    }
    
    // ===================== NAT TRAVERSAL AND STUN =====================
    
    /**
     * Discovers public IP using STUN and adds it to the ignore list.
     * 
     * @param stunServer STUN server hostname (null for default)
     * @param stunPort STUN server port (0 for default)
     * @return SUCCESS on success, error code on failure
     */
    public int discoverAndIgnorePublicIp(String stunServer, int stunPort) {
        return nativeDiscoverAndIgnorePublicIp(nativeClientPtr, stunServer, stunPort);
    }
    
    /**
     * Gets the discovered public IP address.
     * 
     * @return The public IP address or null if not discovered
     */
    public String getPublicIp() {
        return nativeGetPublicIp(nativeClientPtr);
    }
    
    /**
     * Detects NAT type.
     * 
     * @return NAT type as integer value
     */
    public int detectNatType() {
        return nativeDetectNatType(nativeClientPtr);
    }
    
    /**
     * Gets NAT characteristics as JSON.
     * 
     * @return JSON string containing NAT characteristics
     */
    public String getNatCharacteristicsJson() {
        return nativeGetNatCharacteristicsJson(nativeClientPtr);
    }
    
    /**
     * Adds an IP address to the ignore list.
     * 
     * @param ipAddress IP address to ignore
     */
    public void addIgnoredAddress(String ipAddress) {
        nativeAddIgnoredAddress(nativeClientPtr, ipAddress);
    }
    
    /**
     * Gets NAT traversal statistics as JSON.
     * 
     * @return JSON string containing NAT traversal statistics
     */
    public String getNatTraversalStatisticsJson() {
        return nativeGetNatTraversalStatisticsJson(nativeClientPtr);
    }
    
    // ===================== ICE COORDINATION =====================
    
    /**
     * Creates an ICE offer for a peer.
     * 
     * @param peerId Target peer ID
     * @return ICE offer as JSON string
     */
    public String createIceOffer(String peerId) {
        return nativeCreateIceOffer(nativeClientPtr, peerId);
    }
    
    /**
     * Connects to a peer using ICE coordination.
     * 
     * @param peerId Target peer ID
     * @param iceOfferJson ICE offer from remote peer
     * @return SUCCESS on success, error code on failure
     */
    public int connectWithIce(String peerId, String iceOfferJson) {
        return nativeConnectWithIce(nativeClientPtr, peerId, iceOfferJson);
    }
    
    /**
     * Handles an ICE answer from a peer.
     * 
     * @param peerId Source peer ID
     * @param iceAnswerJson ICE answer from the peer
     * @return SUCCESS on success, error code on failure
     */
    public int handleIceAnswer(String peerId, String iceAnswerJson) {
        return nativeHandleIceAnswer(nativeClientPtr, peerId, iceAnswerJson);
    }
    
    // ===================== AUTOMATIC DISCOVERY =====================
    
    /**
     * Starts automatic peer discovery.
     */
    public void startAutomaticPeerDiscovery() {
        nativeStartAutomaticPeerDiscovery(nativeClientPtr);
    }
    
    /**
     * Stops automatic peer discovery.
     */
    public void stopAutomaticPeerDiscovery() {
        nativeStopAutomaticPeerDiscovery(nativeClientPtr);
    }
    
    /**
     * Checks if automatic discovery is running.
     * 
     * @return true if automatic discovery is running, false otherwise
     */
    public boolean isAutomaticDiscoveryRunning() {
        return nativeIsAutomaticDiscoveryRunning(nativeClientPtr);
    }
    
    /**
     * Gets the discovery hash for current protocol configuration.
     * 
     * @return Discovery hash string
     */
    public String getDiscoveryHash() {
        return nativeGetDiscoveryHash(nativeClientPtr);
    }
    
    /**
     * Gets the well-known RATS peer discovery hash.
     * 
     * @return Standard RATS discovery hash
     */
    public static String getRatsPeerDiscoveryHash() {
        return nativeGetRatsPeerDiscoveryHash();
    }
    
    // ===================== ADVANCED LOGGING API =====================
    
    /**
     * Sets the log file path.
     * 
     * @param filePath Path to the log file
     */
    public void setLogFilePath(String filePath) {
        nativeSetLogFilePath(nativeClientPtr, filePath);
    }
    
    /**
     * Gets the current log file path.
     * 
     * @return Current log file path
     */
    public String getLogFilePath() {
        return nativeGetLogFilePath(nativeClientPtr);
    }
    
    /**
     * Enables or disables colored log output.
     * 
     * @param enabled true to enable colors, false to disable
     */
    public void setLogColorsEnabled(boolean enabled) {
        nativeSetLogColorsEnabled(nativeClientPtr, enabled);
    }
    
    /**
     * Checks if colored log output is enabled.
     * 
     * @return true if colors are enabled, false otherwise
     */
    public boolean isLogColorsEnabled() {
        return nativeIsLogColorsEnabled(nativeClientPtr);
    }
    
    /**
     * Enables or disables timestamps in log output.
     * 
     * @param enabled true to enable timestamps, false to disable
     */
    public void setLogTimestampsEnabled(boolean enabled) {
        nativeSetLogTimestampsEnabled(nativeClientPtr, enabled);
    }
    
    /**
     * Checks if timestamps are enabled in log output.
     * 
     * @return true if timestamps are enabled, false otherwise
     */
    public boolean isLogTimestampsEnabled() {
        return nativeIsLogTimestampsEnabled(nativeClientPtr);
    }
    
    /**
     * Sets log file rotation size.
     * 
     * @param maxSizeBytes Maximum size in bytes before log rotation
     */
    public void setLogRotationSize(long maxSizeBytes) {
        nativeSetLogRotationSize(nativeClientPtr, maxSizeBytes);
    }
    
    /**
     * Sets the number of log files to retain during rotation.
     * 
     * @param count Number of old log files to keep
     */
    public void setLogRetentionCount(int count) {
        nativeSetLogRetentionCount(nativeClientPtr, count);
    }
    
    /**
     * Clears/resets the current log file.
     */
    public void clearLogFile() {
        nativeClearLogFile(nativeClientPtr);
    }
    
    // ===================== CONFIGURATION PERSISTENCE =====================
    
    /**
     * Loads configuration from files.
     * 
     * @return SUCCESS on success, error code on failure
     */
    public int loadConfiguration() {
        return nativeLoadConfiguration(nativeClientPtr);
    }
    
    /**
     * Saves configuration to files.
     * 
     * @return SUCCESS on success, error code on failure
     */
    public int saveConfiguration() {
        return nativeSaveConfiguration(nativeClientPtr);
    }
    
    /**
     * Sets the directory where data files will be stored.
     * 
     * @param directoryPath Path to directory
     * @return SUCCESS on success, error code on failure
     */
    public int setDataDirectory(String directoryPath) {
        return nativeSetDataDirectory(nativeClientPtr, directoryPath);
    }
    
    /**
     * Gets the current data directory path.
     * 
     * @return Current data directory path
     */
    public String getDataDirectory() {
        return nativeGetDataDirectory(nativeClientPtr);
    }
    
    /**
     * Loads saved peers and attempts to reconnect.
     * 
     * @return Number of connection attempts made
     */
    public int loadAndReconnectPeers() {
        return nativeLoadAndReconnectPeers(nativeClientPtr);
    }
    
    /**
     * Loads historical peers from a file.
     * 
     * @return true if successful, false otherwise
     */
    public boolean loadHistoricalPeers() {
        return nativeLoadHistoricalPeers(nativeClientPtr);
    }
    
    /**
     * Saves current peers to a historical file.
     * 
     * @return true if successful, false otherwise
     */
    public boolean saveHistoricalPeers() {
        return nativeSaveHistoricalPeers(nativeClientPtr);
    }
    
    /**
     * Clears all historical peers.
     */
    public void clearHistoricalPeers() {
        nativeClearHistoricalPeers(nativeClientPtr);
    }
    
    /**
     * Gets all historical peer IDs.
     * 
     * @return Array of historical peer ID strings
     */
    public String[] getHistoricalPeerIds() {
        return nativeGetHistoricalPeerIds(nativeClientPtr);
    }
    
    // ===================== ADVANCED FILE TRANSFER =====================
    
    /**
     * Sends an entire directory to a peer.
     * 
     * @param peerId Target peer ID
     * @param directoryPath Local directory path to send
     * @param remoteDirectoryName Optional remote directory name
     * @param recursive Whether to include subdirectories
     * @return Transfer ID on success, null on failure
     */
    public String sendDirectory(String peerId, String directoryPath, String remoteDirectoryName, boolean recursive) {
        return nativeSendDirectory(nativeClientPtr, peerId, directoryPath, remoteDirectoryName, recursive);
    }
    
    /**
     * Requests a file from a remote peer.
     * 
     * @param peerId Target peer ID
     * @param remoteFilePath Path to file on remote peer
     * @param localPath Local path where file should be saved
     * @return Transfer ID on success, null on failure
     */
    public String requestFile(String peerId, String remoteFilePath, String localPath) {
        return nativeRequestFile(nativeClientPtr, peerId, remoteFilePath, localPath);
    }
    
    /**
     * Requests a directory from a remote peer.
     * 
     * @param peerId Target peer ID
     * @param remoteDirectoryPath Path to directory on remote peer
     * @param localDirectoryPath Local path where directory should be saved
     * @param recursive Whether to include subdirectories
     * @return Transfer ID on success, null on failure
     */
    public String requestDirectory(String peerId, String remoteDirectoryPath, String localDirectoryPath, boolean recursive) {
        return nativeRequestDirectory(nativeClientPtr, peerId, remoteDirectoryPath, localDirectoryPath, recursive);
    }
    
    /**
     * Accepts an incoming directory transfer.
     * 
     * @param transferId Transfer identifier from request
     * @param localPath Local path where directory should be saved
     * @return SUCCESS on success, error code on failure
     */
    public int acceptDirectoryTransfer(String transferId, String localPath) {
        return nativeAcceptDirectoryTransfer(nativeClientPtr, transferId, localPath);
    }
    
    /**
     * Rejects an incoming directory transfer.
     * 
     * @param transferId Transfer identifier from request
     * @param reason Optional reason for rejection
     * @return SUCCESS on success, error code on failure
     */
    public int rejectDirectoryTransfer(String transferId, String reason) {
        return nativeRejectDirectoryTransfer(nativeClientPtr, transferId, reason);
    }
    
    /**
     * Pauses an active file transfer.
     * 
     * @param transferId Transfer to pause
     * @return SUCCESS on success, error code on failure
     */
    public int pauseFileTransfer(String transferId) {
        return nativePauseFileTransfer(nativeClientPtr, transferId);
    }
    
    /**
     * Resumes a paused file transfer.
     * 
     * @param transferId Transfer to resume
     * @return SUCCESS on success, error code on failure
     */
    public int resumeFileTransfer(String transferId) {
        return nativeResumeFileTransfer(nativeClientPtr, transferId);
    }
    
    /**
     * Gets file transfer progress information as JSON.
     * 
     * @param transferId Transfer to query
     * @return Progress information as JSON or null if not found
     */
    public String getFileTransferProgressJson(String transferId) {
        return nativeGetFileTransferProgressJson(nativeClientPtr, transferId);
    }
    
    /**
     * Gets file transfer statistics as JSON.
     * 
     * @return JSON string containing transfer statistics
     */
    public String getFileTransferStatisticsJson() {
        return nativeGetFileTransferStatisticsJson(nativeClientPtr);
    }
    
    /**
     * Gets all validated peer IDs.
     * 
     * @return Array of validated peer ID strings
     */
    public String[] getValidatedPeerIds() {
        return nativeGetValidatedPeerIds(nativeClientPtr);
    }
    
    // Callback setters
    public void setConnectionCallback(ConnectionCallback callback) {
        nativeSetConnectionCallback(nativeClientPtr, callback);
    }
    
    public void setStringCallback(StringMessageCallback callback) {
        nativeSetStringCallback(nativeClientPtr, callback);
    }
    
    public void setBinaryCallback(BinaryMessageCallback callback) {
        nativeSetBinaryCallback(nativeClientPtr, callback);
    }
    
    public void setJsonCallback(JsonMessageCallback callback) {
        nativeSetJsonCallback(nativeClientPtr, callback);
    }
    
    public void setDisconnectCallback(DisconnectCallback callback) {
        nativeSetDisconnectCallback(nativeClientPtr, callback);
    }
    
    // GossipSub callback setters
    public void setTopicMessageCallback(String topic, TopicMessageCallback callback) {
        nativeSetTopicMessageCallback(nativeClientPtr, topic, callback);
    }
    
    public void setTopicJsonMessageCallback(String topic, TopicJsonMessageCallback callback) {
        nativeSetTopicJsonMessageCallback(nativeClientPtr, topic, callback);
    }
    
    public void setTopicPeerJoinedCallback(String topic, TopicPeerJoinedCallback callback) {
        nativeSetTopicPeerJoinedCallback(nativeClientPtr, topic, callback);
    }
    
    public void setTopicPeerLeftCallback(String topic, TopicPeerLeftCallback callback) {
        nativeSetTopicPeerLeftCallback(nativeClientPtr, topic, callback);
    }
    
    public void clearTopicCallbacks(String topic) {
        nativeClearTopicCallbacks(nativeClientPtr, topic);
    }
    
    // File transfer callback setters
    public void setFileRequestCallback(FileRequestCallback callback) {
        nativeSetFileRequestCallback(nativeClientPtr, callback);
    }
    
    public void setDirectoryRequestCallback(DirectoryRequestCallback callback) {
        nativeSetDirectoryRequestCallback(nativeClientPtr, callback);
    }
    
    public void setDirectoryProgressCallback(DirectoryProgressCallback callback) {
        nativeSetDirectoryProgressCallback(nativeClientPtr, callback);
    }
    
    // Static utility methods
    public static String getVersionString() {
        return nativeGetVersionString();
    }
    
    public static int[] getVersion() {
        return nativeGetVersion();
    }
    
    public static String getGitDescribe() {
        return nativeGetGitDescribe();
    }
    
    public static int getAbi() {
        return nativeGetAbi();
    }
    
    public static void setLoggingEnabled(boolean enabled) {
        nativeSetLoggingEnabled(enabled);
    }
    
    public static void setLogLevel(String level) {
        nativeSetLogLevel(level);
    }
    
    // Native method declarations
    private static native String nativeGetVersionString();
    private static native int[] nativeGetVersion();
    private static native String nativeGetGitDescribe();
    private static native int nativeGetAbi();
    private static native void nativeSetLoggingEnabled(boolean enabled);
    private static native void nativeSetLogLevel(String level);
    
    private native long nativeCreate(int listenPort);
    private native void nativeDestroy(long clientPtr);
    private native int nativeStart(long clientPtr);
    private native void nativeStop(long clientPtr);
    
    private native int nativeConnect(long clientPtr, String host, int port);
    private native int nativeConnectWithStrategy(long clientPtr, String host, int port, int strategy);
    private native int nativeSendString(long clientPtr, String peerId, String message);
    private native int nativeBroadcastString(long clientPtr, String message);
    private native int nativeSendBinary(long clientPtr, String peerId, byte[] data);
    private native int nativeBroadcastBinary(long clientPtr, byte[] data);
    private native int nativeSendJson(long clientPtr, String peerId, String jsonStr);
    private native int nativeBroadcastJson(long clientPtr, String jsonStr);
    
    private native int nativeGetPeerCount(long clientPtr);
    private native int nativeGetListenPort(long clientPtr);
    private native String nativeGetOurPeerId(long clientPtr);
    private native String nativeGetConnectionStatisticsJson(long clientPtr);
    private native String[] nativeGetPeerIds(long clientPtr);
    
    private native int nativeSetEncryptionEnabled(long clientPtr, boolean enabled);
    private native boolean nativeIsEncryptionEnabled(long clientPtr);
    private native String nativeGenerateEncryptionKey(long clientPtr);
    private native int nativeSetEncryptionKey(long clientPtr, String key);
    
    private native int nativeStartDhtDiscovery(long clientPtr, int dhtPort);
    private native void nativeStopDhtDiscovery(long clientPtr);
    private native boolean nativeIsDhtRunning(long clientPtr);
    
    private native int nativeStartMdnsDiscovery(long clientPtr, String serviceName);
    private native void nativeStopMdnsDiscovery(long clientPtr);
    private native boolean nativeIsMdnsRunning(long clientPtr);
    
    private native String nativeSendFile(long clientPtr, String peerId, String filePath, String remoteFilename);
    private native int nativeAcceptFileTransfer(long clientPtr, String transferId, String localPath);
    private native int nativeRejectFileTransfer(long clientPtr, String transferId, String reason);
    
    private native void nativeSetConnectionCallback(long clientPtr, ConnectionCallback callback);
    private native void nativeSetStringCallback(long clientPtr, StringMessageCallback callback);
    private native void nativeSetBinaryCallback(long clientPtr, BinaryMessageCallback callback);
    private native void nativeSetJsonCallback(long clientPtr, JsonMessageCallback callback);
    private native void nativeSetDisconnectCallback(long clientPtr, DisconnectCallback callback);
    
    // GossipSub native methods
    private native boolean nativeIsGossipsubAvailable(long clientPtr);
    private native boolean nativeIsGossipsubRunning(long clientPtr);
    private native int nativeSubscribeToTopic(long clientPtr, String topic);
    private native int nativeUnsubscribeFromTopic(long clientPtr, String topic);
    private native boolean nativeIsSubscribedToTopic(long clientPtr, String topic);
    private native String[] nativeGetSubscribedTopics(long clientPtr);
    private native int nativePublishToTopic(long clientPtr, String topic, String message);
    private native int nativePublishJsonToTopic(long clientPtr, String topic, String jsonStr);
    private native String[] nativeGetTopicPeers(long clientPtr, String topic);
    private native String[] nativeGetTopicMeshPeers(long clientPtr, String topic);
    private native String nativeGetGossipsubStatisticsJson(long clientPtr);
    
    // GossipSub callback setters
    private native void nativeSetTopicMessageCallback(long clientPtr, String topic, TopicMessageCallback callback);
    private native void nativeSetTopicJsonMessageCallback(long clientPtr, String topic, TopicJsonMessageCallback callback);
    private native void nativeSetTopicPeerJoinedCallback(long clientPtr, String topic, TopicPeerJoinedCallback callback);
    private native void nativeSetTopicPeerLeftCallback(long clientPtr, String topic, TopicPeerLeftCallback callback);
    private native void nativeClearTopicCallbacks(long clientPtr, String topic);
    
    // NAT traversal and STUN
    private native int nativeDiscoverAndIgnorePublicIp(long clientPtr, String stunServer, int stunPort);
    private native String nativeGetPublicIp(long clientPtr);
    private native int nativeDetectNatType(long clientPtr);
    private native String nativeGetNatCharacteristicsJson(long clientPtr);
    private native void nativeAddIgnoredAddress(long clientPtr, String ipAddress);
    private native String nativeGetNatTraversalStatisticsJson(long clientPtr);
    
    // ICE coordination
    private native String nativeCreateIceOffer(long clientPtr, String peerId);
    private native int nativeConnectWithIce(long clientPtr, String peerId, String iceOfferJson);
    private native int nativeHandleIceAnswer(long clientPtr, String peerId, String iceAnswerJson);
    
    // Automatic discovery
    private native void nativeStartAutomaticPeerDiscovery(long clientPtr);
    private native void nativeStopAutomaticPeerDiscovery(long clientPtr);
    private native boolean nativeIsAutomaticDiscoveryRunning(long clientPtr);
    private native String nativeGetDiscoveryHash(long clientPtr);
    private static native String nativeGetRatsPeerDiscoveryHash();
    
    // Advanced logging API
    private native void nativeSetLogFilePath(long clientPtr, String filePath);
    private native String nativeGetLogFilePath(long clientPtr);
    private native void nativeSetLogColorsEnabled(long clientPtr, boolean enabled);
    private native boolean nativeIsLogColorsEnabled(long clientPtr);
    private native void nativeSetLogTimestampsEnabled(long clientPtr, boolean enabled);
    private native boolean nativeIsLogTimestampsEnabled(long clientPtr);
    private native void nativeSetLogRotationSize(long clientPtr, long maxSizeBytes);
    private native void nativeSetLogRetentionCount(long clientPtr, int count);
    private native void nativeClearLogFile(long clientPtr);
    
    // Configuration persistence
    private native int nativeLoadConfiguration(long clientPtr);
    private native int nativeSaveConfiguration(long clientPtr);
    private native int nativeSetDataDirectory(long clientPtr, String directoryPath);
    private native String nativeGetDataDirectory(long clientPtr);
    private native int nativeLoadAndReconnectPeers(long clientPtr);
    private native boolean nativeLoadHistoricalPeers(long clientPtr);
    private native boolean nativeSaveHistoricalPeers(long clientPtr);
    private native void nativeClearHistoricalPeers(long clientPtr);
    private native String[] nativeGetHistoricalPeerIds(long clientPtr);
    
    // Advanced file transfer
    private native String nativeSendDirectory(long clientPtr, String peerId, String directoryPath, String remoteDirectoryName, boolean recursive);
    private native String nativeRequestFile(long clientPtr, String peerId, String remoteFilePath, String localPath);
    private native String nativeRequestDirectory(long clientPtr, String peerId, String remoteDirectoryPath, String localDirectoryPath, boolean recursive);
    private native int nativeAcceptDirectoryTransfer(long clientPtr, String transferId, String localPath);
    private native int nativeRejectDirectoryTransfer(long clientPtr, String transferId, String reason);
    private native int nativePauseFileTransfer(long clientPtr, String transferId);
    private native int nativeResumeFileTransfer(long clientPtr, String transferId);
    private native String nativeGetFileTransferProgressJson(long clientPtr, String transferId);
    private native String nativeGetFileTransferStatisticsJson(long clientPtr);
    
    // File transfer callback setters
    private native void nativeSetFileRequestCallback(long clientPtr, FileRequestCallback callback);
    private native void nativeSetDirectoryRequestCallback(long clientPtr, DirectoryRequestCallback callback);
    private native void nativeSetDirectoryProgressCallback(long clientPtr, DirectoryProgressCallback callback);
    
    // Peer validation info
    private native String[] nativeGetValidatedPeerIds(long clientPtr);
}
