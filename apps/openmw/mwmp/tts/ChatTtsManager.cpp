#include "ChatTtsManager.hpp"

#include "PcmDecoder.hpp"

#include <components/openmw-mp/Base/BasePlayer.hpp>
#include <components/openmw-mp/TimedLog.hpp>
#include <components/settings/settings.hpp>

#include "../DedicatedPlayer.hpp"
#include "../LocalPlayer.hpp"
#include "../Main.hpp"
#include "../PlayerList.hpp"

#include "../../mwbase/environment.hpp"
#include "../../mwbase/soundmanager.hpp"

#include <boost/filesystem.hpp>

#include <algorithm>
#include <cmath>
#include <cctype>
#include <limits>
#include <sstream>
#include <utility>

namespace mwmp
{
    namespace
    {
        bool isHex(char value)
        {
            return (value >= '0' && value <= '9') || (value >= 'a' && value <= 'f')
                || (value >= 'A' && value <= 'F');
        }

        std::string lowerAscii(std::string value)
        {
            for (char& ch : value)
                ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            return value;
        }

        bool containsUrl(const std::string& token)
        {
            const std::string lower = lowerAscii(token);
            return lower.find("http://") != std::string::npos
                || lower.find("https://") != std::string::npos
                || lower.find("www.") != std::string::npos;
        }

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
    }

    ChatTtsManager::ChatTtsManager()
        : mEnabled(false)
        , mStopping(false)
        , mPiperLoadAttempted(false)
        , mReadOwnMessages(false)
        , mSkipUrls(true)
        , mSkipCommands(true)
        , mUseRaceProfiles(true)
        , mMaximumMessageLength(240)
        , mMaximumQueueSize(4)
        , mVolume(0.8f)
    {
    }

    ChatTtsManager::~ChatTtsManager()
    {
        shutdown();
    }

    void ChatTtsManager::initialize()
    {
        shutdown();

        mEnabled = Settings::Manager::getBool("enabled", "Chat TTS");
        if (!mEnabled)
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Chat TTS is disabled");
            return;
        }

        mReadOwnMessages = Settings::Manager::getBool("read own messages", "Chat TTS");
        mSkipUrls = Settings::Manager::getBool("skip urls", "Chat TTS");
        mSkipCommands = Settings::Manager::getBool("skip commands", "Chat TTS");
        mUseRaceProfiles = Settings::Manager::getBool("race voice profiles", "Chat TTS");
        mMaximumMessageLength = std::max(16, Settings::Manager::getInt("maximum message length", "Chat TTS"));
        mMaximumQueueSize = std::max(1, Settings::Manager::getInt("maximum queue size", "Chat TTS"));
        mVolume = std::max(0.f, std::min(1.f, Settings::Manager::getFloat("volume", "Chat TTS")));

        mResourceDirectory = Main::getResDir();
        mLibraryPath = resolveResourcePath(Settings::Manager::getString("library path", "Chat TTS"));
        mEspeakDataPath = resolveResourcePath(Settings::Manager::getString("espeak data path", "Chat TTS"));

        mRussianMale.model = resolveResourcePath(Settings::Manager::getString("russian male model", "Chat TTS"));
        mRussianFemale.model = resolveResourcePath(Settings::Manager::getString("russian female model", "Chat TTS"));
        mEnglishMale.model = resolveResourcePath(Settings::Manager::getString("english male model", "Chat TTS"));
        mEnglishFemale.model = resolveResourcePath(Settings::Manager::getString("english female model", "Chat TTS"));

        mRussianMale.config = mRussianMale.model + ".json";
        mRussianFemale.config = mRussianFemale.model + ".json";
        mEnglishMale.config = mEnglishMale.model + ".json";
        mEnglishFemale.config = mEnglishFemale.model + ".json";

