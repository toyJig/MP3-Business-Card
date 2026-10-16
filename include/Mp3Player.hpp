#pragma once
#include <cstdint>
#include <queue>
#include "DAC.hpp"
#include "mp3dec.h"
#include "pico/mutex.h"

#define MAX_PCM_SAMPLES (1152 * 2) // mp3 format specifies 1152 samples/s & Stereo (2 channels)
#define MAX_PCM_QUEUE 60000
class Mp3Player {
private:
    std::queue<uint16_t> pcm_queue;
    mutex_t pcm_queue_mutex;
    DAC_C DAC;

    uint16_t mult_uint16_to_12(int16_t n16bits);
public:
    bool isPlaying;
    bool initiated;
    uint32_t samp_interval_us;


    Mp3Player(DAC_C DAC);
    
    void init();
    void setSampRate(MP3FrameInfo frame_info);
    
    void playMusicFromPCM();

    struct repeating_timer rt;
    static bool timer_callback_wrapper(struct repeating_timer *rt);
    bool playOnePCMSample();

    void stopMusic();
    void addPCMToQueue(std::array<int16_t, MAX_PCM_SAMPLES> pcm_buffer, MP3FrameInfo frame_info);
    bool tryPopPCM(uint16_t& sample);
    bool tryPushPCM(uint16_t& sample);
    uint16_t getPCMQueueSize();
};

