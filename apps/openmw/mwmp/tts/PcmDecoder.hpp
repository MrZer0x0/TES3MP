#ifndef OPENMW_MWMP_PCMDECODER_HPP
#define OPENMW_MWMP_PCMDECODER_HPP

#include "../../mwsound/sound_decoder.hpp"

#include <memory>
#include <string>
#include <vector>

namespace mwmp
{
    class PcmDecoder : public MWSound::Sound_Decoder
    {
    public:
        PcmDecoder(const std::shared_ptr<std::vector<float> >& samples, int sampleRate,
            const std::string& name);

        void open(const std::string& fileName) override;
        void close() override;
        std::string getName() override;
        void getInfo(int* sampleRate, MWSound::ChannelConfig* channels,
            MWSound::SampleType* sampleType) override;
        std::size_t read(char* buffer, std::size_t bytes) override;
        std::size_t getSampleOffset() override;

    private:
        std::shared_ptr<std::vector<float> > mSamples;
        int mSampleRate;
        std::string mName;
        std::size_t mByteOffset;
    };
}

#endif
