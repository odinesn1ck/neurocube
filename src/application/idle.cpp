#include "neurocube/idle.hpp"
#include <algorithm>
#include <cmath>
namespace nc {
IdleEvent IdleController::update(bool eligible, bool action, float dt) {
    dt = std::isfinite(dt) ? std::max(0.f, dt) : 0.f;
    if (wake_guard_) {
        idle_seconds_ = 0;
        if (action)
            quiet_ = 0;
        else
            quiet_ += dt;
        if (quiet_ >= .08f)
            wake_guard_ = false;
        return IdleEvent::Consumed;
    }
    if (phase_ == IdlePhase::Awake) {
        if (!eligible || action) {
            idle_seconds_ = 0;
            return IdleEvent::None;
        }
        idle_seconds_ += dt;
        if (idle_seconds_ + 1e-6 < Threshold)
            return IdleEvent::None;
        phase_ = IdlePhase::Crawl;
        progress_ = fade_ = 0;
        episode_ = next_episode_;
        next_episode_ = (next_episode_ + 1) % CrawlStoryCount;
        return IdleEvent::Started;
    }
    if (action) {
        phase_ = IdlePhase::Awake;
        idle_seconds_ = 0;
        wake_guard_ = true;
        quiet_ = 0;
        return IdleEvent::Woke;
    }
    if (phase_ == IdlePhase::Crawl) {
        progress_ += CrawlSpeed * dt;
        if (crawl_finished(progress_, crawl_story(episode_))) {
            phase_ = IdlePhase::Fading;
            fade_ = 0;
        }
    } else if (phase_ == IdlePhase::Fading) {
        fade_ += dt;
        if (fade_ >= 1.5f)
            phase_ = IdlePhase::Dark;
    }
    return IdleEvent::Consumed;
}
uint8_t IdleController::brightness() const {
    if (phase_ == IdlePhase::Dark)
        return 0;
    if (phase_ == IdlePhase::Fading)
        return uint8_t(255 * std::clamp(1 - fade_ / 1.5f, 0.f, 1.f));
    return 255;
}
} // namespace nc
