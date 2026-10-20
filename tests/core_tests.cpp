#include "neurocube/application.hpp"
#include "neurocube/framebuffer.hpp"
#include "neurocube/games.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
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
    for (int k = 0; k < 200 && s.playback(); k++) {
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
    for (int k = 0; k < 100; k++)
        wrong.update({}, .01f);
    action.direction = (first + 1) % 4;
    wrong.update(action, .01f);
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
        c.update(cmd, .01f);
    }
    CHECK(c.result().finished && c.result().score == 30);
    c.start(Difficulty::Hard, 1);
    c.update({}, 20);
    CHECK(c.result().finished);
    CHECK(circle_hits_wall(10, 10, 5, {12, 8, 4, 4}));
    CHECK(!circle_hits_wall(0, 0, 5, {12, 8, 4, 4}));
    MazeGame m;
    m.start(Difficulty::Hard, 1);
    action = {};
    action.tilt_x = 1;
    for (int k = 0; k < 300; k++)
        m.update(action, .02f);
    CHECK(m.ball_x() < 60 && m.ball_y() >= 55);
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
    CHECK(ball.answer_index() != prev && !ball.result().finished);
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
            sequence.update(a, .01f);
        }
        CHECK(sequence.result().score == uint32_t(round));
    }
    CHECK(sequence.result().finished);
    MazeGame maze;
    maze.start(Difficulty::Easy, 1);
    const float targets[][2] = {{26, 172}, {100, 172}, {100, 70},
                                {160, 70}, {160, 178}, {222, 182}};
    for (auto &target : targets) {
        for (int n = 0; n < 1000 && !maze.result().finished; n++) {
            float dx = target[0] - maze.ball_x(), dy = target[1] - maze.ball_y();
            if (dx * dx + dy * dy < 9)
                break;
            Actions a;
            a.tilt_x = std::clamp(dx * .04f, -.8f, .8f);
            a.tilt_y = std::clamp(dy * .04f, -.8f, .8f);
            maze.update(a, .02f);
        }
    }
    CHECK(maze.result().finished && maze.result().score > 0);
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
int main() {
    records_test();
    app_test();
    gesture_test();
    games_test();
    framebuffer_test();
    completed_games_test();
    recovery_and_extension_test();
    std::printf("PASS %d checks (portable native tests)\n", checks);
}
