#include "StreamServer.h"
#include "../utils/Logger.h"
#include <cstring>
#include <algorithm>

#ifdef PLATFORM_WINDOWS
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
#endif

StreamServer::StreamServer(const std::string& address, int port) 
    : address_(address), port_(port), running_(false), listen_socket_(INVALID_SOCKET),
      next_client_id_(1), sequence_number_(0) {
    
    std::string msg = "StreamServer created: " + address + ":" + std::to_string(port);
    Logger::log(Logger::LogLevel::INFO, msg);
}

StreamServer::~StreamServer() {
    stop();
    Logger::log(Logger::LogLevel::INFO, "StreamServer destroyed");
}

bool StreamServer::start() {
    if (running_) {
        Logger::log(Logger::LogLevel::WARN, "StreamServer already running");
        return false;
    }

    // Initialize socket system
    static SocketInitializer sockInit;

    // Create listening socket
    listen_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket_ == INVALID_SOCKET) {
        Logger::log(Logger::LogLevel::WARN, "Failed to create listen socket");
        return false;
    }

    // Set socket options
    int reuse = 1;
    if (setsockopt(listen_socket_, SOL_SOCKET, SO_REUSEADDR, 
                   (const char*)&reuse, sizeof(reuse)) < 0) {
        Logger::log(Logger::LogLevel::WARN, "Failed to set SO_REUSEADDR");
    }

    // Bind socket
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    
    if (address_.empty() || address_ == "0.0.0.0") {
        server_addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        server_addr.sin_addr.s_addr = inet_addr(address_.c_str());
    }

    if (bind(listen_socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        Logger::log(Logger::LogLevel::WARN, "Failed to bind socket");
        closesocket(listen_socket_);
        listen_socket_ = INVALID_SOCKET;
        return false;
    }

    // Listen for connections
    if (listen(listen_socket_, Config::MAX_CLIENTS) < 0) {
        Logger::log(Logger::LogLevel::WARN, "Failed to listen on socket");
        closesocket(listen_socket_);
        listen_socket_ = INVALID_SOCKET;
        return false;
    }

    running_ = true;
    
    // Start accept thread
    accept_thread_ = std::thread(&StreamServer::acceptConnections, this);
    
    // Start heartbeat monitor thread
    heartbeat_thread_ = std::thread(&StreamServer::heartbeatMonitor, this);

    std::string msg = "StreamServer started on " + address_ + ":" + std::to_string(port_);
    Logger::log(Logger::LogLevel::INFO, msg);
    return true;
}

void StreamServer::stop() {
    if (!running_) {
        return;
    }

    running_ = false;
    Logger::log(Logger::LogLevel::INFO, "StreamServer stopping...");

    // Close listening socket
    if (listen_socket_ != INVALID_SOCKET) {
        closesocket(listen_socket_);
        listen_socket_ = INVALID_SOCKET;
    }

    // Wait for accept thread
    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }

    // Wait for heartbeat thread
    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }

    // Disconnect all clients
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        for (auto& pair : clients_) {
            pair.second->active = false;
            if (pair.second->socket != INVALID_SOCKET) {
                closesocket(pair.second->socket);
            }
        }
        clients_.clear();
    }

    // Wait for all client threads
    for (auto& thread : client_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    client_threads_.clear();

    Logger::log(Logger::LogLevel::INFO, "StreamServer stopped");
}

