#include "legacy_records.hpp"
#include "neurocube/application.hpp"
#include "neurocube/framebuffer.hpp"
#include "neurocube/games.hpp"
#include "neurocube/ui.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
using namespace nc;
static int checks = 0;
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        ++checks;                                                                                  \
        if (!(x)) {                                                                                \
            std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x);                              \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (0)
struct MemoryStore : RecordStore {
    Records data{};
    bool valid = false, fail = false;
    int writes = 0;
    bool load(Records &r) override {
        if (valid)
            r = data;
        return valid;
    }
    bool save(const Records &r) override {
        writes++;
        if (fail)
            return false;
        data = r;
        valid = true;
        return true;
    }
};
struct StubGame : Game {
    Result value;
    int starts = 0, updates = 0, pauses = 0, resumes = 0;
    Metadata metadata() const override {
        return {"stub", "stub", "stub"};
    }
    void start(Difficulty, uint32_t) override {
        starts++;
        value = {};
    }
    void update(const Actions &, float) override {
        updates++;
    }
    void render(Renderer &) const override {}
    void pause() override {
        pauses++;
    }
    void resume() override {
        resumes++;
    }
    Result result() const override {
        return value;
    }
};
void records_test() {
    Records a;
    a.sequence = 0xFFFFFFFE;
    a.best[2][1] = 989;
    auto bytes = encode_records(a);
    Records b;
    CHECK(decode_records(bytes.data(), bytes.size(), b));
    CHECK(b.best == a.best && b.sequence == a.sequence);
    bytes[19] ^= 1;
    CHECK(!decode_records(bytes.data(), bytes.size(), b));
    CHECK(!decode_records(bytes.data(), 200, b));
    CHECK(newer_sequence(0, 0xFFFFFFFF));
    CHECK(!newer_sequence(5, 5));
}
void app_test() {
    MemoryStore store;
    StubGame s, m, c, b;
    GameRegistry reg{{&s, &m, &c, &b}};
    App app(store, reg);
    CHECK(app.screen() == Screen::Menu);
    app.launch(0, false);
    CHECK(s.starts == 1);
    app.pause();
    app.update({}, .02f);
    CHECK(s.updates == 0 && s.pauses == 1);
    app.resume();
    app.update({}, .02f);
    CHECK(s.updates == 1 && s.resumes == 1);
    app.restart();
    CHECK(s.starts == 2);
    app.next();
    CHECK(app.selected() == 1 && app.screen() == Screen::Rules);
    InputFrame touch;
    touch.tap = true;
    touch.y = 90;
    app.update(touch, .02f);
    CHECK(app.screen() == Screen::Playing);
    m.value = {321, true};
    app.update({}, .02f);
    CHECK(app.screen() == Screen::Result);
    CHECK(store.writes == 1 && store.data.best[1][1] == 321);
    app.menu();
    CHECK(store.writes == 1);
    App loaded(store, reg);
    CHECK(loaded.records().best[1][1] == 321);
    loaded.launch(2, false);
    c.value = {17, true};
    store.fail = true;
    loaded.update({}, .02f);
    CHECK(!loaded.save_ok());
    store.fail = false;
    CHECK(loaded.flush());
    CHECK(store.data.best[2][1] == 17);
    InputFrame shake;
    shake.imu = true;
    shake.az = 4;
    loaded.menu();
    loaded.update(shake, .1f);
    CHECK(loaded.screen() == Screen::Menu);
    for (int i = 0; i < 4; i++) {
        loaded.launch(i, false);
        CHECK(loaded.selected() == i);
    }
    loaded.menu();
    touch.y = 220;
    touch.x = 230;
    loaded.update(touch, .02f);
    CHECK(loaded.screen() == Screen::Settings);
}
void gesture_test() {
    GestureRecognizer g;
    InputFrame i;
    i.imu = true;
    for (int k = 0; k < 100; k++)
        CHECK(!g.update(i, .02f).shake);
    i.az = 3;
    CHECK(g.update(i, .02f).shake);
    CHECK(!g.update(i, .02f).shake);
    i.az = 1;
    for (int k = 0; k < 70; k++)
        g.update(i, .02f);
    i.ax = .8f;
    int events = 0;
    for (int k = 0; k < 100; k++) {
        auto a = g.update(i, .02f);
        if (a.direction >= 0) {
            CHECK(a.direction == 1);
            events++;
        }
    }
    CHECK(events == 1);
    i.ax = 0;
    for (int k = 0; k < 50; k++)
        g.update(i, .02f);
    i.ax = -.8f;
    events = 0;
    for (int k = 0; k < 50; k++)
        events += g.update(i, .02f).direction == 3;
    CHECK(events == 1);
    g.calibrate(-.8f, 0);
    for (int k = 0; k < 50; k++)
        CHECK(g.update(i, .02f).direction == -1);
}
void games_test() {
    auto r = default_registry();
    CHECK(r.find("magic") != nullptr);
    CHECK(r.find("missing") == nullptr);
    Random a, b;
    for (int k = 0; k < 100; k++) {
        CHECK(a.next() == b.next());
        CHECK(a.state != 0);
    }
    SequenceGame s;
    s.start(Difficulty::Normal, 123);
    int first = -1;
    for (int k = 0; k < 800 && s.playback(); k++) {
        s.update({}, .01f);
        if (s.cue() >= 0)
            first = s.cue();
    }
    CHECK(first >= 0 && !s.playback());
    Actions action;
    action.direction = first;
    s.update(action, .01f);
    CHECK(s.result().score == 1);
    SequenceGame wrong;
    wrong.start(Difficulty::Normal, 123);
    for (int k = 0; k < 800 && wrong.playback(); k++)
        wrong.update({}, .01f);
    action.direction = (first + 1) % 4;
    wrong.update(action, .01f);
    for (int n = 0; n < 80; n++)
        wrong.update({}, .01f);
    CHECK(wrong.result().finished);
    CommandsGame c;
    c.start(Difficulty::Easy, 7);
    for (int k = 0; k < 30; k++) {
        Actions cmd;
        int e = c.command();
        if (e < 4)
            cmd.direction = e;
        else if (e == 4)
            cmd.twist = true;
        else
            cmd.tap = true;
        if (cmd.tap) {
            cmd.x = int(c.object_x());
            cmd.y = int(c.object_y());
        }
        c.update(cmd, .01f);
        for (int n = 0; n < 60 && c.feedback() > 0; n++)
            c.update({}, .01f);
    }
    CHECK(c.result().finished && c.result().score == 30);
    c.start(Difficulty::Hard, 1);
    c.update({}, 20);
    c.update({}, 1);
    CHECK(c.result().finished);
    CHECK(circle_hits_wall(10, 10, 5, {12, 8, 4, 4}));
    CHECK(!circle_hits_wall(0, 0, 5, {12, 8, 4, 4}));
    MazeGame m;
    m.start(Difficulty::Hard, 1);
    action = {};
    action.tilt_x = 1;
    for (int k = 0; k < 300; k++)
        m.update(action, .02f);
    CHECK(m.ball_x() >= 14 && m.ball_x() <= 226 && m.ball_y() >= 67);
    MagicBall ball;
    ball.start(Difficulty::Easy, 77);
    CHECK(ball.answer_index() == -1);
    Actions ask;
    ask.shake = true;
    ball.update(ask, .01f);
    int prev = ball.answer_index();
    CHECK(prev >= 0);
    ask.shake = false;
    ask.tap = true;
    ball.update(ask, .01f);
    CHECK(ball.answer_index() == prev && ball.requests() == 1);
    ball.update({}, 1);
    ball.update(ask, .01f);
    CHECK(ball.requests() == 2 && !ball.result().finished);
}
void framebuffer_test() {
    Framebuffer f;
    f.clear(0);
    f.rect(-4, -4, 8, 8, Ink);
    CHECK(f.pixels[0] == Ink && f.pixels[5] == 0);
    f.circle(239, 239, 12, Accent);
    f.text(10, 15, "Привет, ёж!", Gold);
    bool ink = false;
    for (int y = 15; y < 31; y++)
        for (int x = 10; x < 120; x++)
            ink |= f.pixels[y * 240 + x] == Gold;
    CHECK(ink);
    MemoryStore store;
    App app(store);
    for (int i = 0; i < 4; i++) {
        app.launch(i);
        app.render(f);
        app.launch(i, false);
        app.update({}, .02f);
        app.render(f);
    }
}

