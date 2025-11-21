#ifndef CONFIG_H
#define CONFIG_H

// Configuration constants
constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;
constexpr const char* WINDOW_TITLE = "Multimedia Streaming App";

// Audio configuration
constexpr int AUDIO_SAMPLE_RATE = 44100;
constexpr int AUDIO_CHANNELS = 2;
constexpr int AUDIO_BUFFER_SIZE = 512;

// Network configuration
constexpr int SERVER_PORT = 12345;
constexpr const char* SERVER_ADDRESS = "127.0.0.1";

// TLS/SSL configuration
constexpr const char* TLS_CERTIFICATE_FILE = "path/to/certificate.pem";
constexpr const char* TLS_PRIVATE_KEY_FILE = "path/to/private_key.pem";

// Microphone capture configuration
constexpr bool ENABLE_MICROPHONE_CAPTURE = true;

#endif // CONFIG_H