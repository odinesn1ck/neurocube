#pragma once
#include "domain.hpp"
namespace nc {
constexpr int AudioSampleRate = 24000, AudioPeak = 2560;
constexpr size_t MaxAudioSamples = 4320;
constexpr int AudioPreparationMs = 180, AudioReleaseMs = 120;
size_t sound_samples(SoundEffect);
size_t synthesize_sound(SoundEffect, uint32_t *output, size_t capacity);
class SimAudio final : public AudioOutput {
    float remaining_ = 0;

  public:
    unsigned played = 0, dropped = 0;
    SoundEffect last = SoundEffect::Error;
    bool play(SoundEffect effect) override;
    void stop() override {
        remaining_ = 0;
    }
    void tick(float dt);
    bool busy() const {
        return remaining_ > 0;
    }
};
} // namespace nc
