#pragma once
#include "domain.hpp"
namespace nc {
class Framebuffer final : public Renderer {
  public:
    std::array<uint16_t, Width * Height> pixels{};
    void clear(uint16_t) override;
    void rect(int, int, int, int, uint16_t) override;
    void circle(int, int, int, uint16_t) override;
    void text(int, int, const char *, uint16_t, int scale = 1) override;
};
} // namespace nc
