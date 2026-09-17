#include "playback_clock.h"
#include "subtitle_runtime.h"
#include "hooks/asm_hooks.h"
#include "pause_manager_probe.h"
#include "pattern_scan.h"
#include <Windows.h>
#include <MinHook.h>
#include <xbyak/xbyak.h>
#include <iostream>
#include <stdexcept>

void check(bool condition, const char* reason) { if (!condition) throw std::runtime_error(reason); }
int main() {
    try {
        const uint8_t pauseManagerBytes[] = {
            0x90, 0xA1, 0x70, 0x19, 0xA1, 0x01, 0x83, 0xEC, 0x0C, 0x85, 0xC0,
            0x0F, 0x85, 0xB8, 0x00, 0x00, 0x00, 0xA1, 0x8C, 0x5A, 0x91, 0x01,
            0x8B, 0x0D, 0x44, 0x37, 0xA1, 0x01, 0x8B, 0x09, 0x8B, 0x11, 0xF7,
            0xD8, 0x1B, 0xC0, 0x25, 0x20, 0x26, 0x68, 0x01, 0x56, 0x50, 0x90
        };
        const auto pauseManagerMatches = PatternScan::FindOffsets(pauseManagerBytes,
            sizeof(pauseManagerBytes), PauseManagerProbe::Pattern);
        check(pauseManagerMatches.size() == 1 && pauseManagerMatches[0] == 1,
            "PauseManager signature resolves one exact initialization sequence");

        EscapePauseTracker escape;
        check(escape.update(false, false) == EscapePauseAction::None, "released Escape has no edge");
        check(escape.update(true, false) == EscapePauseAction::Pause, "first Escape press pauses");
        check(escape.update(true, true) == EscapePauseAction::None, "holding Escape does not repeat");
        check(escape.update(false, true) == EscapePauseAction::None, "opening-key release stays paused");
        check(escape.update(true, true) == EscapePauseAction::None, "closing press defers resume");
        check(escape.update(true, true) == EscapePauseAction::None, "held closing key stays deferred");
        check(escape.update(false, true) == EscapePauseAction::Resume, "closing-key release resumes");

        ResumeDelayGate resumeGate;
        const auto gateZero = ResumeDelayGate::Clock::time_point{};
        check(!resumeGate.request(std::chrono::milliseconds(500), gateZero), "configured delay defers release resume");
        check(resumeGate.pending(), "deferred resume remains pending");
        check(!resumeGate.update(gateZero + std::chrono::milliseconds(499)), "delay keeps subtitle frozen");
        check(resumeGate.update(gateZero + std::chrono::milliseconds(500)), "deadline releases subtitle clock");
        check(!resumeGate.pending(), "completed resume clears the gate");
        check(resumeGate.request(std::chrono::milliseconds(0), gateZero), "zero delay resumes immediately");
        check(!resumeGate.pending(), "immediate resume never leaves playback stuck");
        check(!resumeGate.request(std::chrono::milliseconds(500), gateZero), "gate can defer another pause cycle");
        resumeGate.cancel();
        check(!resumeGate.pending(), "new pause cancels a stale resume request");

        PlaybackClock clock;
        SubtitleRuntime runtime;
        const auto zero = PlaybackClock::Clock::time_point{};
        using namespace std::chrono;
        runtime.start({{"first", 2}, {"second", 0}}, seconds(5), clock.state(zero).now);
        clock.setPaused(true, zero+seconds(1));
        check(clock.paused(), "fast callback state reports pause");
        check(!playbackSubtitleVisible(runtime.active(), clock.paused()), "active subtitle hidden in menu");
        check(!clock.setPaused(true, zero+seconds(20)), "repeated pause must not move the pause boundary");
        runtime.update(clock.state(zero+seconds(100)).now);
        check(runtime.currentText()=="first", "long pause keeps the current segment");
        clock.setPaused(false, zero+seconds(101));
        check(!clock.paused(), "fast callback state reports resume");
        check(playbackSubtitleVisible(runtime.active(), clock.paused()), "same active subtitle returns after menu");
        runtime.update(clock.state(zero+milliseconds(101999)).now);
        check(runtime.currentText()=="first", "remaining segment time preserved");
        runtime.update(clock.state(zero+seconds(102)).now);
        check(runtime.currentText()=="second", "segment resumes at exact deadline");
        clock.setPaused(true,zero+seconds(103)); clock.setPaused(false,zero+seconds(153));
        runtime.update(clock.state(zero+seconds(154)).now);
        check(runtime.active(), "second pause extends total deadline");
        runtime.update(clock.state(zero+seconds(155)).now);
        check(!runtime.active(), "total duration expires after active time only");

        check(MH_Initialize()==MH_OK,"initialize MinHook");
        uint8_t target[16]{};
        uint32_t state[80]{}; state[0x108/4]=1; state[108/4]=0;
        Xbyak::CodeGenerator pause, resume;
        for (unsigned playing=0;playing<2;++playing) {
            auto& c=playing?resume:pause;
            c.push(c.edi); c.push(c.esi);
            c.mov(c.edi,reinterpret_cast<uintptr_t>(target));
            c.mov(c.esi,reinterpret_cast<uintptr_t>(state));
            const auto site=reinterpret_cast<uintptr_t>(c.getCurr());
            c.mov(c.byte[c.edi+8],playing); c.cmp(c.dword[c.esi+0x108],0);
            c.setne(c.al); c.movzx(c.eax,c.al); c.pop(c.esi); c.pop(c.edi); c.ret(); c.ready();
            check(PauseHook::Install(site,playing),"install actual pause/resume detour");
            check(c.getCode<unsigned(*)()>()()==1,"preserve original 0x108 comparison and flags");
            check(target[8]==playing,"preserve original game state write");
            check(g_playbackClock.state().paused==(playing==0),"observe real state transition");
        }
        MH_DisableHook(MH_ALL_HOOKS); MH_Uninitialize();
        std::cout<<"PASS: release-gated resume, repeated pauses, deadline preservation, native x86 state and flags\n";
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n'; return 1;}
}
