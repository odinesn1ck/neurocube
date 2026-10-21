#include "neurocube/domain.hpp"
#include <cmath>
#include <cstring>
namespace nc {
static uint32_t crc(const uint8_t *p, size_t n) {
    uint32_t c = ~0u;
    for (size_t i = 0; i < n; i++) {
        c ^= p[i];
        for (int b = 0; b < 8; b++)
            c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1)));
    }
    return ~c;
}
static void put(uint8_t *p, uint32_t x) {
    for (int i = 0; i < 4; i++)
        p[i] = uint8_t(x >> (8 * i));
}
static uint32_t get(const uint8_t *p) {
    uint32_t x = 0;
    for (int i = 0; i < 4; i++)
        x |= uint32_t(p[i]) << (8 * i);
    return x;
}
static uint32_t bits(float f) {
    uint32_t b;
    std::memcpy(&b, &f, 4);
    return b;
}
static float number(uint32_t b) {
    float f;
    std::memcpy(&f, &b, 4);
    return f;
}
RecordPage encode_records(const Records &r) {
    RecordPage p;
    p.fill(0xFF);
    put(p.data(), 0x4E435231);
    put(p.data() + 4, 3);
    put(p.data() + 8, r.sequence);
    int o = 12;
    for (auto &g : r.best)
        for (auto s : g) {
            put(p.data() + o, s);
            o += 4;
        }
    for (auto &geometry : r.maze_best)
        for (auto &variant : geometry)
            for (auto s : variant) {
                put(p.data() + o, s);
                o += 4;
            }
    put(p.data() + 420, r.calibrated ? 1 : 0);
    put(p.data() + 424, bits(r.calibration_x));
    put(p.data() + 428, bits(r.calibration_y));
    put(p.data() + 432, bits(r.sensitivity));
    put(p.data() + 436, 0x534E4400u | (r.sound_enabled ? 1u : 0u));
    put(p.data() + 508, crc(p.data(), 508));
    return p;
}
bool decode_records(const uint8_t *p, size_t n, Records &r) {
    if (n < 256 || get(p) != 0x4E435231)
        return false;
    uint32_t version = get(p + 4);
    if (version < 1 || version > 3)
        return false;
    size_t bytes = version == 3 ? 512 : 256;
    if (n < bytes || get(p + bytes - 4) != crc(p, bytes - 4))
        return false;
    Records v;
    v.sequence = get(p + 8);
    int o = 12;
    size_t slots = version == 1 ? 3 : GameCapacity;
    for (size_t i = 0; i < slots; i++)
        for (auto &s : v.best[i]) {
            s = get(p + o);
            o += 4;
        }
    if (version == 3) {
        for (auto &geometry : v.maze_best)
            for (auto &variant : geometry)
                for (auto &s : variant) {
                    s = get(p + o);
                    o += 4;
                }
        v.sound_enabled = get(p + 436) == 0x534E4401u; // Old v3 reserved bytes are FF: mute.
        v.calibrated = get(p + 420) == 1;
        v.calibration_x = number(get(p + 424));
        v.calibration_y = number(get(p + 428));
        v.sensitivity = number(get(p + 432));
        if (!std::isfinite(v.calibration_x) || !std::isfinite(v.calibration_y) ||
            std::abs(v.calibration_x) > 4 || std::abs(v.calibration_y) > 4 ||
            !std::isfinite(v.sensitivity) || v.sensitivity < .5f || v.sensitivity > 2)
            return false;
    }
    r = v;
    return true;
}
bool newer_sequence(uint32_t a, uint32_t b) {
    return a != b && uint32_t(a - b) < 0x80000000u;
}
int recover_records(const uint8_t *a, const uint8_t *b, Records &r) {
    Records x, y;
    bool vx = decode_records(a, RecordBytes, x), vy = decode_records(b, RecordBytes, y);
    if (!vx && !vy)
        return -1;
    int active = vx && (!vy || newer_sequence(x.sequence, y.sequence)) ? 0 : 1;
    r = active == 0 ? x : y;
    return active;
}
} // namespace nc
