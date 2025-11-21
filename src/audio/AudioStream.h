#ifndef AUDIOSTREAM_H
#define AUDIOSTREAM_H

#include <vector>
#include <cstdint>
#include <memory>

class AudioStream {
public:
    AudioStream();
    ~AudioStream();

    void startStreaming();
    void stopStreaming();
    void processAudioData(const std::vector<int16_t>& audioData);

private:
    void initialize();
    void cleanup();
    void streamAudio();

    bool isStreaming;
    std::unique_ptr<std::thread> streamingThread;
};

#endif // AUDIOSTREAM_H