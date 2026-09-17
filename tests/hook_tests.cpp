#include "hooks/asm_hooks.h"
#include "audio_system.h"
#include "playback_clock.h"
#include <Windows.h>
#include <MinHook.h>
#include <xbyak/xbyak.h>
#include <iostream>

int main() {
    // Real x86 execution: CALL relocation, original side effect/result, repeated
    // captures, and caller's ESI survive the actual production detour.
    unsigned calls = 0;
    uint32_t action[] = {0x00a80305, 1, 2};
    uint32_t table[12]{};
    table[4] = 0x10a80305; table[5] = reinterpret_cast<uintptr_t>(action);
    table[8] = 0x20a802f9;
    uint32_t manager[] = {3, 3, reinterpret_cast<uintptr_t>(table)};
    Xbyak::CodeGenerator callee;
    callee.mov(callee.eax, reinterpret_cast<uintptr_t>(&calls));
    callee.inc(callee.dword[callee.eax]);
    callee.mov(callee.eax, 0x12345678);
    callee.ret();
    callee.ready();
    Xbyak::CodeGenerator caller;
    caller.push(caller.esi);
    // Match the production call site's scoped-handle slot at ESP+0x18.
    caller.sub(caller.esp, 16);
    caller.mov(caller.dword[caller.esp + 12], 1);
    caller.mov(caller.esi, 0x10a80305);
    caller.mov(caller.eax, reinterpret_cast<uintptr_t>(manager));
    caller.push(1); caller.push(1); caller.push(caller.esi);
    auto site = reinterpret_cast<uintptr_t>(caller.getCurr());
    caller.call(callee.getCode());
    caller.add(caller.esp, 12);
    caller.cmp(caller.esi, 0x10a80305);
    caller.je("good");
    caller.xor_(caller.eax, caller.eax);
    caller.L("good");
    caller.add(caller.esp, 16);
    caller.pop(caller.esi);
    caller.ret();
    caller.ready();
    auto run = caller.getCode<unsigned(*)()>();
    if (run() != 0x12345678 || calls != 1) return 1;
    if (MH_Initialize() != MH_OK || !AudioHook::Install(site)) return 2;
    AudioEvent event;
    g_playbackClock.setPaused(true);
    if (run() != 0x12345678 || calls != 2 || !g_AudioQueue.pop(event) ||
        event.id != 0x20a802f9 || event.rawId != 0x10a80305) return 3;
    g_playbackClock.setPaused(false);
    if (run() != 0x12345678 || run() != 0x12345678 || calls != 4) return 4;
    for (int i=0; i<2; ++i)
        if (!g_AudioQueue.pop(event) || event.id != 0x20a802f9 || event.rawId != 0x10a80305 ||
            event.resolution != Resolution::DirectReference || !event.threadId) return 5;
    if (g_AudioQueue.pop(event)) return 6;
    MH_DisableHook(reinterpret_cast<void*>(site));
    MH_Uninitialize();
    if (run() != 0x12345678 || calls != 5) return 7;
    std::cout << "PASS: native x86 detour preserves CALL/result/ESI, captures pause-boundary dialogue and repeats events\n";
}
