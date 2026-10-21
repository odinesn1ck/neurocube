#pragma once
#include <stdint.h>
#include <stdbool.h>
typedef struct { uint32_t mclk_freq, sample_freq; uint8_t res_in, res_out; } pico_audio_t;
