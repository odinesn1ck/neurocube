#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
namespace nc {
constexpr int Width = 240, Height = 240, GameCapacity = 16;
struct InputFrame {
    float ax = 0, ay = 0, az = 1, gz = 0;
    bool imu = false, touch = false, tap = false;
    int x = 0, y = 0;
    bool short_button = false, long_button = false;
    bool power_button = false;
};
struct Actions {
    float tilt_x = 0, tilt_y = 0;
    int direction = -1;
    bool twist = false, shake = false, tap = false;
    int x = 0, y = 0;
};
enum class Difficulty : uint8_t { Easy, Normal, Hard };
struct Result {
    uint32_t score = 0;
    bool finished = false;
    bool won = false;
    uint16_t contacts = 0;
};
struct Metadata {
    const char *id;
    const char *title;
    const char *rule;
    bool scored = true;
    bool restart_on_shake = true;
};
struct Renderer {
    virtual ~Renderer() = default;
    virtual void clear(uint16_t) = 0;
    virtual void rect(int, int, int, int, uint16_t) = 0;
    virtual void circle(int, int, int, uint16_t) = 0;
    virtual void text(int, int, const char *, uint16_t, int scale = 1) = 0;
};
struct Random {
    uint32_t state = 0x735C1234;
    uint32_t next() {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }
    uint32_t bounded(uint32_t n) {
        return n ? next() % n : 0;
    }
};
constexpr std::array<int, 6> ContactLimits{{0, 3, 5, 8, 12, 20}}; // zero means unlimited
struct GameOptions {
    uint8_t geometry = 1, contact_index = 2, variant = 0;
};
struct Game {
    virtual ~Game() = default;
    virtual Metadata metadata() const = 0;
    virtual void start(Difficulty, uint32_t) = 0;
    virtual void update(const Actions &, float) = 0;
    virtual void render(Renderer &) const = 0;
    virtual void configure(const GameOptions &) {};
    virtual bool countdown() const {
        return false;
    }
    virtual bool setup() const {
        return false;
    }
    virtual void pause() {};
    virtual void resume() {};
    virtual void stop() {};
    virtual Result result() const = 0;
};
constexpr uint16_t Bg = 0x0843, Ink = 0xFFFF, Muted = 0x9D36, Accent = 0x3F39, Gold = 0xFEA0,
                   Red = 0xF986;
using MazeScores = std::array<std::array<std::array<uint32_t, 6>, 3>, 3>;
struct Records {
    std::array<std::array<uint32_t, 3>, GameCapacity> best{};
    uint32_t sequence = 0;
    MazeScores maze_best{};
    bool calibrated = false;
    float calibration_x = 0, calibration_y = 0, sensitivity = 1;
};
constexpr size_t RecordBytes = 512;
using RecordPage = std::array<uint8_t, RecordBytes>;
RecordPage encode_records(const Records &);
bool decode_records(const uint8_t *, size_t, Records &);
bool newer_sequence(uint32_t, uint32_t);
int recover_records(const uint8_t *, const uint8_t *, Records &);
struct RecordStore {
    virtual ~RecordStore() = default;
    virtual bool load(Records &) = 0;
    virtual bool save(const Records &) = 0;
};
} // namespace nc
