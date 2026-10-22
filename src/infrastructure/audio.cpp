#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "neurocube/audio_hardware.hpp"
#include "pico/stdlib.h"
extern "C" {
#include "es8311.h"
}
#include "audio_pio.pio.h"
// NS4150B datasheet: startup 120 ms, wake 35 ms, shutdown 80 ms.
// Feed zeroes before/after PCM; never cut the codec clock during PA shutdown.
static constexpr size_t Lead = nc::AudioPreparationMs * nc::AudioSampleRate / 1000,
                        Tail = nc::AudioReleaseMs * nc::AudioSampleRate / 1000;
static_assert(nc::AudioPreparationMs - 50 >= 120 &&
              nc::AudioReleaseMs - 10 >= 80); // 180/120 ms at 24 kHz.
static bool codec_io_ok = true;
static volatile bool completed = false, timed_out = false, voice_gate = false;
static volatile bool amp_live = false, amp_seen = false;
static volatile uint32_t amp_off_at = 0, voice_started_at = 0;
static int audio_dma = -1;
static void cut_amp() {
    gpio_put(0, false);
    if (amp_live) {
        amp_off_at = time_us_32();
        amp_live = false;
    }
}
static int64_t enable_after_settling(alarm_id_t, void *) {
    if (voice_gate && !completed) {
        // A delayed IRQ must not enable an amplifier with too little warmup left.
        if (uint32_t(time_us_32() - voice_started_at) > 50000) {
            timed_out = completed = true;
            voice_gate = false;
            return 0;
        }
        amp_seen = amp_live = true;
        gpio_put(0, true);
    }
    return 0;
}
static int64_t release_before_tail(alarm_id_t, void *) {
    voice_gate = false;
    cut_amp();
    return 0;
}
static int64_t hard_silence(alarm_id_t, void *) {
    if (!completed) {
        voice_gate = false;
        cut_amp();
        timed_out = completed = true;
    }
    return 0;
}
static void codec_delay(void *, uint32_t ms) {
    sleep_ms(ms);
}
static bool codec_write(void *, uint8_t reg, uint8_t value) {
    uint8_t bytes[] = {reg, value};
    bool ok = i2c_write_timeout_us(i2c1, 0x18, bytes, 2, false, 2000) == 2;
    codec_io_ok = codec_io_ok && ok;
    return ok;
}
static bool codec_read(void *, uint8_t reg, uint8_t *value) {
    bool ok = i2c_write_timeout_us(i2c1, 0x18, &reg, 1, true, 2000) == 1 &&
              i2c_read_timeout_us(i2c1, 0x18, value, 1, false, 2000) == 1;
    codec_io_ok = codec_io_ok && ok;
    return ok;
}
static es8311_t codec{{nullptr, codec_write, codec_read, codec_delay}};
static void codec_mute(bool mute) {
    codec_io_ok = codec_io_ok && es8311_voice_mute(&codec, mute) == ES8311_OK;
}
static void audio_complete() {
    if (audio_dma >= 0 && (dma_hw->ints1 & (1u << audio_dma))) {
        dma_hw->ints1 = 1u << audio_dma;
        voice_gate = false;
        cut_amp();
        completed = true;
    }
}
namespace nc {
bool AudioHardware::init() {
    gpio_init(0);
    gpio_put(0, false);
    gpio_set_dir(0, GPIO_OUT);
    uint16_t id = 0;
    codec_io_ok = es8311_read_id(&codec, &id) == ES8311_OK;
    if (!codec_io_ok || (id != 0x1183 && id != 0x8311)) {
        ++faults;
        return false;
    }
    pio_sm_claim(pio1, 2);
    pio_sm_claim(pio2, 0);
    auto mclk = pio_add_program(pio1, &mclk_pio_program);
    mclk_pio_program_init(pio1, 2, mclk, 3);
    pio_sm_set_clkdiv(pio1, 2, float(clock_get_hz(clk_sys)) / (24000.f * 256.f * 5.f));
    pio_sm_set_enabled(pio1, 2, true);
    codec_io_ok = codec_io_ok && es8311_init_24k(&codec) == ES8311_OK;
    codec_mute(true);
    codec_io_ok = codec_io_ok && es8311_voice_volume_set(&codec, 60, nullptr) == ES8311_OK;
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
    finish_quiet();
    return true;
}
void AudioHardware::transfer(size_t count, bool pcm) {
    auto config = dma_channel_get_default_config(dma_);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_32);
    channel_config_set_read_increment(&config, pcm);
    channel_config_set_write_increment(&config, false);
    channel_config_set_dreq(&config, pio_get_dreq(pio2, 0, true));
    dma_channel_set_irq1_enabled(dma_, pcm);
    dma_channel_configure(dma_, &config, &pio2->txf[0], pcm ? samples_ : &zero_, count, true);
}
void AudioHardware::finish_quiet() {
    dma_channel_set_irq1_enabled(dma_, false);
    dma_channel_abort(dma_);
    dma_hw->ints1 = 1u << dma_;
    pio_sm_set_enabled(pio2, 0, false);
    codec_mute(true);
    pio_sm_set_enabled(pio1, 2, false);
    active_ = cooling_ = false;
    completed = false;
    if (!codec_io_ok) {
        ready_ = false;
        ++faults;
    }
}
void AudioHardware::stop() {
    voice_gate = false;
    cut_amp();
    if (!ready_)
        return;
    for (auto *id : {&warmup_alarm_, &release_alarm_, &alarm_})
        if (*id > 0) {
            cancel_alarm(*id);
            *id = 0;
        }
    if (cooling_)
        return;
    dma_channel_set_irq1_enabled(dma_, false);
    dma_channel_abort(dma_);
    dma_hw->ints1 = 1u << dma_;
    if (amp_seen && uint32_t(time_us_32() - amp_off_at) < 90000) {
        // Explicit mute/shutdown can interrupt PCM: switch to zeroes immediately,
        // then let the amplifier's 80 ms shutdown finish without a clock transient.
        pio_sm_clear_fifos(pio2, 0);
        transfer(AudioSampleRate / 10, false);
        active_ = false;
        cooling_ = true;
        completed = false;
        return;
    }
    finish_quiet();
}
void AudioHardware::poll() {
    if (cooling_ && uint32_t(time_us_32() - amp_off_at) >= 90000)
        finish_quiet();
    if (active_ && (completed || time_us_64() - started_ > 550000)) {
        if (!completed || timed_out)
            ++faults;
        stop();
    }
}
bool AudioHardware::play(SoundEffect effect) {
    poll();
    if (!ready_ || busy()) {
        ++dropped;
        return false;
    }
    for (size_t n = 0; n < Lead; ++n)
        samples_[n] = 0;
    auto signal = synthesize_sound(effect, samples_ + Lead, MaxAudioSamples);
    if (!signal)
        return false;
    for (size_t n = Lead + signal; n < Lead + signal + Tail; ++n)
        samples_[n] = 0;
    pio_sm_clear_fifos(pio2, 0);
    pio_sm_restart(pio2, 0);
    auto ctrl = pio2->sm[0].execctrl;
    uint bottom = (ctrl & PIO_SM0_EXECCTRL_WRAP_BOTTOM_BITS) >> PIO_SM0_EXECCTRL_WRAP_BOTTOM_LSB;
    pio_sm_exec(pio2, 0, pio_encode_jmp(bottom));
    pio_sm_set_enabled(pio1, 2, true);
    codec_mute(false);
    if (!codec_io_ok) {
        ++faults;
        stop();
        return false;
    }
    completed = timed_out = amp_seen = amp_live = false;
    voice_gate = true;
    started_ = time_us_64();
    voice_started_at = time_us_32();
    active_ = true;
    warmup_alarm_ = add_alarm_in_ms(15, enable_after_settling, nullptr, true);
    release_alarm_ = add_alarm_in_ms(AudioPreparationMs + int(signal * 1000 / AudioSampleRate) + 10,
                                     release_before_tail, nullptr, true);
    alarm_ = add_alarm_in_ms(550, hard_silence, nullptr, true);
    if (warmup_alarm_ <= 0 || release_alarm_ <= 0 || alarm_ <= 0) {
        ++faults;
        stop();
        return false;
    }
    transfer(Lead + signal + Tail, true);
    pio_sm_set_enabled(pio2, 0, true);
    ++played;
    return true;
}
} // namespace nc
