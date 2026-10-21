#pragma once
#include "domain.hpp"
namespace nc::ui {
struct Rect {
    int x, y, w, h;
    bool contains(int px, int py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
};
constexpr Rect CalibrationItem{8, 140, 224, 36}, CalibrationConfirm{12, 102, 216, 36},
    CalibrationRetry{12, 146, 100, 34}, CalibrationBack{128, 146, 100, 34};
constexpr Rect menu_item(int row) { return {10, 43 + row * 39, 220, 34}; }
constexpr Rect MazeGeometry{12, 67, 216, 22}, MazeContacts{12, 111, 216, 22},
    MazeNew{20, 140, 200, 27}, MazePlay{20, 174, 200, 28},
    MazeNewAlternate{20, 174, 200, 28};
struct Layout {
    std::array<std::array<char, 128>, 4> lines{};
    int count = 0;
    bool fits = true;
};
int text_width(const char *, int scale = 1);
Layout layout(const char *, int width, int height, int scale = 1);
bool text_box(Renderer &, Rect, const char *, uint16_t, int scale = 1, bool centered = true);
void button(Renderer &, Rect, const char *, uint16_t color = Accent);
void slider(Renderer &, Rect, int value, int maximum);
void line(Renderer &, int, int, int, int, uint16_t);
void orb(Renderer &, float, uint16_t);
} // namespace nc::ui
