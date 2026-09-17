#include "pause_manager_probe.h"

#include "diagnostics.h"
#include "pattern_scan.h"

#include <Windows.h>
#include <cstring>

namespace {
bool readMemory(uintptr_t address, void* output, size_t size) {
    SIZE_T copied = 0;
    return address && ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(address),
        output, size, &copied) && copied == size;
}
}

bool PauseManagerProbe::initialize() {
    const auto instruction = PatternScan::Find(Pattern);
    if (!instruction) {
        Diagnostics::log("pause_manager_probe unavailable reason=signature");
        return false;
    }

    static_assert(sizeof(uintptr_t) == 4, "AC1 probe requires the x86 build");
    std::memcpy(&m_pointerSlot, reinterpret_cast<const void*>(instruction + 1), sizeof(m_pointerSlot));
    if (!m_pointerSlot) {
        Diagnostics::log("pause_manager_probe unavailable reason=pointer-slot");
        return false;
    }

    Diagnostics::log("pause_manager_probe enabled instruction=%p pointer_slot=%p",
        reinterpret_cast<void*>(instruction), reinterpret_cast<void*>(m_pointerSlot));
    return true;
}

bool PauseManagerProbe::sample(PauseManagerSnapshot& snapshot) const {
    uintptr_t object = 0;
    if (!readMemory(m_pointerSlot, &object, sizeof(object)) || !object)
        return false;

    uint32_t counts[2]{};
    if (!readMemory(object + 0x24, counts, sizeof(counts)))
        return false;

    snapshot.object = object;
    snapshot.activeCount = counts[0];
    snapshot.committedCount = counts[1];
    return true;
}
