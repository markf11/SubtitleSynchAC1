#include "subtitle_layout.h"
#include <algorithm>
#include <cmath>

namespace {
float bounded(float value, float fallback, float low, float high) {
    return std::clamp(std::isfinite(value) ? value : fallback, low, high);
}
}

float subtitleFontSize(const SubtitleSettings& s, ImVec2 display) {
    const float base = bounded(s.fontSize, 30, 8, 120);
    const float factor = bounded(s.scale, 1, 0.5f, 3);
    const float reference = bounded(s.referenceHeight, 1080, 480, 4320);
    const float resolution = s.autoScale ? bounded(display.y, reference, 1, 16384) / reference : 1;
    return std::clamp(base * factor * resolution, 8.0f, 512.0f);
}

SubtitleLayout measureSubtitle(const SubtitleSettings& s, ImVec2 display, const char* text) {
    const float width = bounded(display.x, 1, 1, 32768);
    const float height = bounded(display.y, 1, 1, 16384);
    const float maxWidth = width * bounded(s.maxWidthPercent, 90, 20, 100) / 100;
    const float ratio = s.autoScale ? height / bounded(s.referenceHeight, 1080, 480, 4320) : 1;
    // Old configs used negative vertical padding; never let that clip glyphs.
    ImVec2 padding(bounded(s.padding.x, 8, 0, 100) * ratio,
                   bounded(s.padding.y, 6, 0, 100) * ratio);
    padding.x = std::min(padding.x, std::max(0.0f, (maxWidth - 1) / 2));
    const float wrap = std::max(1.0f, maxWidth - 2 * padding.x);
    const ImVec2 textSize = ImGui::CalcTextSize(text, nullptr, false, wrap);
    ImVec2 size(std::min(maxWidth, textSize.x + 2 * padding.x), textSize.y + 2 * padding.y);
    ImVec2 pos;
    if (s.autoPosition) {
        pos = ImVec2((width - size.x) / 2, height - 100 * height / 1080 - size.y);
    } else {
        pos = ImVec2(bounded(s.position.x, width / 2, 0, width) - size.x / 2,
                     bounded(s.position.y, height / 2, 0, height) - size.y / 2);
    }
    pos.x = std::clamp(pos.x, 0.0f, std::max(0.0f, width - size.x));
    pos.y = std::clamp(pos.y, 0.0f, std::max(0.0f, height - size.y));
    return {pos, size, padding, wrap};
}
