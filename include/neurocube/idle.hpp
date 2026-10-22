#pragma once
#include "crawl.hpp"
namespace nc {
enum class IdlePhase { Awake, Crawl, Fading, Dark };
enum class IdleEvent { None, Consumed, Started, Woke };
class IdleController {
    IdlePhase phase_ = IdlePhase::Awake;
    double idle_seconds_ = 0;
    float progress_ = 0, fade_ = 0, quiet_ = 0;
    bool wake_guard_ = false;
    int episode_ = 0, next_episode_ = 0;

  public:
    static constexpr double Threshold = 120;
    IdleEvent update(bool eligible, bool user_action, float dt);
    IdlePhase phase() const {
        return phase_;
    }
    float progress() const {
        return progress_;
    }
    int episode() const {
        return episode_;
    }
    double idle_seconds() const {
        return idle_seconds_;
    }
    uint8_t brightness() const;
    void reset_timer() {
        idle_seconds_ = 0;
    }
};
} // namespace nc