void completed_games_test() {
    SequenceGame sequence;
    Random expected;
    expected.state = 456;
    int pattern[32];
    for (auto &v : pattern)
        v = int(expected.bounded(4));
    sequence.start(Difficulty::Hard, 456);
    for (int round = 1; round <= 32; round++) {
        for (int tick = 0; sequence.playback() && tick < 4000; tick++)
            sequence.update({}, .01f);
        CHECK(!sequence.playback());
        for (int j = 0; j < round; j++) {
            Actions a;
            a.direction = pattern[j];
            while (sequence.playback())
                sequence.update({}, .01f);
            sequence.update(a, .01f);
        }
        CHECK(sequence.result().score == uint32_t(round));
    }
    for (int n = 0; n < 80 && !sequence.result().finished; n++)
        sequence.update({}, .01f);
    CHECK(sequence.result().finished);
    for (int geometry = 0; geometry < 3; geometry++)
        for (int variant = 0; variant < 3; variant++) {
            MazeGame maze;
            maze.configure({uint8_t(geometry), 0, uint8_t(variant)});
            maze.start(Difficulty(geometry), 1);
            const auto &map = maze.map();
            std::array<int, 35> parent;
            parent.fill(-1);
            std::array<int, 35> queue{};
            int head = 0, tail = 1;
            parent[0] = 0;
            const int delta[] = {-map.columns, 1, map.columns, -1};
            while (head < tail) {
                int c = queue[head++];
                for (int d = 0; d < 4; d++)
                    if (map.openings[c] & (1 << d)) {
                        int next = c + delta[d];
                        CHECK(next >= 0 && next < map.columns * map.rows);
                        if (parent[next] < 0) {
                            parent[next] = c;
                            queue[tail++] = next;
                        }
                    }
            }
            CHECK(tail == map.columns * map.rows);
            std::array<int, 35> route{};
            int count = 0, c = map.columns * map.rows - 1;
            while (c) {
                route[count++] = c;
                c = parent[c];
            }
            for (int j = count - 1; j >= 0; j--) {
                int cell = route[j];
                for (int n = 0; n < 2500 && !maze.result().finished; n++) {
                    float dx = map.center_x(cell) - maze.ball_x(),
                          dy = map.center_y(cell) - maze.ball_y();
                    if (dx * dx + dy * dy < .16f)
                        break;
                    Actions a;
                    a.tilt_x = std::clamp(dx * .07f, -.7f, .7f);
                    a.tilt_y = std::clamp(dy * .07f, -.7f, .7f);
                    maze.update(a, .02f);
                }
            }
            CHECK(maze.result().finished && maze.result().won && maze.result().score > 0);
        }
}

