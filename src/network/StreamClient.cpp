#include "StreamClient.h"
#include "../utils/Logger.h"
#include <cstring>
#include <sstream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#endif

StreamClient::StreamClient(const std::string& server_address, int server_port)
    : server_address_(server_address)
    , server_port_(server_port)
    , socket_(INVALID_SOCKET)
    , connected_(false)
    , video_frames_received_(0)
    , audio_frames_received_(0)
    , bytes_received_(0) {
    
    static SocketInitializer socket_init;
    Logger::log(Logger::LogLevel::INFO, "StreamClient created for " + server_address + ":" + std::to_string(server_port));
}

StreamClient::~StreamClient() {
    disconnect();
    Logger::log(Logger::LogLevel::INFO, "StreamClient destroyed");
}

bool StreamClient::connect() {
    if (connected_) {
        Logger::log(Logger::LogLevel::WARN, "Already connected");
        return true;
    }
    
    // Create socket
    socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_ == INVALID_SOCKET) {
        Logger::log(Logger::LogLevel::ERROR_LEVEL, "Failed to create socket");
        return false;
    }
    
    // Resolve server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port_);
    
    if (inet_pton(AF_INET, server_address_.c_str(), &server_addr.sin_addr) <= 0) {
        // Try resolving as hostname
        struct hostent* host = gethostbyname(server_address_.c_str());
        if (!host) {
            Logger::log(Logger::LogLevel::ERROR_LEVEL, "Failed to resolve server address: " + server_address_);
#ifdef _WIN32
            closesocket(socket_);
#else
            close(socket_);
#endif
            socket_ = INVALID_SOCKET;
            return false;
        }
        memcpy(&server_addr.sin_addr, host->h_addr_list[0], host->h_length);
    }
    
    // Connect to server
    if (::connect(socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        Logger::log(Logger::LogLevel::ERROR_LEVEL, "Failed to connect to server");
#ifdef _WIN32
        closesocket(socket_);
#else
        close(socket_);
#endif
        socket_ = INVALID_SOCKET;
        return false;
    }
    
    Logger::log(Logger::LogLevel::INFO, "Connected to server " + server_address_ + ":" + std::to_string(server_port_));
    
    // Send handshake
    if (!sendHandshake()) {
        Logger::log(Logger::LogLevel::ERROR_LEVEL, "Handshake failed");
        disconnect();
        return false;
    }
    
    connected_ = true;
    
    // Start receive thread
    receive_thread_ = std::thread(&StreamClient::receiveLoop, this);
    
    // Start heartbeat thread
    heartbeat_thread_ = std::thread(&StreamClient::heartbeatLoop, this);
    
    Logger::log(Logger::LogLevel::INFO, "Client threads started");
    return true;
}

void StreamClient::disconnect() {
    if (!connected_) {
        return;
    }
    
    Logger::log(Logger::LogLevel::INFO, "Disconnecting client...");
    connected_ = false;
    
    // Close socket to unblock receive
    if (socket_ != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(socket_);
#else
        close(socket_);
#endif
        socket_ = INVALID_SOCKET;
    }
    
    // Wait for threads
    if (receive_thread_.joinable()) {
        receive_thread_.join();
    }
    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }
    
    Logger::log(Logger::LogLevel::INFO, "Client disconnected");
    
    if (disconnect_callback_) {
        disconnect_callback_();
    }
}

bool StreamClient::sendHandshake() {
    HandshakeRequest request;
    strncpy(request.client_name, "TestClient", sizeof(request.client_name) - 1);
    request.capabilities = 0x03; // Video + Audio
    request.max_width = 1920;
    request.max_height = 1080;
    
    PacketHeader header;
    header.magic = MAGIC_NUMBER;
    header.version = PROTOCOL_VERSION;
    header.packet_type = static_cast<uint8_t>(PacketType::HANDSHAKE);
    header.flags = 0;
    header.sequence_number = 0;
    header.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    header.payload_size = sizeof(HandshakeRequest);
    
    // Send header
    if (send(socket_, reinterpret_cast<const char*>(&header), sizeof(header), 0) != sizeof(header)) {
        Logger::log(Logger::LogLevel::ERROR_LEVEL, "Failed to send handshake header");
        return false;
    }
    
    // Send payload
    if (send(socket_, reinterpret_cast<const char*>(&request), sizeof(request), 0) != sizeof(request)) {
        Logger::log(Logger::LogLevel::ERROR_LEVEL, "Failed to send handshake payload");
        return false;
    }
    
    // Receive handshake response
    PacketHeader response_header;
    std::vector<uint8_t> response_payload;
    
    if (!receivePacket(response_header, response_payload)) {
        Logger::log(Logger::LogLevel::ERROR_LEVEL, "Failed to receive handshake response");
        return false;
    }
    
    if (response_header.packet_type != static_cast<uint8_t>(PacketType::HANDSHAKE)) {
        Logger::log(Logger::LogLevel::ERROR_LEVEL, "Invalid handshake response type");
        return false;
    }
    
    if (response_payload.size() != sizeof(HandshakeResponse)) {
        Logger::log(Logger::LogLevel::ERROR_LEVEL, "Invalid handshake response size");
        return false;
    }
    
    HandshakeResponse* response = reinterpret_cast<HandshakeResponse*>(response_payload.data());
    if (response->accepted == 0) {
        std::stringstream ss;
        ss << "Handshake rejected: " << response->server_info;
        Logger::log(Logger::LogLevel::ERROR_LEVEL, ss.str());
        return false;
    }
    
    Logger::log(Logger::LogLevel::INFO, "Handshake successful, assigned client ID: " + std::to_string(response->assigned_id));
    return true;
}

