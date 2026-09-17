#pragma once

#include <cstdint>
#include <deque>
#include <Windows.h>

#include "subtitle_runtime.h"
#include "audio_system.h"
#include "overlay_ui.h"
#include "ini_config.h"
#include "playback_clock.h"
#include "pause_manager_probe.h"
#include "save_activity_monitor.h"

class ModRuntime {
public:
    void init();
    void update();

    void onPause(bool paused);
    void onFocus(bool focused);

    bool handleVoiceline(uint32_t id);

    AudioQueue m_audio;
    SubtitleEngine m_engine;
    SubtitleOverlay m_overlay;
    SubtitleRuntime m_runtime;
    HWND m_window;

    bool m_paused = false;
    std::string m_lastDiagnosticText;
    bool m_lastDiagnosticVisible = false;
    bool m_lastDiagnosticPaused = false;
    EscapePauseTracker m_escapePause;
    ResumeDelayGate m_resumeGate;
    PauseManagerProbe m_pauseManagerProbe;
    SaveActivityMonitor m_saveActivityMonitor;
    bool m_escapeSampled = false;
    bool m_lastEscapeDown = false;
    ULONGLONG m_lastEscapeReleaseMs = 0;
    bool m_pauseManagerSampled = false;
    bool m_lastPauseManagerPaused = false;
    uint32_t m_lastPauseManagerActiveCount = 0;
    uint32_t m_lastPauseManagerCommittedCount = 0;
    uint32_t m_activeSubtitleId = 0;
    SubtitleRuntime::clock::time_point m_activeSubtitleStart{};
    double m_activeSubtitleDuration = 0.0;
    bool m_lastDiagnosticDebugWindow = false;
    bool m_lastDiagnosticDebugPreview = false;
    bool m_lastDiagnosticSaveActivity = false;
    std::deque<AudioEvent> m_pausedDialogueEvents;
};
