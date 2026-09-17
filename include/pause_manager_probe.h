#pragma once

#include <cstdint>
#include <string_view>

struct PauseManagerSnapshot {
    uintptr_t object = 0;
    uint32_t activeCount = 0;
    uint32_t committedCount = 0;
};

class PauseManagerProbe {
public:
    // Matches the DX10 PauseManager singleton initialization. The first MOV
    // contains the relocated address of the singleton pointer slot.
    inline static constexpr std::string_view Pattern =
        "A1 ?? ?? ?? ?? 83 EC 0C 85 C0 0F 85 B8 00 00 00 "
        "A1 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 8B 09 8B 11 F7 D8 1B C0 25 ?? ?? ?? ?? 56 50";

    bool initialize();
    bool sample(PauseManagerSnapshot& snapshot) const;
    uintptr_t pointerSlot() const { return m_pointerSlot; }

private:
    uintptr_t m_pointerSlot = 0;
};
