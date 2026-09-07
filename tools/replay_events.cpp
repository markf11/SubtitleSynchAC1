#include "subtitle_core.h"
#include "subtitle_runtime.h"
#include <fstream>
#include <iostream>
#include <regex>

// Replay recorded audio timestamps against a subtitle DB without the game.
// This reports matching behavior, not whether a spoken sentence was correct.
int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: replay_events <SubtitleSynchAC1.log> <subtitles.json>\n";
        return 1;
    }
    SubtitleEngine engine;
    std::ifstream log(argv[1]);
    if (!log || !engine.load(argv[2])) return 2;
    SubtitleRuntime runtime;
    std::regex event(R"(audio event_t=(\d+) thread=(\d+) id=0x([0-9a-fA-F]{8}))");
    std::string line;
    size_t count=0, matched=0;
    std::cout << "event_ms,id,exact_match,legacy_candidate_match\n";
    while (std::getline(log, line)) {
        std::smatch m;
        if (!std::regex_search(line, m, event)) continue;
        auto ms=std::stoull(m[1]);
        auto id=static_cast<uint32_t>(std::stoul(m[3], nullptr, 16));
        auto t=SubtitleRuntime::clock::time_point(std::chrono::duration_cast<SubtitleRuntime::clock::duration>(std::chrono::milliseconds(ms)));
        runtime.update(t);
        const auto raw=engine.getRaw(id);
        ++count;
        if (!raw.empty()) {
            ++matched;
            double duration=SubtitleEngine::extractDuration(raw);
            if (duration<=0.0) duration=3.0;
            runtime.start(engine.getSegments(id), std::chrono::duration_cast<SubtitleRuntime::clock::duration>(std::chrono::duration<double>(duration)), t);
        }
        const bool candidate=!engine.getRaw((id & 0x0fffffff) | 0x20000000).empty();
        std::cout << ms << ",0x" << m[3] << ',' << !raw.empty() << ',' << candidate << '\n';
    }
    std::cerr << "events=" << count << " exact_matches=" << matched << '\n';
    return count ? 0 : 3;
}