void recovery_and_extension_test() {
    Records first, second, loaded;
    first.sequence = 9;
    first.best[0][0] = 33;
    second.sequence = 10;
    second.best[0][0] = 44;
    auto a = encode_records(first), b = encode_records(second);
    CHECK(recover_records(a.data(), b.data(), loaded) == 1 && loaded.best[0][0] == 44);
    b[100] ^= 1;
    CHECK(recover_records(a.data(), b.data(), loaded) == 0 && loaded.best[0][0] == 33);
    a[100] ^= 1;
    CHECK(recover_records(a.data(), b.data(), loaded) == -1);
    MemoryStore store;
    StubGame g1, g2, g3, g4, g5;
    GameRegistry registry{{&g1, &g2, &g3, &g4, &g5}, 5};
    App app(store, registry);
    InputFrame tap;
    tap.tap = true;
    tap.x = 200;
    tap.y = 220;
    app.update(tap, .02f);
    tap.x = 100;
    tap.y = 60;
    app.update(tap, .02f);
    CHECK(app.selected() == 4);
    app.next();
    CHECK(app.selected() == 0);
}
void button_and_priority_test() {
    ButtonRecognizer button;
    for (int n = 0; n < 8; n++) {
        auto e = button.update(n % 2, .005f);
        CHECK(!e.short_press && !e.long_press);
    }
    for (int n = 0; n < 20; n++)
        button.update(false, .01f);
    for (int n = 0; n < 20; n++)
        button.update(true, .01f);
    int shorts = 0, longs = 0;
    for (int n = 0; n < 10; n++) {
        auto e = button.update(false, .01f);
        shorts += e.short_press;
        longs += e.long_press;
    }
    CHECK(shorts == 1 && longs == 0);
    for (int n = 0; n < 150; n++) {
        auto e = button.update(true, .01f);
        shorts += e.short_press;
        longs += e.long_press;
    }
    for (int n = 0; n < 10; n++) {
        auto e = button.update(false, .01f);
        shorts += e.short_press;
        longs += e.long_press;
    }
    CHECK(shorts == 1 && longs == 1);
    MemoryStore store;
    StubGame g1, g2, g3, g4;
    GameRegistry registry{{&g1, &g2, &g3, &g4}};
    App app(store, registry);
    app.launch(0, false);
    InputFrame frame;
    frame.imu = true;
    frame.az = 3;
    frame.tap = true;
    frame.x = 30;
    frame.y = 220;
    app.update(frame, .02f);
    CHECK(app.screen() == Screen::Paused && g1.starts == 1);
}

