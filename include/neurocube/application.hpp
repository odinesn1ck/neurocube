#pragma once
#include "domain.hpp"
namespace nc {
class GestureRecognizer {
    float fx = 0, fy = 0, offset_x = 0, offset_y = 0, cooldown = 0, dwell = 0, twist_cooldown = 0;
    int held = -1;

  public:
    float sensitivity = 1;
    Actions update(const InputFrame &, float dt);
    void calibrate(float x, float y) {
        offset_x = x;
        offset_y = y;
        fx = fy = 0;
        held = -1;
        dwell = 0;
    }
    void reset() {
        fx = fy = 0;
        held = -1;
        dwell = 0;
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
enum class Screen { Menu, Rules, Playing, Paused, Result, Settings };
class App {
    GameRegistry registry_;
    RecordStore &store_;
    GestureRecognizer gestures_;
    Records records_{};
    Screen screen_ = Screen::Menu;
    int selected_ = 0;
    int menu_page_ = 0;
    Difficulty difficulty_ = Difficulty::Normal;
    uint32_t seed_ = 0x4E435542;
    bool dirty_ = false, save_ok_ = true;
    float clock_ = 0;
    void finish();

  public:
    explicit App(RecordStore &, GameRegistry registry = default_registry());
    void update(const InputFrame &, float dt);
    void render(Renderer &) const;
    void launch(int index, bool rules = true);
    void restart();
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
};
} // namespace nc
