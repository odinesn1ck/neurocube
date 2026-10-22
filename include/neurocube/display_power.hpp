#pragma once
#include <cstdint>
namespace nc {
// Infrastructure adapter. GPIO13 only; never owns BAT_EN or charger signals.
class DisplayPower {
    unsigned slice_ = 0, channel_ = 0;
    uint8_t requested_ = 255;

  public:
    void init();
    void apply(uint8_t level);
    void diagnostics(bool idle) const;
};
} // namespace nc
