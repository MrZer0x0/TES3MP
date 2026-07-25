#include "ChatMessageParser.hpp"

#include <cctype>

namespace mwmp
{
    namespace
    {
        std::string trim(const std::string& value)
        {
            std::size_t first = 0;
            while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first])))
                ++first;
            std::size_t last = value.size();
            while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1])))
                --last;
            return value.substr(first, last - first);
        }

        bool isPid(const std::string& value, std::size_t begin, std::size_t end)
        {
            if (begin >= end)
                return false;
            for (std::size_t i = begin; i < end; ++i)
            {
                if (!std::isdigit(static_cast<unsigned char>(value[i])))
                    return false;
            }
            return true;
        }
    }

    bool ChatMessageParser::parse(const std::string& text, const std::string& expectedSpeaker,
        ParsedPlayerChatMessage& parsed)
    {
        parsed = ParsedPlayerChatMessage();
        const std::string cleaned = trim(text);
        if (cleaned.empty())
            return false;

        const std::size_t colon = cleaned.find(':');
        if (colon == std::string::npos || colon + 1 >= cleaned.size())
            return false;

        const std::string prefix = trim(cleaned.substr(0, colon));
        const std::size_t close = prefix.size();
        if (close < 4 || prefix[close - 1] != ')')
            return false;

        const std::size_t open = prefix.rfind(" (");
        if (open == std::string::npos || !isPid(prefix, open + 2, close - 1))
            return false;

        const std::string speaker = trim(prefix.substr(0, open));
        if (speaker.empty())
            return false;
        if (!expectedSpeaker.empty() && speaker != expectedSpeaker)
            return false;

        const std::string body = trim(cleaned.substr(colon + 1));
        if (body.empty())
            return false;

        parsed.speakerName = speaker;
        parsed.text = body;
        return true;
    }
}
