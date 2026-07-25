#include "LanguageDetector.hpp"

#include <cctype>
#include <cstdint>

namespace mwmp
{
    namespace
    {
        bool nextCodepoint(const std::string& text, std::size_t& offset, std::uint32_t& codepoint)
        {
            if (offset >= text.size())
                return false;

            const unsigned char first = static_cast<unsigned char>(text[offset++]);
            if (first < 0x80)
            {
                codepoint = first;
                return true;
            }

            int continuationCount = 0;
            std::uint32_t value = 0;
            if ((first & 0xE0) == 0xC0)
            {
                continuationCount = 1;
                value = first & 0x1F;
            }
            else if ((first & 0xF0) == 0xE0)
            {
                continuationCount = 2;
                value = first & 0x0F;
            }
            else if ((first & 0xF8) == 0xF0)
            {
                continuationCount = 3;
                value = first & 0x07;
            }
            else
            {
                codepoint = 0xFFFD;
                return true;
            }

            for (int i = 0; i < continuationCount; ++i)
            {
                if (offset >= text.size())
                {
                    codepoint = 0xFFFD;
                    return true;
                }
                const unsigned char next = static_cast<unsigned char>(text[offset++]);
                if ((next & 0xC0) != 0x80)
                {
                    codepoint = 0xFFFD;
                    return true;
                }
                value = (value << 6) | (next & 0x3F);
            }
            codepoint = value;
            return true;
        }

        void countScripts(const std::string& text, std::size_t& cyrillic, std::size_t& latin)
        {
            cyrillic = 0;
            latin = 0;
            std::size_t offset = 0;
            std::uint32_t cp = 0;
            while (nextCodepoint(text, offset, cp))
            {
                if ((cp >= 0x0400 && cp <= 0x052F)
                    || (cp >= 0x2DE0 && cp <= 0x2DFF)
                    || (cp >= 0xA640 && cp <= 0xA69F))
                    ++cyrillic;
                else if ((cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z'))
                    ++latin;
            }
        }
    }

    TtsLanguage LanguageDetector::detect(const std::string& utf8, TtsLanguage fallback)
    {
        std::size_t cyrillic = 0;
        std::size_t latin = 0;
        countScripts(utf8, cyrillic, latin);
        if (cyrillic == 0 && latin == 0)
            return fallback;
        return cyrillic >= latin ? TtsLanguage::Russian : TtsLanguage::English;
    }

    std::vector<LanguageSegment> LanguageDetector::split(const std::string& utf8, TtsLanguage fallback)
    {
        std::vector<LanguageSegment> result;
        std::size_t begin = 0;
        TtsLanguage previous = fallback;

        while (begin < utf8.size())
        {
            std::size_t end = begin;
            while (end < utf8.size() && !std::isspace(static_cast<unsigned char>(utf8[end])))
                ++end;
            while (end < utf8.size() && std::isspace(static_cast<unsigned char>(utf8[end])))
                ++end;

            const std::string token = utf8.substr(begin, end - begin);
            const TtsLanguage language = detect(token, previous);
            if (!result.empty() && result.back().language == language)
                result.back().text += token;
            else
                result.push_back(LanguageSegment{language, token});

            previous = language;
            begin = end;
        }

        if (result.empty() && !utf8.empty())
            result.push_back(LanguageSegment{fallback, utf8});
        return result;
    }
}
