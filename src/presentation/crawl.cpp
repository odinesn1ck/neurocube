#include "neurocube/crawl.hpp"
#include "font_data.hpp"
#include "neurocube/ui.hpp"
#include "qr_data.hpp"
#include <algorithm>
#include <cmath>
namespace nc {
static uint32_t codepoint(const char *&p) {
    uint8_t c = uint8_t(*p++);
    if (c < 128)
        return c;
    int n = c < 0xE0 ? 1 : c < 0xF0 ? 2 : 3;
    uint32_t value = c & ((1 << (6 - n)) - 1);
    for (int i = 0; i < n; i++) {
        uint8_t b = uint8_t(*p);
        if (!b || (b & 0xC0) != 0x80)
            return '?';
        value = (value << 6) | (b & 63);
        p++;
    }
    return value;
}
void render_crawl(Renderer &r, float progress, int episode) {
    r.clear(0);
    Random stars;
    stars.state = 0x1234ABCD;
    for (int i = 0; i < 35; i++) {
        int x = int(stars.bounded(240)), y = int(stars.bounded(240));
        r.rect(x, y, 1, 1, i % 4 ? 0x4208 : 0x8410);
    }
    const auto &story = crawl_story(episode);
    for (size_t line = 0; line < story.count; line++) {
        std::array<size_t, 40> glyphs{};
        int count = 0;
        const char *p = story.lines[line];
        while (*p && count < int(glyphs.size())) {
            uint32_t cp = codepoint(p);
            size_t g = 0;
            while (g < font_count && font_codes[g] != cp)
                g++;
            glyphs[count++] = g < font_count ? g : 0;
        }
        if (!count)
            continue;
        for (int row = 0; row < 16; row++) {
            float depth = crawl_depth(progress, int(line), row), top = crawl_y(depth),
                  bottom = crawl_y(crawl_depth(progress, int(line), row + 1));
            if (bottom < 0 || top >= 240 || depth <= .01f)
                continue;
            int y = std::max(0, int(std::floor(top))), end = std::min(240, int(std::ceil(bottom)));
            if (end <= y)
                continue;
            float scale = 1.2f / depth, left = 120 - (count * 8 + 2) * scale * .5f;
            for (int g = 0; g < count; g++)
                for (int col = 0; col < 10; col++)
                    if (font_rows[glyphs[g]][row] & (1u << (9 - col))) {
                        int x = std::max(0, int(std::floor(left + (g * 8 + col) * scale))),
                            right = std::min(240, int(std::ceil(left + (g * 8 + col + 1) * scale)));
                        if (right > x)
                            r.rect(x, y, right - x, end - y, 0xFFE0);
                    }
        }
    }
}
void render_project_qr(Renderer &r) {
    r.clear(0);
    ui::text_box(r, {8, 1, 224, 22}, "NeuroCube / GitHub", Ink);
    ui::text_box(r, {8, 215, 224, 24}, "Коснись: в меню", Muted);
    constexpr int scale = 5, side = qr_size * scale, left = (240 - side) / 2, top = 27;
    r.rect(left, top, side, side, Ink);
    for (int y = 0; y < qr_size; y++)
        for (int x = 0; x < qr_size; x++)
            if (qr_rows[y] & (uint64_t(1) << x))
                r.rect(left + x * scale, top + y * scale, scale, scale, 0);
}
} // namespace nc