void StreamServer::acceptConnections() {
    Logger::log(Logger::LogLevel::INFO, "Accept thread started");

    while (running_) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        
        SOCKET client_socket = accept(listen_socket_, 
                                      (struct sockaddr*)&client_addr, 
                                      &addr_len);
        
        if (client_socket == INVALID_SOCKET) {
            if (running_) {
                Logger::log(Logger::LogLevel::WARN, "Accept failed");
            }
            continue;
        }

        // Create client info
        auto client = std::make_shared<ClientInfo>();
        client->socket = client_socket;
        client->client_id = next_client_id_++;
        client->address = inet_ntoa(client_addr.sin_addr);
        client->port = ntohs(client_addr.sin_port);
        client->active = true;
        client->last_heartbeat = get_timestamp_us();
        
        // Default config
        client->config.fps = Config::DEFAULT_FPS;
        client->config.jpeg_quality = Config::DEFAULT_JPEG_QUALITY;
        client->config.audio_sample_rate = Config::DEFAULT_AUDIO_SAMPLE_RATE;
        client->config.audio_channels = 1;
        client->config.enable_audio = 1;
        client->config.enable_video = 1;

        std::string msg = "New client connected: " + client->address + ":" + 
                         std::to_string(client->port) + " (ID: " + 
                         std::to_string(client->client_id) + ")";
        Logger::log(Logger::LogLevel::INFO, msg);

        // Add to client list
        {
            std::lock_guard<std::mutex> lock(clients_mutex_);
            clients_[client->client_id] = client;
        }

        // Start client handler thread
        client_threads_.push_back(
            std::thread(&StreamServer::handleClient, this, client)
        );
    }

    Logger::log(Logger::LogLevel::INFO, "Accept thread ended");
}

