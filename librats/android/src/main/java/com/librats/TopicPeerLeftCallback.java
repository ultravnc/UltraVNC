package com.librats;

/**
 * Callback interface for GossipSub topic peer left events.
 */
public interface TopicPeerLeftCallback {
    /**
     * Called when a peer leaves a subscribed topic.
     * 
     * @param peerId The ID of the peer who left
     * @param topic The topic name
     */
    void onTopicPeerLeft(String peerId, String topic);
}
