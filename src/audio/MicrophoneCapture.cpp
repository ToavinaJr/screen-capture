#include "MicrophoneCapture.h"
#include "../utils/Logger.h"
#include <iostream>
#include <cstring>

MicrophoneCapture::MicrophoneCapture() 
    : deviceID(0), isCapturing(false), userCallback(nullptr) {
    
    // Initialize SDL Audio subsystem if not already done
    if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            std::string msg = "Failed to initialize SDL Audio: " + std::string(SDL_GetError());
            Logger::log(Logger::LogLevel::WARN, msg);
        }
    }
    
    Logger::log(Logger::LogLevel::INFO, "MicrophoneCapture created");
}

MicrophoneCapture::~MicrophoneCapture() {
    if (isCapturing) {
        stopCapture();
    }
    Logger::log(Logger::LogLevel::INFO, "MicrophoneCapture destroyed");
}

void MicrophoneCapture::audioCallbackWrapper(void* userdata, Uint8* stream, int len) {
    MicrophoneCapture* capture = static_cast<MicrophoneCapture*>(userdata);
    if (capture) {
        capture->processAudioData(stream, len);
    }
}

void MicrophoneCapture::processAudioData(const Uint8* stream, int len) {
    std::lock_guard<std::mutex> lock(captureMutex);
    
    if (userCallback && isCapturing) {
        userCallback(stream, len);
    }
}

bool MicrophoneCapture::startCapture(const std::function<void(const void*, size_t)>& audioCallback) {
    if (isCapturing) {
        Logger::log(Logger::LogLevel::WARN, "Microphone capture already started");
        return false;
    }

    userCallback = audioCallback;

    // Configure desired audio specification
    SDL_zero(desiredSpec);
    desiredSpec.freq = 44100;           // 44.1kHz sample rate
    desiredSpec.format = AUDIO_F32SYS;  // 32-bit float samples
    desiredSpec.channels = 1;           // Mono
    desiredSpec.samples = 4096;         // Buffer size
    desiredSpec.callback = audioCallbackWrapper;
    desiredSpec.userdata = this;

    // Open audio device for recording
    deviceID = SDL_OpenAudioDevice(
        nullptr,                         // Use default recording device
        1,                              // Non-zero for recording
        &desiredSpec,
        &obtainedSpec,
        0                               // No allowed changes
    );

    if (deviceID == 0) {
        std::string msg = "Failed to open audio device: " + std::string(SDL_GetError());
        Logger::log(Logger::LogLevel::WARN, msg);
        return false;
    }

    // Log obtained audio specs
    std::string msg = "Audio device opened - Freq: " + std::to_string(obtainedSpec.freq) +
                     " Hz, Channels: " + std::to_string(obtainedSpec.channels) +
                     ", Format: " + std::to_string(obtainedSpec.format) +
                     ", Samples: " + std::to_string(obtainedSpec.samples);
    Logger::log(Logger::LogLevel::INFO, msg);

    // Check if we need to request microphone permission (platform specific)
#ifdef __ANDROID__
    // On Android, you would need to request RECORD_AUDIO permission
    Logger::log(Logger::LogLevel::INFO, "Note: Microphone permission should be requested on Android");
#elif defined(__APPLE__)
    // On macOS/iOS, system will automatically prompt for microphone access
    Logger::log(Logger::LogLevel::INFO, "System will prompt for microphone access");
#endif

    // Start audio capture
    SDL_PauseAudioDevice(deviceID, 0);  // 0 = unpause (start)
    isCapturing = true;

    Logger::log(Logger::LogLevel::INFO, "Microphone capture started successfully");
    return true;
}

void MicrophoneCapture::stopCapture() {
    if (!isCapturing) {
        Logger::log(Logger::LogLevel::WARN, "Microphone capture not running");
        return;
    }

    // Pause audio capture
    if (deviceID != 0) {
        SDL_PauseAudioDevice(deviceID, 1);  // 1 = pause (stop)
        SDL_CloseAudioDevice(deviceID);
        deviceID = 0;
    }

    {
        std::lock_guard<std::mutex> lock(captureMutex);
        isCapturing = false;
        userCallback = nullptr;
    }

    Logger::log(Logger::LogLevel::INFO, "Microphone capture stopped");
}
