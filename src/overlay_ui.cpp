#include <imgui.h>
#include <chrono>
#include <misc/cpp/imgui_stdlib.h>

#include "overlay_ui.h"
#include "reshaper/arabic.h"
#include "ini_config.h"
#include "subtitle_settings.h"
#include "subtitle_layout.h"
#include "globals.h"

SubtitleSettings g_subtitleSettings;

void SubtitleOverlay::init() {
    m_config.load();

    ImGuiIO& io = ImGui::GetIO();
    g_subtitleSettings.font = io.Fonts->AddFontDefaultVector();
    io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;

    std::string fullPath = dllPath;

    fullPath = fullPath.substr(0, fullPath.find_last_of("\\/"));
    fullPath += "\\subtitles.ttf";

    printf("Font full path: %s\n", fullPath.c_str());
    ImFontConfig config;
    config.MergeMode = false;

    static const ImWchar fontRanges[] =
    {
        0x0, 0xFFFF, 0x0,
    };

    ImFont* f = io.Fonts->AddFontFromFileTTF(
        fullPath.c_str(),
        30.0f,
        &config,
        fontRanges
    );

    if (!f)
        printf("Font failed to load!\n");

    if (f)
        g_subtitleSettings.font = f;
}

static std::wstring reshape(const std::string& s)
{
    return ShapingEngine::wrender(ShapingEngine::Helper::widen(s));
}

void SubtitleOverlay::setText(const std::string& text)
{
    m_rawText = text;
    m_currentText = text;
    
    m_segments.clear();
    m_index = 0;
}

void SubtitleOverlay::setSegments(const std::vector<SubtitleSegment>& segments)
{
    m_segments = segments;
    m_index = 0;

    if (!m_segments.empty())
    {
        m_currentText = m_segments[0].text;

        double wait = m_segments[0].waitAfter;

        if (wait > 0.0)
        {
            m_segmentUntil = std::chrono::steady_clock::now() +
                std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                    std::chrono::duration<double>(wait));
        }
        else
        {
            m_segmentUntil = std::chrono::steady_clock::time_point{};
        }
    }
}

void SubtitleOverlay::update()
{
    if (!m_visible || m_segments.empty())
        return;

    auto now = std::chrono::steady_clock::now();

    if (m_segmentUntil != std::chrono::steady_clock::time_point{} &&
        now >= m_segmentUntil)
    {
        advanceSegment();
    }

    if (m_visibleUntil != std::chrono::steady_clock::time_point{} &&
        now >= m_visibleUntil)
    {
        m_visible = false;
        m_segments.clear();
        m_index = 0;
    }
}

void SubtitleOverlay::advanceSegment()
{
    if (m_segments.empty())
        return;

    ++m_index;

    if (m_index >= m_segments.size())
    {
        m_visible = false;
        return;
    }

    m_currentText = m_segments[m_index].text;

    double wait = m_segments[m_index].waitAfter;

    if (wait > 0.0)
    {
        m_segmentUntil = std::chrono::steady_clock::now() +
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                std::chrono::duration<double>(wait));
    }
    else
    {
        m_segmentUntil = std::chrono::steady_clock::time_point{};
    }
}


void SubtitleOverlay::drawDebugWindow() {
    ImGui::SetNextWindowSize(ImVec2(450, 430), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(0, 0));

    ImGui::Begin("SubtitleSynchAC1 by bloxtbc", &m_debugWindow, ImGuiWindowFlags_None);

    ImGui::Separator();

    ImGui::TextWrapped("This mod was developed and created by bloxtbc on NexusMods, support me and report any bugs if there are any on the NexusMods page for SubtitleSynchAC1.");

    ImGui::Checkbox("Show subtitle", &m_debugVisible);
    ImGui::Text("Loaded font: %s", g_subtitleSettings.font ? "yes" : "no");

    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 screenSize = io.DisplaySize;
    ImGui::Text("Game display: %.0f x %.0f", screenSize.x, screenSize.y);
    ImGui::SliderFloat("Subtitle scale", &g_subtitleSettings.scale, 0.5f, 3.0f);
    ImGui::Text("Effective font size: %.1f px", subtitleFontSize(g_subtitleSettings, screenSize));
    ImGui::Text("Save activity: %s", m_saveIndicatorActive ? "detected" : "idle");
    ImGui::TextWrapped("Resolution scaling, wrapping, centering and save-indicator positioning are automatic. Advanced settings remain available in SubtitleSynchAC1.ini.");

    ImGui::Separator();

    ImGui::InputTextMultiline(
        "##debug_output",
        &m_debugInput,
        ImVec2(-1.0f, ImGui::GetTextLineHeight() * 8),
        ImGuiInputTextFlags_AllowTabInput
    );

    if (ImGui::Button("Save to .ini"))
       m_config.save();

    ImGui::TextWrapped("F1 toggles this debug window.");

    ImGui::End();
}

void SubtitleOverlay::render(HWND window)
{
    if (GetAsyncKeyState(VK_F1) & 1) {
        m_debugWindow = !m_debugWindow;
    }

    ImGui::GetIO().MouseDrawCursor = m_debugWindow;

    if (m_debugWindow)
        drawDebugWindow();

    if (!m_visible || m_currentText.empty())
        return;

    std::wstring reshapedW = ShapingEngine::wrender(ShapingEngine::Helper::widen(m_currentText));
    std::string reshaped = ShapingEngine::Helper::narrow(reshapedW);

    // Win32 refreshes DisplaySize from the game's client area every frame,
    // including fullscreen resolution changes. Never use desktop dimensions.
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    if (display.x <= 0 || display.y <= 0) return;
    ImGui::PushFont(g_subtitleSettings.font, subtitleFontSize(g_subtitleSettings, display));
    const float saveLift = m_saveIndicatorActive ? g_subtitleSettings.saveIndicatorLift : 0.0f;
    const auto layout = measureSubtitle(g_subtitleSettings, display, reshaped.c_str(), saveLift);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, g_subtitleSettings.backgroundColor);
    ImGui::PushStyleColor(ImGuiCol_Text, g_subtitleSettings.textColor);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(1, 1));
    ImGui::SetNextWindowPos(layout.position, ImGuiCond_Always);
    ImGui::SetNextWindowSize(layout.size, ImGuiCond_Always);

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (ImGui::Begin("SubtitleOverlay", nullptr, flags))
    {
        // Draw the measured lines individually so short final lines share
        // the same horizontal center as the rest of the subtitle block.
        const ImVec2 origin = ImGui::GetWindowPos();
        const ImU32 color = ImGui::GetColorU32(ImGuiCol_Text);
        for (const auto& line : layout.lines)
            ImGui::GetWindowDrawList()->AddText(
                ImVec2(origin.x + line.offset.x, origin.y + line.offset.y), color, line.text.c_str());
    }
    ImGui::End();
    ImGui::PopStyleVar(4);
    ImGui::PopStyleColor(2);
    ImGui::PopFont();
}
