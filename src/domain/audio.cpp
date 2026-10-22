#include "neurocube/audio.hpp"
#include <algorithm>
#include <cmath>
namespace nc {
static constexpr int16_t sine[64] = {
    0,      3212,   6393,   9512,   12539,  15446,  18204,  20787,  23170,  25329,  27245,
    28898,  30273,  31356,  32137,  32609,  32767,  32609,  32137,  31356,  30273,  28898,
    27245,  25329,  23170,  20787,  18204,  15446,  12539,  9512,   6393,   3212,   0,
    -3212,  -6393,  -9512,  -12539, -15446, -18204, -20787, -23170, -25329, -27245, -28898,
    -30273, -31356, -32137, -32609, -32767, -32609, -32137, -31356, -30273, -28898, -27245,
    -25329, -23170, -20787, -18204, -15446, -12539, -9512,  -6393,  -3212};
size_t sound_samples(SoundEffect effect) {
    return effect == SoundEffect::Error ? 2400 : 4320;
}
size_t synthesize_sound(SoundEffect effect, uint32_t *out, size_t capacity) {
    size_t count = sound_samples(effect);
    if (!out || capacity < count)
        return 0;
    uint32_t phase = 0, noise = 0x631A891u;
    for (size_t n = 0; n < count; ++n) {
        float t = float(n) / float(count - 1);
        float hz = effect == SoundEffect::Error
                       ? 700.f - 400.f * t
                       : (t < .4f ? 600.f + 1250.f * t : 1100.f - 500.f * (t - .4f));
        phase += uint32_t(hz * (4294967296.0 / AudioSampleRate));
        int value = sine[phase >> 26];
        if (effect == SoundEffect::Error) {
            noise ^= noise << 13;
            noise ^= noise >> 17;
            noise ^= noise << 5;
            value = (value * 3 + (int(noise & 65535) - 32768)) / 4;
        }
        float envelope = std::min(1.f, float(n) / 144.f) * (1.f - t) * (1.f - t);
        int16_t pcm = int16_t(value * (float(AudioPeak) / 32768.f) * envelope);
        uint32_t word = uint16_t(pcm);
        out[n] = (word << 16) | word;
    }
    return count;
}
bool SimAudio::play(SoundEffect effect) {
    if (busy()) {
        ++dropped;
        return false;
    }
    remaining_ = float(sound_samples(effect)) / AudioSampleRate +
                 float(AudioPreparationMs + AudioReleaseMs) / 1000;
    last = effect;
    ++played;
    return true;
}
void SimAudio::tick(float dt) {
    if (std::isfinite(dt))
        remaining_ = std::max(0.f, remaining_ - std::max(0.f, dt));
}
} // namespace nc
