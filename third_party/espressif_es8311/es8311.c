/*
 * SPDX-FileCopyrightText: 2015-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Modified by NeuroCube, 2026: portable callback transport, no ESP-IDF or heap,
 * fixed 24 kHz/16-bit/6.144 MHz profile, codec master, unused APIs removed.
 * Derived only from the Apache-2.0 esp-bsp revision recorded in provenance.json. */

#include "es8311.h"
#include "es8311_reg.h"
#include <stddef.h>
#define BIT(n) (1u << (n))
#define NC_TRY(operation)                                                                          \
    do {                                                                                           \
        int result = (operation);                                                                  \
        if (result != ES8311_OK)                                                                   \
            return result;                                                                         \
    } while (0)

typedef enum {
    ES8311_RESOLUTION_16 = 16,
    ES8311_RESOLUTION_18 = 18,
    ES8311_RESOLUTION_20 = 20,
    ES8311_RESOLUTION_24 = 24,
    ES8311_RESOLUTION_32 = 32
} es8311_resolution_t;
typedef struct {
    bool mclk_from_mclk_pin, mclk_inverted, sclk_inverted;
    int mclk_frequency, sample_frequency;
} es8311_clock_config_t;

static int es8311_write_reg(es8311_t *dev, uint8_t reg, uint8_t value) {
    return dev->bus.write(dev->bus.context, reg, value) ? ES8311_OK : ES8311_IO_ERROR;
}
static int es8311_read_reg(es8311_t *dev, uint8_t reg, uint8_t *value) {
    return dev->bus.read(dev->bus.context, reg, value) ? ES8311_OK : ES8311_IO_ERROR;
}

/*
 * Clock coefficient structure
 */
struct _coeff_div {
    uint32_t mclk;     /* mclk frequency */
    uint32_t rate;     /* sample rate */
    uint8_t pre_div;   /* the pre divider with range from 1 to 8 */
    uint8_t pre_multi; /* the pre multiplier with 0: 1x, 1: 2x, 2: 4x, 3: 8x selection */
    uint8_t adc_div;   /* adcclk divider */
    uint8_t dac_div;   /* dacclk divider */
    uint8_t fs_mode;   /* double speed or single speed, =0, ss, =1, ds */
    uint8_t lrck_h;    /* adclrck divider and daclrck divider */
    uint8_t lrck_l;
    uint8_t bclk_div; /* sclk divider */
    uint8_t adc_osr;  /* adc osr */
    uint8_t dac_osr;  /* dac osr */
};

