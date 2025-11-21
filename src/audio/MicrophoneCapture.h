#ifndef MICROPHONECAPTURE_H
#define MICROPHONECAPTURE_H

#include <string>
#include <functional>
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <mutex>
#include <vector>

class MicrophoneCapture {
public:
    MicrophoneCapture();
    ~MicrophoneCapture();

    bool startCapture(const std::function<void(const void*, size_t)>& audioCallback);
    void stopCapture();
    bool isRecording() const { return isCapturing; }

private:
    static void audioCallbackWrapper(void* userdata, Uint8* stream, int len);
    void processAudioData(const Uint8* stream, int len);

    SDL_AudioDeviceID deviceID;
    SDL_AudioSpec desiredSpec;
    SDL_AudioSpec obtainedSpec;
    
    bool isCapturing;
    std::mutex captureMutex;
    std::function<void(const void*, size_t)> userCallback;
};

#endif // MICROPHONECAPTURE_H