#ifndef OPENMW_MWMP_PIPERAPI_HPP
#define OPENMW_MWMP_PIPERAPI_HPP

#include <cstddef>
#include <cstdint>
#include <string>

namespace mwmp
{
    struct PiperSynthesizer;

    struct PiperAudioChunk
    {
        const float* samples;
        std::size_t numSamples;
        int sampleRate;
        bool isLast;
        const char32_t* phonemes;
        std::size_t numPhonemes;
        const int* phonemeIds;
        std::size_t numPhonemeIds;
        const int* alignments;
        std::size_t numAlignments;
    };

    struct PiperSynthesizeOptions
    {
        int speakerId;
        float lengthScale;
        float noiseScale;
        float noiseWScale;
    };

    class PiperApi
    {
    public:
        PiperApi();
        ~PiperApi();

        bool load(const std::string& explicitPath, const std::string& resourceDirectory, std::string& error);
        void unload();
        bool isLoaded() const;

        PiperSynthesizer* create(const char* modelPath, const char* configPath, const char* espeakDataPath) const;
        void free(PiperSynthesizer* synthesizer) const;
        PiperSynthesizeOptions defaultOptions(PiperSynthesizer* synthesizer) const;
        int synthesizeStart(PiperSynthesizer* synthesizer, const char* text,
            const PiperSynthesizeOptions* options) const;
        int synthesizeNext(PiperSynthesizer* synthesizer, PiperAudioChunk* chunk) const;
        const char* version() const;

        static const int Ok = 0;
        static const int Done = 1;

    private:
        typedef PiperSynthesizer* (*CreateFunction)(const char*, const char*, const char*);
        typedef void (*FreeFunction)(PiperSynthesizer*);
        typedef PiperSynthesizeOptions (*DefaultOptionsFunction)(PiperSynthesizer*);
        typedef int (*SynthesizeStartFunction)(PiperSynthesizer*, const char*, const PiperSynthesizeOptions*);
        typedef int (*SynthesizeNextFunction)(PiperSynthesizer*, PiperAudioChunk*);
        typedef const char* (*VersionFunction)();

        void* mLibrary;
        CreateFunction mCreate;
        FreeFunction mFree;
        DefaultOptionsFunction mDefaultOptions;
        SynthesizeStartFunction mSynthesizeStart;
        SynthesizeNextFunction mSynthesizeNext;
        VersionFunction mVersion;
    };
}

#endif
