package com.librats;

/**
 * Callback interface for GossipSub topic message events.
 */
public interface TopicMessageCallback {
    /**
     * Called when a message is received on a subscribed topic.
     * 
     * @param peerId The ID of the peer who sent the message
     * @param topic The topic name
     * @param message The message content
     */
    void onTopicMessage(String peerId, String topic, String message);
}
