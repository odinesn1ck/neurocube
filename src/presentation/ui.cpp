#include "neurocube/ui.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace nc::ui {
int text_width(const char *s, int scale) {
    int n = 0;
    for (auto *p = (const unsigned char *)s; *p; p++)
        if ((*p & 0xC0) != 0x80)
            n++;
    return n ? ((n * 8 + 2) * scale) : 0;
}
Layout layout(const char *s, int width, int height, int scale) {
    Layout out;
    int capacity = std::min(4, height / (18 * scale));
    if (capacity < 1) {
        out.fits = false;
        return out;
    }
    out.count = 1;
    int len = 0;
    while (*s) {
        while (*s == ' ')
            s++;
        if (!*s)
            break;
        const char *end = s;
        while (*end && *end != ' ' && *end != '\n')
            end++;
        int bytes = int(end - s);
        if (bytes >= 120) {
            out.fits = false;
            return out;
        }
        char word[128]{};
        std::memcpy(word, s, bytes);
        auto &row = out.lines[out.count - 1];
        int needed =
            text_width(row.data(), scale) + (len ? 8 * scale : 0) + text_width(word, scale);
        if (needed > width && len) {
            if (out.count == capacity) {
                out.fits = false;
                return out;
            }
            out.count++;
            len = 0;
        }
        if (text_width(word, scale) > width) {
            out.fits = false;
            return out;
        }
        auto &dest = out.lines[out.count - 1];
        if (len)
            dest[len++] = ' ';
        if (len + bytes >= 127) {
            out.fits = false;
            return out;
        }
        std::memcpy(dest.data() + len, s, bytes);
        len += bytes;
        dest[len] = 0;
        s = end;
        if (*s == '\n') {
            s++;
            if (out.count == capacity && *s) {
                out.fits = false;
                return out;
            }
            if (*s) {
                out.count++;
                len = 0;
            }
        }
    }
    return out;
}
bool text_box(Renderer &r, Rect box, const char *s, uint16_t color, int scale, bool centered) {
    auto l = layout(s, box.w, box.h, scale);
    int y = box.y + (box.h - l.count * 18 * scale) / 2;
    for (int i = 0; i < l.count; i++) {
        int x = centered ? box.x + (box.w - text_width(l.lines[i].data(), scale)) / 2 : box.x;
        r.text(x, y, l.lines[i].data(), color, scale);
        y += 18 * scale;
    }
    return l.fits;
}
void button(Renderer &r, Rect box, const char *s, uint16_t color) {
    r.rect(box.x, box.y, box.w, box.h, color);
    text_box(r, {box.x + 3, box.y + 2, box.w - 6, box.h - 4}, s, Bg);
}
void slider(Renderer &r, Rect box, int value, int maximum) {
    int left = box.x + 12, right = box.x + box.w - 12, y = box.y + box.h / 2;
    r.rect(left, y - 3, right - left, 6, 0x39CC);
    int x = left + (right - left) * value / std::max(1, maximum);
    r.rect(left, y - 3, x - left, 6, Accent);
    r.circle(x, y, 11, Ink);
    r.circle(x, y, 5, Accent);
}
void line(Renderer &r, int x, int y, int ex, int ey, uint16_t c) {
    int dx = std::abs(ex - x), sx = x < ex ? 1 : -1, dy = -std::abs(ey - y), sy = y < ey ? 1 : -1,
        error = dx + dy;
    while (true) {
        r.rect(x, y, 1, 1, c);
        if (x == ex && y == ey)
            break;
        int e = 2 * error;
        if (e >= dy) {
            error += dy;
            x += sx;
        }
        if (e <= dx) {
            error += dx;
            y += sy;
        }
    }
}
static uint16_t rgb(int r, int g, int b) {
    return uint16_t((std::clamp(r, 0, 255) >> 3) << 11 | (std::clamp(g, 0, 255) >> 2) << 5 |
                    (std::clamp(b, 0, 255) >> 3));
}
void orb(Renderer &r, float angle, uint16_t glow) {
    // Procedural original sphere: layered diffuse shading and moving specular highlights.
    r.circle(120, 111, 88, 0x0865);
    for (int n = 24; n >= 0; n--) {
        float f = n / 24.f;
        int radius = 8 + int(76 * f);
        r.circle(116 - int((1 - f) * 8), 106 - int((1 - f) * 12), radius,
                 rgb(13 + int((1 - f) * 30), 16 + int((1 - f) * 34), 34 + int((1 - f) * 56)));
    }
    r.circle(89 + int(std::sin(angle) * 6), 61, 14, 0x5AF4);
    r.circle(85 + int(std::sin(angle) * 6), 57, 7, 0xB63C);
    r.circle(84 + int(std::sin(angle) * 6), 55, 3, Ink);
    for (int k = 0; k < 15; k++) {
        float a = angle * .7f + k * .09f;
        int x = 120 + int(std::cos(a) * 80), y = 111 + int(std::sin(a) * 80);
        r.circle(x, y, 1, glow);
    }
    r.rect(12, 91, 216, 72, glow);
    r.rect(14, 93, 212, 68, 0x108B);
    r.rect(16, 95, 208, 64, 0x0847);
}
} // namespace nc::ui
