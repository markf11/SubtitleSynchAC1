#include "diagnostics.h"
#include <Windows.h>
#include <cstdio>
#include <cstdarg>
#include <mutex>
namespace {
std::mutex logMutex;
FILE* logFile = nullptr;
}
namespace Diagnostics {
void init(const std::string& directory) {
    const auto ini = directory + "\\SubtitleSynchAC1.ini";
    if (!GetPrivateProfileIntA("Diagnostics", "Enabled", 0, ini.c_str())) return;
    logFile = std::fopen((directory + "\\SubtitleSynchAC1.log").c_str(), "a");
    if (!logFile) { OutputDebugStringA("SubtitleSynchAC1: cannot open diagnostic log\n"); return; }
    char exe[MAX_PATH]{};
    GetModuleFileNameA(nullptr, exe, MAX_PATH);
    auto base = reinterpret_cast<const unsigned char*>(GetModuleHandleA(nullptr));
    auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    log("session exe=%s pe_timestamp=%08lx image_size=%08lx build=audio-sync-v2-resolved", exe,
        nt->FileHeader.TimeDateStamp, nt->OptionalHeader.SizeOfImage);
}
bool enabled() { return logFile != nullptr; }
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
}
