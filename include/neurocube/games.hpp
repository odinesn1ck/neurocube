#pragma once
#include "domain.hpp"
namespace nc {
float sequence_signal_seconds(Difficulty, int round);
enum class SequencePhase { Showing, Recall, Correct, Failure, Complete };
class SequenceGame final : public Game {
    std::array<int, 32> sequence{};
    Random rng;
    int length = 1, cursor = 0, shown = -1;
    float timer = 0, step = 1.2f;
    Difficulty difficulty_ = Difficulty::Normal;
    SequencePhase phase_ = SequencePhase::Showing;
    Result outcome;

  public:
    Metadata metadata() const override {
        return {"sequence", "Повтори ритм",
                "Запомни огни. Повтори наклонами или касаниями."};
    }
    bool countdown() const override {
        return true;
    }
    void start(Difficulty, uint32_t) override;
    void update(const Actions &, float) override;
    void render(Renderer &) const override;
    Result result() const override {
        return outcome;
    }
    int cue() const {
        return shown;
    }
    bool playback() const {
        return phase_ == SequencePhase::Showing || phase_ == SequencePhase::Correct;
    }
    float signal_seconds() const { return step; }
    SequencePhase phase() const {
        return phase_;
    }
};
struct Wall {
    float x, y, w, h;
};
bool circle_hits_wall(float x, float y, float radius, const Wall &);
struct MazeMap {
    int columns = 5, rows = 4, wall_count = 0;
    std::array<Wall, 96> walls{};
    std::array<uint8_t, 40> openings{};
    float cell_width() const {
        return 220.f / columns;
    }
    float cell_height() const {
        return 138.f / rows;
    }
    float center_x(int cell) const {
        return 10 + (cell % columns + .5f) * cell_width();
    }
    float center_y(int cell) const {
        return 63 + (cell / columns + .5f) * cell_height();
    }
};
MazeMap make_maze(int geometry, uint32_t seed);
class ContactCounter {
    bool armed_ = true;
    float clear_ = 0;
    uint16_t count_ = 0;

  public:
    void reset() {
        armed_ = true;
        clear_ = 0;
        count_ = 0;
    }
    void update(bool contact, float dt, bool separated = true) {
        if (contact) {
            clear_ = 0;
            if (armed_) {
                if (count_ < UINT16_MAX)
                    count_++;
                armed_ = false;
            }
        } else if (!separated)
            clear_ = 0;
        else if ((clear_ += dt) >= .14f)
            armed_ = true;
    }
    uint16_t count() const {
        return count_;
    }
};
class MazeGame final : public Game {
    float x = 0, y = 0, vx = 0, vy = 0, elapsed = 0;
    GameOptions options_;
    MazeMap map_;
    ContactCounter contacts_;
    Result outcome;

  public:
    Metadata metadata() const override {
        return {"maze", "Лабиринт",
                "Наклоняй куб. Доведи шар до зелёной цели. Береги лимит касаний."};
    }
    bool setup() const override {
        return true;
    }
    void configure(const GameOptions &o) override {
        options_ = o;
    }
    void start(Difficulty, uint32_t) override;
    void update(const Actions &, float) override;
    void render(Renderer &) const override;
    Result result() const override {
        return outcome;
    }
    float ball_x() const {
        return x;
    }
    float ball_y() const {
        return y;
    }
    const MazeMap &map() const {
        return map_;
    }
    uint16_t contacts() const {
        return contacts_.count();
    }
};
class CommandsGame final : public Game {
    Random rng;
    int expected = 0;
    float remaining = 0, limit = 3.4f, feedback_ = 0, angle_ = 0, object_x_ = 120, object_y_ = 132;
    bool failed_ = false, dragging_ = false, swipe_sent_ = false;
    int drag_x_ = 0, drag_y_ = 0;
    Result outcome;
    void choose();

  public:
    Metadata metadata() const override {
        return {"commands", "Лови команду",
                "Следи за командами. Наклон или свайп. Поворот: куб/касание."};
    }
    bool countdown() const override {
        return true;
    }
    void start(Difficulty, uint32_t) override;
    void update(const Actions &, float) override;
    void render(Renderer &) const override;
    Result result() const override {
        return outcome;
    }
    int command() const {
        return expected;
    }
    float feedback() const {
        return feedback_;
    }
    float object_x() const {
        return object_x_;
    }
    float angle() const { return angle_; }
    float object_y() const {
        return object_y_;
    }
};
enum class AnswerCategory { Positive, Uncertain, Negative, Rare };
struct AnswerPair {
    const char *classic;
    const char *it;
    AnswerCategory category;
};
const std::array<AnswerPair, 24> &answer_pairs();
class MagicBall final : public Game {
    Random rng;
    int answer = -1;
    float age = 100, time = 0;
    uint32_t requests_ = 0;
    void request_answer();

  public:
    Metadata metadata() const override {
        return {"magic", "Шар ответов", "Задумай ответ, встряхни шар", false,
                false};
    }
    void start(Difficulty, uint32_t) override;
    void update(const Actions &, float) override;
    void render(Renderer &) const override;
    Result result() const override {
        return {};
    }
    int answer_index() const {
        return answer;
    }
    bool classic_visible() const {
        return answer >= 0 && age >= .25f;
    }
    bool it_visible() const {
        return answer >= 0 && age >= .65f;
    }
    uint32_t requests() const {
        return requests_;
    }
};
} // namespace nc
