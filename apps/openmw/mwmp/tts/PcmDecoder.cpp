#include "PcmDecoder.hpp"

#include <algorithm>
#include <cstring>

namespace mwmp
{
    PcmDecoder::PcmDecoder(const std::shared_ptr<std::vector<float> >& samples, int sampleRate,
        const std::string& name)
        : MWSound::Sound_Decoder(nullptr)
        , mSamples(samples)
        , mSampleRate(sampleRate)
        , mName(name)
        , mByteOffset(0)
    {
    }

    void PcmDecoder::open(const std::string&)
    {
        mByteOffset = 0;
    }

    void PcmDecoder::close()
    {
        mByteOffset = 0;
    }

    std::string PcmDecoder::getName()
    {
        return mName;
    }

    void PcmDecoder::getInfo(int* sampleRate, MWSound::ChannelConfig* channels,
        MWSound::SampleType* sampleType)
    {
        if (sampleRate)
            *sampleRate = mSampleRate;
        if (channels)
            *channels = MWSound::ChannelConfig_Mono;
        if (sampleType)
            *sampleType = MWSound::SampleType_Float32;
    }

    std::size_t PcmDecoder::read(char* buffer, std::size_t bytes)
    {
        if (!mSamples || !buffer)
            return 0;
        const std::size_t totalBytes = mSamples->size() * sizeof(float);
        if (mByteOffset >= totalBytes)
            return 0;
        const std::size_t available = totalBytes - mByteOffset;
        const std::size_t copied = std::min(bytes, available);
        std::memcpy(buffer, reinterpret_cast<const char*>(mSamples->data()) + mByteOffset, copied);
        mByteOffset += copied;
        return copied;
    }

    std::size_t PcmDecoder::getSampleOffset()
    {
        return mByteOffset / sizeof(float);
    }
}
