#pragma once
#include "domain.hpp"
namespace nc {
class SequenceGame final : public Game {
    std::array<int, 32> sequence{};
    Random rng;
    int length = 1, cursor = 0, shown = -1;
    float timer = 0, step = .65f;
    bool showing = true;
    Result outcome;

  public:
    Metadata metadata() const override {
        return {"sequence", "Повтори ритм", "Запомни огни. Повтори наклонами или касаниями."};
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
        return showing;
    }
};
struct Wall {
    float x, y, w, h;
};
bool circle_hits_wall(float x, float y, float radius, const Wall &);
class MazeGame final : public Game {
    float x = 22, y = 65, vx = 0, vy = 0, elapsed = 0;
    Difficulty level{};
    Result outcome;

  public:
    Metadata metadata() const override {
        return {"maze", "Лабиринт", "Наклоняй куб. Доведи шар до зелёной цели."};
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
};
class CommandsGame final : public Game {
    Random rng;
    int expected = 0;
    float remaining = 0, limit = 2.8f;
    Result outcome;
    void choose();

  public:
    Metadata metadata() const override {
        return {"commands", "Лови команду", "Наклон, поворот или касание. Успей за подсказкой!"};
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
};
class MagicBall final : public Game {
    Random rng;
    int answer = -1;
    float animation = 0, time = 0;

  public:
    Metadata metadata() const override {
        return {"magic", "Шар ответов", "Задай вопрос. Встряхни куб или коснись кнопки.", false,
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
};
} // namespace nc
