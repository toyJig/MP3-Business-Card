
#include "DAC.hpp"

#include "hardware/gpio.h"
#include <math.h>
#include <stdio.h>
#include <numbers>

// Prefills sin_table with compputed 0-2^12 in 16 bit binary
void DAC_C::fill_sine_table() {
    double c{ two_pi / (SINE_SAMPLES - 1) };
    for (size_t i = 0; i < SINE_SAMPLES; i++) {
        sin_table[i] =
            static_cast<uint16_t>((std::sin(c * i) + 1) * (SINE_SAMPLES - 1) * Amplitude / 2);
    }
}

// Config should be 4 bits, data should be *12 bits* for MCP4822
void DAC_C::write_to_DAC(const uint16_t config, const uint16_t data) {
    uint8_t msg[2];

    msg[0] = config | ((data >> 8) & 0x0F);
    msg[1] = data & 0xFF;

    gpio_put(PIN_CS, 0);  // pull down cs pin
    spi_write_blocking(spi, msg, 2);
    gpio_put(PIN_CS, 1);
}

DAC_C::DAC_C(spi_inst_t* spi, int PIN_CS, int PIN_SCK, int PIN_MOSI, double Amplitude)
    : spi(spi), PIN_CS(PIN_CS), PIN_SCK(PIN_SCK), PIN_MOSI(PIN_MOSI), Amplitude(Amplitude) {};

void DAC_C::generate_AC_wave(double Hz) {
    printf("Sin val 1 = %u", sin_table[1]);

    uint32_t lastTime{ time_us_32() };
    while (true) {
        double period_us{ 1.0 / Hz * 1.0e6 };
        uint32_t time{ time_us_32() };
        uint32_t elapsedTime{ time - lastTime };
        if (elapsedTime > static_cast<uint32_t>(period_us)) {
            lastTime = time;
            elapsedTime -= period_us;
            // extreme unlikely case
            while (elapsedTime > static_cast<uint32_t>(period_us)) {
                lastTime = time_us_32();
                elapsedTime -= period_us;
            }
        }

        double periodProg{ static_cast<double>(elapsedTime) / period_us };
        uint16_t voltage_12bit{ sin_table[static_cast<int>(periodProg * (SINE_SAMPLES - 1))] };
        write_to_DAC(DAC::DAC_config_ON_regA, voltage_12bit);

        // read analog and print to teleplot
        // float adc_raw = adc_read();
        // float voltageReading = (adc_raw * 3.3f) / (SINE_SAMPLES - 1);
        // printf(">Waveform:%.3f\n", voltageReading);
        // printf(">Waveform:%.3u\n", voltage_12bit);
        // sleep_ms(1); // stall while loop
    }
}

void DAC_C::init() { fill_sine_table(); }
