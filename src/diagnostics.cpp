#include "diagnostics.h"
#include <Windows.h>
#include <cstdio>
#include <cstdarg>
#include <mutex>
#include <string>
namespace {
std::mutex logMutex;
FILE* logFile = nullptr;
FILE* captureFile = nullptr;
uint64_t captureSessionStart = 0;

std::string oneLine(const std::string& value) {
    std::string result;
    result.reserve(value.size());
    for (char c : value) {
        if (c == '\r' || c == '\n' || c == '\t') result.push_back(' ');
        else result.push_back(c);
    }
    return result;
}

void capturePrefix(const char* type, uint64_t atMs) {
    if (!captureFile) return;
    const uint64_t relative = atMs >= captureSessionStart ? atMs - captureSessionStart : 0;
    std::fprintf(captureFile, "%s\t%llu\t%llu\t", type,
        static_cast<unsigned long long>(atMs),
        static_cast<unsigned long long>(relative));
}
}
namespace Diagnostics {
void init(const std::string& directory) {
    const auto ini = directory + "\\SubtitleSynchAC1.ini";
    captureSessionStart = GetTickCount64();
    if (GetPrivateProfileIntA("Capture", "Enabled", 0, ini.c_str())) {
        captureFile = std::fopen((directory + "\\SubtitleSynchAC1.capture.tsv").c_str(), "a");
        if (captureFile) {
            std::setvbuf(captureFile, nullptr, _IOFBF, 64 * 1024);
            std::fprintf(captureFile,
                "session\t%llu\t0\tbuild=audio-sync-v5.11-capture-priority\n"
                "#type\ttick_ms\tsession_ms\tfields\n",
                static_cast<unsigned long long>(captureSessionStart));
            std::fflush(captureFile);
        } else {
            OutputDebugStringA("SubtitleSynchAC1: cannot open capture log\n");
        }
    }
    if (!GetPrivateProfileIntA("Diagnostics", "Enabled", 0, ini.c_str())) return;
    logFile = std::fopen((directory + "\\SubtitleSynchAC1.log").c_str(), "a");
    if (!logFile) { OutputDebugStringA("SubtitleSynchAC1: cannot open diagnostic log\n"); return; }
    char exe[MAX_PATH]{};
    GetModuleFileNameA(nullptr, exe, MAX_PATH);
    auto base = reinterpret_cast<const unsigned char*>(GetModuleHandleA(nullptr));
    auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    log("session exe=%s pe_timestamp=%08lx image_size=%08lx build=audio-sync-v5.11-capture-priority", exe,
        nt->FileHeader.TimeDateStamp, nt->OptionalHeader.SizeOfImage);
}
bool enabled() { return logFile != nullptr; }
bool captureEnabled() { return captureFile != nullptr; }
void log(const char* format, ...) {
    if (!logFile) return;
    std::lock_guard<std::mutex> lock(logMutex);
    std::fprintf(logFile, "t=%llu ", static_cast<unsigned long long>(GetTickCount64()));
    va_list args;
    va_start(args, format);
    std::vfprintf(logFile, format, args);
    va_end(args);
    std::fputc('\n', logFile);
    std::fflush(logFile);
}
void error(const char* message) {
    OutputDebugStringA(message);
    log("error=%s", message);
}

void captureAudio(uint64_t eventMs, uint32_t id, uint32_t rawId, uint32_t threadId,
                  uint32_t resolution, const char* mapping) {
    if (!captureFile) return;
    std::lock_guard<std::mutex> lock(logMutex);
    capturePrefix("audio_dispatch", eventMs);
    std::fprintf(captureFile, "id=0x%08lx\traw=0x%08lx\tthread=%lu\tresolution=%lu\tmapping=%s\n",
        static_cast<unsigned long>(id), static_cast<unsigned long>(rawId),
        static_cast<unsigned long>(threadId), static_cast<unsigned long>(resolution), mapping);
    static uint64_t lastFlush = 0;
    if (eventMs >= lastFlush + 1000) {
        std::fflush(captureFile);
        lastFlush = eventMs;
    }
}

void captureSubtitleStart(uint64_t eventMs, uint64_t displayMs, uint32_t id,
                          const char* priority, double databaseDuration,
                          double effectiveDuration, const std::string& text) {
    if (!captureFile) return;
    std::lock_guard<std::mutex> lock(logMutex);
    capturePrefix("subtitle_start", displayMs);
    const auto clean = oneLine(text);
    std::fprintf(captureFile,
        "id=0x%08lx\tpriority=%s\taudio_dispatch_ms=%llu\tdispatch_to_display_ms=%lld"
        "\tdb_duration_ms=%.0f\teffective_duration_ms=%.0f\ttext=%s\n",
        static_cast<unsigned long>(id), priority,
        static_cast<unsigned long long>(eventMs),
        static_cast<long long>(displayMs) - static_cast<long long>(eventMs),
        databaseDuration * 1000.0, effectiveDuration * 1000.0, clean.c_str());
    std::fflush(captureFile);
}

void captureSubtitleEnd(uint64_t startMs, uint64_t endMs, uint32_t id,
                        const char* priority, double playbackElapsed,
                        double databaseDuration, double effectiveDuration,
                        const char* reason, uint32_t replacementId) {
    if (!captureFile) return;
    std::lock_guard<std::mutex> lock(logMutex);
    capturePrefix("subtitle_end", endMs);
    std::fprintf(captureFile,
        "id=0x%08lx\tpriority=%s\tstart_ms=%llu\tend_ms=%llu\twall_elapsed_ms=%llu"
        "\tplayback_elapsed_ms=%.0f\tdb_duration_ms=%.0f\teffective_duration_ms=%.0f"
        "\treason=%s\treplacement_id=0x%08lx\n",
        static_cast<unsigned long>(id), priority,
        static_cast<unsigned long long>(startMs), static_cast<unsigned long long>(endMs),
        static_cast<unsigned long long>(endMs >= startMs ? endMs - startMs : 0),
        playbackElapsed * 1000.0, databaseDuration * 1000.0, effectiveDuration * 1000.0,
        reason, static_cast<unsigned long>(replacementId));
    std::fflush(captureFile);
}

void captureSegment(const char* action, uint64_t atMs, uint32_t id, size_t index,
                    double playbackElapsed, const std::string& text) {
    if (!captureFile) return;
    std::lock_guard<std::mutex> lock(logMutex);
    capturePrefix(action, atMs);
    const auto clean = oneLine(text);
    std::fprintf(captureFile, "id=0x%08lx\tsegment=%zu\tplayback_elapsed_ms=%.0f\ttext=%s\n",
        static_cast<unsigned long>(id), index, playbackElapsed * 1000.0, clean.c_str());
    std::fflush(captureFile);
}

void captureSuppressed(uint64_t atMs, uint32_t id, const char* priority,
                       uint32_t activeId, const char* activePriority) {
    if (!captureFile) return;
    std::lock_guard<std::mutex> lock(logMutex);
    capturePrefix("subtitle_suppressed", atMs);
    std::fprintf(captureFile, "id=0x%08lx\tpriority=%s\tactive_id=0x%08lx\tactive_priority=%s\n",
        static_cast<unsigned long>(id), priority, static_cast<unsigned long>(activeId), activePriority);
    std::fflush(captureFile);
}

void captureMarker(uint64_t atMs, uint32_t activeId, const char* activePriority,
                   double playbackElapsed, const std::string& text) {
    if (!captureFile) return;
    std::lock_guard<std::mutex> lock(logMutex);
    capturePrefix("review_marker_f2", atMs);
    const auto clean = oneLine(text);
    std::fprintf(captureFile, "active_id=0x%08lx\tactive_priority=%s\tplayback_elapsed_ms=%.0f\ttext=%s\n",
        static_cast<unsigned long>(activeId), activePriority, playbackElapsed * 1000.0, clean.c_str());
    std::fflush(captureFile);
}

void captureDisplay(uint64_t atMs, bool visible, uint32_t activeId, bool paused,
                    const std::string& text) {
    if (!captureFile) return;
    std::lock_guard<std::mutex> lock(logMutex);
    capturePrefix("display_state", atMs);
    const auto clean = oneLine(text);
    std::fprintf(captureFile, "visible=%u\tactive_id=0x%08lx\tpaused=%u\ttext=%s\n",
        visible, static_cast<unsigned long>(activeId), paused, clean.c_str());
    std::fflush(captureFile);
}
}
