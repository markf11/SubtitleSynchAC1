#include "subtitle_core.h"
#include "subtitle_runtime.h"
#include "audio_system.h"
#include "pattern_scan.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <cstring>

uint32_t memory[256]{};
bool readFixture(uintptr_t address, void* output, size_t size) {
    if (address < 0x1000 || address + size > 0x1000 + sizeof(memory)) return false;
    std::memcpy(output, reinterpret_cast<const char*>(memory) + address - 0x1000, size);
    return true;
}

void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
int main() {
    try {
        // Observed Ubisoft DX9 reference: event 10a80305 -> audio 20a802f9,
        // NOT the old high-nibble substitution 20a80305.
        memory[0] = 3; memory[1] = 3; memory[2] = 0x1040;
        memory[20] = 0x10a80305; memory[21] = 0x1100; // table handle 1
        memory[24] = 0x20a802f9; // table handle 2
        memory[64] = 0x00a80305; memory[65] = 1; memory[66] = 2;
        auto resolved = resolveAudio(0x10a80305, 0x1000, 1, readFixture);
        check(resolved.id == 0x20a802f9 && resolved.resolution == Resolution::DirectReference, "observed event-to-audio reference");
        memory[65] = 4;
        check(resolveAudio(0x10a80305, 0x1000, 1, readFixture).resolution == Resolution::Unresolved, "do not guess composite branches");
        memory[65] = 1; memory[66] = 3;
        check(!resolveAudio(0x10a80305, 0x1000, 1, readFixture).id, "out of bounds handle");
        memory[66] = 2;
        check(!resolveAudio(0x10a80306, 0x1000, 1, readFixture).id, "stale root ID");
        check(!resolveAudio(0x10a80305, 0x2000, 1, readFixture).id, "unreadable manager");
        check(resolveAudio(0x20a802f9, 0, 0, nullptr).id == 0x20a802f9, "exact audio ID needs no conversion");
        const char* fixture = "lookup_fixture.json";
        { std::ofstream f(fixture); f << R"({"0x10a80001":"exact<duration=2>","0x20a80001":"other<duration=2>","0x20a80002":"first<wait=1>second<duration=2>"})"; }
        SubtitleEngine engine;
        check(engine.load(fixture), "load fixture");
        check(engine.getRaw(0x10a80001) == "exact<duration=2>", "exact ID must win over transformed ID");
        check(engine.getRaw(0x30a80001).empty(), "unknown resource type must not alias voice");
        check(engine.getRaw(0).empty(), "unknown zero");
        SubtitleRuntime runtime;
        using C = SubtitleRuntime::clock;
        const auto t = C::time_point{};
        const auto segments = engine.getSegments(0x20a80002);
        runtime.start(segments, std::chrono::seconds(2), t);
        runtime.update(t + std::chrono::milliseconds(1100));
        check(runtime.currentText() == "second", "advance segment");
        runtime.update(t + std::chrono::seconds(2));
        check(!runtime.active(), "total duration caps last segment");
        runtime.start(segments, std::chrono::seconds(2), t);
        runtime.update(t + std::chrono::seconds(20));
        check(!runtime.active(), "delayed render expires subtitle");
        runtime.start(segments, std::chrono::seconds(2), t);
        runtime.start(segments, std::chrono::seconds(2), t + std::chrono::seconds(1));
        runtime.update(t + std::chrono::milliseconds(1500));
        check(runtime.currentText() == "first", "repeated playback restarts");
        AudioQueue queue;
        check(queue.push({0, 1, 1}) && queue.push({42, 2, 2}) && queue.push({42, 2, 3}), "enqueue repeated and zero IDs");
        AudioEvent event;
        check(queue.pop(event) && event.id == 0, "zero is an event, not empty sentinel");
        check(queue.pop(event) && event.timestampMs == 2, "first playback");
        check(queue.pop(event) && event.timestampMs == 3, "repeated playback preserved");
        check(!queue.pop(event), "empty queue");
        check(queue.push({11,0,0}) && queue.push({12,0,0}) && queue.discardAll() == 2,
              "pause transition discards queued events");
        check(!queue.pop(event), "discard leaves queue empty");
        for (size_t i=0; i<AudioQueue::Capacity; ++i) check(queue.push({42,0,i}), "fill queue");
        check(!queue.push({42,0,0}) && queue.takeDropped() == 1, "overflow counted");
        while (queue.pop(event)) {}
        // Concurrent producers: accepted + explicitly dropped must account for all events.
        std::thread a([&] { for (int i=0;i<10000;++i) queue.push({1,1,0}); });
        std::thread b([&] { for (int i=0;i<10000;++i) queue.push({2,2,0}); });
        a.join(); b.join();
        size_t accepted=0;
        while (queue.pop(event)) { ++accepted; check(event.id == event.threadId, "event fields stay together"); }
        check(accepted + queue.takeDropped() == 20000, "concurrent event accounting");
        uint8_t bytes[] = {0xE8,1,2,0xE8,1,3};
        check(PatternScan::FindOffsets(bytes, sizeof bytes, "E8 01 ??").size() == 2, "ambiguous pattern detected");
        check(PatternScan::FindOffsets(bytes, sizeof bytes, "E8 01 03").size() == 1, "unique pattern");
        check(PatternScan::FindOffsets(bytes, 1, "E8 01 03").empty(), "short buffer no unsigned underflow");
        check(PatternScan::FindOffsets(bytes, sizeof bytes, "").empty(), "empty pattern rejected");
        std::remove(fixture);
        std::cout << "PASS: lookup, timing, replay, queue concurrency/overflow, pattern matching\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
