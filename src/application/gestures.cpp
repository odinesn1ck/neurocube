#include "neurocube/application.hpp"
#include <algorithm>
#include <cmath>
namespace nc {
Actions GestureRecognizer::update(const InputFrame &i, float dt) {
    dt = std::clamp(dt, 0.f, 0.1f);
    Actions a;
    a.tap = i.tap;
    a.x = i.x;
    a.y = i.y;
    cooldown = std::max(0.f, cooldown - dt);
    twist_cooldown = std::max(0.f, twist_cooldown - dt);
    if (!i.imu)
        return a;
    const float alpha = 1 - std::exp(-dt * 12.f);
    fx += (i.ax - offset_x - fx) * alpha;
    fy += (i.ay - offset_y - fy) * alpha;
    a.tilt_x = std::clamp(fx * sensitivity, -1.f, 1.f);
    a.tilt_y = std::clamp(fy * sensitivity, -1.f, 1.f);
    float norm = std::sqrt(i.ax * i.ax + i.ay * i.ay + i.az * i.az);
    if (norm > 2.3f && cooldown <= 0) {
        a.shake = true;
        cooldown = 1.2f;
        held = -1;
        dwell = 0;
        return a;
    }
    if (std::abs(i.gz) > 160 && twist_cooldown <= 0 && cooldown <= 0) {
        a.twist = true;
        twist_cooldown = 0.65f;
    }
    int dir = -1;
    float threshold = .34f / sensitivity;
    if (std::max(std::abs(fx), std::abs(fy)) > threshold) {
        dir = std::abs(fx) > std::abs(fy) ? (fx > 0 ? 1 : 3) : (fy > 0 ? 2 : 0);
    }
    if (std::max(std::abs(fx), std::abs(fy)) < threshold * .55f) {
        held = -1;
        dwell = 0;
    } else if (dir >= 0 && held == -1 && cooldown <= 0) {
        dwell += dt;
        if (dwell >= .10f) {
            a.direction = dir;
            held = dir;
            dwell = 0;
        }
    }
    return a;
}
Game *GameRegistry::find(const char *id) const {
    for (int idx = 0; idx < count; idx++) {
        const char *a = games[idx]->metadata().id, *b = id;
        while (*a && *a == *b) {
            ++a;
            ++b;
        }
        if (*a == *b)
            return games[idx];
    }
    return nullptr;
}
} // namespace nc
