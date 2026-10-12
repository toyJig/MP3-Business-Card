#pragma once

#include "hardware/spi.h"
#include "pico/stdlib.h"

#include <cstdint>

#define SINE_SAMPLES 4096  // 2^12
inline constexpr double two_pi{ 3.1415926535898 * 2 };

class DAC_C {
private:
    inline static uint16_t sin_table[SINE_SAMPLES];
    double Amplitude;

    spi_inst_t* spi;
    int PIN_CS;
    int PIN_SDI;
    int PIN_SCK;

    void fillSineTable();
    
public:
    DAC_C(spi_inst_t* spi, int PIN_CS, int PIN_SDI, int PIN_SCK, double Amplitude);
    void generateAC_Wave(double Hz);
    void write_to_DAC(const uint8_t config, const uint16_t data);
    void init();
};

namespace DAC {
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

    inline constexpr uint8_t DAC_config_ON_regA{ 0b0001'0000 };
    inline constexpr uint8_t DAC_config_ON_regB{ 0b1001'0000 };
    inline constexpr uint8_t DAC_config_OFF_regA{ 0b0000'0000 };
    inline constexpr uint8_t DAC_config_OFF_regB{ 0b1000'0000 };

    inline constexpr double PEAK_AMP_NO_CLIPPING{ 0.805664062 }; //gives theoretical peak voltage w/o clipping (3.3v)
}  // namespace DAC