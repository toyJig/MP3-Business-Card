#include "Mp3Player.hpp"

#include "pico/stdlib.h"
// #include <stdio.h>

Mp3Player::Mp3Player(DAC_C DAC) : DAC(DAC), isPlaying(false){};


uint16_t Mp3Player::mult_uint16_to_12 (uint16_t n16bits){
    return static_cast<uint16_t>(static_cast<double>(n16bits) / 65535.0 * 4095.0);
}

void Mp3Player::playMusicFromPCM(){
    // const uint32_t sample_period_us {
    //     1000000u / static_cast<uint32_t>(frame_info.samprate)
    // };
    isPlaying = true;
    while (true) {
        if (pcm_queue.size() > 0){
            DAC.write_to_DAC(DAC::DAC_config_ON_regA, pcm_queue.front());
            pcm_queue.pop();
        }
    }
}
void Mp3Player::addPCMToQueue(int16_t pcmBuffer[], MP3FrameInfo frame_info){
    if (frame_info.samprate < 0) { return; }

    int channels {frame_info.nChans};
    for (int i = 0; i < frame_info.outputSamps; i++){
        uint16_t data12bit {mult_uint16_to_12(pcmBuffer[i])};

        pcm_queue.push(data12bit);

        if (channels == 2){ //TODO: skip playing stereo music for now. 
            i++;
        }
    }
}

uint16_t Mp3Player::getPCMQueueSize(){
    return pcm_queue.size();
}


