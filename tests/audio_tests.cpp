
#include "legacy_audio_v3.hpp"
#include "legacy_records.hpp"
#include "neurocube/application.hpp"
#include "neurocube/audio.hpp"
#include "neurocube/games.hpp"
#include "neurocube/ui.hpp"
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
            std::fprintf(stderr, "audio FAIL %d: %s\n", __LINE__, #x);                             \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (0)
struct Memory : RecordStore {
    Records r{};
    int writes = 0;
    bool valid = false, reject_writes = false;
    bool load(Records &v) override {
        if (valid)
            v = r;
        return valid;
    }
    bool save(const Records &v) override {
        if (reject_writes)
            return false;
        r = v;
        ++writes;
        valid = true;
        return true;
    }
};
static void wait(App &app, SimAudio &audio, int ticks) {
    for (int n = 0; n < ticks; ++n) {
        audio.tick(.02f);
        app.update({}, .02f);
    }
}
int main() {
    Records old;
    old.sound_enabled = false;
    CHECK(decode_records(legacy_v1.data(), legacy_v1.size(), old) && old.sound_enabled);
    old.sound_enabled = false;
    CHECK(decode_records(legacy_v2.data(), legacy_v2.size(), old) && old.sound_enabled);
    Records legacy;
    CHECK(decode_records(legacy_audio_v3.data(), 512, legacy));
    CHECK(legacy.sound_enabled && legacy.sequence == 42 && legacy.calibrated &&
          legacy.best[15][2] == 1047 && legacy.maze_best[2][2][5] == 2053);
    for (bool enabled : {false, true}) {
        legacy.sound_enabled = enabled;
        auto p = encode_records(legacy);
        Records out;
        CHECK(decode_records(p.data(), p.size(), out));
        CHECK(out.sound_enabled == enabled);
        CHECK(out.best == legacy.best && out.maze_best == legacy.maze_best && out.sequence == 42);
        CHECK(out.calibration_x == legacy.calibration_x &&
              out.calibration_y == legacy.calibration_y && out.sensitivity == legacy.sensitivity);
        CHECK(std::equal(p.begin(), p.begin() + 436,
                         legacy_audio_v3.begin())); // All prior fields exact.
        p[436] ^= 1;
        CHECK(!decode_records(p.data(), p.size(), out));
    }
    for (auto effect : {SoundEffect::Error, SoundEffect::Answer}) {
        static uint32_t pcm[MaxAudioSamples];
        auto count = synthesize_sound(effect, pcm, MaxAudioSamples);
        CHECK(count == sound_samples(effect));
        CHECK(pcm[0] == 0 && pcm[count - 1] == 0);
        int peak = 0;
        long long energy = 0;
        for (size_t n = 0; n < count; ++n) {
            int sample = int16_t(pcm[n]);
            CHECK(uint16_t(pcm[n]) == uint16_t(pcm[n] >> 16));
            peak = std::max(peak, std::abs(sample));
            energy += sample * sample;
        }
        CHECK(peak > 0 && peak <= AudioPeak && energy > 0);
        CHECK(synthesize_sound(effect, pcm, count - 1) == 0);
    }
    SimAudio voice;
    CHECK(voice.play(SoundEffect::Answer));
    CHECK(!voice.play(SoundEffect::Error));
    voice.tick(.47f);
    CHECK(voice.busy());
    voice.tick(.02f);
    CHECK(!voice.busy());
    CHECK(voice.play(SoundEffect::Error));
    voice.stop();
    CHECK(!voice.busy());
    SequenceGame sequence;
    for (uint32_t seed = 1; seed < 30; ++seed) {
        sequence.start(Difficulty::Normal, seed);
        int cue = -1;
        while (sequence.playback()) {
            sequence.update({}, .02f);
            if (sequence.cue() >= 0)
                cue = sequence.cue();
        }
        CHECK(cue >= 0);
        Actions wrong;
        wrong.direction = (cue + 1) % 4;
        sequence.update(wrong, .02f);
        CHECK(sequence.take_event() == GameEvent::ErrorOccurred);
        CHECK(sequence.take_event() == GameEvent::None);
        for (int n = 0; n < 100; ++n) {
            sequence.update({}, .02f);
            CHECK(sequence.take_event() == GameEvent::None);
        }
    }
    CommandsGame commands;
    commands.start(Difficulty::Normal, 42);
    commands.update({}, 4); // dt clamps; wait for timeout.
    for (int n = 0; n < 50; ++n)
        commands.update({}, .1f);
    CHECK(commands.take_event() == GameEvent::ErrorOccurred);
    for (int n = 0; n < 100; ++n) {
        commands.update({}, .02f);
        CHECK(commands.take_event() == GameEvent::None);
    }
    for (int expected = 0; expected < 6; ++expected) {
        for (uint32_t seed = 1;; ++seed) {
            commands.start(Difficulty::Normal, seed);
            if (commands.command() == expected)
                break;
        }
        Actions wrong;
        wrong.direction = (expected + 1) % 4;
        wrong.tilt_x = 1;
        wrong.tilt_y = 1;
        commands.update(wrong, .02f);
        CHECK(commands.take_event() == GameEvent::ErrorOccurred);
    }
    MazeGame maze;
    GameOptions options;
    options.contact_index = 0;
    maze.configure(options);
    maze.start(Difficulty::Normal, 1);
    Actions into_wall;
    into_wall.tilt_x = -1;
    for (int n = 0; n < 200; ++n) {
        maze.update(into_wall, .02f);
        CHECK(maze.take_event() == GameEvent::None);
    }
    CHECK(maze.contacts() == 1); // Contact alone is not a failure.
    options.contact_index = 1;
    maze.configure(options);
    maze.start(Difficulty::Normal, 1);
    int errors = 0;
    for (int cycle = 0; cycle < 10 && !maze.result().finished; ++cycle) {
        Actions push;
        push.tilt_x = cycle % 2 ? 1.f : -1.f;
        for (int n = 0; n < 150; ++n) {
            maze.update(push, .02f);
            errors += maze.take_event() == GameEvent::ErrorOccurred;
        }
    }
    CHECK(maze.result().finished && !maze.result().won && errors == 1);
    CHECK(maze.take_event() == GameEvent::None);
    MagicBall ball;
    ball.start(Difficulty::Normal, 1);
    Actions ask;
    ask.tap = true;
    ball.update(ask, .02f);
    CHECK(ball.take_event() == GameEvent::AnswerAccepted);
    CHECK(ball.take_event() == GameEvent::None);
    for (int n = 0; n < 20; ++n) {
        ball.update(ask, .02f);
        CHECK(ball.take_event() == GameEvent::None);
    }
    ball.update({}, .5f);
    ball.update(ask, .02f);
    CHECK(ball.take_event() == GameEvent::AnswerAccepted);
    Memory store;
    CHECK(Records{}.sound_enabled);
    store.valid = true;
    store.r.sound_enabled = false; // An explicitly saved mute stays off.
    SimAudio audio;
    App app(store, default_registry(), false, config::Recipient, &audio);
    CHECK(!app.records().sound_enabled);
    app.launch(3, false);
    InputFrame tap;
    tap.tap = true;
    tap.x = 100;
    tap.y = 180;
    app.update(tap, .02f);
    CHECK(audio.played == 0); // Muted requests still drain the event.
    wait(app, audio, 60);
    CHECK(audio.played == 0);
    app.launch(3);
    tap.x = 212;
    tap.y = 48;
    tap.imu = true;
    tap.az = 3;
    app.update(tap, .02f);
    CHECK(app.records().sound_enabled && app.screen() == Screen::Rules && store.writes == 1 &&
          audio.played == 0);
    for (int n = 0; n < 10; ++n)
        app.update(tap, .02f);
    CHECK(store.writes == 1 && app.screen() == Screen::Rules);
    wait(app, audio, 60);
    InputFrame shake;
    shake.imu = true;
    shake.az = 3;
    app.update(shake, .02f);
    CHECK(app.screen() == Screen::Playing && audio.played == 1 &&
          audio.last == SoundEffect::Answer);
    app.update(shake, .02f);
    CHECK(audio.played == 1);
    wait(app, audio, 60);
    CHECK(audio.played == 1);
    app.launch(0);
    CHECK(app.records().sound_enabled && !audio.busy());
    app.launch(1);
    CHECK(app.screen() == Screen::MazeSetup && app.records().sound_enabled);
    tap.imu = false;
    tap.x = 216;
    tap.y = 20;
    app.update(tap, .02f);
    CHECK(!app.records().sound_enabled && store.writes == 2 && app.screen() == Screen::MazeSetup);
    app.update({}, .02f);
    App restored(store);
    CHECK(!restored.records().sound_enabled);
    app.launch(3);
    tap.x = 212;
    tap.y = 48;
    app.update(tap, .02f);
    wait(app, audio, 2);
    App enabled(store);
    CHECK(enabled.records().sound_enabled);
    app.launch(3, false);
    tap.x = 100;
    tap.y = 180;
    app.update(tap, .02f);
    CHECK(audio.busy());
    app.pause();
    CHECK(!audio.busy());
    app.resume();
    app.update({}, .02f);
    InputFrame pwr;
    pwr.power_button = true;
    // Stop a game voice first; shutdown cue waits for presentation and PA release.
    for (int n = 0; n < 89; ++n)
        app.update(pwr, .02f);
    audio.tick(1);
    CHECK(audio.play(SoundEffect::Answer));
    for (int n = 0; n < 4; ++n)
        app.update(pwr, .02f);
    CHECK(app.screen() == Screen::ShuttingDown && !audio.busy());
    const auto before_shutdown = audio.played;
    app.frame_presented();
    wait(app, audio, 8);
    CHECK(audio.played == before_shutdown + 1 && audio.last == SoundEffect::Error);
    wait(app, audio, 60);
    CHECK(audio.played == before_shutdown + 1 && !audio.busy() && app.power_cut_requested());
    Memory welcome_store;
    SimAudio welcome_audio;
    App welcome(welcome_store, default_registry(), true, config::Recipient, &welcome_audio);
    CHECK(welcome.records().sound_enabled && welcome_audio.played == 0);
    welcome.frame_presented();
    CHECK(welcome_audio.played == 0); // Initial black fade frame is not the greeting.
    welcome.update({}, .02f);
    welcome.frame_presented();
    CHECK(welcome_audio.played == 1 && welcome_audio.last == SoundEffect::Answer);
    for (int n = 0; n < 100; ++n) {
        welcome_audio.tick(.02f);
        welcome.update({}, .02f);
        welcome.frame_presented();
    }
    CHECK(welcome_audio.played == 1);
    Memory muted_store;
    muted_store.valid = true;
    muted_store.r.sound_enabled = false;
    SimAudio muted_audio;
    App muted(muted_store, default_registry(), true, config::Recipient, &muted_audio);
    muted.update({}, .04f);
    muted.frame_presented();
    CHECK(muted_audio.played == 1 && muted_audio.last == SoundEffect::Answer);
    muted.update({}, .02f);
    for (int n = 0; n < 93; ++n) {
        muted_audio.tick(.02f);
        muted.update(pwr, .02f);
    }
    muted.frame_presented();
    wait(muted, muted_audio, 65);
    CHECK(muted.screen() == Screen::ShuttingDown && muted.power_cut_requested() &&
          muted_audio.played == 2 && muted_audio.last == SoundEffect::Error);

    CHECK(ui::SoundToggle.w >= 40 && ui::SoundToggle.h >= 40 &&
          ui::SoundToggle.y + ui::SoundToggle.h <= 72);
    Memory failed_save;
    failed_save.valid = true;
    failed_save.reject_writes = true;
    SimAudio failed_audio;
    App failed_app(failed_save, default_registry(), false, config::Recipient, &failed_audio);
    failed_app.launch(3);
    InputFrame mute_tap;
    mute_tap.tap = true;
    mute_tap.x = 212;
    mute_tap.y = 48;
    failed_app.update(mute_tap, .02f); // Failed setting write leaves dirty data.
    failed_app.update({}, .02f);
    for (int n = 0; n < 93; ++n)
        failed_app.update(pwr, .02f);
    failed_app.frame_presented();
    wait(failed_app, failed_audio, 65);
    CHECK(failed_app.screen() == Screen::PowerNotice && !failed_app.power_cut_requested() &&
          failed_audio.played == 0);
    Memory failures;
    failures.valid = true;
    failures.r.sound_enabled = true;
    SimAudio error_audio;
    App error_app(failures, default_registry(), false, config::Recipient, &error_audio);
    error_app.launch(0, false);
    int cue = -1;
    auto &seq = static_cast<SequenceGame &>(error_app.game());
    while (seq.playback()) {
        error_app.update({}, .02f);
        if (seq.cue() >= 0)
            cue = seq.cue();
    }
    InputFrame wrong_tap;
    wrong_tap.tap = true;
    wrong_tap.x = 120;
    wrong_tap.y = cue == 0 ? 185 : 90;
    error_app.update(wrong_tap, .02f);
    CHECK(error_audio.played == 1 && error_audio.last == SoundEffect::Error);
    wait(error_app, error_audio, 100);
    CHECK(error_audio.played == 1);
    error_app.menu();
    error_app.next();
    CHECK(error_audio.played == 1);
    std::printf("PASS %d audio checks\n", checks);
}
