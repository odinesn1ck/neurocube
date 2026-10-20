#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
namespace nc {
constexpr int Width = 240, Height = 240, GameCapacity = 16;
struct InputFrame {
    float ax = 0, ay = 0, az = 1, gz = 0; // g, degrees/sec
    bool imu = false, touch = false, tap = false;
    int x = 0, y = 0;
    bool short_button = false, long_button = false;
};
struct Actions {
    float tilt_x = 0, tilt_y = 0;
    int direction = -1; // up right down left
    bool twist = false, shake = false, tap = false;
    int x = 0, y = 0;
};
enum class Difficulty : uint8_t { Easy, Normal, Hard };
struct Result {
    uint32_t score = 0;
    bool finished = false;
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
    virtual void clear(uint16_t color) = 0;
    virtual void rect(int x, int y, int w, int h, uint16_t color) = 0;
    virtual void circle(int x, int y, int r, uint16_t color) = 0;
    virtual void text(int x, int y, const char *utf8, uint16_t color, int scale = 1) = 0;
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
struct Game {
    virtual ~Game() = default;
    virtual Metadata metadata() const = 0;
    virtual void start(Difficulty, uint32_t seed) = 0;
    virtual void update(const Actions &, float dt) = 0;
    virtual void render(Renderer &) const = 0;
    virtual void pause() {}
    virtual void resume() {}
    virtual void stop() {}
    virtual Result result() const = 0;
};
constexpr uint16_t Bg = 0x0843, Ink = 0xFFFF, Muted = 0x9D36, Accent = 0x3F39, Gold = 0xFEA0,
                   Red = 0xF986;
struct Records {
    std::array<std::array<uint32_t, 3>, GameCapacity> best{};
    uint32_t sequence = 0;
};
constexpr size_t RecordBytes = 256;
using RecordPage = std::array<uint8_t, RecordBytes>;
RecordPage encode_records(const Records &);
bool decode_records(const uint8_t *, size_t, Records &);
bool newer_sequence(uint32_t a, uint32_t b);
int recover_records(const uint8_t *first, const uint8_t *second, Records &);
struct RecordStore {
    virtual ~RecordStore() = default;
    virtual bool load(Records &) = 0;
    virtual bool save(const Records &) = 0;
};
} // namespace nc