void StreamServer::handleClient(std::shared_ptr<ClientInfo> client) {
    std::string msg = "Handling client " + std::to_string(client->client_id);
    Logger::log(Logger::LogLevel::INFO, msg);

    // Process handshake
    if (!processHandshake(client)) {
        Logger::log(Logger::LogLevel::WARN, "Handshake failed");
        client->active = false;
        closesocket(client->socket);
        
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients_.erase(client->client_id);
        return;
    }

    // Set socket to non-blocking mode
#ifdef PLATFORM_WINDOWS
    u_long mode = 1;
    ioctlsocket(client->socket, FIONBIO, &mode);
#else
    int flags = fcntl(client->socket, F_GETFL, 0);
    fcntl(client->socket, F_SETFL, flags | O_NONBLOCK);
#endif

    // Main client loop - receive commands and handle disconnection
    while (running_ && client->active) {
        PacketHeader header;
        std::vector<uint8_t> payload;
        
        // Try to receive packet (non-blocking)
        int received = recv(client->socket, (char*)&header, sizeof(header), 0);
        
        if (received == sizeof(header)) {
            // Validate and receive full packet
            if (header.magic == MAGIC_NUMBER && header.payload_size < Config::MAX_PACKET_SIZE) {
                payload.resize(header.payload_size);
                if (header.payload_size > 0) {
                    int payload_received = 0;
                    while (payload_received < (int)header.payload_size && running_ && client->active) {
                        int n = recv(client->socket, (char*)payload.data() + payload_received,
                                   header.payload_size - payload_received, 0);
                        if (n > 0) {
                            payload_received += n;
                        } else if (n == 0) {
                            Logger::log(Logger::LogLevel::INFO, "Client disconnected");
                            client->active = false;
                            break;
                        } else {
#ifdef PLATFORM_WINDOWS
                            int err = WSAGetLastError();
                            if (err != WSAEWOULDBLOCK) {
#else
                            int err = errno;
                            if (err != EWOULDBLOCK && err != EAGAIN) {
#endif
                                Logger::log(Logger::LogLevel::INFO, "Client recv error: " + std::to_string(err));
                                client->active = false;
                                break;
                            }
                            std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        }
                    }
                }
                
                // Update heartbeat
                client->last_heartbeat = get_timestamp_us();

                // Process packet type
                switch ((PacketType)header.packet_type) {
                    case PacketType::HEARTBEAT:
                        // Send ACK
                        sendPacket(client->socket, PacketType::ACK, nullptr, 0);
                        break;
                        
                    case PacketType::CONFIG:
                        if (payload.size() >= sizeof(StreamConfig)) {
                            memcpy(&client->config, payload.data(), sizeof(StreamConfig));
                            Logger::log(Logger::LogLevel::INFO, "Client config updated");
                        }
                        break;
                        
                    case PacketType::DISCONNECT:
                        Logger::log(Logger::LogLevel::INFO, "Client requested disconnect");
                        client->active = false;
                        break;
                        
                    default:
                        Logger::log(Logger::LogLevel::WARN, "Unknown packet type");
                        break;
                }
            }
        } else if (received == 0) {
            Logger::log(Logger::LogLevel::INFO, "Client disconnected");
            break;
        } else {
            // Check if it's just WOULDBLOCK (no data available)
#ifdef PLATFORM_WINDOWS
            int err = WSAGetLastError();
            if (err != WSAEWOULDBLOCK) {
#else
            int err = errno;
            if (err != EWOULDBLOCK && err != EAGAIN) {
#endif
                Logger::log(Logger::LogLevel::INFO, "Client recv error: " + std::to_string(err));
                break;
            }
        }
        
        // Sleep briefly to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Cleanup
    closesocket(client->socket);
    client->socket = INVALID_SOCKET;
    client->active = false;

    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients_.erase(client->client_id);
    }

    msg = "Client " + std::to_string(client->client_id) + " handler ended";
    Logger::log(Logger::LogLevel::INFO, msg);
}

bool StreamServer::processHandshake(std::shared_ptr<ClientInfo> client) {
    PacketHeader header;
    std::vector<uint8_t> payload;
    
    // Wait for handshake request
    if (!receivePacket(client->socket, header, payload)) {
        return false;
    }

    if ((PacketType)header.packet_type != PacketType::HANDSHAKE) {
        Logger::log(Logger::LogLevel::WARN, "Expected handshake packet");
        return false;
    }

    if (payload.size() < sizeof(HandshakeRequest)) {
        Logger::log(Logger::LogLevel::WARN, "Invalid handshake size");
        return false;
    }

    HandshakeRequest request;
    memcpy(&request, payload.data(), sizeof(HandshakeRequest));

    // Initialize client config based on capabilities
    client->config.enable_video = (request.capabilities & 0x01) ? 1 : 0;
    client->config.enable_audio = (request.capabilities & 0x02) ? 1 : 0;
    client->config.fps = 30;
    client->config.jpeg_quality = 80;
    client->config.audio_sample_rate = 44100;
    client->config.audio_channels = 1;

    // Send handshake response
    HandshakeResponse response;
    response.accepted = 1;
    response.assigned_id = client->client_id;
    snprintf(response.server_info, sizeof(response.server_info), 
             "StreamServer v%d", PROTOCOL_VERSION);

    sendPacket(client->socket, PacketType::HANDSHAKE, &response, sizeof(response));

    Logger::log(Logger::LogLevel::INFO, "Handshake completed - Video:" + 
        std::to_string(client->config.enable_video) + " Audio:" + 
        std::to_string(client->config.enable_audio));
    return true;
}

void StreamServer::sendPacket(SOCKET sock, PacketType type, const void* data, size_t size) {
    PacketHeader header;
    header.magic = MAGIC_NUMBER;
    header.version = PROTOCOL_VERSION;
    header.packet_type = (uint8_t)type;
    header.flags = 0;
    header.payload_size = (uint32_t)size;
    header.sequence_number = sequence_number_++;
    header.timestamp = get_timestamp_us();

    // Send header (retry on WOULDBLOCK)
    size_t total_sent = 0;
    while (total_sent < sizeof(header)) {
        int sent = send(sock, (const char*)&header + total_sent, sizeof(header) - total_sent, 0);
        if (sent > 0) {
            total_sent += sent;
        } else if (sent == SOCKET_ERROR) {
#ifdef PLATFORM_WINDOWS
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK) {
#else
            int err = errno;
            if (err == EWOULDBLOCK || err == EAGAIN) {
#endif
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            } else {
                return; // Connection error
            }
        } else {
            return; // Connection closed
        }
    }

    // Send payload if any (retry on WOULDBLOCK)
    if (size > 0 && data) {
        total_sent = 0;
        while (total_sent < size) {
            int sent = send(sock, (const char*)data + total_sent, size - total_sent, 0);
            if (sent > 0) {
                total_sent += sent;
            } else if (sent == SOCKET_ERROR) {
#ifdef PLATFORM_WINDOWS
                int err = WSAGetLastError();
                if (err == WSAEWOULDBLOCK) {
#else
                int err = errno;
                if (err == EWOULDBLOCK || err == EAGAIN) {
#endif
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                } else {
                    return; // Connection error
                }
            } else {
                return; // Connection closed
            }
        }
    }
}

bool StreamServer::receivePacket(SOCKET sock, PacketHeader& header, std::vector<uint8_t>& payload) {
    // Receive header
    int received = recv(sock, (char*)&header, sizeof(header), 0);
    if (received != sizeof(header)) {
        return false;
    }

    // Validate header
    if (header.magic != MAGIC_NUMBER) {
        Logger::log(Logger::LogLevel::WARN, "Invalid magic number");
        return false;
    }

    // Receive payload
    if (header.payload_size > 0) {
        if (header.payload_size > Config::MAX_PACKET_SIZE) {
            Logger::log(Logger::LogLevel::WARN, "Payload too large");
            return false;
        }

        payload.resize(header.payload_size);
        received = recv(sock, (char*)payload.data(), header.payload_size, 0);
        if (received != (int)header.payload_size) {
            return false;
        }
    } else {
        payload.clear();
    }

    return true;
}

void StreamServer::broadcastVideoFrame(const VideoFrame& frame) {
    if (!running_) return;

    std::lock_guard<std::mutex> lock(clients_mutex_);
    
    static uint32_t log_counter = 0;
    if (++log_counter % 30 == 0) {
        std::string msg = "Broadcasting frame " + std::to_string(frame.frame_number) +
                         " to " + std::to_string(clients_.size()) + " client(s)";
        Logger::log(Logger::LogLevel::INFO, msg);
    }
    
    for (auto& pair : clients_) {
        if (pair.second->active && pair.second->config.enable_video) {
            // Create serialized packet: header + pixel data
            struct VideoFrameHeader {
                uint32_t frame_number;
                uint16_t width;
                uint16_t height;
                uint8_t quality;
                uint8_t padding;
                uint64_t timestamp;
            };
            
            VideoFrameHeader header;
            header.frame_number = frame.frame_number;
            header.width = frame.width;
            header.height = frame.height;
            header.quality = frame.quality;
            header.padding = 0;
            header.timestamp = frame.timestamp;
            
            // Combine header + data
            std::vector<uint8_t> packet;
            packet.resize(sizeof(VideoFrameHeader) + frame.data.size());
            memcpy(packet.data(), &header, sizeof(VideoFrameHeader));
            memcpy(packet.data() + sizeof(VideoFrameHeader), frame.data.data(), frame.data.size());
            
            // Send combined packet
            sendPacket(pair.second->socket, PacketType::VIDEO_FRAME, 
                      packet.data(), packet.size());
        }
    }
}

void StreamServer::broadcastAudioFrame(const AudioFrame& frame) {
    if (!running_) return;

    std::lock_guard<std::mutex> lock(clients_mutex_);
    
    for (auto& pair : clients_) {
        if (pair.second->active && pair.second->config.enable_audio) {
            // Send frame data
            sendPacket(pair.second->socket, PacketType::AUDIO_FRAME,
                      frame.samples.data(), 
                      frame.samples.size() * sizeof(float));
        }
    }
}

void StreamServer::heartbeatMonitor() {
    Logger::log(Logger::LogLevel::INFO, "Heartbeat monitor started");

    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(5));

        uint64_t now = get_timestamp_us();
        uint64_t timeout = 30 * 1000000; // 30 seconds

        std::lock_guard<std::mutex> lock(clients_mutex_);
        
        for (auto& pair : clients_) {
            if (pair.second->active) {
                if (now - pair.second->last_heartbeat > timeout) {
                    std::string msg = "Client " + std::to_string(pair.second->client_id) + 
                                     " timeout - disconnecting";
                    Logger::log(Logger::LogLevel::WARN, msg);
                    pair.second->active = false;
                }
            }
        }
    }

    Logger::log(Logger::LogLevel::INFO, "Heartbeat monitor ended");
}

size_t StreamServer::getClientCount() const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    return clients_.size();
}

void StreamServer::disconnectClient(uint16_t client_id) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    
    auto it = clients_.find(client_id);
    if (it != clients_.end()) {
        it->second->active = false;
        if (it->second->socket != INVALID_SOCKET) {
            sendPacket(it->second->socket, PacketType::DISCONNECT, nullptr, 0);
            closesocket(it->second->socket);
        }
    }
}