bool StreamClient::sendHeartbeat() {
    PacketHeader header;
    header.magic = MAGIC_NUMBER;
    header.version = PROTOCOL_VERSION;
    header.packet_type = static_cast<uint8_t>(PacketType::HEARTBEAT);
    header.flags = 0;
    header.sequence_number = 0;
    header.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    header.payload_size = 0;
    
    if (send(socket_, reinterpret_cast<const char*>(&header), sizeof(header), 0) != sizeof(header)) {
        return false;
    }
    
    return true;
}

bool StreamClient::receivePacket(PacketHeader& header, std::vector<uint8_t>& payload) {
    // Receive header
    int received = recv(socket_, reinterpret_cast<char*>(&header), sizeof(header), MSG_WAITALL);
    if (received != sizeof(header)) {
        return false;
    }
    
    // Validate header
    if (header.magic != MAGIC_NUMBER) {
        Logger::log(Logger::LogLevel::ERROR_LEVEL, "Invalid magic number");
        return false;
    }
    
    // Receive payload if present
    if (header.payload_size > 0) {
        payload.resize(header.payload_size);
        received = recv(socket_, reinterpret_cast<char*>(payload.data()), header.payload_size, MSG_WAITALL);
        if (received != static_cast<int>(header.payload_size)) {
            return false;
        }
    } else {
        payload.clear();
    }
    
    bytes_received_ += sizeof(header) + header.payload_size;
    return true;
}

void StreamClient::receiveLoop() {
    Logger::log(Logger::LogLevel::INFO, "Receive loop started");
    
    while (connected_) {
        PacketHeader header;
        std::vector<uint8_t> payload;
        
        if (!receivePacket(header, payload)) {
            if (connected_) {
                Logger::log(Logger::LogLevel::ERROR_LEVEL, "Failed to receive packet");
                connected_ = false;
            }
            break;
        }
        
        PacketType type = static_cast<PacketType>(header.packet_type);
        
        switch (type) {
            case PacketType::VIDEO_FRAME:
                handleVideoFrame(header, payload);
                break;
                
            case PacketType::AUDIO_FRAME:
                handleAudioFrame(header, payload);
                break;
                
            case PacketType::DISCONNECT:
                Logger::log(Logger::LogLevel::INFO, "Server requested disconnect");
                connected_ = false;
                break;
                
            case PacketType::HEARTBEAT:
                // Heartbeat received, no action needed
                break;
                
            case PacketType::ACK:
                // ACK received, no action needed (could be used for reliability in the future)
                break;
                
            default:
                Logger::log(Logger::LogLevel::WARN, "Unknown packet type: " + std::to_string(header.packet_type));
                break;
        }
    }
    
    Logger::log(Logger::LogLevel::INFO, "Receive loop ended");
}

void StreamClient::heartbeatLoop() {
    Logger::log(Logger::LogLevel::INFO, "Heartbeat loop started");
    
    while (connected_) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        
        if (connected_) {
            if (!sendHeartbeat()) {
                Logger::log(Logger::LogLevel::ERROR_LEVEL, "Failed to send heartbeat");
                connected_ = false;
                break;
            }
        }
    }
    
    Logger::log(Logger::LogLevel::INFO, "Heartbeat loop ended");
}

void StreamClient::handleVideoFrame(const PacketHeader& header, const std::vector<uint8_t>& payload) {
    // Video frame format: VideoFrameHeader + pixel data
    struct VideoFrameHeader {
        uint32_t frame_number;
        uint16_t width;
        uint16_t height;
        uint8_t quality;
        uint8_t padding;
        uint64_t timestamp;
    };
    
    if (payload.size() < sizeof(VideoFrameHeader)) {
        Logger::log(Logger::LogLevel::ERROR_LEVEL, "Invalid video frame size");
        return;
    }
    
    const VideoFrameHeader* frame_header = reinterpret_cast<const VideoFrameHeader*>(payload.data());
    
    // Create VideoFrame
    VideoFrame frame;
    frame.frame_number = frame_header->frame_number;
    frame.width = frame_header->width;
    frame.height = frame_header->height;
    frame.quality = frame_header->quality;
    frame.timestamp = frame_header->timestamp;
    
    // Extract pixel data
    std::vector<uint8_t> frame_data(payload.begin() + sizeof(VideoFrameHeader), payload.end());
    frame.data = frame_data;
    
    video_frames_received_++;
    
    if (video_callback_) {
        video_callback_(frame, frame_data);
    }
}

void StreamClient::handleAudioFrame(const PacketHeader& header, const std::vector<uint8_t>& payload) {
    if (payload.size() < sizeof(AudioFrame)) {
        Logger::log(Logger::LogLevel::ERROR_LEVEL, "Invalid audio frame size");
        return;
    }
    
    const AudioFrame* frame = reinterpret_cast<const AudioFrame*>(payload.data());
    
    // Extract frame data
    std::vector<uint8_t> frame_data(payload.begin() + sizeof(AudioFrame), payload.end());
    
    audio_frames_received_++;
    
    if (audio_callback_) {
        audio_callback_(*frame, frame_data);
    }
}
