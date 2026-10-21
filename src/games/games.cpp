#include "neurocube/games.hpp"
#include "neurocube/ui.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
namespace nc {
static const uint16_t colors[] = {0x3E7F, 0xFEA0, 0xFB0C, 0x9BFF};
static const char *directions[] = {"Вверх", "Вправо", "Вниз", "Влево", "Поверни", "Коснись"};
static void score(Renderer &r, uint32_t s) {
    char b[48];
    std::snprintf(b, sizeof b, "Счёт: %lu", (unsigned long)s);
    r.text(12, 28, b, Gold);
}
void SequenceGame::start(Difficulty d, uint32_t seed) {
    rng.state = seed ? seed : 1;
    length = 1;
    cursor = 0;
    timer = -.7f;
    shown = -1;
    phase_ = SequencePhase::Showing;
    outcome = {};
    step = 1.4f - int(d) * .18f;
    for (auto &v : sequence)
        v = int(rng.bounded(4));
}
void SequenceGame::update(const Actions &a, float dt) {
    if (outcome.finished)
        return;
    if (phase_ == SequencePhase::Failure) {
        timer -= dt;
        if (timer <= 0)
            outcome.finished = true;
        return;
    }
    if (phase_ == SequencePhase::Correct) {
        timer -= dt;
        if (timer > 0)
            return;
        shown = -1;
        if (cursor == length) {
            if (length == 32) {
                phase_ = SequencePhase::Complete;
                outcome.finished = true;
                outcome.won = true;
                return;
            }
            length++;
            cursor = 0;
            timer = -.7f;
            phase_ = SequencePhase::Showing;
        } else
            phase_ = SequencePhase::Recall;
        return;
    }
    if (phase_ == SequencePhase::Showing) {
        timer += dt;
        if (timer < 0) {
            shown = -1;
            return;
        }
        int slot = int(timer / step);
        if (slot >= length) {
            phase_ = SequencePhase::Recall;
            timer = 0;
            cursor = 0;
            shown = -1;
        } else
            shown = std::fmod(timer, step) < step * .65f ? sequence[slot] : -1;
        return;
    }
    int direction = a.direction;
    if (a.tap) {
        if (a.y < 110)
            direction = 0;
        else if (a.y > 162)
            direction = 2;
        else
            direction = a.x > 120 ? 1 : 3;
    }
    if (direction < 0 || direction > 3)
        return;
    shown = direction;
    if (direction != sequence[cursor]) {
        phase_ = SequencePhase::Failure;
        timer = .75f;
        return;
    }
    cursor++;
    if (cursor == length)
        outcome.score = uint32_t(length);
    phase_ = SequencePhase::Correct;
    timer = .36f;
}
void SequenceGame::render(Renderer &r) const {
    score(r, outcome.score);
    const char *label = phase_ == SequencePhase::Showing   ? "Смотри и запоминай"
                        : phase_ == SequencePhase::Correct ? "Верно!"
                        : phase_ == SequencePhase::Failure ? "Не тот огонь"
                                                           : "Твоя очередь";
    ui::text_box(r, {10, 48, 220, 22}, label, phase_ == SequencePhase::Failure ? Red : Ink);
    const int xs[] = {120, 184, 120, 56}, ys[] = {93, 137, 181, 137};
    for (int i = 0; i < 4; i++) {
        bool active = i == shown;
        int grow = active ? 3 : 0;
        uint16_t c = active ? (phase_ == SequencePhase::Failure ? Red : Ink) : colors[i];
        if (active)
            r.rect(xs[i] - 29 - grow, ys[i] - 22 - grow, 58 + 2 * grow, 44 + 2 * grow, c);
        r.rect(xs[i] - 26, ys[i] - 19, 52, 38, active ? colors[i] : 0x18C9);
        if (i == 0) {
            for (int y = 0; y < 20; y++)
                r.rect(xs[i] - y / 2, ys[i] - 10 + y, y + 1, 1, c);
        } else if (i == 1) {
            for (int y = -11; y <= 11; y++)
                r.rect(xs[i] - (11 - std::abs(y)), ys[i] + y, 2 * (11 - std::abs(y)) + 1, 1, c);
        } else if (i == 2)
            r.circle(xs[i], ys[i], 11, c);
        else
            r.rect(xs[i] - 11, ys[i] - 11, 22, 22, c);
    }
}
bool circle_hits_wall(float x, float y, float radius, const Wall &w) {
    float cx = std::clamp(x, w.x, w.x + w.w), cy = std::clamp(y, w.y, w.y + w.h);
    return (x - cx) * (x - cx) + (y - cy) * (y - cy) < radius * radius;
}
MazeMap make_maze(int geometry, uint32_t seed) {
    geometry = std::clamp(geometry, 0, 2);
    MazeMap m;
    m.columns = 5 + std::clamp(geometry, 0, 2);
    m.rows = geometry ? 5 : 4;
    Random rng;
    rng.state = seed ^ (0xC0B0123u + uint32_t(geometry) * 97u);
    if (!rng.state) rng.state = 1;
    std::array<bool, 35> visited{};
    std::array<int, 35> stack{};
    int top = 0;
    // Vary the DFS root as well as branch choices; start/goal remain fixed for play.
    stack[0] = int(rng.bounded(uint32_t(m.columns * m.rows)));
    visited[stack[0]] = true;
    const int dx[] = {0, 1, 0, -1}, dy[] = {-1, 0, 1, 0};
    while (top >= 0) {
        int c = stack[top], choices[4], n = 0;
        for (int d = 0; d < 4; d++) {
            int nx = c % m.columns + dx[d], ny = c / m.columns + dy[d];
            if (nx >= 0 && nx < m.columns && ny >= 0 && ny < m.rows &&
                !visited[ny * m.columns + nx])
                choices[n++] = d;
        }
        if (!n) {
            top--;
            continue;
        }
        int d = choices[rng.bounded(n)], next = c + dx[d] + dy[d] * m.columns;
        m.openings[c] |= uint8_t(1 << d);
        m.openings[next] |= uint8_t(1 << ((d + 2) % 4));
        visited[next] = true;
        stack[++top] = next;
    }
    auto wall = [&](float x, float y, float w, float h) { m.walls[m.wall_count++] = {x, y, w, h}; };
    wall(8, 61, 224, 2);
    wall(8, 201, 224, 2);
    wall(8, 61, 2, 142);
    wall(230, 61, 2, 142);
    float cw = m.cell_width(), ch = m.cell_height();
    for (int c = 0; c < m.columns * m.rows; c++) {
        int col = c % m.columns, row = c / m.columns;
        if (col < m.columns - 1 && !(m.openings[c] & 2))
            wall(10 + (col + 1) * cw - 1, 63 + row * ch, 2, ch);
        if (row < m.rows - 1 && !(m.openings[c] & 4))
            wall(10 + col * cw, 63 + (row + 1) * ch - 1, cw, 2);
    }
    return m;
}
void MazeGame::start(Difficulty d, uint32_t seed) {
    options_.geometry = uint8_t(std::clamp(int(d), 0, 2));
    map_ = make_maze(options_.geometry, seed);
    x = map_.center_x(0);
    y = map_.center_y(0);
    vx = vy = elapsed = 0;
    contacts_.reset();
    outcome = {};
}
void MazeGame::update(const Actions &a, float dt) {
    if (outcome.finished)
        return;
    dt = std::clamp(dt, 0.f, .1f);
    elapsed += dt;
    vx = (vx + std::clamp(a.tilt_x, -1.f, 1.f) * 220 * dt) * std::exp(-dt * 4);
    vy = (vy + std::clamp(a.tilt_y, -1.f, 1.f) * 220 * dt) * std::exp(-dt * 4);
    bool touched = false;
    auto hit = [&](float px, float py, float radius) {
        for (int i = 0; i < map_.wall_count; i++)
            if (circle_hits_wall(px, py, radius, map_.walls[i]))
                return true;
        return false;
    };
    for (int n = 0; n < 8; n++) {
        float nx = x + vx * dt / 8;
        if (hit(nx, y, 4)) {
            vx = 0;
            touched = true;
        } else
            x = nx;
        float ny = y + vy * dt / 8;
        if (hit(x, ny, 4)) {
            vy = 0;
            touched = true;
        } else
            y = ny;
    }
    // Nearness only delays rearming; it never creates a penalty without collision.
    contacts_.update(touched, dt, !hit(x, y, 5.5f));
    outcome.contacts = contacts_.count();
    int limit = ContactLimits[std::min<int>(options_.contact_index, 5)];
    if (limit && outcome.contacts > limit) {
        outcome.finished = true;
        return;
    }
    int goal = map_.columns * map_.rows - 1;
    float dx = x - map_.center_x(goal), dy = y - map_.center_y(goal);
    if (dx * dx + dy * dy < 49) {
        outcome.score = uint32_t(std::max(1.f, 2000 - elapsed * 5 - outcome.contacts * 15));
        outcome.finished = outcome.won = true;
    }
}
void MazeGame::render(Renderer &r) const {
    char b[80];
    int limit = ContactLimits[std::min<int>(options_.contact_index, 5)];
    if (limit)
        std::snprintf(b, sizeof b, "Касания: %u / %d", outcome.contacts, limit);
    else
        std::snprintf(b, sizeof b, "Касания: %u / без лимита", outcome.contacts);
    ui::text_box(r, {4, 28, 232, 30}, b, Gold);
    r.rect(8, 61, 224, 142, 0x18E7);
    int goal = map_.columns * map_.rows - 1;
    r.circle(int(map_.center_x(goal)), int(map_.center_y(goal)), 8, Accent);
    for (int i = 0; i < map_.wall_count; i++) {
        auto &w = map_.walls[i];
        r.rect(int(w.x), int(w.y), int(std::ceil(w.w)), int(std::ceil(w.h)), Muted);
    }
    r.circle(int(x), int(y), 4, Gold);
}
void CommandsGame::choose() {
    expected = int(rng.bounded(6));
    remaining = limit;
    object_x_ = 120;
    object_y_ = 132;
}
void CommandsGame::start(Difficulty d, uint32_t seed) {
    rng.state = seed ? seed : 1;
    outcome = {};
    limit = 3.8f - int(d) * .5f;
    feedback_ = angle_ = 0;
    failed_ = false;
    object_x_ = 120;
    object_y_ = 132;
    choose();
}
void CommandsGame::update(const Actions &a, float dt) {
    if (outcome.finished)
        return;
    dt = std::clamp(dt, 0.f, .1f);
    // Largest outline radius is 27; keep it between command and hint.
    object_x_ = std::clamp(object_x_ + a.tilt_x * 120.f * dt, 37.f, 203.f);
    object_y_ = std::clamp(object_y_ + a.tilt_y * 120.f * dt, 117.f, 139.f);
    angle_ = std::fmod(angle_ + a.turn_rate * .01745329252f * dt, 6.283185307f);
    if (feedback_ > 0) {
        feedback_ -= dt;
        if (feedback_ <= 0) {
            if (failed_) {
                outcome.finished = true;
                return;
            }
            if (outcome.score >= 30) {
                outcome.finished = true;
                outcome.won = true;
                return;
            }
            choose();
        }
        return;
    }
    remaining -= dt;
    if (remaining <= 0) {
        failed_ = true;
        feedback_ = .7f;
        return;
    }
    int action = a.direction;
    if (a.twist)
        action = 4;
    if (a.tap && (a.x - object_x_) * (a.x - object_x_) +
                     (a.y - object_y_) * (a.y - object_y_) <= 30 * 30)
        action = 5;
    if (action < 0)
        return;
    if (action != expected) {
        failed_ = true;
        feedback_ = .7f;
        return;
    }
    outcome.score++;
    feedback_ = .55f;
    limit = std::max(1.2f, limit * .97f);

}
void CommandsGame::render(Renderer &r) const {
    score(r, outcome.score);
    r.rect(8, 49, 224, 39, 0x212B);
    ui::text_box(r, {10, 50, 220, 36},
                 feedback_ > 0 ? (failed_ ? "Ошибка" : "Верно!") : directions[expected],
                 failed_ ? Red : Ink, 2);
    uint16_t color = failed_ ? Red : feedback_ > 0 ? Accent : Gold;
    int radius = expected == 5 ? 24 : 18;
    r.circle(int(object_x_), int(object_y_), radius + 3, Ink);
    r.circle(int(object_x_), int(object_y_), radius, color);
    int dx = int(std::cos(angle_) * 13), dy = int(std::sin(angle_) * 13);
    ui::line(r, int(object_x_), int(object_y_), int(object_x_) + dx, int(object_y_) + dy, Bg);
    r.circle(int(object_x_) + dx, int(object_y_) + dy, 3, Bg);
    ui::text_box(r, {10, 168, 220, 19},
                 expected == 4   ? "Поворот вокруг экрана"
                 : expected == 5 ? "Коснись светлого круга"
                                 : "Наклон двигает круг",
                 Muted);
    r.rect(12, 193, 216, 8, 0x2128);
    r.rect(12, 193, int(216 * std::clamp(remaining / limit, 0.f, 1.f)), 8, Accent);
}
const std::array<AnswerPair, 24> &answer_pairs() {
    static const std::array<AnswerPair, 24> pairs{{
        {"Бесспорно", "Тесты зелёные", AnswerCategory::Positive},
        {"Несомненно", "Апрув получен", AnswerCategory::Positive},
        {"Определённо да", "Мержим в main", AnswerCategory::Positive},
        {"Можешь рассчитывать", "SLA гарантирует", AnswerCategory::Positive},
        {"Скорее всего", "На staging работает", AnswerCategory::Positive},
        {"Хорошие перспективы", "Релиз одобрен", AnswerCategory::Positive},
        {"Да", "HTTP 200 OK", AnswerCategory::Positive},
        {"Знаки говорят «да»", "CI пропустил", AnswerCategory::Positive},
        {"Да, определённо", "В проде работает", AnswerCategory::Positive},
        {"Без сомнений", "Баг не воспроизвёлся", AnswerCategory::Positive},
        {"Ответ неясен", "Нужны логи", AnswerCategory::Uncertain},
        {"Спроси позже", "Ждём деплоя", AnswerCategory::Uncertain},
        {"Сейчас не скажу", "Request timeout", AnswerCategory::Uncertain},
        {"Невозможно предсказать", "Undefined behavior", AnswerCategory::Uncertain},
        {"Спроси ещё раз", "Повтори запрос", AnswerCategory::Uncertain},
        {"Не рассчитывай", "Доступ запрещён", AnswerCategory::Negative},
        {"Мой ответ — нет", "HTTP 403 Forbidden", AnswerCategory::Negative},
        {"Источники говорят «нет»", "Тесты упали", AnswerCategory::Negative},
        {"Перспективы не очень", "Техдолг победил", AnswerCategory::Negative},
        {"Весьма сомнительно", "Не прошло ревью", AnswerCategory::Negative},
        {"Всё возможно", "Но сначала бэкап", AnswerCategory::Rare},
        {"Судьба решит", "Зависит от DNS", AnswerCategory::Rare},
        {"Не сегодня", "Пятница, 17:59", AnswerCategory::Rare},
        {"Попробуй ещё", "Нужна перезагрузка", AnswerCategory::Rare},
    }};
    return pairs;
}
void MagicBall::start(Difficulty, uint32_t seed) {
    rng.state = seed ? seed : 1;
    answer = -1;
    age = 100;
    time = 0;
    requests_ = 0;
}
void MagicBall::request_answer() {
    if (age < .85f)
        return;
    uint32_t roll = rng.bounded(10000);
    if (roll < 400)
        answer = 20 + int(rng.bounded(4));
    else {
        uint32_t category = rng.bounded(100);
        answer = category < 45   ? int(rng.bounded(10))
                 : category < 70 ? 10 + int(rng.bounded(5))
                                 : 15 + int(rng.bounded(5));
    }
    age = 0;
    requests_++;
}
void MagicBall::update(const Actions &a, float dt) {
    time += dt;
    age += dt;
    if (a.shake || a.tap)
        request_answer();
}
void MagicBall::render(Renderer &r) const {
    uint16_t glow = answer < 0                                                    ? 0x8CFF
                    : answer_pairs()[answer].category == AnswerCategory::Positive ? Accent
                    : answer_pairs()[answer].category == AnswerCategory::Negative ? 0xF9CD
                                                                                  : Gold;
    ui::orb(r, time + (age < .65f ? age * 5 : 0), glow);
    if (classic_visible()) {
        ui::text_box(r, {22, 94, 196, 36}, answer_pairs()[answer].classic, Ink);
        if (it_visible())
            ui::text_box(r, {22, 134, 196, 26}, answer_pairs()[answer].it, Gold);
    } else
        ui::text_box(r, {22, 101, 196, 40}, answer < 0 ? "Задумай ответ, встряхни шар" : "Шар размышляет...",
                     Ink);
    ui::button(r, {12, 177, 216, 25}, "Спросить ещё", Accent);
}

} // namespace nc
