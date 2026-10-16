#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <numbers>
#include <array>
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
// #include "hardware/adc.h"

#include "DAC.hpp"
#include "Mp3Player.hpp"

// Generated in CMakeLists as a stable embedded binary blob.
#include "mp3dec.h"
extern "C" const unsigned char audio_mp3[];
extern "C" const size_t audio_mp3_size;

#define SPI_PORT spi0
#define PIN_CS   8
#define PIN_MOSI 7
#define PIN_SCK  6

//init DAC controller
double Amplitude{ 1.0 };
DAC_C DAC1 {DAC_C(SPI_PORT, PIN_CS, PIN_MOSI, PIN_SCK, Amplitude)};
//init Mp3Player
Mp3Player Mp3Player1 { Mp3Player(DAC1) };
MP3FrameInfo frame_info;

bool core1Launched = false;
void core1Wrapper(){
    // while (Mp3Player1.getPCMQueueSize() < MAX_PCM_SAMPLES){
    //     busy_wait_ms(10);
    //     printf("core1: %u\n", Mp3Player1.getPCMQueueSize());
    // }
    printf("initing playback loop\n");
    while (true){
        Mp3Player1.playMusicFromPCM();
    }
}

int main() {
    stdio_init_all();
    multicore_reset_core1();

    spi_init(SPI_PORT, 2e6);  // 2MHz SPI
    spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_init(PIN_CS);
    gpio_init(PIN_MOSI);
    gpio_init(PIN_SCK);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_set_dir(PIN_MOSI, GPIO_OUT);
    gpio_set_dir(PIN_SCK, GPIO_OUT);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    // Metro RP2040 A0 pin for debugging
    // adc_init();
    // adc_gpio_init(26);  
    // adc_select_input(0);

    sleep_ms(2000);
    
    DAC1.init();

    //init helix mp3 decoder
    std::array<int16_t, MAX_PCM_SAMPLES> pcm_buffer;
    const unsigned char* mp3_bytes_ptr {audio_mp3};
    const size_t mp3_size {audio_mp3_size};
    unsigned char* read_ptr = {(unsigned char*)mp3_bytes_ptr};
    int bytes_left { static_cast<int>(mp3_size) };

    HMP3Decoder hMP3Decoder {MP3InitDecoder()};
    if (!hMP3Decoder) {
        printf("Error: MP3 decoder initialization failed\n");
        return -1;
    }

    struct repeating_timer timer;
    add_repeating_timer_us(9, Mp3Player1.timer_callback_wrapper, &Mp3Player1, &timer);

    
    printf("initing decode loop\n");
    bool done {false};
    while (bytes_left > 0 && done == false) {
        uint16_t queueSize {Mp3Player1.getPCMQueueSize()};
        printf("queue size: %u\n", queueSize);
        while (done == false) {
            int sync_offset {MP3FindSyncWord(read_ptr, bytes_left)};
            if (sync_offset < 0) {
                done = true;
                printf("No more syn words found, can't continue\n");
                break; // No more sync words, can't continue mp3.
            }
            read_ptr += sync_offset;
            bytes_left -= sync_offset;
            
            //decode 1 frame
            int err {MP3Decode(hMP3Decoder, &read_ptr, &bytes_left, pcm_buffer.data(), 0)};
            if (err == ERR_MP3_NONE){
                MP3GetLastFrameInfo(hMP3Decoder, &frame_info);
                int total_samples {frame_info.outputSamps};
                
                //send data to Mp3Player PWM Queue
                Mp3Player1.addPCMToQueue(pcm_buffer, frame_info);
                if (!Mp3Player1.initiated){ Mp3Player1.setSampRate(frame_info); };
            } else if (err == ERR_MP3_INDATA_UNDERFLOW){
                printf("Data underflow\n");
                done = true;
                break;
            } else { // other mp3 errors -> skip
                if (bytes_left > 0) {
                    read_ptr++;
                    bytes_left--;
                }
            }
            queueSize = Mp3Player1.getPCMQueueSize();
            // printf("queue size: %u\n", queueSize);
            // sleep_ms(10);
        }
        // if (!core1Launched){
        //     core1Launched = true;
        //     multicore_launch_core1(core1Wrapper);
        // }
   }
    
    MP3FreeDecoder(hMP3Decoder);

    printf("Decoding done\n");
    fflush(stdout);
}