#ifndef OPENMW_MWMP_LANGUAGEDETECTOR_HPP
#define OPENMW_MWMP_LANGUAGEDETECTOR_HPP

#include <string>
#include <vector>

namespace mwmp
{
    enum class TtsLanguage
    {
        Russian,
        English
    };

    struct LanguageSegment
    {
        TtsLanguage language;
        std::string text;
    };

    class LanguageDetector
    {
    public:
        static TtsLanguage detect(const std::string& utf8, TtsLanguage fallback = TtsLanguage::Russian);
        static std::vector<LanguageSegment> split(const std::string& utf8,
            TtsLanguage fallback = TtsLanguage::Russian);
    };
}

#endif
