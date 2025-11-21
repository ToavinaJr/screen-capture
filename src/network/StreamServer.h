#ifndef STREAMSERVER_H
#define STREAMSERVER_H

#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <memory>
#include <map>
#include <mutex>
#include "common.h"

// Forward declaration to avoid including TLSConnection.h when TLS is disabled
class TLSConnection;

struct ClientInfo {
    SOCKET socket;
    uint16_t client_id;
    std::string address;
    uint16_t port;
    std::atomic<bool> active;
    uint64_t last_heartbeat;
    StreamConfig config;
};

class StreamServer {
public:
    StreamServer(const std::string& address, int port);
    ~StreamServer();

    bool start();
    void stop();
    bool isRunning() const { return running_; }
    
    // Broadcast frames to all clients
    void broadcastVideoFrame(const VideoFrame& frame);
    void broadcastAudioFrame(const AudioFrame& frame);
    
    // Client management
    size_t getClientCount() const;
    void disconnectClient(uint16_t client_id);

private:
    void acceptConnections();
    void handleClient(std::shared_ptr<ClientInfo> client);
    bool processHandshake(std::shared_ptr<ClientInfo> client);
    void sendPacket(SOCKET sock, PacketType type, const void* data, size_t size);
    bool receivePacket(SOCKET sock, PacketHeader& header, std::vector<uint8_t>& payload);
    void heartbeatMonitor();
    
    std::string address_;
    int port_;
    std::atomic<bool> running_;
    SOCKET listen_socket_;
    
    std::vector<std::thread> client_threads_;
    std::thread accept_thread_;
    std::thread heartbeat_thread_;
    
    std::map<uint16_t, std::shared_ptr<ClientInfo>> clients_;
    mutable std::mutex clients_mutex_;
    
    std::atomic<uint16_t> next_client_id_;
    std::atomic<uint32_t> sequence_number_;
};

#endif // STREAMSERVER_H