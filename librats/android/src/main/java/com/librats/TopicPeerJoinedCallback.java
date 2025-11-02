package com.librats;

/**
 * Callback interface for GossipSub topic peer joined events.
 */
public interface TopicPeerJoinedCallback {
    /**
     * Called when a peer joins a subscribed topic.
     * 
     * @param peerId The ID of the peer who joined
     * @param topic The topic name
     */
    void onTopicPeerJoined(String peerId, String topic);
}
