#pragma once
#include <cstdint>
#include "DAC.hpp"
#include "mp3dec.h"
#include <array>
// #include <vector>
// #include <queue>

// inline constexpr int MAX_PCM_SAMPLES (1152 * 2); // mp3 format specifies 1152 samples/s & Stereo (2 channels)
inline constexpr int MAX_PCM_SAMPLES (1152); // mp3 format specifies 1152 samples/s & Stereo (2 channels)

class Mp3Player {
private:
    DAC_C DAC;
    uint16_t mult_uint16_to_12(int16_t n16bits);
    std::array<uint16_t, MAX_PCM_SAMPLES> & pcm_queue1;
    std::array<uint16_t, MAX_PCM_SAMPLES> & pcm_queue2;
    size_t pcm_queue_size;
public:
    bool isPlaying;
    volatile bool buffer_1_needs_refill;
    volatile bool buffer_2_needs_refill;

    Mp3Player(DAC_C DAC, std::array<uint16_t, MAX_PCM_SAMPLES>& pcm_queue1, std::array<uint16_t, MAX_PCM_SAMPLES> & pcm_queue2);
    
    void init();
    void play(int control_channel);
    void playMusicFromPCM(MP3FrameInfo frame_info);
    void addPCMToQueue(int16_t pcmBuffer[], MP3FrameInfo frame_info);

    // uint16_t getPCMQueueSize();
};

