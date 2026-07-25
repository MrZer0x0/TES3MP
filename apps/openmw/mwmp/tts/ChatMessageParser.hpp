#ifndef OPENMW_MWMP_CHAT_MESSAGE_PARSER_HPP
#define OPENMW_MWMP_CHAT_MESSAGE_PARSER_HPP

#include <string>

namespace mwmp
{
    struct ParsedPlayerChatMessage
    {
        std::string speakerName;
        std::string text;
    };

    class ChatMessageParser
    {
    public:
        // Parses server-formatted player chat such as "Name (12): message".
        // System messages are intentionally rejected.
        static bool parse(const std::string& text, const std::string& expectedSpeaker,
            ParsedPlayerChatMessage& parsed);
    };
}

#endif
