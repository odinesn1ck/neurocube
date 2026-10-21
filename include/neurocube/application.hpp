#pragma once
#include "config.hpp"
#include "domain.hpp"
#include "idle.hpp"
namespace nc {
struct ButtonEvents {
    bool short_press = false, long_press = false;
};
class ButtonRecognizer {
    bool previous = false, stable = false, long_sent = false;
    float debounce = 0, held = 0;

  public:
    ButtonEvents update(bool, float);
};
class GestureRecognizer {
    float fx = 0, fy = 0, offset_x = 0, offset_y = 0, cooldown = 0, dwell = 0, twist_cooldown = 0, twist_dwell = 0;
    int held = -1;

  public:
    float sensitivity = 1;
    Actions update(const InputFrame &, float);
    void calibrate(float x, float y) {
        offset_x = x;
        offset_y = y;
        fx = fy = 0;
        held = -1;
        dwell = twist_dwell = 0;
    }
    void reset() {
        fx = fy = 0;
        held = -1;
        dwell = twist_dwell = 0;
        cooldown = 1;
        twist_cooldown = 0;
    }
};
struct GameRegistry {
    std::array<Game *, GameCapacity> games;
    int count = 4;
    Game *find(const char *) const;
};
GameRegistry default_registry();
enum class Screen {
    Menu,
    Rules,
    Playing,
    Paused,
    Result,
    Settings,
    Calibration,
    Countdown,
    MazeSetup,
    Splash,
    ProjectQr,
    Crawl,
    DisplayIdle,
    ShuttingDown,
    PowerNotice
};
class App {
    GameRegistry registry_;
    RecordStore &store_;
    AudioOutput *audio_ = nullptr;
    bool sound_touch_guard_ = false;
    bool splash_sound_sent_ = false, shutdown_sound_sent_ = false;
    void play_sound(SoundEffect);
    void update_game(const Actions &, float);
    void silence() { if (audio_) audio_->stop(); }
    GestureRecognizer gestures_;
    Records records_{};
    Screen screen_ = Screen::Menu;
    Screen idle_return_ = Screen::Menu;
    IdleController idle_;
    int selected_ = 0, menu_page_ = 0;
    Difficulty difficulty_ = Difficulty::Normal;
    uint32_t seed_ = 0x4E435542, maze_seed_ = 0;
    bool power_armed_ = false, shutdown_presented_ = false, power_released_ = false;
    float power_hold_ = 0, shutdown_time_ = 0;
    bool dirty_ = false, save_ok_ = true;
    float clock_ = 0, phase_time_ = 0;
    GameOptions maze_options_{}, active_options_{};
    bool calibration_ready_ = false, calibrating_ = false;
    float cal_time_ = 0, cal_x_ = 0, cal_y_ = 0, cal_min_x_ = 0, cal_max_x_ = 0, cal_min_y_ = 0,
          cal_max_y_ = 0;
    int cal_samples_ = 0;
    InputFrame latest_{};
    const char *recipient_;
    void finish();
    void begin_play();
    void update_calibration(const InputFrame &, float);

  public:
    explicit App(RecordStore &, GameRegistry registry = default_registry(), bool splash = false,
                 const char *recipient = config::Recipient, AudioOutput *audio = nullptr);
    void update(const InputFrame &, float);
    void render(Renderer &) const;
    void frame_presented();
    bool power_cut_requested() const {
        return screen_ == Screen::ShuttingDown && shutdown_presented_ &&
               shutdown_time_ >= 1.f && !latest_.power_button && !power_released_;
    }
    void power_cut_applied() { power_released_ = true; shutdown_time_ = 0; }
    bool power_restore_requested() const { return power_released_ && shutdown_time_ >= 1.f; }
    void power_restored() { power_released_ = false; screen_ = Screen::PowerNotice; }

    void launch(int, bool rules = true);
    void restart();
    void retry_same_maze();
    void new_maze();
    uint32_t maze_seed() const { return maze_seed_; }
    void pause();
    void resume();
    void menu();
    void next();
    bool flush();
    Screen screen() const {
        return screen_;
    }
    int selected() const {
        return selected_;
    }
    Difficulty difficulty() const {
        return difficulty_;
    }
    void set_difficulty(Difficulty d) {
        difficulty_ = d;
    }
    const Records &records() const {
        return records_;
    }
    bool save_ok() const {
        return save_ok_;
    }
    Game &game() {
        return *registry_.games[selected_];
    }
    void reseed(uint32_t seed) {
        seed_ = seed ? seed : 1;
    }
    void calibrate(float x, float y) {
        gestures_.calibrate(x, y);
    }
    const GameOptions &maze_options() const {
        return maze_options_;
    }
    float phase_time() const {
        return phase_time_;
    }
    const IdleController &idle() const {
        return idle_;
    }
    uint8_t brightness() const {
        return idle_.brightness();
    }
    bool display_idle() const {
        return idle_.phase() == IdlePhase::Dark;
    }
    uint32_t current_record() const;
};
} // namespace nc
