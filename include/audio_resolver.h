#pragma once
#include <cstddef>
#include <cstdint>

enum class Resolution : uint32_t { Exact = 0, DirectReference = 1, Unresolved = 2 };
struct ResolvedAudio {
    uint32_t id = 0;
    Resolution resolution = Resolution::Unresolved;
};
using ReadAudioMemory = bool (*)(uintptr_t, void*, size_t);

// AC1's manager has a 16-byte resource table; type-1 events hold an audio
// resource handle at object+8. Resource handles are NOT subtitle identifiers.
ResolvedAudio resolveAudio(uint32_t eventId, uintptr_t manager, uint32_t handle,
                           ReadAudioMemory read);
