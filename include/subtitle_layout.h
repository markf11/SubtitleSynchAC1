#pragma once
#include "subtitle_settings.h"

struct SubtitleLayout {
    ImVec2 position, size, padding;
    float wrapWidth;
};

float subtitleFontSize(const SubtitleSettings& settings, ImVec2 display);
// Call with the same ImGui font/size active as will be used for drawing.
SubtitleLayout measureSubtitle(const SubtitleSettings& settings, ImVec2 display, const char* text);
