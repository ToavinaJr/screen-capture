#ifndef STREAMCLIENT_H
#define STREAMCLIENT_H

#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <vector>
#include "common.h"

class StreamClient {
public:
    using VideoFrameCallback = std::function<void(const VideoFrame&, const std::vector<uint8_t>&)>;
    using AudioFrameCallback = std::function<void(const AudioFrame&, const std::vector<uint8_t>&)>;
    using DisconnectCallback = std::function<void()>;

    StreamClient(const std::string& server_address, int server_port);
    ~StreamClient();

    bool connect();
    void disconnect();
    bool isConnected() const { return connected_; }
    
    // Callbacks for received frames
    void setVideoFrameCallback(VideoFrameCallback callback) { video_callback_ = callback; }
    void setAudioFrameCallback(AudioFrameCallback callback) { audio_callback_ = callback; }
    void setDisconnectCallback(DisconnectCallback callback) { disconnect_callback_ = callback; }
    
    // Statistics
    uint64_t getReceivedVideoFrames() const { return video_frames_received_; }
    uint64_t getReceivedAudioFrames() const { return audio_frames_received_; }
    uint64_t getBytesReceived() const { return bytes_received_; }

private:
    void receiveLoop();
    void heartbeatLoop();
    bool sendHandshake();
    bool sendHeartbeat();
    bool receivePacket(PacketHeader& header, std::vector<uint8_t>& payload);
    void handleVideoFrame(const PacketHeader& header, const std::vector<uint8_t>& payload);
    void handleAudioFrame(const PacketHeader& header, const std::vector<uint8_t>& payload);
    
    std::string server_address_;
    int server_port_;
    SOCKET socket_;
    std::atomic<bool> connected_;
    
    std::thread receive_thread_;
    std::thread heartbeat_thread_;
    
    VideoFrameCallback video_callback_;
    AudioFrameCallback audio_callback_;
    DisconnectCallback disconnect_callback_;
    
    std::atomic<uint64_t> video_frames_received_;
    std::atomic<uint64_t> audio_frames_received_;
    std::atomic<uint64_t> bytes_received_;
};

#endif // STREAMCLIENT_H
