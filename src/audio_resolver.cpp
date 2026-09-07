#include "audio_resolver.h"

ResolvedAudio resolveAudio(uint32_t eventId, uintptr_t manager, uint32_t handle,
                           ReadAudioMemory read) {
    if ((eventId & 0xf0000000) == 0x20000000)
        return {eventId, Resolution::Exact};
    if ((eventId & 0xf0000000) != 0x10000000 || !manager || !read)
        return {};
    uint32_t header[3]{};
    if (!read(manager, header, sizeof header)) return {};
    const uint32_t capacity=header[0], count=header[1], table=header[2];
    if (!table || !count || count > capacity || capacity > 1000000 || handle >= count)
        return {};
    // Validate every copied record. The callback does not retain game pointers.
    uint32_t root[4]{}, action[3]{}, audio[4]{};
    const auto entry = [table](uint32_t index) -> uintptr_t {
        const uint64_t address = uint64_t(table) + uint64_t(index) * 16;
        return address <= UINT32_MAX - 16 ? static_cast<uintptr_t>(address) : 0;
    };
    const auto rootAddress = entry(handle);
    if (!rootAddress || !read(rootAddress, root, sizeof root) || root[0] != eventId || !root[1])
        return {};
    if (!read(root[1], action, sizeof action) || action[0] != (eventId & 0x0fffffff))
        return {};
    // Only the direct-play event layout has been verified. Never choose a
    // random/conditional/sequence branch by walking all possible children.
    if (action[1] != 1 || !action[2] || action[2] >= count) return {};
    const auto audioAddress = entry(action[2]);
    if (!audioAddress || !read(audioAddress, audio, sizeof audio) ||
        (audio[0] & 0xf0000000) != 0x20000000) return {};
    uint32_t again[3]{}, rootAgain[4]{};
    if (!read(manager, again, sizeof again) || again[2] != table || again[1] < count ||
        !read(rootAddress, rootAgain, sizeof rootAgain) ||
        rootAgain[0] != root[0] || rootAgain[1] != root[1]) return {};
    return {audio[0], Resolution::DirectReference};
}
