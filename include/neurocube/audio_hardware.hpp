#pragma once
#include "audio.hpp"
namespace nc {
class AudioHardware final : public AudioOutput {
    uint32_t samples_[MaxAudioSamples + 4320 + 2880]{};
    int dma_ = -1;
    int32_t alarm_ = 0, warmup_alarm_ = 0, release_alarm_ = 0;
    bool ready_ = false, active_ = false, cooling_ = false;
    uint32_t zero_ = 0;
    void transfer(size_t count, bool pcm);
    void finish_quiet();
    uint64_t started_ = 0;

  public:
    unsigned played = 0, dropped = 0, faults = 0;
    bool init();
    bool play(SoundEffect) override;
    void stop() override;
    void poll();
    bool ready() const {
        return ready_;
    }
    bool busy() const {
        return active_ || cooling_;
    }
};
} // namespace nc
