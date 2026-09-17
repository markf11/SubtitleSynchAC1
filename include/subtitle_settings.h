#pragma once

#include <imgui.h>

struct SubtitleSettings
{
    ImVec2 position { 960.0f, 980.0f };
    ImVec2 padding  { 8.0f, 6.0f };

    ImVec4 textColor       { 1.f, 1.f, 1.f, 1.f };
    ImVec4 backgroundColor { 0.f, 0.f, 0.f, 0.5f };

    float scale = 1.0f;
    float fontSize = 30.0f;
    bool autoScale = true;
    float referenceHeight = 1080.0f;
    float maxWidthPercent = 75.0f;
    int resumeDelayMs = 500;
    int tailExtensionMs = 1500;
    float bottomMargin = 100.0f;
    float saveIndicatorLift = 90.0f;
    int saveIndicatorDurationMs = 3300;

    bool autoPosition = true;

    ImFont* font = nullptr;
};

extern SubtitleSettings g_subtitleSettings;
