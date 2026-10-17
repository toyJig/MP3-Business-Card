#pragma once

#include <vector>
#include <cstdint>
#include "DAC.hpp"
#include "mp3dec.h"
#include "hardware/pio.h"

// mp3 format specifies 1152 samples/s & Stereo (2 channels)
inline constexpr int MAX_PCM_SAMPLES (1152 * 2); 

class Mp3Player {
private:
    DAC_C DAC;
    inline static std::vector<uint16_t> pcm_buffer1;
    inline static std::vector<uint16_t> pcm_buffer2;
    inline static int outputSamps;
    double amplitude;
    
    // PIO STATE MACHINE
    PIO pio;
    uint offset;
    pio_sm_config pio_config;
    uint sm;
    
    static void DMA_done_handler();
    uint16_t mult_uint16_to_12(int16_t n16bits);
public:
    bool isPlaying;
    inline static volatile bool buffer_1_needs_refill;
    inline static volatile bool buffer_2_needs_refill;

    Mp3Player(DAC_C DAC, double amplitude = 0.10);

    void init();
    void play(MP3FrameInfo frame_info);
    void stop();
    void addPCMToQueue(int16_t pcmBuffer[], MP3FrameInfo frame_info);
};
