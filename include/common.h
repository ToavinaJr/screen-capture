#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <functional>
#include <cstdint>
#include <stdexcept>

// Détection de plateforme
#ifdef _WIN32
    #define PLATFORM_WINDOWS
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #define PLATFORM_LINUX
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    typedef int SOCKET;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

// SDL2 optionnel (seulement pour le client)
#ifdef USE_SDL
#include <SDL2/SDL.h>
#endif

#ifdef ENABLE_TLS
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

#ifdef ENABLE_AUDIO
#include <portaudio.h>
#endif

// Configuration globale
namespace Config {
    constexpr uint16_t DEFAULT_PORT = 9999;
    constexpr int DEFAULT_FPS = 30;
    constexpr int DEFAULT_JPEG_QUALITY = 80;
    constexpr int DEFAULT_AUDIO_SAMPLE_RATE = 44100;
    constexpr int MAX_CLIENTS = 10;
    constexpr size_t MAX_PACKET_SIZE = 65536;
    constexpr size_t THREAD_POOL_SIZE = 4;
}

// Types de paquets
enum class PacketType : uint8_t {
    HANDSHAKE = 0x01,
    VIDEO_FRAME = 0x02,
    AUDIO_FRAME = 0x03,
    DISCONNECT = 0x04,
    CONFIG = 0x05,
    HEARTBEAT = 0x06,
    ACK = 0x07
};

// En-tête de paquet
#pragma pack(push, 1)
struct PacketHeader {
    uint32_t magic;
    uint8_t version;
    uint8_t packet_type;
    uint16_t flags;
    uint32_t payload_size;
    uint32_t sequence_number;
    uint64_t timestamp;
};
#pragma pack(pop)

constexpr uint32_t MAGIC_NUMBER = 0x5343524E;
constexpr uint8_t PROTOCOL_VERSION = 1;

// Structure pour une frame vidéo
struct VideoFrame {
    uint32_t frame_number;
    uint16_t width;
    uint16_t height;
    uint8_t quality;
    std::vector<uint8_t> data;
    uint64_t timestamp;
};

// Structure pour une frame audio
struct AudioFrame {
    uint32_t frame_number;
    uint32_t sample_rate;
    uint16_t channels;
    std::vector<float> samples;
    uint64_t timestamp;
};

// Structure pour handshake
struct HandshakeRequest {
    char client_name[64];
    uint8_t capabilities;
    uint16_t max_width;
    uint16_t max_height;
};

struct HandshakeResponse {
    uint8_t accepted;
    uint16_t assigned_id;
    char server_info[128];
};

// Structure pour configuration
struct StreamConfig {
    uint16_t fps;
    uint8_t jpeg_quality;
    uint16_t audio_sample_rate;
    uint8_t audio_channels;
    uint8_t enable_audio;
    uint8_t enable_video;
};

// Utilitaires de temps
inline uint64_t get_timestamp_us() {
    using namespace std::chrono;
    return duration_cast<microseconds>(
        system_clock::now().time_since_epoch()
    ).count();
}

// Initialisation des sockets (Windows)
class SocketInitializer {
public:
    SocketInitializer() {
#ifdef PLATFORM_WINDOWS
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }
#endif
    }
    
    ~SocketInitializer() {
#ifdef PLATFORM_WINDOWS
        WSACleanup();
#endif
    }
};

#endif // COMMON_H