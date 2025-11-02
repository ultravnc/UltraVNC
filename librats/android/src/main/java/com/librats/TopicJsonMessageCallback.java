package com.librats;

/**
 * Callback interface for GossipSub topic JSON message events.
 */
public interface TopicJsonMessageCallback {
    /**
     * Called when a JSON message is received on a subscribed topic.
     * 
     * @param peerId The ID of the peer who sent the message
     * @param topic The topic name
     * @param jsonStr The JSON message content
     */
    void onTopicJsonMessage(String peerId, String topic, String jsonStr);
}
