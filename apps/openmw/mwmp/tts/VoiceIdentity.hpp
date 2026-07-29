#ifndef OPENMW_MWMP_VOICE_IDENTITY_HPP
#define OPENMW_MWMP_VOICE_IDENTITY_HPP

#include <cstdint>
#include <string>

namespace mwmp
{
    struct NicknameVoiceProfile
    {
        std::uint64_t numericValue;
        int type;
        float lengthScale;
        float noiseScale;
        float noiseWScale;
        float bass;
        float brightness;
        float saturation;
    };

    class VoiceIdentity
    {
    public:
        // Produces the same profile for the same nickname on every client and restart.
        static NicknameVoiceProfile fromNickname(const std::string& nickname);
        static const char* typeName(int type);
    };
}

#endif
