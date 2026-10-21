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
#include <set>
#include <vector>
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
    for (int n = 0; n < 300; ++n) c.update({}, .02f);
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

std::vector<int> observe_round(SequenceGame &game) {
    std::vector<int> cues;
    int previous = -1;
    for (int tick = 0; game.playback() && tick < 5000; ++tick) {
        game.update({}, .01f);
        int cue = game.phase() == SequencePhase::Showing ? game.cue() : -1;
        if (cue >= 0 && cue != previous) cues.push_back(cue);
        previous = cue;
    }
    return cues;
}
void answer_round(SequenceGame &game, const std::vector<int> &cues) {
    for (int cue : cues) {
        while (game.playback()) game.update({}, .01f);
        Actions a; a.direction = cue; game.update(a, .01f);
    }
}

void completed_games_test() {
    SequenceGame sequence;
    sequence.start(Difficulty::Hard, 456);
    std::vector<int> previous;
    for (int round = 1; round <= 32; round++) {
        auto cues = observe_round(sequence);
        CHECK(cues.size() == size_t(round));
        CHECK(std::equal(previous.begin(), previous.end(), cues.begin()));
        previous = cues;
        answer_round(sequence, cues);
        CHECK(sequence.result().score == uint32_t(round));
    }
    for (int n = 0; n < 80 && !sequence.result().finished; n++)
        sequence.update({}, .01f);
    CHECK(sequence.result().finished);
    for (int geometry = 0; geometry < 3; geometry++)
        for (int variant = 0; variant < 100; variant++) {
            MazeGame maze;
            maze.configure({uint8_t(geometry), 0, uint8_t(variant)});
            maze.start(Difficulty(geometry), uint32_t(variant));
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
    CHECK(splash.screen() == Screen::ProjectQr);
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
    for (int limit_index : {0, 1}) {
        MazeGame maze;
        maze.configure({0, uint8_t(limit_index), 0});
        maze.start(Difficulty::Easy, 1);
        for (int collision = 1; collision <= 4; collision++) {
            Actions left;
            left.tilt_x = -1;
            for (int n = 0; n < 350; n++)
                maze.update(left, .02f);
            CHECK(maze.contacts() == collision);
            CHECK(maze.result().finished == (limit_index == 1 && collision == 4));
            Actions away;
            away.tilt_x = 1;
            for (int n = 0; n < 20; n++)
                maze.update(away, .02f);
        }
    }
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
void idle_test() {
    IdleController wall_clock;
    wall_clock.update(true, false, 119.5f);
    CHECK(wall_clock.phase() == IdlePhase::Awake);
    CHECK(wall_clock.update(true, false, .5f) == IdleEvent::Started);
    IdleController timer;
    for (int n = 0; n < 1199; n++)
        timer.update(true, false, .1f);
    CHECK(timer.phase() == IdlePhase::Awake);
    CHECK(timer.update(true, false, .1f) == IdleEvent::Started);
    CHECK(timer.phase() == IdlePhase::Crawl && timer.episode() == 0);
    for (int n = 0; n < 2000 && timer.phase() == IdlePhase::Crawl; n++) {
        CHECK(!crawl_finished(timer.progress(), crawl_story(timer.episode())));
        timer.update(false, false, .1f);
    }
    CHECK(timer.phase() == IdlePhase::Fading);
    CHECK(crawl_finished(timer.progress(), crawl_story(timer.episode())));
    CHECK(timer.brightness() == 255);
    for (int n = 0; n < 5; n++)
        timer.update(false, false, .1f);
    CHECK(timer.brightness() > 0 && timer.brightness() < 255);
    for (int n = 0; n < 20; n++)
        timer.update(false, false, .1f);
    CHECK(timer.phase() == IdlePhase::Dark && timer.brightness() == 0);
    float final_progress = timer.progress();
    for (int n = 0; n < 3000; n++)
        timer.update(false, false, .1f);
    CHECK(timer.phase() == IdlePhase::Dark && timer.progress() == final_progress);
    CHECK(timer.update(false, true, .02f) == IdleEvent::Woke && timer.brightness() == 255);
    for (int n = 0; n < 60; n++)
        CHECK(timer.update(true, true, .02f) == IdleEvent::Consumed);
    for (int n = 0; n < 5; n++)
        timer.update(true, false, .02f);
    for (int n = 0; n < 1200; n++)
        timer.update(true, false, .1f);
    CHECK(timer.phase() == IdlePhase::Crawl && timer.episode() == 1);
    IdleController reset;
    for (int n = 0; n < 1000; n++)
        reset.update(true, false, .1f);
    reset.update(true, true, .1f);
    CHECK(reset.idle_seconds() == 0);
    for (int n = 0; n < 1300; n++)
        reset.update(false, false, .1f);
    CHECK(reset.phase() == IdlePhase::Awake && reset.idle_seconds() == 0);

    auto wait = [](App &app, int ticks) {
        for (int n = 0; n < ticks; n++)
            app.update({}, .1f);
    };
    MemoryStore store;
    App app(store);
    wait(app, 1199);
    CHECK(app.screen() == Screen::Menu);
    wait(app, 1);
    CHECK(app.screen() == Screen::Crawl);
    InputFrame motion;
    motion.imu = true;
    motion.ax = 3;
    motion.az = 4;
    motion.gz = 400;
    for (int n = 0; n < 500; n++)
        app.update(motion, .1f);
    CHECK(app.screen() == Screen::DisplayIdle);
    InputFrame touch;
    touch.tap = touch.touch = true;
    touch.x = 100;
    touch.y = 60;
    app.update(touch, .02f);
    CHECK(app.screen() == Screen::Menu);
    touch.tap = false;
    for (int n = 0; n < 100; n++)
        app.update(touch, .02f);
    CHECK(app.screen() == Screen::Menu);
    touch.touch = false;
    wait(app, 1);
    touch.tap = true;
    app.update(touch, .02f);
    CHECK(app.screen() == Screen::Rules);
    wait(app, 1800);
    CHECK(app.screen() == Screen::Rules);
    app.launch(3, false);
    wait(app, 1800);
    CHECK(app.screen() == Screen::Playing);
    app.pause();
    wait(app, 1800);
    CHECK(app.screen() == Screen::Paused);
    app.menu();
    touch.x = 100;
    touch.y = 220;
    app.update(touch, .02f);
    wait(app, 1800);
    CHECK(app.screen() == Screen::Settings);
    touch.y = 155;
    app.update(touch, .02f);
    wait(app, 1800);
    CHECK(app.screen() == Screen::Calibration);
    app.launch(1);
    wait(app, 1800);
    CHECK(app.screen() == Screen::MazeSetup);
    App splash(store, default_registry(), true);
    wait(splash, 20);
    CHECK(splash.screen() == Screen::Splash);
    touch.y = 100;
    splash.update(touch, .02f);
    wait(splash, 1800);
    CHECK(splash.screen() == Screen::ProjectQr);
    splash.update(touch, .02f);
    CHECK(splash.screen() == Screen::Menu);
    App timed_splash(store, default_registry(), true);
    wait(timed_splash, 24);
    CHECK(timed_splash.screen() == Screen::Menu);

    StubGame game;
    App result(store, {{&game, &game, &game, &game}});
    result.launch(0, false);
    game.value = {321, true};
    result.update({}, .02f);
    int writes = store.writes;
    wait(result, 1200);
    CHECK(result.screen() == Screen::Crawl);
    InputFrame button;
    button.button_down = true;
    result.update(button, .02f);
    CHECK(result.screen() == Screen::Result);
    button.long_button = true;
    result.update(button, .02f);
    CHECK(result.screen() == Screen::Result);
    button = {};
    result.update(button, .02f);
    button.short_button = true;
    result.update(button, .02f);
    CHECK(result.screen() == Screen::Result);
    wait(result, 2);
    CHECK(store.writes == writes && result.game().result().score == 321);
    touch.y = 100;
    result.update(touch, .02f);
    CHECK(result.screen() == Screen::Rules);

    struct BoundsRenderer : Renderer {
        Framebuffer f;
        bool valid = true;
        void clear(uint16_t c) override {
            f.clear(c);
        }
        void rect(int x, int y, int w, int h, uint16_t c) override {
            valid &= x >= 0 && y >= 0 && w > 0 && h > 0 && x + w <= 240 && y + h <= 240;
            f.rect(x, y, w, h, c);
        }
        void circle(int x, int y, int radius, uint16_t c) override {
            f.circle(x, y, radius, c);
        }
        void text(int x, int y, const char *s, uint16_t c, int scale = 1) override {
            f.text(x, y, s, c, scale);
        }
    } renderer;
    for (int episode = 0; episode < 3; episode++)
        for (float progress = 0; progress < 3000; progress += 13)
            render_crawl(renderer, progress, episode);
    CHECK(renderer.valid);
    render_crawl(renderer, 900, 0);
    int yellow = 0;
    for (auto pixel : renderer.f.pixels)
        yellow += pixel == 0xFFE0;
    CHECK(yellow > 100);
}

void review_generation_test() {
    for (int geometry = 0; geometry < 3; ++geometry) {
        std::set<std::array<uint8_t, 35>> distinct;
        for (uint32_t seed = 0; seed < 2000; ++seed) {
            auto m = make_maze(geometry, seed), same = make_maze(geometry, seed);
            CHECK(m.openings == same.openings && m.wall_count == same.wall_count);
            int cells = m.columns * m.rows;
            CHECK(cells <= 35 && m.wall_count <= int(m.walls.size()));
            CHECK(m.wall_count == (m.columns - 1) * (m.rows - 1) + 4);
            distinct.insert(m.openings);
            std::array<bool, 35> visited{};
            std::array<int, 35> queue{};
            int head = 0, tail = 1, edges = 0;
            visited[0] = true;
            const int delta[] = {-m.columns, 1, m.columns, -1};
            while (head < tail) {
                int c = queue[head++];
                for (int d = 0; d < 4; ++d) if (m.openings[c] & (1 << d)) {
                    int next = c + delta[d];
                    CHECK(next >= 0 && next < cells);
                    CHECK(d != 1 || c % m.columns < m.columns - 1);
                    CHECK(d != 3 || c % m.columns > 0);
                    CHECK(m.openings[next] & (1 << ((d + 2) % 4)));
                    edges++;
                    // Every open corridor admits the actual ball radius.
                    for (int step = 0; step <= 10; ++step) {
                        float t = step / 10.f;
                        float x = m.center_x(c) * (1-t) + m.center_x(next) * t;
                        float y = m.center_y(c) * (1-t) + m.center_y(next) * t;
                        for (int w = 0; w < m.wall_count; ++w)
                            CHECK(!circle_hits_wall(x, y, 4, m.walls[w]));
                    }
                    if (!visited[next]) { visited[next] = true; queue[tail++] = next; }
                }
            }
            CHECK(tail == cells && edges == 2 * (cells - 1));
            for (int w = 0; w < m.wall_count; ++w) {
                const auto &wall = m.walls[w];
                CHECK(wall.x >= 8 && wall.y >= 61 && wall.w > 0 && wall.h > 0);
                CHECK(wall.x + wall.w <= 232.01f && wall.y + wall.h <= 203.01f);
            }
        }
        std::printf("geometry %d: %zu distinct / 2000 seeds\n", geometry, distinct.size());
        CHECK(distinct.size() > 1900);
    }
    for (uint32_t seed : {0xffffffffu, 0xc0b0123u, 0x80000000u}) {
        auto map = make_maze(2, seed);
        CHECK(map.wall_count > 0 && map.wall_count < 96);
    }
    // Approach the outer wall without touching it: radius 5.5 is only a rearm margin.
    MazeGame approach; approach.configure({0, 0, 0}); approach.start(Difficulty::Easy, 22);
    for (int tick = 0; tick < 2000; ++tick) {
        Actions a; a.tilt_x = std::clamp((15.2f - approach.ball_x()) * .03f, -.3f, .3f);
        approach.update(a, .01f);
    }
    CHECK(approach.ball_x() > 14.f && approach.ball_x() < 15.5f && approach.contacts() == 0);
    Actions impact; impact.tilt_x = -1;
    for (int tick=0;tick<200;++tick) approach.update(impact,.01f);
    CHECK(approach.contacts()==1);
    MemoryStore store;
    store.valid = true;
    store.data.best[1][1] = 885;
    store.data.maze_best[1][0][2] = 777;
    auto before = encode_records(store.data);
    App app(store);
    app.launch(1);
    auto seed = app.maze_seed();
    InputFrame tap; tap.tap = true; tap.x = 216; tap.y = 75;
    app.update(tap, .02f); // hard geometry
    tap.y = 122; app.update(tap, .02f); // 20 contacts
    tap.x = 120; tap.y = 185; app.update(tap, .02f);
    CHECK(app.screen() == Screen::Playing);
    auto hard = static_cast<MazeGame &>(app.game()).map().openings;
    app.restart();
    CHECK(app.maze_seed() == seed && app.maze_options().geometry == 2 && app.maze_options().contact_index == 5);
    CHECK(static_cast<MazeGame &>(app.game()).map().openings == hard);
    tap.y = 153; app.update(tap, .02f); // explicit new map
    CHECK(app.maze_seed() != seed && app.maze_options().geometry == 2 && app.maze_options().contact_index == 5);
    CHECK(static_cast<MazeGame &>(app.game()).map().openings != hard);
    seed = app.maze_seed();
    tap.y = 170; app.update(tap, .02f); // gap between buttons
    CHECK(app.screen() == Screen::MazeSetup && app.maze_seed() == seed);
    tap.y = 185; app.update(tap, .02f); app.pause();
    app.update(tap, .02f); // pause new-map button
    CHECK(app.screen() == Screen::MazeSetup && app.maze_seed() != seed);
    CHECK(app.current_record() == 0);
    app.menu();
    CHECK(store.writes == 0 && encode_records(store.data) == before);
    // Even a completed generated-map result cannot overwrite the legacy journal.
    struct GeneratedResult : StubGame { bool setup() const override { return true; } } generated;
    App result_app(store, {{&generated}, 1});
    result_app.launch(0); tap.y = 185; result_app.update(tap, .02f);
    generated.value = {9999, true, true, 0}; result_app.update({}, .02f);
    CHECK(result_app.screen() == Screen::Result && store.writes == 0);
    seed = result_app.maze_seed(); result_app.update(tap, .02f);
    CHECK(result_app.screen() == Screen::MazeSetup && result_app.maze_seed() != seed);
    CHECK(encode_records(store.data) == before);
    CHECK(ui::layout("Новый лабиринт", 194, 23).fits);
}
void review_input_test() {
    MemoryStore store; store.valid = true; store.data.sensitivity = .8f;
    store.data.calibrated = true; store.data.calibration_x = .12f; store.data.calibration_y = -.2f;
    for (float expected : {1.f, 1.5f, .8f, 1.f}) {
        App app(store); InputFrame tap; tap.tap = true; tap.x = 100; tap.y = 220;
        app.update(tap, .02f); tap.y = 110; app.update(tap, .02f);
        CHECK(std::abs(app.records().sensitivity - expected) < .001f);
        auto page = encode_records(store.data); Records decoded;
        CHECK(decode_records(page.data(), page.size(), decoded));
        CHECK(decoded.sensitivity == expected && decoded.calibrated &&
              decoded.calibration_x == .12f && decoded.calibration_y == -.2f);
    }
    StubGame g[6]; GameRegistry reg{{&g[0], &g[1], &g[2], &g[3], &g[4], &g[5]}, 6};
    for (int page = 0; page < 2; ++page) {
        for (int y = 0; y < 210; ++y) for (int x : {0,9,10,100,229,230,239}) {
            App app(store, reg); InputFrame tap; tap.tap = true;
            if (page) { tap.x = 200; tap.y = 220; app.update(tap,.02f); }
            tap.x = x; tap.y = y; app.update(tap,.02f);
            int expected = -1;
            for (int row=0; row<4 && row+page*4<6; ++row)
                if (ui::menu_item(row).contains(x,y)) expected = row+page*4;
            CHECK((app.screen() == Screen::Rules) == (expected >= 0));
            if (expected >= 0) CHECK(app.selected() == expected);
        }
    }
    ContactCounter counter;
    counter.update(false, 1, false); CHECK(counter.count() == 0); // near, no impact
    counter.update(true, .02f, false); CHECK(counter.count() == 1);
    for (int n=0;n<100;++n) counter.update(false,.02f,false); // resting near wall
    counter.update(true,.02f,false); CHECK(counter.count() == 1);
    counter.update(false,.1f,true); counter.update(true,.02f,false); CHECK(counter.count()==1);
    counter.update(false,.15f,true); counter.update(true,.02f,false); CHECK(counter.count()==2);
    for (int command=0;command<6;++command) {
        CommandsGame game;
        for (uint32_t seed=1;;++seed) {game.start(Difficulty::Normal,seed);if(game.command()==command)break;}
        auto x=game.object_x(); auto angle=game.angle();
        Actions motion; motion.tilt_x=.2f; motion.tilt_y=.2f;
        game.update(motion,.02f);
        CHECK(game.object_x()>x && game.object_y()>132 && game.result().score==0 && game.feedback()==0);
        CHECK(game.angle()==angle);
        motion.turn_rate=80; game.update(motion,.02f); CHECK(game.angle()!=angle);
        angle=game.angle(); game.update({},.02f); CHECK(game.angle()==angle);
        motion={}; motion.tilt_x=1; motion.tilt_y=1;
        for(int n=0;n<25;++n)game.update(motion,.1f);
        CHECK(game.object_x()+27<=230 && game.object_y()+27<168 && game.object_y()-27>88);
    }
    CommandsGame game; for(uint32_t seed=1;;++seed){game.start(Difficulty::Normal,seed);if(game.command()<4)break;}
    Actions answer;answer.direction=game.command();answer.tap=true;answer.x=0;answer.y=0;
    game.update(answer,.02f);CHECK(game.result().score==1); // stray tap no longer masks tilt
    GestureRecognizer gestures; InputFrame imu;imu.imu=true;imu.gz=90;
    CHECK(gestures.update(imu,.02f).turn_rate==90);imu.gz=1;
    CHECK(gestures.update(imu,.02f).turn_rate==0);
}


void player_feedback_test() {
    int first_counts[4]{};
    for (uint32_t seed = 1; seed <= 256; ++seed) {
        SequenceGame game; game.start(Difficulty::Hard, seed);
        std::vector<int> first_four;
        for (int round=1;round<=4;++round) {
            auto cues = observe_round(game);
            CHECK(cues.size()==size_t(round));
            answer_round(game,cues);first_four=cues;
        }
        CHECK(std::set<int>(first_four.begin(),first_four.end()).size()==4);
        first_counts[first_four[0]]++;
        SequenceGame repeated; repeated.start(Difficulty::Hard,seed);
        CHECK(observe_round(repeated)[0]==first_four[0]);
    }
    for(int count:first_counts)CHECK(count>35 && count<95);
    SequenceGame sequence; CommandsGame commands;
    CHECK(ui::layout(sequence.metadata().rule,216,62).fits);
    CHECK(ui::layout(commands.metadata().rule,216,62).fits);
    CHECK(std::string(commands.metadata().rule).find("Следи за командами")!=std::string::npos);
    auto start_command=[](CommandsGame &game,int command) {
        for(uint32_t seed=1;;++seed){game.start(Difficulty::Normal,seed);if(game.command()==command)break;}
    };
    for(int command=0;command<5;++command) {
        CommandsGame game;start_command(game,command);
        GestureRecognizer gestures;InputFrame motion;motion.imu=true;
        motion.gz=190; // turn accepted for turn task, incidental to a clear tilt
        if(command==4)motion.ax=.45f;
        else if(command==0)motion.ay=-.55f;
        else if(command==1)motion.ax=.55f;
        else if(command==2)motion.ay=.55f;
        else motion.ax=-.55f;
        for(int tick=0;tick<30 && game.feedback()==0;++tick)
            game.update(gestures.update(motion,.02f),.02f);
        CHECK(game.result().score==1 && game.feedback()>0);
    }
    for (int command : {4,5}) {
        CommandsGame touch; start_command(touch,command);
        auto angle=touch.angle();Actions tap;tap.tap=true;tap.x=int(touch.object_x());tap.y=int(touch.object_y());
        touch.update(tap,.02f);CHECK(touch.result().score==1);
        CHECK((touch.angle()!=angle)==(command==4));
    }
    CommandsGame missed;start_command(missed,4);
    Actions miss;miss.tap=true;miss.x=0;miss.y=0;missed.update(miss,.02f);
    CHECK(missed.result().score==0 && missed.feedback()==0);
    CommandsGame wrong;start_command(wrong,4);
    Actions tilt;tilt.direction=1;tilt.tilt_x=.85f;wrong.update(tilt,.02f);
    for(int tick=0;tick<50;++tick)wrong.update({},.02f);
    CHECK(wrong.result().finished && wrong.result().score==0);
    start_command(wrong,0);Actions turn;turn.twist=true;turn.turn_rate=300;wrong.update(turn,.02f);
    for(int tick=0;tick<50;++tick)wrong.update({},.02f);
    CHECK(wrong.result().finished && wrong.result().score==0);
    GestureRecognizer g;InputFrame spike;spike.imu=true;spike.gz=300;
    CHECK(!g.update(spike,.02f).twist);spike.gz=0;CHECK(!g.update(spike,.02f).twist);
    spike.gz=190;int turns=0;for(int tick=0;tick<10;++tick)turns+=g.update(spike,.02f).twist;
    CHECK(turns==1);
    MemoryStore store; App app(store); app.launch(3);
    for(int tick=0;tick<60;++tick)app.update({},.02f);
    InputFrame shake;shake.imu=true;shake.az=3;
    app.update(shake,.02f);
    CHECK(app.screen()==Screen::Playing);
    CHECK(static_cast<MagicBall &>(app.game()).requests()==1);
    app.update(shake,.02f);CHECK(static_cast<MagicBall &>(app.game()).requests()==1);
    app.launch(3);InputFrame tap;tap.tap=true;tap.x=100;tap.y=185;app.update(tap,.02f);
    CHECK(app.screen()==Screen::Playing);
    app.launch(0);for(int tick=0;tick<60;++tick)app.update({},.02f);
    app.update(shake,.02f);CHECK(app.screen()==Screen::Rules);
}

int main() {
    player_feedback_test();
    review_generation_test();
    review_input_test();
    idle_test();
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
