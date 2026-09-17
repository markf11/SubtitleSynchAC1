#include <xmmintrin.h>
#include <filesystem>

#include "mod_runtime.h"
#include "hooks/asm_hooks.h"
#include "globals.h"
#include "diagnostics.h"
#include "playback_clock.h"

void ModRuntime::init(void) {
    std::string baseDir = std::filesystem::path(dllPath).parent_path().string();
    std::string jsonPath = baseDir + "\\subtitles.json";

    Diagnostics::init(baseDir);
    if (!m_engine.load(jsonPath)) {
        Diagnostics::error("subtitle database could not be loaded");
    } else {
        applyASMPatches();
        PauseHook::InstallGameHooks();
    }
    m_overlay.init();
}

void ModRuntime::update(void) {
    //some float bullshit for msvc :(
    unsigned int mxcsr_saved = _mm_getcsr();
    unsigned int cw_saved = 0;
    _controlfp_s(&cw_saved, 0, 0);

    _mm_setcsr((mxcsr_saved & ~_MM_ROUND_MASK) | _MM_ROUND_NEAREST);
    _controlfp_s(nullptr, _RC_NEAR, _MCW_RC);

    const auto dropped = g_AudioQueue.takeDropped();
    if (dropped) Diagnostics::log("queue_dropped count=%u", dropped);
    AudioEvent event;
    const auto playback = g_playbackClock.state();
    // Bound render-thread work even if producers keep adding events.
    for (size_t n = 0; !playback.paused && n < AudioQueue::Capacity && g_AudioQueue.pop(event); ++n)
    {
        Diagnostics::log("audio event_t=%llu thread=%lu id=0x%08lx raw=0x%08lx resolution=%lu",
            static_cast<unsigned long long>(event.timestampMs),
            static_cast<unsigned long>(event.threadId), static_cast<unsigned long>(event.id),
            static_cast<unsigned long>(event.rawId), static_cast<unsigned long>(event.resolution));
        if (event.resolution == Resolution::Unresolved) continue;
        handleVoiceline(event.id);
    }

    m_runtime.update(playback.now);

    // Keep the current subtitle in the frozen runtime, but do not draw it over
    // the game's pause menu. Resuming reveals the same segment again.
    bool visible = playbackSubtitleVisible(m_runtime.active(), playback.paused);
    std::string text = m_runtime.currentText();
    if (m_overlay.isDebugWindowVisible()) {
        visible = m_overlay.isDebugVisible() && !playback.paused;
        text = m_overlay.debugText();
    }

    m_overlay.setVisible(visible);
    m_overlay.setText(text);

    if (Diagnostics::enabled() && (visible != m_lastDiagnosticVisible || text != m_lastDiagnosticText)) {
        Diagnostics::log("display visible=%d bytes=%zu debug=%d paused=%d", visible, text.size(),
            m_overlay.isDebugWindowVisible(), playback.paused);
        m_lastDiagnosticVisible = visible;
        m_lastDiagnosticText = text;
    }
    if (Diagnostics::enabled() && playback.paused != m_lastDiagnosticPaused) {
        Diagnostics::log("playback_state paused=%d subtitle_active=%d text_bytes=%zu",
            playback.paused, m_runtime.active(), text.size());
        m_lastDiagnosticPaused = playback.paused;
    }

    m_overlay.render(m_window);
    _mm_setcsr(mxcsr_saved);
    _controlfp_s(nullptr, cw_saved, _MCW_RC);
}


bool ModRuntime::handleVoiceline(uint32_t id)
{
    const auto raw = m_engine.getRaw(id);
    Diagnostics::log("lookup id=0x%08lx key=0x%08lx hit=%d",
        static_cast<unsigned long>(id), static_cast<unsigned long>(id), !raw.empty());
    if (raw.empty()) return false;
    auto segments = SubtitleEngine::parseSegments(SubtitleEngine::stripTags(raw));
    if (segments.empty()) return false;
    auto duration = SubtitleEngine::extractDuration(raw);
    if (duration <= 0.0) duration = 3.0;
    Diagnostics::log("subtitle_start id=0x%08lx replaces_active=%d segments=%zu duration=%.3f",
        static_cast<unsigned long>(id), m_runtime.active(), segments.size(), duration);
    m_runtime.start(segments, std::chrono::duration_cast<SubtitleRuntime::clock::duration>(
        std::chrono::duration<double>(duration)), g_playbackClock.state().now);
    m_overlay.setSegments(segments);
    m_overlay.setVisible(true);
    return true;
}
