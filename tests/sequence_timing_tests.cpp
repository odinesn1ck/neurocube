#include "neurocube/framebuffer.hpp"
#include "neurocube/games.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
using namespace nc;
static int checks = 0;
#define CHECK(condition)                                                                           \
    do {                                                                                           \
        ++checks;                                                                                  \
        if (!(condition)) {                                                                        \
            std::fprintf(stderr, "timing FAIL %d: %s\n", __LINE__, #condition);                    \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (0)
static bool near(float a, float b) {
    return std::abs(a - b) < .00001f;
}
static void exercise(Difficulty difficulty, uint32_t seed) {
    SequenceGame game, twin;
    game.start(difficulty, seed);
    twin.start(difficulty, seed);
    auto advance = [&](const Actions &a, float dt) {
        game.update(a, dt);
        twin.update(a, dt);
        CHECK(game.phase() == twin.phase() && game.cue() == twin.cue());
        CHECK(near(game.signal_seconds(), twin.signal_seconds()));
    };
    int prefix[32]{};
    static Framebuffer lit, dark;
    for (int round = 1; round <= 32; ++round) {
        CHECK(game.phase() == SequencePhase::Showing);
        const float duration = sequence_signal_seconds(difficulty, round);
        CHECK(near(game.signal_seconds(), duration));
        int pattern[32]{}, count = 0, previous = -1;
        int on_ticks[32]{}, off_ticks[32]{};
        for (int guard = 0; game.playback(); ++guard) {
            CHECK(guard < 15000);
            advance({}, .005f);
            CHECK(near(game.signal_seconds(), duration));
            CHECK(game.take_event() == GameEvent::None && twin.take_event() == GameEvent::None);
            int cue = game.cue();
            if (cue >= 0) {
                if (previous < 0) {
                    CHECK(count < round);
                    pattern[count++] = cue;
                }
                ++on_ticks[count - 1];
                if (count == 1 && on_ticks[0] == 1)
                    game.render(lit);
            } else if (count && game.phase() == SequencePhase::Showing) {
                ++off_ticks[count - 1];
                if (count == 1 && off_ticks[0] == 1)
                    game.render(dark);
            }
            previous = cue;
        }
        CHECK(game.phase() == SequencePhase::Recall && count == round);
        CHECK(lit.pixels != dark.pixels); // Active figure remains visibly distinct at round 32.
        for (int n = 0; n < round; ++n) {
            CHECK(n == round - 1 || pattern[n] == prefix[n]);
            prefix[n] = pattern[n];
            CHECK(std::abs(on_ticks[n] * .005f - duration * .65f) < .02f);
            CHECK(std::abs(off_ticks[n] * .005f - duration * .35f) < .02f);
        }
        game.pause();
        game.resume();
        CHECK(near(game.signal_seconds(), duration));
        advance({}, 3600.f); // A full hour of waiting must not fail Recall.
        CHECK(game.phase() == SequencePhase::Recall && !game.result().finished);
        SequenceGame wrong = game;
        Actions bad;
        bad.direction = (pattern[0] + 1) % 4;
        wrong.update(bad, .005f);
        CHECK(wrong.phase() == SequencePhase::Failure && !wrong.result().finished);
        CHECK(near(wrong.signal_seconds(), duration));
        wrong.update({}, .76f);
        CHECK(wrong.result().finished && !wrong.result().won);
        for (int n = 0; n < round; ++n) {
            Actions answer;
            answer.direction = pattern[n];
            advance(answer, .005f);
            CHECK(game.phase() == SequencePhase::Correct);
            CHECK(near(game.signal_seconds(), duration));
            const auto event = n + 1 == round ? GameEvent::Success : GameEvent::None;
            CHECK(game.take_event() == event && twin.take_event() == event);
            int ticks = 0;
            while (game.phase() == SequencePhase::Correct) {
                advance({}, .005f);
                ++ticks;
            }
            CHECK(std::abs(ticks * .005f - .36f) < .01f);
            if (n + 1 < round)
                CHECK(game.phase() == SequencePhase::Recall);
        }
    }
    CHECK(game.phase() == SequencePhase::Complete && game.result().won &&
          game.result().score == 32);
    game.start(difficulty, seed);
    CHECK(game.phase() == SequencePhase::Showing &&
          near(game.signal_seconds(), sequence_signal_seconds(difficulty, 1)));
    game.update({}, .71f);
    CHECK(game.cue() == prefix[0]);
}
int main() {
    const float initial[] = {1.40f, 1.22f, 1.04f};
    for (int d = 0; d < 3; ++d) {
        float previous = initial[d];
        for (int round = 1; round <= 32; ++round) {
            float duration = sequence_signal_seconds(Difficulty(d), round);
            CHECK(duration >= .5f && duration <= previous);
            CHECK(previous - duration <= .02401f);
            if (round == 1)
                CHECK(near(duration, initial[d]));
            previous = duration;
        }
        CHECK(near(sequence_signal_seconds(Difficulty(d), -100), initial[d]));
        CHECK(near(sequence_signal_seconds(Difficulty(d), 100), previous));
        for (uint32_t seed : {1u, 42u})
            exercise(Difficulty(d), seed);
    }
    std::puts("round,easy,normal,hard");
    for (int round : {1, 5, 10, 15, 20, 25, 32})
        std::printf("%d,%.3f,%.3f,%.3f\n", round, sequence_signal_seconds(Difficulty::Easy, round),
                    sequence_signal_seconds(Difficulty::Normal, round),
                    sequence_signal_seconds(Difficulty::Hard, round));
    std::printf("PASS %d sequence timing checks\n", checks);
}
