#include "VoiceIdentity.hpp"

#include <algorithm>
#include <cctype>

namespace mwmp
{
    namespace
    {
        float unit(std::uint64_t value, unsigned int shift)
        {
            return static_cast<float>((value >> shift) & 0xFFFFULL) / 65535.f;
        }

        float clamp(float value, float minimum, float maximum)
        {
            return std::max(minimum, std::min(maximum, value));
        }
    }

    NicknameVoiceProfile VoiceIdentity::fromNickname(const std::string& nickname)
    {
        // FNV-1a over UTF-8 bytes. ASCII is folded to lower case so simple case-only
        // nickname changes do not create a completely different voice.
        std::uint64_t hash = 1469598103934665603ULL;
        for (unsigned char byte : nickname)
        {
            if (byte < 0x80)
                byte = static_cast<unsigned char>(std::tolower(byte));
            hash ^= byte;
            hash *= 1099511628211ULL;
        }
        if (nickname.empty())
            hash ^= 0x9E3779B97F4A7C15ULL;

        NicknameVoiceProfile profile;
        profile.numericValue = hash;
        profile.type = static_cast<int>(hash % 5ULL);
        profile.lengthScale = 0.95f + unit(hash, 8) * 0.12f;
        profile.noiseScale = 0.90f + unit(hash, 24) * 0.20f;
        profile.noiseWScale = 0.92f + unit(hash, 40) * 0.16f;
        profile.bass = -0.04f + unit(hash, 12) * 0.13f;
        profile.brightness = -0.07f + unit(hash, 28) * 0.14f;
        profile.saturation = 0.01f + unit(hash, 44) * 0.045f;

        switch (profile.type)
        {
            case 0: // deep
                profile.lengthScale *= 1.045f;
                profile.bass += 0.11f;
                profile.brightness -= 0.045f;
                break;
            case 1: // warm
                profile.lengthScale *= 1.02f;
                profile.bass += 0.065f;
                profile.brightness -= 0.015f;
                break;
            case 2: // neutral
                break;
            case 3: // bright
                profile.lengthScale *= 0.965f;
                profile.brightness += 0.10f;
                profile.bass -= 0.025f;
                break;
            case 4: // rough
                profile.lengthScale *= 1.015f;
                profile.bass += 0.035f;
                profile.brightness -= 0.03f;
                profile.saturation += 0.07f;
                profile.noiseScale *= 1.06f;
                break;
        }

        profile.lengthScale = clamp(profile.lengthScale, 0.92f, 1.12f);
        profile.noiseScale = clamp(profile.noiseScale, 0.82f, 1.18f);
        profile.noiseWScale = clamp(profile.noiseWScale, 0.86f, 1.14f);
        profile.bass = clamp(profile.bass, -0.08f, 0.24f);
        profile.brightness = clamp(profile.brightness, -0.12f, 0.16f);
        profile.saturation = clamp(profile.saturation, 0.f, 0.13f);
        return profile;
    }

    const char* VoiceIdentity::typeName(int type)
    {
        switch (type)
        {
            case 0: return "deep";
            case 1: return "warm";
            case 2: return "neutral";
            case 3: return "bright";
            case 4: return "rough";
            default: return "unknown";
        }
    }
}
