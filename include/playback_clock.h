#pragma once
#include <chrono>
#include <mutex>

// One shared logical clock: pause time never advances subtitle deadlines.
class PlaybackClock {
public:
    using Clock = std::chrono::steady_clock;
    struct State { Clock::time_point now; bool paused; };
    bool setPaused(bool paused, Clock::time_point wall = Clock::now()) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (paused == m_paused) return false;
        if (paused) m_pauseStart = wall;
        else m_excluded += wall - m_pauseStart;
        m_paused = paused;
        return true;
    }
    State state(Clock::time_point wall = Clock::now()) {
        std::lock_guard<std::mutex> lock(m_mutex);
        return {(m_paused ? m_pauseStart : wall) - m_excluded, m_paused};
    }
private:
    std::mutex m_mutex;
    bool m_paused = false;
    Clock::time_point m_pauseStart{};
    Clock::duration m_excluded{};
};
extern PlaybackClock g_playbackClock;
