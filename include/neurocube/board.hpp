#pragma once
#include "application.hpp"
#include "domain.hpp"
#include "framebuffer.hpp"
namespace nc {
struct BoardStatus {
    bool lcd_sent = false, touch_ok = false, imu_ok = false, storage_ok = true;
    uint8_t touch_id = 0, imu_id = 0, imu_addr = 0;
    uint32_t frames = 0, touches = 0, button_events = 0, i2c_errors = 0;
};
class Board {
    bool previous_touch = false;
    uint64_t last_tap = 0, last_sample = 0;
    ButtonRecognizer button;
    void command(uint8_t);
    void data(uint8_t);

  public:
    BoardStatus status;
    static void hold_power();
    static void release_power();
    static void power_diagnostics();
    void init();
    InputFrame sample();
    void present(const Framebuffer &);
};
class FlashStore final : public RecordStore {
  public:
    bool load(Records &) override;
    bool save(const Records &) override;
};
} // namespace nc