/* codec hifi mclk clock divider coefficients */
static const struct _coeff_div coeff_div[] = {
    {6144000, 24000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
};

static int get_coeff(uint32_t mclk, uint32_t rate) {
    for (size_t i = 0; i < (sizeof(coeff_div) / sizeof(coeff_div[0])); i++) {
        if (coeff_div[i].rate == rate && coeff_div[i].mclk == mclk) {
            return i;
        }
    }

    return -1;
}

static int es8311_sample_frequency_config(es8311_t *dev, int mclk_frequency, int sample_frequency) {
    uint8_t regv;

    /* Get clock coefficients from coefficient table */
    int coeff = get_coeff(mclk_frequency, sample_frequency);

    if (coeff < 0) {

        return ES8311_INVALID_ARGUMENT;
    }

    const struct _coeff_div *const selected_coeff = &coeff_div[coeff];

    /* register 0x02 */
    NC_TRY(es8311_read_reg(dev, ES8311_CLK_MANAGER_REG02, &regv));
    regv &= 0x07;
    regv |= (selected_coeff->pre_div - 1) << 5;
    regv |= selected_coeff->pre_multi << 3;
    NC_TRY(es8311_write_reg(dev, ES8311_CLK_MANAGER_REG02, regv));

    /* register 0x03 */
    const uint8_t reg03 = (selected_coeff->fs_mode << 6) | selected_coeff->adc_osr;
    NC_TRY(es8311_write_reg(dev, ES8311_CLK_MANAGER_REG03, reg03));

    /* register 0x04 */
    NC_TRY(es8311_write_reg(dev, ES8311_CLK_MANAGER_REG04, selected_coeff->dac_osr));

    /* register 0x05 */
    const uint8_t reg05 = ((selected_coeff->adc_div - 1) << 4) | (selected_coeff->dac_div - 1);
    NC_TRY(es8311_write_reg(dev, ES8311_CLK_MANAGER_REG05, reg05));

    /* register 0x06 */
    NC_TRY(es8311_read_reg(dev, ES8311_CLK_MANAGER_REG06, &regv));
    regv &= 0xE0;

    if (selected_coeff->bclk_div < 19) {
        regv |= (selected_coeff->bclk_div - 1) << 0;
    } else {
        regv |= (selected_coeff->bclk_div) << 0;
    }

    NC_TRY(es8311_write_reg(dev, ES8311_CLK_MANAGER_REG06, regv));

    /* register 0x07 */
    NC_TRY(es8311_read_reg(dev, ES8311_CLK_MANAGER_REG07, &regv));
    regv &= 0xC0;
    regv |= selected_coeff->lrck_h << 0;
    NC_TRY(es8311_write_reg(dev, ES8311_CLK_MANAGER_REG07, regv));

    /* register 0x08 */
    NC_TRY(es8311_write_reg(dev, ES8311_CLK_MANAGER_REG08, selected_coeff->lrck_l));

    return ES8311_OK;
}

static int es8311_clock_config(es8311_t *dev, const es8311_clock_config_t *const clk_cfg,
                               es8311_resolution_t res) {
    uint8_t reg06;
    uint8_t reg01 = 0x3F; // Enable all clocks
    int mclk_hz;

    /* Select clock source for internal MCLK and determine its frequency */
    if (clk_cfg->mclk_from_mclk_pin) {
        mclk_hz = clk_cfg->mclk_frequency;
    } else {
        mclk_hz = clk_cfg->sample_frequency * (int)res * 2;
        reg01 |= BIT(7); // Select BCLK (a.k.a. SCK) pin
    }

    if (clk_cfg->mclk_inverted) {
        reg01 |= BIT(6); // Invert MCLK pin
    }
    NC_TRY(es8311_write_reg(dev, ES8311_CLK_MANAGER_REG01, reg01));

    NC_TRY(es8311_read_reg(dev, ES8311_CLK_MANAGER_REG06, &reg06));
    if (clk_cfg->sclk_inverted) {
        reg06 |= BIT(5);
    } else {
        reg06 &= ~BIT(5);
    }
    NC_TRY(es8311_write_reg(dev, ES8311_CLK_MANAGER_REG06, reg06));

    /* Configure clock dividers */
    return es8311_sample_frequency_config(dev, mclk_hz, clk_cfg->sample_frequency);
}

static int es8311_resolution_config(const es8311_resolution_t res, uint8_t *reg) {
    switch (res) {
    case ES8311_RESOLUTION_16:
        *reg |= (3 << 2);
        break;
    case ES8311_RESOLUTION_18:
        *reg |= (2 << 2);
        break;
    case ES8311_RESOLUTION_20:
        *reg |= (1 << 2);
        break;
    case ES8311_RESOLUTION_24:
        *reg |= (0 << 2);
        break;
    case ES8311_RESOLUTION_32:
        *reg |= (4 << 2);
        break;
    default:
        return ES8311_INVALID_ARGUMENT;
    }
    return ES8311_OK;
}

static int es8311_fmt_config(es8311_t *dev, const es8311_resolution_t res_in,
                             const es8311_resolution_t res_out) {
    uint8_t reg09 = 0; // SDP In
    uint8_t reg0a = 0; // SDP Out

    uint8_t reg00;
    NC_TRY(es8311_read_reg(dev, ES8311_RESET_REG00, &reg00));
    reg00 |= (1u << 6); // Board needs codec-generated BCLK/LRCK; ES8311 User Guide section 8.2.
    NC_TRY(es8311_write_reg(dev, ES8311_RESET_REG00,
                            reg00)); // Master serial port (NeuroCube adaptation).

    /* Setup SDP In and Out resolution */
    NC_TRY(es8311_resolution_config(res_in, &reg09));
    NC_TRY(es8311_resolution_config(res_out, &reg0a));

    NC_TRY(es8311_write_reg(dev, ES8311_SDPIN_REG09, reg09));
    NC_TRY(es8311_write_reg(dev, ES8311_SDPOUT_REG0A, reg0a));

    return ES8311_OK;
}

static int initialize(es8311_t *dev, const es8311_clock_config_t *const clk_cfg,
                      const es8311_resolution_t res_in, const es8311_resolution_t res_out) {
    if (!((clk_cfg->sample_frequency >= 8000) && (clk_cfg->sample_frequency <= 96000)))
        return ES8311_INVALID_ARGUMENT;
    if (!clk_cfg->mclk_from_mclk_pin) {
        if (res_out != res_in)
            return ES8311_INVALID_ARGUMENT;
    }

    /* Reset ES8311 to its default */
    NC_TRY(es8311_write_reg(dev, ES8311_RESET_REG00, 0x1F));
    dev->bus.delay_ms(dev->bus.context, 20);
    NC_TRY(es8311_write_reg(dev, ES8311_RESET_REG00, 0x00));
    NC_TRY(es8311_write_reg(dev, ES8311_RESET_REG00, 0x80)); // Power-on command

    /* Setup clock: source, polarity and clock dividers */
    NC_TRY(es8311_clock_config(dev, clk_cfg, res_out));

    /* Setup audio format (fmt): master/slave, resolution, I2S */
    NC_TRY(es8311_fmt_config(dev, res_in, res_out));

    NC_TRY(es8311_write_reg(dev, ES8311_SYSTEM_REG0D,
                            0x01)); // Power up analog circuitry - NOT default
    NC_TRY(es8311_write_reg(dev, ES8311_SYSTEM_REG0E,
                            0x02)); // Enable analog PGA, enable ADC modulator - NOT default
    NC_TRY(es8311_write_reg(dev, ES8311_SYSTEM_REG12, 0x00)); // power-up DAC - NOT default
    NC_TRY(es8311_write_reg(dev, ES8311_SYSTEM_REG13,
                            0x10)); // Enable output to HP drive - NOT default
    NC_TRY(es8311_write_reg(dev, ES8311_ADC_REG1C,
                            0x6A)); // ADC Equalizer bypass, cancel DC offset in digital domain
    NC_TRY(es8311_write_reg(dev, ES8311_DAC_REG37, 0x08)); // Bypass DAC equalizer - NOT default

    return ES8311_OK;
}

int es8311_voice_volume_set(es8311_t *dev, int volume, int *volume_set) {
    if (!dev || !dev->bus.write)
        return ES8311_INVALID_ARGUMENT;
    if (volume < 0) {
        volume = 0;
    } else if (volume > 100) {
        volume = 100;
    }

    int reg32;
    if (volume == 0) {
        reg32 = 0;
    } else {
        reg32 = ((volume) * 256 / 100) - 1;
    }

    NC_TRY(es8311_write_reg(dev, ES8311_DAC_REG32, reg32));
    // provide user with real volume set
    if (volume_set != NULL) {
        *volume_set = volume;
    }
    return ES8311_OK;
}

int es8311_voice_mute(es8311_t *dev, bool mute) {
    if (!dev || !dev->bus.read || !dev->bus.write)
        return ES8311_INVALID_ARGUMENT;
    uint8_t reg31;
    NC_TRY(es8311_read_reg(dev, ES8311_DAC_REG31, &reg31));

    if (mute) {
        reg31 |= BIT(6) | BIT(5);
    } else {
        reg31 &= ~(BIT(6) | BIT(5));
    }

    return es8311_write_reg(dev, ES8311_DAC_REG31, reg31);
}

int es8311_init_24k(es8311_t *dev) {
    if (!dev || !dev->bus.write || !dev->bus.read || !dev->bus.delay_ms)
        return ES8311_INVALID_ARGUMENT;
    const es8311_clock_config_t clock = {true, false, false, 6144000, 24000};
    return initialize(dev, &clock, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16);
}
int es8311_read_id(es8311_t *dev, uint16_t *id) {
    if (!dev || !id || !dev->bus.read)
        return ES8311_INVALID_ARGUMENT;
    uint8_t high, low;
    NC_TRY(es8311_read_reg(dev, ES8311_CHD1_REGFD, &high));
    NC_TRY(es8311_read_reg(dev, ES8311_CHD2_REGFE, &low));
    *id = ((uint16_t)high << 8) | low;
    return ES8311_OK;
}