void calibration_and_migration_test() {
    for (auto *fixture : {&legacy_v1, &legacy_v2}) {
        Records r;
        CHECK(decode_records(fixture->data(), fixture->size(), r));
        CHECK(r.best[0][1] == 42 && r.best[1][1] == 700 && r.best[2][1] == 8);
        auto page = encode_records(r);
        Records migrated;
        CHECK(decode_records(page.data(), page.size(), migrated));
        CHECK(migrated.best == r.best);
    }
    struct Capture : StubGame {
        Actions last;
        void update(const Actions &a, float dt) override {
            StubGame::update(a, dt);
            last = a;
        }
    };
    Capture g;
    MemoryStore store;
    App app(store, {{&g, &g, &g, &g}});
    InputFrame tap;
    tap.tap = true;
    tap.x = 100;
    tap.y = 220;
    app.update(tap, .02f);
    CHECK(app.screen() == Screen::Settings);
    tap.y = 155;
    app.update(tap, .02f);
    CHECK(app.screen() == Screen::Calibration);
    CHECK(store.writes == 0);
    tap.imu = true;
    tap.ax = .31f;
    tap.ay = -.23f;
    tap.y = 120;
    app.update(tap, .1f);
    tap.tap = false;
    for (int n = 0; n < 8; n++)
        app.update(tap, .1f);
    CHECK(app.records().calibrated && store.writes == 1);
    CHECK(std::abs(app.records().calibration_x - .31f) < .001f);
    App restored(store, {{&g, &g, &g, &g}});
    restored.launch(0, false);
    for (int n = 0; n < 20; n++)
        restored.update(tap, .05f);
    CHECK(std::abs(g.last.tilt_x) < .001f && std::abs(g.last.tilt_y) < .001f);
    App splash(store, {{&g, &g, &g, &g}}, true);
    CHECK(splash.screen() == Screen::Splash);
    splash.update({}, .1f);
    CHECK(splash.screen() == Screen::Splash);
    tap.tap = true;
    splash.update(tap, .01f);
    CHECK(splash.screen() == Screen::Menu);
    splash.launch(0, false);
    CHECK(splash.screen() != Screen::Splash);
    CHECK(ui::CalibrationItem.contains(12, 150));
    CHECK(!ui::CalibrationItem.contains(12, 180));
    CHECK(ui::layout("Привет, Андрей", 232, 44, 2).fits);
}
void polish_test() {
    ContactCounter counter;
    for (int n = 0; n < 200; n++)
        counter.update(true, .02f);
    CHECK(counter.count() == 1);
    counter.update(false, .1f);
    counter.update(true, .02f);
    CHECK(counter.count() == 1);
    counter.update(false, .15f);
    counter.update(true, .02f);
    CHECK(counter.count() == 2);
    MagicBall ball;
    ball.start(Difficulty::Easy, 987);
    Actions ask;
    ask.tap = ask.shake = true;
    ball.update(ask, .01f);
    CHECK(ball.requests() == 1 && !ball.classic_visible());
    ball.update({}, .25f);
    CHECK(ball.classic_visible() && !ball.it_visible());
    ball.update({}, .41f);
    CHECK(ball.it_visible());
    int counts[4]{};
    for (int n = 0; n < 50000; n++) {
        ball.update(ask, 1);
        counts[int(answer_pairs()[ball.answer_index()].category)]++;
    }
    CHECK(counts[3] > 1500 && counts[3] < 2500);
    CHECK(counts[0] > 20500 && counts[0] < 22700);
    CHECK(counts[1] > 11000 && counts[1] < 13000);
    CHECK(counts[2] > 13500 && counts[2] < 15300);
    for (const auto &p : answer_pairs()) {
        CHECK(ui::layout(p.classic, 196, 36).fits);
        CHECK(ui::layout(p.it, 196, 26).fits);
    }
    CHECK(std::string(answer_pairs()[23].it) == "Нужна перезагрузка");
    for (const char *s : {"Вверх", "Вправо", "Вниз", "Влево", "Поверни", "Коснись"})
        CHECK(ui::layout(s, 220, 36, 2).fits);
    MemoryStore store;
    App app(store);
    InputFrame tap;
    tap.tap = true;
    tap.x = 100;
    tap.y = 100;
    app.launch(0);
    CHECK(app.screen() == Screen::Rules);
    app.update(tap, .02f);
    CHECK(app.screen() == Screen::Countdown);
    for (int n = 0; n < 20; n++)
        app.update(tap, .1f);
    CHECK(app.screen() == Screen::Countdown && app.game().result().score == 0);
    for (int n = 0; n < 11; n++)
        app.update({}, .1f);
    CHECK(app.screen() == Screen::Playing);
    app.restart();
    CHECK(app.screen() == Screen::Rules);
    app.launch(1);
    CHECK(app.screen() == Screen::MazeSetup);
    tap.x = 216;
    tap.y = 75;
    app.update(tap, .02f);
    CHECK(app.screen() == Screen::MazeSetup);
    tap.y = 135;
    app.update(tap, .02f);
    CHECK(app.screen() == Screen::MazeSetup);
    tap.y = 180;
    app.update(tap, .02f);
    CHECK(app.screen() == Screen::Playing);
    app.restart();
    CHECK(app.screen() == Screen::MazeSetup);
    app.launch(3, false);
    tap.x = 3;
    tap.y = 36;
    app.update(tap, .02f);
    auto &magic = static_cast<MagicBall &>(app.game());
    CHECK(magic.requests() == 1);
    for (int n = 0; n < 10; n++)
        app.update({}, .1f);
    tap.y = 220;
    tap.x = 30;
    app.update(tap, .02f);
    CHECK(app.screen() == Screen::Paused && magic.requests() == 1);
    Records r;
    r.best[1][1] = 885;
    r.maze_best[2][1][5] = 777;
    r.calibrated = true;
    r.calibration_x = .2f;
    auto bytes = encode_records(r);
    Records back;
    CHECK(decode_records(bytes.data(), bytes.size(), back));
    CHECK(back.best[1][1] == 885 && back.maze_best[2][1][5] == 777 &&
          back.maze_best[2][1][4] == 0 && back.calibrated);
}
int main() {
    polish_test();
    records_test();
    app_test();
    gesture_test();
    games_test();
    framebuffer_test();
    completed_games_test();
    recovery_and_extension_test();
    button_and_priority_test();
    calibration_and_migration_test();
    std::printf("PASS %d checks (portable native tests)\n", checks);
}
