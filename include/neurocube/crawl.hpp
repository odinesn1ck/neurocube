#pragma once
#include "domain.hpp"
namespace nc {
struct CrawlStory {
    const char *const *lines;
    size_t count;
};
constexpr int CrawlStoryCount = 3;
constexpr float CrawlSpeed = 80.f, CrawlLineSpacing = 48.f;
const CrawlStory &crawl_story(int index);
// Shared projection: both renderer and completion use the last glyph's lower edge.
inline float crawl_depth(float progress, int line, int row = 0) {
    return .5f + (progress - line * CrawlLineSpacing - row) / 240.f;
}
inline float crawl_y(float depth) {
    return depth > .01f ? -30.f + 260.f / depth : 100000.f;
}
inline bool crawl_finished(float progress, const CrawlStory &s) {
    return crawl_y(crawl_depth(progress, int(s.count) - 1, 16)) < 0;
}
void render_crawl(Renderer &, float progress, int episode);
void render_project_qr(Renderer &);
} // namespace nc
