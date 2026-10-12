#pragma once
#include <cstdint>
#include <queue>
#include "DAC.hpp"
#include "mp3dec.h"

uint16_t mult_uint16_to_12 (uint16_t n16bits);
class Mp3Player {
private:
    std::queue<uint16_t> pcm_queue;
    DAC_C DAC;

    uint16_t mult_uint16_to_12(uint16_t n16bits);
public:
    bool isPlaying;

    Mp3Player(DAC_C DAC);
    
    void init();
    void playMusicFromPCM();
    void addPCMToQueue(int16_t pcmBuffer[], MP3FrameInfo frame_info);

    uint16_t getPCMQueueSize();
};

