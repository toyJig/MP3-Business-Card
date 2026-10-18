#include <stdio.h>
#include "hardware/spi.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

#include "dac.hpp"
#include "mp3_player.hpp"

// Generated in CMakeLists as a stable embedded binary blob.
#include "mp3dec.h"
extern "C" const unsigned char audio_mp3[];
extern "C" const size_t audio_mp3_size;

#define SPI_PORT spi0
#define PIN_SCK 6
#define PIN_CS 7
#define PIN_MOSI 8

// init DAC and Mp3Player class
//  double Amplitude{ 0.05 };
double Amplitude{ 0.87 };
DAC_C DAC1{ DAC_C(SPI_PORT, PIN_CS, PIN_SCK, PIN_MOSI, Amplitude) };
Mp3Player Mp3Player1{ Mp3Player(DAC1, Amplitude) };

int main() {
    stdio_init_all();
    sleep_ms(2000);  // wait for usb serial monitor

    // init helix mp3 decoder
    int16_t pcm_decoder_buffer[MAX_PCM_SAMPLES];
    const unsigned char* mp3_bytes_ptr{ audio_mp3 };
    const size_t mp3_size{ audio_mp3_size };
    unsigned char* read_ptr = { (unsigned char*)mp3_bytes_ptr };
    int bytes_left{ static_cast<int>(mp3_size) };

    MP3FrameInfo frame_info;
    HMP3Decoder hMP3Decoder{ MP3_init_decoder() };
    if (!hMP3Decoder) {
        printf("Error: MP3 decoder initialization failed\n");
        return -1;
    }
    while (bytes_left > 0) {
        int sync_offset{ MP3_find_sync_word(read_ptr, bytes_left) };
        if (sync_offset < 0) {
            break;  // No more sync words, can't continue mp3.
        }
        read_ptr += sync_offset;
        bytes_left -= sync_offset;

        // decode 1 frame
        int err{ MP3_decode(hMP3Decoder, &read_ptr, &bytes_left, pcm_decoder_buffer, 0) };
        if (err == ERR_MP3_NONE) {
            MP3_get_last_frame_info(hMP3Decoder, &frame_info);
            int total_samples{ frame_info.outputSamps };

            // send data to Mp3Player PCM buffers. Stalls if both buffers are full.
            Mp3Player1.add_PCM_to_queue(pcm_decoder_buffer, frame_info);
            if (!Mp3Player1.isPlaying) {
                Mp3Player1.play(frame_info);
            }
        } else if (err == ERR_MP3_INDATA_UNDERFLOW) {
            break;
        } else {  // other mp3 errors -> skip
            if (bytes_left > 0) {
                read_ptr++;
                bytes_left--;
            }
        }
    }

    MP3_free_decoder(hMP3Decoder);
    Mp3Player1.stop();

    printf("SONG DONE! :D\n");
}