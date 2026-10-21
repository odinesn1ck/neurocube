#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "neurocube/audio_hardware.hpp"
#include "pico/stdlib.h"
extern "C" {
#include "DEV_Config.h"
#include "es8311.h"
}
#include "audio_pio.pio.h"
static bool codec_io_ok = true;
static volatile bool completed = false, timed_out = false;
static int64_t hard_silence(alarm_id_t, void *) {
    if (!completed) {
        gpio_put(0, false);
        timed_out = true;
        completed = true;
    }
    return 0;
}
static int audio_dma = -1;
extern "C" void DEV_Delay_Ms(uint32_t ms) {
    sleep_ms(ms);
}
extern "C" void DEV_I2C_Write_Byte(i2c_inst_t *port, uint8_t addr, uint8_t reg, uint8_t value) {
    uint8_t bytes[] = {reg, value};
    if (i2c_write_timeout_us(port, addr, bytes, 2, false, 2000) != 2)
        codec_io_ok = false;
}
extern "C" uint8_t DEV_I2C_Read_Byte(i2c_inst_t *port, uint8_t addr, uint8_t reg) {
    uint8_t value = 0;
    if (i2c_write_timeout_us(port, addr, &reg, 1, true, 2000) != 1 ||
        i2c_read_timeout_us(port, addr, &value, 1, false, 2000) != 1)
        codec_io_ok = false;
    return value;
}
static void audio_complete() {
    if (audio_dma >= 0 && (dma_hw->ints1 & (1u << audio_dma))) {
        dma_hw->ints1 = 1u << audio_dma;
        gpio_put(0, false); // Immediate amp silence even during LCD or USB work.
        completed = true;
    }
}
namespace nc {
bool AudioHardware::init() {
    gpio_init(0);
    gpio_put(0, false);
    gpio_set_dir(0, GPIO_OUT);
    auto id = Es8311_Read_Id();
    if (!codec_io_ok || (id != 0x1183 && id != 0x8311)) {
        ++faults;
        return false;
    }
    auto mclk = pio_add_program(pio1, &mclk_pio_program);
    mclk_pio_program_init(pio1, 2, mclk, 3);
    pio_sm_set_clkdiv(pio1, 2, float(clock_get_hz(clk_sys)) / (24000.f * 256.f * 5.f));
    pio_sm_set_enabled(pio1, 2, true);
    pico_audio_t config{6144000, 24000, 16, 16};
    Es8311_Init(config);
    Es8311_Voice_Mute(true);
    Es8311_Voice_Volume_Set(60);
    auto output = pio_add_program(pio2, &audio_pio_program);
    audio_pio_program_init(pio2, 0, output, 1, 5);
    dma_ = dma_claim_unused_channel(false);
    if (!codec_io_ok || dma_ < 0) {
        pio_sm_set_enabled(pio1, 2, false);
        ++faults;
        return false;
    }
    audio_dma = dma_;
    irq_add_shared_handler(DMA_IRQ_1, audio_complete,
                           PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
    irq_set_enabled(DMA_IRQ_1, true);
    ready_ = true;
    stop();
    return true;
}
void AudioHardware::stop() {
    gpio_put(0, false);
    if (!ready_)
        return;
    if (alarm_ > 0) {
        cancel_alarm(alarm_);
        alarm_ = 0;
    }
    dma_channel_set_irq1_enabled(dma_, false);
    dma_channel_abort(dma_);
    dma_hw->ints1 = 1u << dma_;
    pio_sm_set_enabled(pio2, 0, false);
    Es8311_Voice_Mute(true);
    pio_sm_set_enabled(pio1, 2, false);
    active_ = false;
    completed = false;
    if (!codec_io_ok) {
        ready_ = false;
        ++faults;
    }
}
void AudioHardware::poll() {
    if (active_ && (completed || time_us_64() - started_ > 250000)) {
        if (!completed || timed_out)
            ++faults;
        stop();
    }
}
bool AudioHardware::play(SoundEffect effect) {
    poll();
    if (!ready_ || active_) {
        ++dropped;
        return false;
    }
    auto count = synthesize_sound(effect, samples_, MaxAudioSamples);
    if (!count)
        return false;
    pio_sm_clear_fifos(pio2, 0);
    pio_sm_restart(pio2, 0);
    // Reinitialise the instruction counter as a prior stop can occur mid-frame.
    auto config = pio2->sm[0].execctrl;
    uint wrap_bottom =
        (config & PIO_SM0_EXECCTRL_WRAP_BOTTOM_BITS) >> PIO_SM0_EXECCTRL_WRAP_BOTTOM_LSB;
    pio_sm_exec(pio2, 0, pio_encode_jmp(wrap_bottom));
    pio_sm_set_enabled(pio1, 2, true);
    Es8311_Voice_Mute(false);
    if (!codec_io_ok) {
        ++faults;
        stop();
        return false;
    }
    auto dma = dma_channel_get_default_config(dma_);
    channel_config_set_transfer_data_size(&dma, DMA_SIZE_32);
    channel_config_set_read_increment(&dma, true);
    channel_config_set_write_increment(&dma, false);
    channel_config_set_dreq(&dma, pio_get_dreq(pio2, 0, true));
    completed = false;
    timed_out = false;
    alarm_ = add_alarm_in_ms(240, hard_silence, nullptr, true);
    if (alarm_ <= 0) {
        ++faults;
        stop();
        return false;
    }
    started_ = time_us_64();
    active_ = true;
    dma_channel_set_irq1_enabled(dma_, true);
    dma_channel_configure(dma_, &dma, &pio2->txf[0], samples_, count, true);
    pio_sm_set_enabled(pio2, 0, true);
    gpio_put(0, true);
    ++played;
    return true;
}
} // namespace nc
