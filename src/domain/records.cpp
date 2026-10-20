#include "neurocube/domain.hpp"
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
RecordPage encode_records(const Records &r) {
    RecordPage p{};
    p.fill(0xFF);
    put(p.data(), 0x4E435231);
    put(p.data() + 4, 2);
    put(p.data() + 8, r.sequence);
    int o = 12;
    for (auto &g : r.best)
        for (auto s : g) {
            put(p.data() + o, s);
            o += 4;
        }
    put(p.data() + 252, crc(p.data(), 252));
    return p;
}
bool decode_records(const uint8_t *p, size_t n, Records &r) {
    if (n != 256 || get(p) != 0x4E435231 || (get(p + 4) != 1 && get(p + 4) != 2) ||
        get(p + 252) != crc(p, 252))
        return false;
    Records v;
    v.sequence = get(p + 8);
    int o = 12;
    const size_t slots = get(p + 4) == 1 ? 3 : GameCapacity;
    for (size_t idx = 0; idx < slots; idx++)
        for (auto &s : v.best[idx]) {
            s = get(p + o);
            o += 4;
        }
    r = v;
    return true;
}
bool newer_sequence(uint32_t a, uint32_t b) {
    return a != b && uint32_t(a - b) < 0x80000000u;
}
int recover_records(const uint8_t *first, const uint8_t *second, Records &r) {
    Records a, b;
    bool va = decode_records(first, 256, a), vb = decode_records(second, 256, b);
    if (!va && !vb)
        return -1;
    int active = va && (!vb || newer_sequence(a.sequence, b.sequence)) ? 0 : 1;
    r = active == 0 ? a : b;
    return active;
}
} // namespace nc
