#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <numbers>
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include <array>
#include "mcp4822.pio.h"

// #include "hardware/adc.h"

#include "DAC.hpp"
#include "Mp3Player.hpp"

// Generated in CMakeLists as a stable embedded binary blob.
#include "mp3dec.h"
extern "C" const unsigned char audio_mp3[];
extern "C" const size_t audio_mp3_size;

#define SPI_PORT spi0
#define PIN_SCK  6
#define PIN_CS   7
#define PIN_MOSI 8

//init DAC and Mp3Player class
double Amplitude{ 1.0 };
DAC_C DAC1 {DAC_C(SPI_PORT, PIN_CS, PIN_MOSI, PIN_SCK, Amplitude)};

std::array<uint16_t, MAX_PCM_SAMPLES> PCM_buffer1;
std::array<uint16_t, MAX_PCM_SAMPLES> PCM_buffer2;
Mp3Player Mp3Player1 { Mp3Player(DAC1, PCM_buffer1, PCM_buffer2) };

//variables that need to be global FOR NOW.
MP3FrameInfo frame_info;
bool stillDecoding = true;

//using dma channels 0 and 1 /12
int data_chan_1 = 0;
int data_chan_2 = 1;

void core1Wrapper(){
    Mp3Player1.playMusicFromPCM(frame_info);
}

void DMA_done_handler(){
    if (dma_hw->ints0 & (1u << data_chan_1)){
        Mp3Player1.buffer_1_needs_refill = true;
        dma_hw->ints0 = (1u << data_chan_1);
        dma_channel_set_read_addr(data_chan_1, PCM_buffer1.data(), false);
        dma_channel_set_transfer_count(data_chan_1, MAX_PCM_SAMPLES, false);
        // if (!stillDecoding){
        //     dma_channel_abort(data_chan_1);
        //     dma_channel_set_irq0_enabled(data_chan_1, false);
        // }
    }
    if (dma_hw->ints0 & (1u << data_chan_2)){
        Mp3Player1.buffer_2_needs_refill = true;
        dma_hw->ints0 = (1u << data_chan_2);
        dma_channel_set_read_addr(data_chan_2, PCM_buffer2.data(), false);
        dma_channel_set_transfer_count(data_chan_2, MAX_PCM_SAMPLES, false);
        // if (!stillDecoding){
        //     dma_channel_abort(data_chan_2);
        //     dma_channel_set_irq0_enabled(data_chan_2, false);
        // }
    }
}

