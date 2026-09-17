#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <unordered_map>

struct SubtitleSegment {
    std::string text;
    double waitAfter = 0.0;
};

enum class SubtitlePriority : uint32_t {
    None = 0,
    Secondary = 1,
    Primary = 2,
};

struct SubtitleMatch {
    std::string raw;
    SubtitlePriority priority = SubtitlePriority::None;

    explicit operator bool() const { return !raw.empty(); }
};

bool shouldDisplaySubtitle(SubtitlePriority active, SubtitlePriority incoming);

class SubtitleEngine {
public:
    bool load(const std::string& filepath);
    bool loadSecondary(const std::string& filepath);

    std::string getRaw(uint32_t voiceId) const;
    SubtitleMatch getMatch(uint32_t voiceId) const;
    SubtitlePriority getPriority(uint32_t voiceId) const;

    std::vector<SubtitleSegment> getSegments(uint32_t voiceId) const;

    static std::vector<SubtitleSegment> parseSegments(const std::string& text);

    static std::string stripTags(const std::string& s);
    static double extractDuration(const std::string& s);

private:
    std::unordered_map<std::string, std::string> m_primaryDb;
    std::unordered_map<std::string, std::string> m_secondaryDb;

    static bool loadFile(const std::string& filepath,
                         std::unordered_map<std::string, std::string>& destination);

    static std::string makeKey(uint32_t voiceId);
};

extern SubtitleEngine g_SubtitleEngine;
