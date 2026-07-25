#ifndef OPENMW_MWMP_CHAT_TTS_MANAGER_HPP
#define OPENMW_MWMP_CHAT_TTS_MANAGER_HPP

#include "LanguageDetector.hpp"
#include "PiperApi.hpp"

#include <RakNetTypes.h>

#include <condition_variable>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace mwmp
{
    class BasePlayer;

    class ChatTtsManager
    {
    public:
        ChatTtsManager();
        ~ChatTtsManager();

        void initialize();
        void shutdown();
        void enqueue(const BasePlayer& player);
        void update();
        bool isEnabled() const;

    private:
        struct Request
        {
            RakNet::RakNetGUID guid;
            std::string speakerName;
            std::string race;
            bool female;
            std::string text;
        };

        struct CompletedSpeech
        {
            RakNet::RakNetGUID guid;
            std::string name;
            std::shared_ptr<std::vector<float> > samples;
            int sampleRate;
        };

        struct VoiceChoice
        {
            std::string model;
            std::string config;
        };

        void workerLoop();
        bool loadPiper();
        bool synthesize(const Request& request, CompletedSpeech& completed);
        bool synthesizeSegment(const Request& request, const LanguageSegment& segment,
            std::vector<float>& output, int& outputSampleRate);
        PiperSynthesizer* getSynthesizer(const VoiceChoice& voice);
        VoiceChoice chooseVoice(TtsLanguage language, bool female) const;
        float raceLengthScale(const Request& request) const;
        float stableVariation(const Request& request) const;
        void applyRaceFilter(const Request& request, std::vector<float>& samples, int sampleRate) const;
        std::string sanitize(const std::string& text) const;
        std::string resolveResourcePath(const std::string& path) const;
        static void resampleAndAppend(const std::vector<float>& input, int inputRate,
            std::vector<float>& output, int outputRate);

        bool mEnabled;
        bool mStopping;
        bool mPiperLoadAttempted;
        bool mReadOwnMessages;
        bool mSkipUrls;
        bool mSkipCommands;
        bool mUseRaceProfiles;
        int mMaximumMessageLength;
        int mMaximumQueueSize;
        float mVolume;

        std::string mResourceDirectory;
        std::string mLibraryPath;
        std::string mEspeakDataPath;
        VoiceChoice mRussianMale;
        VoiceChoice mRussianFemale;
        VoiceChoice mEnglishMale;
        VoiceChoice mEnglishFemale;

        PiperApi mPiper;
        std::map<std::string, PiperSynthesizer*> mSynthesizers;

        std::thread mWorker;
        mutable std::mutex mMutex;
        std::condition_variable mCondition;
        std::deque<Request> mRequests;
        std::deque<CompletedSpeech> mCompleted;
        std::deque<CompletedSpeech> mPlaybackQueue;
    };
}

#endif
