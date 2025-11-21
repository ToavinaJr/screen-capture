#include <gtest/gtest.h>
#include "../src/audio/MicrophoneCapture.h"
#include "../src/audio/AudioStream.h"

class AudioCaptureTest : public ::testing::Test {
protected:
    MicrophoneCapture* micCapture;
    AudioStream* audioStream;

    void SetUp() override {
        micCapture = new MicrophoneCapture();
        audioStream = new AudioStream();
    }

    void TearDown() override {
        delete micCapture;
        delete audioStream;
    }
};

TEST_F(AudioCaptureTest, StartCapture) {
    ASSERT_NO_THROW(micCapture->start());
    EXPECT_TRUE(micCapture->isCapturing());
}

TEST_F(AudioCaptureTest, StopCapture) {
    micCapture->start();
    ASSERT_NO_THROW(micCapture->stop());
    EXPECT_FALSE(micCapture->isCapturing());
}

TEST_F(AudioCaptureTest, StreamAudio) {
    micCapture->start();
    ASSERT_NO_THROW(audioStream->stream(micCapture->getAudioData()));
    micCapture->stop();
}