#include "neurocube/display_power.hpp"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include <cstdio>
namespace nc {
static constexpr unsigned BacklightPin = 13;
void DisplayPower::init() {
    // Waveshare DEV_Config.c uses PWM GPIO13, divider 50. Keep full brightness at handoff.
    slice_ = pwm_gpio_to_slice_num(BacklightPin);
    channel_ = pwm_gpio_to_channel(BacklightPin);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 50.f);
    pwm_config_set_wrap(&config, 255);
    pwm_init(slice_, &config, false);
    pwm_set_chan_level(slice_, channel_, 256);
    gpio_set_function(BacklightPin, GPIO_FUNC_PWM);
    pwm_set_enabled(slice_, true);
    requested_ = 255;
}
void DisplayPower::apply(uint8_t level) {
    if (level == requested_)
        return;
    requested_ = level;
    pwm_set_chan_level(slice_, channel_, level == 255 ? 256 : level);
}
void DisplayPower::diagnostics(bool idle) const {
    unsigned actual = (pwm_hw->slice[slice_].cc >> (channel_ * 16)) & 0xffff;
    printf("{\"v\":1,\"type\":\"display\",\"backlight_requested\":%u,\"pwm_level\":%u,\"idle\":%s,"
           "\"idle_mode\":\"polling_backlight_off\",\"physical_wake\":\"NOT_TESTED\"}\n",
           requested_, actual, idle ? "true" : "false");
}
} // namespace nc
