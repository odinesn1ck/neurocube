#pragma once
#include "audio.hpp"
namespace nc {
class AudioHardware final : public AudioOutput {
    uint32_t samples_[MaxAudioSamples]{};
    int dma_ = -1;
    int32_t alarm_ = 0;
    bool ready_ = false, active_ = false;
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
        return active_;
    }
};
} // namespace nc
