// Copyright (c) 2026 odinesn1ck; MIT.
#include "es8311.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <utility>
static unsigned checks = 0;
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        ++checks;                                                                                  \
        if (!(x)) {                                                                                \
            std::fprintf(stderr, "line %d: %s\n", __LINE__, #x);                                   \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (0)
struct Bus {
    std::array<uint8_t, 256> regs{};
    unsigned operations = 0, fail_at = 0, writes = 0, delay = 0;
    static bool write(void *ctx, uint8_t reg, uint8_t value) {
        auto &b = *static_cast<Bus *>(ctx);
        if (++b.operations == b.fail_at)
            return false;
        ++b.writes;
        b.regs[reg] = value;
        return true;
    }
    static bool read(void *ctx, uint8_t reg, uint8_t *value) {
        auto &b = *static_cast<Bus *>(ctx);
        if (++b.operations == b.fail_at)
            return false;
        *value = b.regs[reg];
        return true;
    }
    static void wait(void *ctx, uint32_t ms) {
        static_cast<Bus *>(ctx)->delay += ms;
    }
    es8311_t device() {
        return {{this, write, read, wait}};
    }
};
int main() {
    Bus good;
    auto dev = good.device();
    CHECK(es8311_init_24k(&dev) == ES8311_OK);
    const std::pair<unsigned, unsigned> expected[] = {
        {0x00, 0xc0}, {0x01, 0x3f}, {0x02, 0},    {0x03, 0x10}, {0x04, 0x10}, {0x05, 0},
        {0x06, 3},    {0x07, 0},    {0x08, 0xff}, {0x09, 0x0c}, {0x0a, 0x0c}, {0x0d, 1},
        {0x0e, 2},    {0x12, 0},    {0x13, 0x10}, {0x1c, 0x6a}, {0x37, 8}};
    // Values derived from the Apache clock coefficient and register specification,
    // including master bit and I2S 16-bit format needed by existing PIO.
    for (auto p : expected)
        CHECK(good.regs[p.first] == p.second);
    CHECK(good.delay == 20);
    for (unsigned fail = 1; fail <= good.operations; ++fail) {
        Bus bad;
        bad.fail_at = fail;
        auto d = bad.device();
        CHECK(es8311_init_24k(&d) == ES8311_IO_ERROR);
        CHECK(bad.operations == fail); // no subsequent writes after any failed bus operation
    }
    for (unsigned seed = 0; seed < 256; ++seed) {
        Bus b;
        b.regs.fill(uint8_t(seed));
        auto d = b.device();
        CHECK(es8311_init_24k(&d) == ES8311_OK);
        CHECK(b.regs[0x02] == (seed & 7));
        CHECK(b.regs[0x06] == ((seed & 0xc0) | 3));
        CHECK(b.regs[0x07] == (seed & 0xc0));
        b.regs[0x31] = uint8_t(seed);
        CHECK(es8311_voice_mute(&d, true) == ES8311_OK);
        CHECK(b.regs[0x31] == (seed | 0x60));
        CHECK(es8311_voice_mute(&d, false) == ES8311_OK);
        CHECK(b.regs[0x31] == (seed & ~0x60));
    }
    for (int volume = -10; volume <= 110; ++volume) {
        int result = -1, clamp = volume < 0 ? 0 : volume > 100 ? 100 : volume;
        CHECK(es8311_voice_volume_set(&dev, volume, &result) == ES8311_OK);
        CHECK(result == clamp);
        CHECK(good.regs[0x32] == (clamp ? clamp * 256 / 100 - 1 : 0));
    }
    CHECK(es8311_voice_volume_set(&dev, 60, nullptr) == ES8311_OK);
    CHECK(good.regs[0x32] == 0x98); // accepted codec gain; PCM amplitude is independent in core
    good.regs[0xfd] = 0x83;
    good.regs[0xfe] = 0x11;
    uint16_t id = 0;
    CHECK(es8311_read_id(&dev, &id) == ES8311_OK);
    CHECK(id == 0x8311);
    for (unsigned fail = 1; fail <= 2; ++fail) {
        Bus b;
        b.fail_at = fail;
        auto d = b.device();
        unsigned oldwrites = b.writes;
        CHECK(es8311_voice_mute(&d, false) == ES8311_IO_ERROR);
        CHECK(b.operations == fail);
        CHECK(b.writes == oldwrites);
        Bus i;
        i.fail_at = fail;
        auto di = i.device();
        id = 0xbeef;
        CHECK(es8311_read_id(&di, &id) == ES8311_IO_ERROR);
        CHECK(id == 0xbeef);
    }
    Bus b;
    b.fail_at = 1;
    auto d = b.device();
    int volume = 7;
    CHECK(es8311_voice_volume_set(&d, 60, &volume) == ES8311_IO_ERROR);
    CHECK(volume == 7);
    CHECK(es8311_init_24k(nullptr) == ES8311_INVALID_ARGUMENT);
    es8311_t empty{};
    CHECK(es8311_init_24k(&empty) == ES8311_INVALID_ARGUMENT);
    CHECK(es8311_read_id(&dev, nullptr) == ES8311_INVALID_ARGUMENT);
    CHECK(es8311_voice_mute(nullptr, true) == ES8311_INVALID_ARGUMENT);
    CHECK(es8311_voice_volume_set(nullptr, 60, nullptr) == ES8311_INVALID_ARGUMENT);
    std::printf("codec_tests: %u checks passed\n", checks);
}