int main() {
    stdio_init_all();

    // PIO STATE MACHINE
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &mcp_4822_spi_program);
    pio_sm_config pio_config = mcp_4822_spi_program_get_default_config(offset);
    uint sm = pio_claim_unused_sm(pio, true);

    sm_config_set_out_pins(&pio_config, PIN_MOSI, 1);
    sm_config_set_sideset_pins(&pio_config, PIN_SCK);
    pio_gpio_init(pio, PIN_CS);
    pio_gpio_init(pio, PIN_SCK);
    pio_gpio_init(pio, PIN_MOSI);

    pio_sm_set_consecutive_pindirs(pio, sm, PIN_SCK, 2, true);
    pio_sm_set_consecutive_pindirs(pio, sm, PIN_MOSI, 1, true);

    sm_config_set_out_shift(&pio_config, false, true, 16);
    float div = (float)clock_get_hz(clk_sys) / (2e7 * 2); //20MHz "SPI" since pio takes 2 cycles
    sm_config_set_clkdiv(&pio_config, div);
    pio_sm_init(pio, sm, offset, &pio_config);
    pio_sm_set_enabled(pio, sm, true);


    //DMA MACHINE

    // (X/Y)*sys_clk, where X is the first 16 bytes and Y is the second
    // sys_clk is 125 MHz unless changed in code. Configured to ~44 kHz. (23/65535)*sys_clk Hz CHANGE WHEN UNDERCLOCKING.
    dma_timer_set_fraction(0, 0x0017, 0xffff);

    //set up data channel 1
    dma_channel_config config1 = dma_channel_get_default_config(data_chan_1);
    channel_config_set_transfer_data_size(&config1, DMA_SIZE_16);
    channel_config_set_read_increment(&config1, true);
    channel_config_set_write_increment(&config1, false);
    channel_config_set_chain_to(&config1, data_chan_2);
    channel_config_set_dreq(&config1, 0x3b); // 0x3b means timer0 (see SDK manual)
    
    dma_channel_set_irq0_enabled(data_chan_1, true);
    dma_channel_configure(
        data_chan_1,                // Channel to be configured
        &config1,                   // The configuration we just created
        &pio->txf[sm],              // write address (SPI data register)
        PCM_buffer1.data(),         // The initial read address. Equiv to &arr[0]
        MAX_PCM_SAMPLES,            // Number of transfers
        false                       // Don't start immediately.
    );

    //set up data channel 2
    dma_channel_config config2 = dma_channel_get_default_config(data_chan_2);
    channel_config_set_transfer_data_size(&config2, DMA_SIZE_16);
    channel_config_set_read_increment(&config2, true);
    channel_config_set_write_increment(&config2, false);
    channel_config_set_chain_to(&config2, data_chan_1);
    channel_config_set_dreq(&config2, 0x3b); // 0x3b means timer0 (see SDK manual)
    
    dma_channel_set_irq0_enabled(data_chan_2, true);
    dma_channel_configure(
        data_chan_2,                // Channel to be configured
        &config2,                   // The configuration we just created
        &pio->txf[sm],              // write address (SPI data register)
        PCM_buffer2.data(),         // The initial read address. Equiv to &arr[0]
        MAX_PCM_SAMPLES,            // Number of transfers
        false                       // Don't start immediately.
    );

    //connect done interrupt to DMA_done_handler
    irq_set_exclusive_handler(DMA_IRQ_0, DMA_done_handler);
    irq_set_enabled(DMA_IRQ_0, true);

    
    // Metro RP2040 A0 pin for debugging
    // adc_init();
    // adc_gpio_init(26);  
    // adc_select_input(0);

    sleep_ms(2000); // wait for usb serial monitor

    //init helix mp3 decoder
    int16_t pcm_decoder_buffer[MAX_PCM_SAMPLES];
    const unsigned char* mp3_bytes_ptr {audio_mp3};
    const size_t mp3_size {audio_mp3_size};
    unsigned char* read_ptr = {(unsigned char*)mp3_bytes_ptr};
    int bytes_left {static_cast<int>(mp3_size)};

    HMP3Decoder hMP3Decoder {MP3InitDecoder()};
    if (!hMP3Decoder) {
        printf("Error: MP3 decoder initialization failed\n");
        return -1;
    }
    while (bytes_left > 0) {
        // const uint16_t queueSize = Mp3Player1.getPCMQueueSize();
        int sync_offset {MP3FindSyncWord(read_ptr, bytes_left)};
        if (sync_offset < 0) {
            break; // No more sync words, can't continue mp3.
        }
        read_ptr += sync_offset;
        bytes_left -= sync_offset;

        //decode 1 frame
        int err {MP3Decode(hMP3Decoder, &read_ptr, &bytes_left, pcm_decoder_buffer, 0)};
        if (err == ERR_MP3_NONE){
            MP3GetLastFrameInfo(hMP3Decoder, &frame_info);
            int total_samples {frame_info.outputSamps};
            
            //send data to Mp3Player PCM buffers. Stalls if both buffers are full.
            Mp3Player1.addPCMToQueue(pcm_decoder_buffer, frame_info);
            if (!Mp3Player1.isPlaying){
                Mp3Player1.play(data_chan_1);
            }
        } else if (err == ERR_MP3_INDATA_UNDERFLOW){
            break;
        } else { // other mp3 errors -> skip
            if (bytes_left > 0) {
                read_ptr++;
                bytes_left--;
            }
        }
    }
    
    stillDecoding = false;
    irq_set_enabled(DMA_IRQ_0, false);
    dma_channel_set_irq0_enabled(data_chan_1, false);
    dma_channel_set_irq0_enabled(data_chan_2, false);
    dma_channel_abort(data_chan_1);
    dma_channel_abort(data_chan_2);
    pio_sm_set_enabled(pio, sm, false);

    MP3FreeDecoder(hMP3Decoder);

    printf("Decoding done!\n");
}