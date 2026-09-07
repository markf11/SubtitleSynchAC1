#pragma once
#include <string_view>
namespace AudioHook {
// Include the result use and argument cleanup: the old CALL-only pattern
// also matched an unrelated routine earlier in the DX10 image.
inline constexpr std::string_view Pattern =
    "E8 ?? ?? ?? ?? 8B 48 08 6A 01 C1 E6 04 8B 34 0E 6A 01 56 E8 ?? ?? ?? ?? 8B F0 8B 44 24 18 83 C4 0C 85 C0 C6 44 24 18 00 74 14";
inline constexpr unsigned CallOffset = 19;
}
