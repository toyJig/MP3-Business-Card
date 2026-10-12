#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <numbers>
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
// #include "hardware/adc.h"

#include "DAC.hpp"
#include "Mp3Player.hpp"

// Generated in CMakeLists as a stable embedded binary blob.
#include "mp3dec.h"
extern "C" const unsigned char audio_mp3[];
extern "C" const size_t audio_mp3_size;

#define MAX_PCM_SAMPLES (1152 * 2) // mp3 format specifies 1152 samples/s & Stereo (2 channels)

#define SPI_PORT spi0
#define PIN_CS   8
#define PIN_MOSI 7
#define PIN_SCK  6

//init DAC controller
double Amplitude{ 1.0 };
DAC_C DAC1 {DAC_C(SPI_PORT, PIN_CS, PIN_MOSI, PIN_SCK, Amplitude)};
//init Mp3Player
Mp3Player Mp3Player1 { Mp3Player(DAC1) };

void core1Wrapper(){
    Mp3Player1.playMusicFromPCM();
}

int main() {
    stdio_init_all();

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
    int16_t pcm_buffer[MAX_PCM_SAMPLES];
    const unsigned char* mp3_bytes_ptr {audio_mp3};
    const size_t mp3_size {audio_mp3_size};
    unsigned char* read_ptr = {(unsigned char*)mp3_bytes_ptr};
    int bytes_left {mp3_size};

    HMP3Decoder hMP3Decoder {MP3InitDecoder()};
    if (!hMP3Decoder) {
        printf("Error: MP3 decoder initialization failed\n");
        return -1;
    }

    
    MP3FrameInfo frame_info;

    while (bytes_left > 0) {
        const uint16_t queueSize = Mp3Player1.getPCMQueueSize();
        if (Mp3Player1.getPCMQueueSize() < 100){
            do {
                int sync_offset {MP3FindSyncWord(read_ptr, bytes_left)};
                if (sync_offset < 0) {
                    break; // No more sync words, can't continue mp3.
                }
                read_ptr += sync_offset;
                bytes_left -= sync_offset;

                //decode 1 frame
                int err {MP3Decode(hMP3Decoder, &read_ptr, &bytes_left, pcm_buffer, 0)};
                if (err == ERR_MP3_NONE){
                    MP3GetLastFrameInfo(hMP3Decoder, &frame_info);
                    int total_samples {frame_info.outputSamps};
                    
                    //send data to Mp3Player PWM Queue
                    Mp3Player1.addPCMToQueue(pcm_buffer, frame_info);
                } else if (err == ERR_MP3_INDATA_UNDERFLOW){
                    break;
                } else { // other mp3 errors -> skip
                    if (bytes_left > 0) {
                        read_ptr++;
                        bytes_left--;
                    }
                }
            } while (queueSize < 1000);

            if (!Mp3Player1.isPlaying){
                multicore_launch_core1(core1Wrapper);
            }
        }
    }
    
    MP3FreeDecoder(hMP3Decoder);

    printf("SONG DONE!!!\n");
    fflush(stdout);
}