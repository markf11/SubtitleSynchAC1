#include "hooks/asm_hooks.h"
#include "hooks/audio_pattern.h"
#include "audio_system.h"
#include "diagnostics.h"
#include "pattern_scan.h"
#include "playback_clock.h"
#include <Windows.h>
#include <MinHook.h>
#include <xbyak/xbyak.h>
#include <memory>

namespace {
std::unique_ptr<Xbyak::CodeGenerator> audioStub;
bool ReadGameAudio(uintptr_t address, void* output, size_t size) {
    SIZE_T copied = 0;
    return address && ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(address),
        output, size, &copied) && copied == size;
}
void __cdecl PushAudioEvent(uint32_t rawId, uintptr_t manager, uint32_t handle) noexcept {
    // Menu/UI audio callbacks continue while the game is paused. They must not
    // be replayed as dialogue when gameplay resumes.
    if (g_playbackClock.paused()) return;
    const auto audio = resolveAudio(rawId, manager, handle, &ReadGameAudio);
    g_AudioQueue.push({audio.id, GetCurrentThreadId(), GetTickCount64(), rawId, audio.resolution});
}
}

bool applyASMPatches() {
    // Pause tracking is installed separately, preserving the game's original
    // 0x108 displacement through MinHook relocation.
    const auto address = PatternScan::Find(AudioHook::Pattern);
    if (!address) {
        Diagnostics::error("audio hook requires exactly one executable-section match; subtitles disabled");
        return false;
    }
    return AudioHook::Install(address + AudioHook::CallOffset);
}

bool AudioHook::Install(uintptr_t address) {
    try {
        auto code = std::make_unique<Xbyak::CodeGenerator>(1024);
        void* trampoline = nullptr;
        auto status = MH_CreateHook(reinterpret_cast<void*>(address),
            const_cast<uint8_t*>(code->getCode()), &trampoline);
        if (status != MH_OK) {
            Diagnostics::log("hook_create status=%d", static_cast<int>(status));
            Diagnostics::error("audio hook creation failed");
            return false;
        }
        try {
            auto& c = *code;
            c.pushad();
            c.pushfd();
            c.mov(c.ebp, c.esp);
            c.and_(c.esp, -16);
            c.sub(c.esp, 528);
            // FXSAVE [ESP+16] (not exposed by the pinned Xbyak version).
            const uint8_t save[] = {0x0F, 0xAE, 0x44, 0x24, 0x10};
            c.db(save, sizeof(save));
            c.cld();
            c.mov(c.dword[c.esp], c.esi);
            // At this exact CALL site EAX is the resource manager, and the
            // scoped event handle is at original ESP+0x18 (PUSHAD/PUSHFD: +36).
            c.mov(c.dword[c.esp + 4], c.eax);
            c.mov(c.edx, c.dword[c.ebp + 0x3c]);
            c.mov(c.dword[c.esp + 8], c.edx);
            c.mov(c.eax, reinterpret_cast<uintptr_t>(&PushAudioEvent));
            c.call(c.eax);
            c.fxrstor(c.ptr[c.esp + 16]);
            c.mov(c.esp, c.ebp);
            c.popfd();
            c.popad();
            // MinHook relocates the original CALL; its EAX and side effects
            // must reach the game's following MOV ESI,EAX unchanged.
            c.jmp(trampoline);
            c.ready();
            FlushInstructionCache(GetCurrentProcess(), c.getCode(), c.getSize());
        } catch (...) {
            MH_RemoveHook(reinterpret_cast<void*>(address));
            throw;
        }
        status = MH_EnableHook(reinterpret_cast<void*>(address));
        if (status != MH_OK) {
            MH_RemoveHook(reinterpret_cast<void*>(address));
            Diagnostics::error("audio hook enable failed");
            return false;
        }
        audioStub = std::move(code);
        Diagnostics::log("hook_enabled address=%p trampoline=%p", reinterpret_cast<void*>(address), trampoline);
        return true;
    } catch (const std::exception& e) {
        Diagnostics::error(e.what());
        return false;
    }
}
