#include <xmmintrin.h>
#include <algorithm>
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
    if (Diagnostics::enabled())
        m_pauseManagerProbe.initialize();
    if (!m_engine.load(jsonPath)) {
        Diagnostics::error("subtitle database could not be loaded");
    } else {
        applyASMPatches();
        // The old executable signatures do not represent the ESC pause menu
        // in the installed DX10 build. Menu pause is observed from the ESC
        // edge in update(), where every Present frame is available.
        Diagnostics::log("pause_tracking source=escape-edge+configured-resume-delay");
    }
    m_overlay.init();
    const bool saveMonitorReady = m_saveActivityMonitor.initialize();
    Diagnostics::log("save_activity_monitor ready=%u directory=%ls",
        saveMonitorReady, saveMonitorReady ? m_saveActivityMonitor.directory().c_str() : L"");
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

    const bool gameHasFocus = GetForegroundWindow() == m_window;
    EscapePauseAction pauseAction = EscapePauseAction::None;
    if (gameHasFocus) {
        const bool escapeDown = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
        if (!m_escapeSampled) {
            m_escapeSampled = true;
            m_lastEscapeDown = escapeDown;
        } else if (escapeDown != m_lastEscapeDown) {
            m_lastEscapeDown = escapeDown;
            const auto edgeTime = GetTickCount64();
            Diagnostics::log("escape_edge down=%u", escapeDown);
            if (!escapeDown)
                m_lastEscapeReleaseMs = edgeTime;
        }
        pauseAction = m_escapePause.update(escapeDown, g_playbackClock.paused());
    }
    PauseManagerSnapshot pauseManager;
    if (Diagnostics::enabled() && m_pauseManagerProbe.sample(pauseManager)) {
        const bool managerPaused = pauseManager.committedCount != 0;
        const bool changed = !m_pauseManagerSampled ||
            pauseManager.activeCount != m_lastPauseManagerActiveCount ||
            pauseManager.committedCount != m_lastPauseManagerCommittedCount;
        if (changed) {
            Diagnostics::log("pause_manager_state object=%p active_count=%lu committed_count=%lu paused=%u",
                reinterpret_cast<void*>(pauseManager.object),
                static_cast<unsigned long>(pauseManager.activeCount),
                static_cast<unsigned long>(pauseManager.committedCount), managerPaused);
        }
        if (m_pauseManagerSampled && m_lastPauseManagerPaused && !managerPaused) {
            const auto now = GetTickCount64();
            if (m_lastEscapeReleaseMs && now >= m_lastEscapeReleaseMs) {
                Diagnostics::log("pause_manager_resume escape_release_delay_ms=%llu",
                    static_cast<unsigned long long>(now - m_lastEscapeReleaseMs));
            } else {
                Diagnostics::log("pause_manager_resume escape_release_delay_ms=unavailable");
            }
        }
        m_pauseManagerSampled = true;
        m_lastPauseManagerPaused = managerPaused;
        m_lastPauseManagerActiveCount = pauseManager.activeCount;
        m_lastPauseManagerCommittedCount = pauseManager.committedCount;
    }

    auto setPlaybackPaused = [](bool paused, const char* source) {
        if (!g_playbackClock.setPaused(paused))
            return;
        Diagnostics::log("playback_pause paused=%u source=%s", paused, source);
    };

    bool resumeNow = false;
    if (pauseAction == EscapePauseAction::Pause) {
        m_resumeGate.cancel();
        setPlaybackPaused(true, "escape-press");
    } else if (pauseAction == EscapePauseAction::Resume) {
        const auto delay = std::chrono::milliseconds(g_subtitleSettings.resumeDelayMs);
        resumeNow = m_resumeGate.request(delay);
        if (!resumeNow) {
            Diagnostics::log("playback_resume_pending source=configured-delay delay_ms=%d",
                g_subtitleSettings.resumeDelayMs);
        }
    }

    if (m_resumeGate.update()) {
        resumeNow = true;
    }
    if (resumeNow)
        setPlaybackPaused(false, "configured-resume-delay");

    const bool saveActivity = m_saveActivityMonitor.update(
        g_subtitleSettings.saveIndicatorDurationMs);
    m_overlay.setSaveIndicatorActive(saveActivity);
    if (Diagnostics::enabled() && saveActivity != m_lastDiagnosticSaveActivity) {
        Diagnostics::log("save_activity active=%u lift=%.1f duration_ms=%d",
            saveActivity, g_subtitleSettings.saveIndicatorLift,
            g_subtitleSettings.saveIndicatorDurationMs);
        m_lastDiagnosticSaveActivity = saveActivity;
    }

    AudioEvent event;
    const auto queuePlayback = g_playbackClock.state();
    if (!queuePlayback.paused) {
        while (!m_pausedDialogueEvents.empty()) {
            event = m_pausedDialogueEvents.front();
            m_pausedDialogueEvents.pop_front();
            Diagnostics::log("paused_dialogue action=replay id=0x%08lx event_t=%llu remaining=%zu",
                static_cast<unsigned long>(event.id),
                static_cast<unsigned long long>(event.timestampMs), m_pausedDialogueEvents.size());
            handleVoiceline(event.id);
        }
    }
    // Bound render-thread work even if producers keep adding events.
    for (size_t n = 0; n < AudioQueue::Capacity && g_AudioQueue.pop(event); ++n)
    {
        Diagnostics::log("audio event_t=%llu thread=%lu id=0x%08lx raw=0x%08lx resolution=%lu",
            static_cast<unsigned long long>(event.timestampMs),
            static_cast<unsigned long>(event.threadId), static_cast<unsigned long>(event.id),
            static_cast<unsigned long>(event.rawId), static_cast<unsigned long>(event.resolution));
        if (event.resolution == Resolution::Unresolved) continue;
        if (queuePlayback.paused) {
            if (m_engine.getRaw(event.id).empty()) {
                Diagnostics::log("paused_dialogue action=discard-unmapped id=0x%08lx",
                    static_cast<unsigned long>(event.id));
                continue;
            }
            if (event.id == m_activeSubtitleId) {
                Diagnostics::log("paused_dialogue action=discard-active-duplicate id=0x%08lx",
                    static_cast<unsigned long>(event.id));
                continue;
            }
            if (!m_pausedDialogueEvents.empty() &&
                m_pausedDialogueEvents.back().id == event.id &&
                event.timestampMs >= m_pausedDialogueEvents.back().timestampMs &&
                event.timestampMs - m_pausedDialogueEvents.back().timestampMs <= 250) {
                Diagnostics::log("paused_dialogue action=discard-near-duplicate id=0x%08lx",
                    static_cast<unsigned long>(event.id));
                continue;
            }
            if (m_pausedDialogueEvents.size() >= 32) {
                Diagnostics::log("paused_dialogue action=discard-capacity id=0x%08lx",
                    static_cast<unsigned long>(event.id));
                continue;
            }
            m_pausedDialogueEvents.push_back(event);
            Diagnostics::log("paused_dialogue action=buffer id=0x%08lx pending=%zu",
                static_cast<unsigned long>(event.id), m_pausedDialogueEvents.size());
            continue;
        }
        handleVoiceline(event.id);
    }

    const auto playback = g_playbackClock.state();
    const auto runtimeUpdate = m_runtime.update(playback.now);
    if (runtimeUpdate.kind == SubtitleUpdateKind::SegmentChanged) {
        const auto elapsed = std::chrono::duration<double>(playback.now - m_activeSubtitleStart).count();
        Diagnostics::log("subtitle_segment id=0x%08lx from=%zu to=%zu count=%zu elapsed=%.3f remaining=%.3f",
            static_cast<unsigned long>(m_activeSubtitleId), runtimeUpdate.previousIndex,
            runtimeUpdate.currentIndex, m_runtime.segmentCount(), elapsed,
            (std::max)(0.0, m_activeSubtitleDuration - elapsed));
    } else if (runtimeUpdate.kind == SubtitleUpdateKind::DurationExpired ||
               runtimeUpdate.kind == SubtitleUpdateKind::SegmentsExhausted) {
        const auto elapsed = std::chrono::duration<double>(playback.now - m_activeSubtitleStart).count();
        Diagnostics::log("subtitle_end id=0x%08lx reason=%s segment=%zu elapsed=%.3f configured_duration=%.3f",
            static_cast<unsigned long>(m_activeSubtitleId),
            runtimeUpdate.kind == SubtitleUpdateKind::DurationExpired ? "duration-expired" : "segments-exhausted",
            runtimeUpdate.previousIndex, elapsed, m_activeSubtitleDuration);
        m_activeSubtitleId = 0;
        m_activeSubtitleDuration = 0.0;
    }

    // Keep the current subtitle in the frozen runtime, but do not draw it over
    // the game's pause menu. Resuming reveals the same segment again.
    bool visible = playbackSubtitleVisible(m_runtime.active(), playback.paused);
    std::string text = m_runtime.currentText();
    const bool debugWindow = m_overlay.isDebugWindowVisible();
    const bool debugPreview = m_overlay.isDebugVisible();
    if (debugWindow && debugPreview) {
        visible = !playback.paused;
        text = m_overlay.debugText();
    }

    if (Diagnostics::enabled() &&
        (debugWindow != m_lastDiagnosticDebugWindow || debugPreview != m_lastDiagnosticDebugPreview)) {
        Diagnostics::log("debug_ui window=%d preview=%d subtitle_active=%d paused=%d",
            debugWindow, debugPreview, m_runtime.active(), playback.paused);
        m_lastDiagnosticDebugWindow = debugWindow;
        m_lastDiagnosticDebugPreview = debugPreview;
    }

    m_overlay.setVisible(visible);
    m_overlay.setText(text);

    if (Diagnostics::enabled() && (visible != m_lastDiagnosticVisible || text != m_lastDiagnosticText)) {
        const char* reason = visible ? (debugWindow && debugPreview ? "debug-preview" : "game-subtitle")
            : playback.paused ? "pause-menu"
            : !m_runtime.active() ? "subtitle-inactive"
            : "empty-or-disabled";
        Diagnostics::log("display visible=%d bytes=%zu debug_window=%d debug_preview=%d paused=%d reason=%s",
            visible, text.size(), debugWindow, debugPreview, playback.paused, reason);
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
    const double baseDuration = SubtitleEngine::extractDuration(raw);
    const double duration = applySubtitleTailExtension(
        baseDuration, g_subtitleSettings.tailExtensionMs);
    const auto now = g_playbackClock.state().now;
    if (m_runtime.active()) {
        const auto elapsed = std::chrono::duration<double>(now - m_activeSubtitleStart).count();
        Diagnostics::log("subtitle_end id=0x%08lx reason=replaced-by-audio replacement_id=0x%08lx segment=%zu elapsed=%.3f configured_duration=%.3f",
            static_cast<unsigned long>(m_activeSubtitleId), static_cast<unsigned long>(id),
            m_runtime.currentIndex(), elapsed, m_activeSubtitleDuration);
    }
    double waitsBeforeFinal = 0.0;
    for (size_t i = 0; i + 1 < segments.size(); ++i)
        if (segments[i].waitAfter > 0.0) waitsBeforeFinal += segments[i].waitAfter;
    Diagnostics::log("subtitle_start id=0x%08lx replaces_active=%d segments=%zu base_duration=%.3f tail_extension_ms=%d duration=%.3f final_window=%.3f",
        static_cast<unsigned long>(id), m_runtime.active(), segments.size(),
        baseDuration > 0.0 ? baseDuration : 3.0, g_subtitleSettings.tailExtensionMs, duration,
        (std::max)(0.0, duration - waitsBeforeFinal));
    m_runtime.start(segments, std::chrono::duration_cast<SubtitleRuntime::clock::duration>(
        std::chrono::duration<double>(duration)), now);
    m_activeSubtitleId = id;
    m_activeSubtitleStart = now;
    m_activeSubtitleDuration = duration;
    m_overlay.setSegments(segments);
    m_overlay.setVisible(true);
    return true;
}
