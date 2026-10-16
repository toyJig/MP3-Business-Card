#include "Mp3Player.hpp"

#include "pico/stdlib.h"
#include <stdio.h>
#include <algorithm>
#include <array>

Mp3Player::Mp3Player(DAC_C DAC) : DAC(DAC), isPlaying(false), initiated(false), samp_interval_us(22u){
    mutex_init(&pcm_queue_mutex);
};

void Mp3Player::setSampRate(const MP3FrameInfo frame_info){
    if (frame_info.samprate <= 0 || isPlaying) { return; }
    const uint32_t sample_period_us {
        1000000u / static_cast<uint32_t>(frame_info.samprate)
    };
    printf("Samp period set: %u\n", samp_interval_us);
    samp_interval_us = sample_period_us;
    initiated = true;
}

uint16_t Mp3Player::mult_uint16_to_12 (int16_t n16bits){
    return static_cast<uint16_t>((static_cast<double>(n16bits) + 32768.0) / 65535.0 * 4095.0);
}

void Mp3Player::playMusicFromPCM(){
    printf("Initing playback loop \n");
    isPlaying = true;

    uint16_t sample;
    while (isPlaying == true) {
        const uint32_t start_time { time_us_32() };
        if (tryPopPCM(sample)){
            DAC.write_to_DAC(DAC::DAC_config_ON_regA, sample);

            const uint32_t timeElapsed {time_us_32() - start_time};
            if (timeElapsed < samp_interval_us){
                busy_wait_us_32(std::max(static_cast<uint32_t>(0), samp_interval_us - timeElapsed));
            }
        }
    }
    
    printf("playback done\n");
    isPlaying = false;
}

bool Mp3Player::playOnePCMSample(){
    isPlaying = true;
    if (!initiated){ return true; }
    
    uint16_t sample;
    printf("queue size: %u\n", getPCMQueueSize());
    if (tryPopPCM(sample)){
        printf("Initing playback loop one pcm \n");
        DAC.write_to_DAC(DAC::DAC_config_ON_regA, sample);
    }
    isPlaying = false;
    return true;
}

bool Mp3Player::timer_callback_wrapper(struct repeating_timer *rt){
    Mp3Player* class_instance = static_cast<Mp3Player*>(rt->user_data);
    if(class_instance != nullptr){
        return class_instance->playOnePCMSample();
    }
    return false;
}

void Mp3Player::addPCMToQueue(std::array<int16_t, MAX_PCM_SAMPLES> pcmBuffer, MP3FrameInfo frame_info){
    if (frame_info.samprate < 0) { return; }

    int channels {frame_info.nChans};
    for (int i = 0; i < frame_info.outputSamps; i++){
        uint16_t data12bit {mult_uint16_to_12(pcmBuffer[i])};

        tryPushPCM(data12bit);

        if (channels == 2){ //TODO: skip playing stereo music for now. 
            i++;
        }
    }
    return;
}

void Mp3Player::stopMusic(){ isPlaying = false; }
// uint16_t Mp3Player::getPCMQueueSize(){ return pcm_queue.size(); }

bool Mp3Player::tryPopPCM(uint16_t& sample){
    mutex_enter_blocking(&pcm_queue_mutex);

    if (pcm_queue.empty()){
        mutex_exit(&pcm_queue_mutex);
        return false;
    }

    sample = pcm_queue.front();
    pcm_queue.pop();
    mutex_exit(&pcm_queue_mutex);
    return true;
}

bool Mp3Player::tryPushPCM(uint16_t& sample){
    mutex_enter_blocking(&pcm_queue_mutex);

    // if (pcm_queue.size() >= MAX_PCM_QUEUE){
    //     mutex_exit(&pcm_queue_mutex);
    //     return false;
    // }

    pcm_queue.push(sample);
    mutex_exit(&pcm_queue_mutex);
    return true;
}

uint16_t Mp3Player::getPCMQueueSize(){
    mutex_enter_blocking(&pcm_queue_mutex);
    const uint16_t size = static_cast<uint16_t>(pcm_queue.size());
    mutex_exit(&pcm_queue_mutex);
    return size;
}