#include "Mp3Player.hpp"

#include "pico/stdlib.h"
#include <stdio.h>
#include <algorithm>
#include <hardware/dma.h>

Mp3Player::Mp3Player(DAC_C DAC, std::array<uint16_t, MAX_PCM_SAMPLES> & pcm_queue1, std::array<uint16_t, MAX_PCM_SAMPLES> & pcm_queue2) 
    : DAC(DAC), pcm_queue1(pcm_queue1), pcm_queue2(pcm_queue2), isPlaying(false), buffer_1_needs_refill(true), buffer_2_needs_refill(true){};

void Mp3Player::init(){
    return;
}

uint16_t Mp3Player::mult_uint16_to_12 (int16_t n16bits){
    return static_cast<uint16_t>((static_cast<double>(n16bits) + 32768.0) / 65535.0 * 4095.0);
}

void Mp3Player::play(int control_channel){
    isPlaying = true;
    dma_start_channel_mask(1u << control_channel);
}

void Mp3Player::addPCMToQueue(int16_t pcmBuffer[], MP3FrameInfo frame_info){
    if (frame_info.samprate < 0) { return; }

    int channels {frame_info.nChans};
    while (!buffer_1_needs_refill && !buffer_2_needs_refill){
        busy_wait_us(0);
    }
    if (buffer_1_needs_refill){
        for (int i = 0; i < MAX_PCM_SAMPLES; i++){
            uint16_t data12bit {mult_uint16_to_12(pcmBuffer[i])};
            pcm_queue1[i] = DAC::DAC_config_ON_regA | data12bit; // just channel 1 A for now
            // if (channels == 2){ //TODO: ONLY TESTING WITH MONO MUSIC FOR NOW
            //     i++;
            // }
        }
        buffer_1_needs_refill = false;
    } else if (buffer_2_needs_refill){
        for (int i = 0; i < MAX_PCM_SAMPLES; i++){
            uint16_t data12bit {mult_uint16_to_12(pcmBuffer[i])};
            pcm_queue2[i] = DAC::DAC_config_ON_regA | data12bit; // just channel 1 A for now
            // if (channels == 2){ //TODO: ONLY TESTING WITH MONO MUSIC FOR NOW
            //     i++;
            // }
        }
        buffer_2_needs_refill = false;
    }
}

// uint16_t Mp3Player::getPCMQueueSize(){
//     return pcm_queue1.size();
// }


