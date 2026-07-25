#include "PiperApi.hpp"

#include <SDL_loadso.h>

#include <algorithm>
#include <vector>

namespace mwmp
{
    namespace
    {
        std::string joinPath(const std::string& left, const std::string& right)
        {
            if (left.empty())
                return right;
            const char last = left[left.size() - 1];
            if (last == '/' || last == '\\')
                return left + right;
            return left + "/" + right;
        }
    }

    PiperApi::PiperApi()
        : mLibrary(nullptr)
        , mOnnxRuntimeLibrary(nullptr)
        , mCreate(nullptr)
        , mFree(nullptr)
        , mDefaultOptions(nullptr)
        , mSynthesizeStart(nullptr)
        , mSynthesizeNext(nullptr)
        , mVersion(nullptr)
    {
    }

    PiperApi::~PiperApi()
    {
        unload();
    }

    bool PiperApi::load(const std::string& explicitPath, const std::string& resourceDirectory, std::string& error)
    {
        unload();

        std::vector<std::string> candidates;
        if (!explicitPath.empty())
            candidates.push_back(explicitPath);

#if defined(_WIN32)
        const char* libraryName = "piper.dll";
#elif defined(__APPLE__)
        const char* libraryName = "libpiper.dylib";
#else
        const char* libraryName = "libpiper.so";
#endif

        candidates.push_back(joinPath(joinPath(resourceDirectory, "tts/runtime"), libraryName));
        candidates.push_back(libraryName);

        for (const std::string& candidate : candidates)
        {
#if defined(_WIN32)
            // SDL_LoadObject uses LoadLibrary. A dependency located beside a
            // DLL in a subdirectory is not reliably searched by Windows, so
            // preload ONNX Runtime by its absolute adjacent path first.
            const std::string::size_type separator = candidate.find_last_of("/\\");
            if (separator != std::string::npos)
            {
                const std::string onnxPath = candidate.substr(0, separator + 1) + "onnxruntime.dll";
                mOnnxRuntimeLibrary = SDL_LoadObject(onnxPath.c_str());
            }
#endif
            mLibrary = SDL_LoadObject(candidate.c_str());
            if (mLibrary)
                break;

            if (mOnnxRuntimeLibrary)
            {
                SDL_UnloadObject(mOnnxRuntimeLibrary);
                mOnnxRuntimeLibrary = nullptr;
            }
            const char* sdlError = SDL_GetError();
            if (sdlError)
                error = candidate + ": " + sdlError;
        }

        if (!mLibrary)
            return false;

#define LOAD_PIPER_SYMBOL(member, name) \
        member = reinterpret_cast<decltype(member)>(SDL_LoadFunction(mLibrary, name)); \
        if (!member) { error = std::string("Missing libpiper symbol: ") + name; unload(); return false; }

        LOAD_PIPER_SYMBOL(mCreate, "piper_create");
        LOAD_PIPER_SYMBOL(mFree, "piper_free");
        LOAD_PIPER_SYMBOL(mDefaultOptions, "piper_default_synthesize_options");
        LOAD_PIPER_SYMBOL(mSynthesizeStart, "piper_synthesize_start");
        LOAD_PIPER_SYMBOL(mSynthesizeNext, "piper_synthesize_next");
        LOAD_PIPER_SYMBOL(mVersion, "piper_version");
#undef LOAD_PIPER_SYMBOL

        return true;
    }

    void PiperApi::unload()
    {
        mCreate = nullptr;
        mFree = nullptr;
        mDefaultOptions = nullptr;
        mSynthesizeStart = nullptr;
        mSynthesizeNext = nullptr;
        mVersion = nullptr;
        if (mLibrary)
        {
            SDL_UnloadObject(mLibrary);
            mLibrary = nullptr;
        }
        if (mOnnxRuntimeLibrary)
        {
            SDL_UnloadObject(mOnnxRuntimeLibrary);
            mOnnxRuntimeLibrary = nullptr;
        }
    }

    bool PiperApi::isLoaded() const
    {
        return mLibrary != nullptr;
    }

    PiperSynthesizer* PiperApi::create(const char* modelPath, const char* configPath,
        const char* espeakDataPath) const
    {
        return mCreate ? mCreate(modelPath, configPath, espeakDataPath) : nullptr;
    }

    void PiperApi::free(PiperSynthesizer* synthesizer) const
    {
        if (mFree && synthesizer)
            mFree(synthesizer);
    }

    PiperSynthesizeOptions PiperApi::defaultOptions(PiperSynthesizer* synthesizer) const
    {
        PiperSynthesizeOptions options = {0, 1.f, 0.667f, 0.8f};
        return mDefaultOptions ? mDefaultOptions(synthesizer) : options;
    }

    int PiperApi::synthesizeStart(PiperSynthesizer* synthesizer, const char* text,
        const PiperSynthesizeOptions* options) const
    {
        return mSynthesizeStart ? mSynthesizeStart(synthesizer, text, options) : -1;
    }

    int PiperApi::synthesizeNext(PiperSynthesizer* synthesizer, PiperAudioChunk* chunk) const
    {
        return mSynthesizeNext ? mSynthesizeNext(synthesizer, chunk) : -1;
    }

    const char* PiperApi::version() const
    {
        return mVersion ? mVersion() : "unknown";
    }
}