        mStopping = false;
        mPiperLoadAttempted = false;
        mWorker = std::thread(&ChatTtsManager::workerLoop, this);
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Chat TTS worker started");
    }

    void ChatTtsManager::shutdown()
    {
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mStopping = true;
            mRequests.clear();
            mCompleted.clear();
            mPlaybackQueue.clear();
        }
        mCondition.notify_all();
        if (mWorker.joinable())
            mWorker.join();

        for (const auto& entry : mSynthesizers)
            mPiper.free(entry.second);
        mSynthesizers.clear();
        mPiper.unload();
        mPiperLoadAttempted = false;
        mEnabled = false;
    }

    bool ChatTtsManager::isEnabled() const
    {
        return mEnabled;
    }

    void ChatTtsManager::enqueue(const BasePlayer& player)
    {
        if (!mEnabled)
            return;
        if (!mReadOwnMessages && Main::get().getLocalPlayer()
            && player.guid == Main::get().getLocalPlayer()->guid)
            return;

        std::string text = sanitize(player.chatMessage);
        if (!player.npc.mName.empty())
        {
            const std::string prefix = player.npc.mName + ":";
            if (text.compare(0, prefix.size(), prefix) == 0)
                text = trim(text.substr(prefix.size()));
        }
        if (text.empty())
            return;

        Request request;
        request.guid = player.guid;
        request.speakerName = player.npc.mName;
        request.race = player.npc.mRace;
        request.female = !player.npc.isMale();
        request.text = text;

        {
            std::lock_guard<std::mutex> lock(mMutex);
            while (static_cast<int>(mRequests.size()) >= mMaximumQueueSize)
                mRequests.pop_front();
            mRequests.push_back(request);
        }
        mCondition.notify_one();
    }

    void ChatTtsManager::update()
    {
        if (!mEnabled)
            return;

        {
            std::lock_guard<std::mutex> lock(mMutex);
            while (!mCompleted.empty())
            {
                while (static_cast<int>(mPlaybackQueue.size()) >= mMaximumQueueSize)
                    mPlaybackQueue.pop_front();
                mPlaybackQueue.push_back(std::move(mCompleted.front()));
                mCompleted.pop_front();
            }
        }

        MWBase::SoundManager* soundManager = MWBase::Environment::get().getSoundManager();
        if (!soundManager)
            return;

        for (auto it = mPlaybackQueue.begin(); it != mPlaybackQueue.end();)
        {
            MWWorld::Ptr actor;
            if (Main::get().getLocalPlayer() && it->guid == Main::get().getLocalPlayer()->guid)
                actor = Main::get().getLocalPlayer()->getPlayerPtr();
            else
            {
                DedicatedPlayer* player = PlayerList::getPlayer(it->guid);
                if (player)
                    actor = player->getPtr();
            }

            if (actor.mRef == nullptr)
            {
                it = mPlaybackQueue.erase(it);
                continue;
            }

            if (soundManager->sayActive(actor))
            {
                ++it;
                continue;
            }

            MWSound::DecoderPtr decoder(new PcmDecoder(it->samples, it->sampleRate, it->name));
            soundManager->say(actor, decoder);
            it = mPlaybackQueue.erase(it);
        }
    }

    void ChatTtsManager::workerLoop()
    {
        for (;;)
        {
            Request request;
            {
                std::unique_lock<std::mutex> lock(mMutex);
                mCondition.wait(lock, [this]() { return mStopping || !mRequests.empty(); });
                if (mStopping)
                    break;
                request = std::move(mRequests.front());
                mRequests.pop_front();
            }

            CompletedSpeech completed;
            if (synthesize(request, completed))
            {
                std::lock_guard<std::mutex> lock(mMutex);
                if (!mStopping)
                {
                    while (static_cast<int>(mCompleted.size()) >= mMaximumQueueSize)
                        mCompleted.pop_front();
                    mCompleted.push_back(std::move(completed));
                }
            }
        }
    }

    bool ChatTtsManager::loadPiper()
    {
        if (mPiper.isLoaded())
            return true;
        if (mPiperLoadAttempted)
            return false;
        mPiperLoadAttempted = true;

        std::string error;
        if (!mPiper.load(mLibraryPath, mResourceDirectory, error))
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR,
                "Chat TTS could not load libpiper: %s", error.c_str());
            return false;
        }

        LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Chat TTS loaded libpiper %s", mPiper.version());
        return true;
    }

    bool ChatTtsManager::synthesize(const Request& request, CompletedSpeech& completed)
    {
        if (!loadPiper())
            return false;

        std::shared_ptr<std::vector<float> > samples(new std::vector<float>());
        int sampleRate = 0;
        const std::vector<LanguageSegment> segments = LanguageDetector::split(request.text);
        for (std::size_t index = 0; index < segments.size(); ++index)
        {
            if (!synthesizeSegment(request, segments[index], *samples, sampleRate))
                return false;
            if (index + 1 < segments.size() && sampleRate > 0)
                samples->insert(samples->end(), static_cast<std::size_t>(sampleRate * 0.055f), 0.f);
        }

        if (samples->empty() || sampleRate <= 0)
            return false;

        completed.guid = request.guid;
        completed.name = std::string("Chat TTS: ") + request.speakerName;
        completed.samples = samples;
        completed.sampleRate = sampleRate;
        return true;
    }

    bool ChatTtsManager::synthesizeSegment(const Request& request, const LanguageSegment& segment,
        std::vector<float>& output, int& outputSampleRate)
    {
        VoiceChoice voice = chooseVoice(segment.language, request.female);
        PiperSynthesizer* synthesizer = getSynthesizer(voice);
        if (!synthesizer)
            return false;

        PiperSynthesizeOptions options = mPiper.defaultOptions(synthesizer);
        options.lengthScale *= raceLengthScale(request) * stableVariation(request);

        if (mPiper.synthesizeStart(synthesizer, segment.text.c_str(), &options) != PiperApi::Ok)
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Chat TTS failed to start synthesis");
            return false;
        }

        std::vector<float> segmentSamples;
        int segmentRate = 0;
        for (;;)
        {
            PiperAudioChunk chunk = {};
            const int result = mPiper.synthesizeNext(synthesizer, &chunk);
            if (result == PiperApi::Done)
                break;
            if (result != PiperApi::Ok)
            {
                LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Chat TTS synthesis failed with code %i", result);
                return false;
            }
            if (chunk.samples && chunk.numSamples > 0)
            {
                segmentRate = chunk.sampleRate;
                const std::size_t oldSize = segmentSamples.size();
                segmentSamples.resize(oldSize + chunk.numSamples);
                for (std::size_t i = 0; i < chunk.numSamples; ++i)
                    segmentSamples[oldSize + i] = std::max(-1.f,
                        std::min(1.f, chunk.samples[i] * mVolume));
            }
            if (chunk.isLast)
                break;
        }

        if (segmentSamples.empty() || segmentRate <= 0)
            return false;
        applyRaceFilter(request, segmentSamples, segmentRate);
        if (outputSampleRate == 0)
            outputSampleRate = segmentRate;
        resampleAndAppend(segmentSamples, segmentRate, output, outputSampleRate);
        return true;
    }

    PiperSynthesizer* ChatTtsManager::getSynthesizer(const VoiceChoice& requestedVoice)
    {
        VoiceChoice voice = requestedVoice;
        if (!boost::filesystem::exists(voice.model))
        {
            const bool russian = voice.model == mRussianFemale.model;
            const bool english = voice.model == mEnglishFemale.model;
            if (russian && boost::filesystem::exists(mRussianMale.model))
                voice = mRussianMale;
            else if (english && boost::filesystem::exists(mEnglishMale.model))
                voice = mEnglishMale;
        }

        const auto found = mSynthesizers.find(voice.model);
        if (found != mSynthesizers.end())
            return found->second;

        if (!boost::filesystem::exists(voice.model))
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR,
                "Chat TTS model is missing: %s", voice.model.c_str());
            return nullptr;
        }
        const char* config = boost::filesystem::exists(voice.config) ? voice.config.c_str() : nullptr;
        PiperSynthesizer* synthesizer = mPiper.create(voice.model.c_str(), config, mEspeakDataPath.c_str());
        if (!synthesizer)
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR,
                "Chat TTS could not load model: %s", voice.model.c_str());
            return nullptr;
        }
        mSynthesizers[voice.model] = synthesizer;
        return synthesizer;
    }

    ChatTtsManager::VoiceChoice ChatTtsManager::chooseVoice(TtsLanguage language, bool female) const
    {
        if (language == TtsLanguage::Russian)
            return female ? mRussianFemale : mRussianMale;
        return female ? mEnglishFemale : mEnglishMale;
    }

    float ChatTtsManager::raceLengthScale(const Request& request) const
    {
        if (!mUseRaceProfiles)
            return 1.f;
        const std::string race = lowerAscii(request.race);
        if (race.find("orc") != std::string::npos)
            return request.female ? 1.06f : 1.11f;
        if (race.find("nord") != std::string::npos)
            return request.female ? 1.03f : 1.07f;
        if (race.find("dark elf") != std::string::npos || race.find("dunmer") != std::string::npos)
            return 1.05f;
        if (race.find("high elf") != std::string::npos || race.find("altmer") != std::string::npos)
            return 1.04f;
        if (race.find("wood elf") != std::string::npos || race.find("bosmer") != std::string::npos)
            return 0.97f;
        if (race.find("khajiit") != std::string::npos)
            return 0.99f;
        if (race.find("argonian") != std::string::npos)
            return 1.02f;
        return 1.f;
    }

    float ChatTtsManager::stableVariation(const Request& request) const
    {
        const std::string identity = request.guid.ToString();
        std::uint64_t hash = 1469598103934665603ULL;
        for (unsigned char ch : identity)
        {
            hash ^= ch;
            hash *= 1099511628211ULL;
        }
        const float normalized = static_cast<float>(hash % 1001ULL) / 1000.f;
        return 0.985f + normalized * 0.03f;
    }

    void ChatTtsManager::applyRaceFilter(const Request& request, std::vector<float>& samples, int sampleRate) const
    {
        if (!mUseRaceProfiles || samples.empty() || sampleRate <= 0)
            return;

        const std::string race = lowerAscii(request.race);
        float bass = 0.f;
        float brightness = 0.f;
        float saturation = 0.f;

        if (race.find("orc") != std::string::npos)
        {
            bass = 0.24f;
            brightness = -0.10f;
            saturation = 0.10f;
        }
        else if (race.find("nord") != std::string::npos)
            bass = 0.15f;
        else if (race.find("dark elf") != std::string::npos || race.find("dunmer") != std::string::npos)
        {
            bass = 0.08f;
            brightness = -0.06f;
            saturation = 0.055f;
        }
        else if (race.find("high elf") != std::string::npos || race.find("altmer") != std::string::npos)
            brightness = 0.12f;
        else if (race.find("wood elf") != std::string::npos || race.find("bosmer") != std::string::npos)
            brightness = 0.08f;
        else if (race.find("khajiit") != std::string::npos)
        {
            brightness = -0.025f;
            saturation = 0.045f;
        }
        else if (race.find("argonian") != std::string::npos)
        {
            brightness = 0.055f;
            saturation = 0.035f;
        }

        if (bass == 0.f && brightness == 0.f && saturation == 0.f)
            return;

        const float cutoff = 420.f;
        const float alpha = 1.f - std::exp(-6.28318530718f * cutoff / static_cast<float>(sampleRate));
        float low = 0.f;
        for (float& sample : samples)
        {
            low += alpha * (sample - low);
            const float high = sample - low;
            float processed = sample + bass * low + brightness * high;
            if (saturation > 0.f)
                processed = std::tanh(processed * (1.f + saturation * 2.f)) / std::tanh(1.f + saturation * 2.f);
            sample = std::max(-1.f, std::min(1.f, processed));
        }
    }

    std::string ChatTtsManager::sanitize(const std::string& text) const
    {
        std::string cleaned;
        cleaned.reserve(text.size());
        for (std::size_t i = 0; i < text.size(); ++i)
        {
            if (text[i] == '#' && i + 6 < text.size())
            {
                bool color = true;
                for (std::size_t j = 1; j <= 6; ++j)
                    color = color && isHex(text[i + j]);
                if (color)
                {
                    i += 6;
                    continue;
                }
            }
            const char ch = text[i];
            if (ch == '\r' || ch == '\n' || ch == '\t')
                cleaned.push_back(' ');
            else if (static_cast<unsigned char>(ch) >= 0x20)
                cleaned.push_back(ch);
        }
        cleaned = trim(cleaned);
        if (cleaned.empty())
            return cleaned;
        if (mSkipCommands && cleaned[0] == '/')
            return std::string();

        if (mSkipUrls)
        {
            std::istringstream input(cleaned);
            std::ostringstream output;
            std::string token;
            bool first = true;
            while (input >> token)
            {
                if (containsUrl(token))
                    continue;
                if (!first)
                    output << ' ';
                output << token;
                first = false;
            }
            cleaned = output.str();
        }

        if (static_cast<int>(cleaned.size()) > mMaximumMessageLength)
        {
            cleaned.resize(static_cast<std::size_t>(mMaximumMessageLength));
            while (!cleaned.empty()
                && (static_cast<unsigned char>(cleaned.back()) & 0xC0) == 0x80)
                cleaned.pop_back();
        }
        return trim(cleaned);
    }

    std::string ChatTtsManager::resolveResourcePath(const std::string& path) const
    {
        if (path.empty())
            return path;
        boost::filesystem::path value(path);
        if (value.is_complete())
            return value.string();
        return (boost::filesystem::path(Main::getResDir()) / value).string();
    }

    void ChatTtsManager::resampleAndAppend(const std::vector<float>& input, int inputRate,
        std::vector<float>& output, int outputRate)
    {
        if (input.empty() || inputRate <= 0 || outputRate <= 0)
            return;
        if (inputRate == outputRate)
        {
            output.insert(output.end(), input.begin(), input.end());
            return;
        }

        const double ratio = static_cast<double>(outputRate) / static_cast<double>(inputRate);
        const std::size_t count = static_cast<std::size_t>(std::ceil(input.size() * ratio));
        const std::size_t oldSize = output.size();
        output.resize(oldSize + count);
        for (std::size_t i = 0; i < count; ++i)
        {
            const double sourcePosition = static_cast<double>(i) / ratio;
            const std::size_t left = static_cast<std::size_t>(sourcePosition);
            const std::size_t right = std::min(left + 1, input.size() - 1);
            const float fraction = static_cast<float>(sourcePosition - left);
            output[oldSize + i] = input[left] + (input[right] - input[left]) * fraction;
        }
    }
}
