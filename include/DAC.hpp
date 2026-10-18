#pragma once

#include "hardware/spi.h"
#include "pico/stdlib.h"
#include <cstdint>

inline constexpr int SINE_SAMPLES{ 4096 };
inline constexpr double two_pi{ 3.1415926535898 * 2.0 };

class DAC_C {
private:
    inline static uint16_t sin_table[SINE_SAMPLES];
    double Amplitude;

    void fill_sine_table();

public:
    spi_inst_t* spi;
    int PIN_CS;
    int PIN_SCK;
    int PIN_MOSI;

    DAC_C(spi_inst_t* spi, int PIN_CS, int PIN_SCK, int PIN_MOSI, double Amplitude);
    void generate_AC_wave(double Hz);
    void write_to_DAC(const uint16_t config, const uint16_t data);
    void init();
};

namespace DAC {
    // DAC bit configurations
    inline constexpr uint16_t DAC_config_ON_regA{ 0b0011'0000'0000'0000 };
    inline constexpr uint16_t DAC_config_ON_regB{ 0b1011'0000'0000'0000 };
    inline constexpr uint16_t DAC_config_OFF_regA{ 0b0000'0000'0000'0000 };
    inline constexpr uint16_t DAC_config_OFF_regB{ 0b1000'0000'0000'0000 };

    // notes
    inline constexpr double C4{ 261.63 };
    inline constexpr double Cs4{ 277.18 };
    inline constexpr double Df4{ 277.18 };
    inline constexpr double D4{ 293.66 };
    inline constexpr double Ds4{ 311.13 };
    inline constexpr double Ef4{ 311.13 };
    inline constexpr double E4{ 329.63 };
    inline constexpr double F4{ 349.23 };
    inline constexpr double Fs4{ 370.00 };
    inline constexpr double Gf4{ 370.00 };
    inline constexpr double G4{ 392.00 };
    inline constexpr double Gs4{ 415.30 };
    inline constexpr double Af4{ 415.30 };
    inline constexpr double A4{ 440.00 };
    inline constexpr double As4{ 466.16 };
    inline constexpr double Bf4{ 466.16 };
    inline constexpr double B4{ 493.88 };
    inline constexpr double C5{ 523.25 };
}  // namespace DAC