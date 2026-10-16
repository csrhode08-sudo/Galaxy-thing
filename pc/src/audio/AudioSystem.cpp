// Host side of the audio pipeline: SDL3 pulls audio, we run JAudio2's DSP sub-frame update and the
// CPU DSP mixer for each 80-sample sub frame. Adapted from Dusklight (CC0).

#include "pc/audio.h"

#include "Dsp.hpp"

#include "JSystem/JAudio2/JASAiCtrl.hpp"
#include "JSystem/JAudio2/JASAudioThread.hpp"
#include "JSystem/JAudio2/JASChannel.hpp"
#include "JSystem/JAudio2/JASCriticalSection.hpp"
#include "JSystem/JAudio2/JASDSPChannel.hpp"
#include "JSystem/JAudio2/JASDriverIF.hpp"
#include "JSystem/JAudio2/JASHeapCtrl.hpp"

#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_init.h>

#include <array>
#include <cmath>
#include <cstdio>
#include <mutex>
#include <span>

using namespace pc::audio;

// ---- JASCriticalSection --------------------------------------------------------------------------------------

static std::recursive_mutex& audioMutex() {
    static std::recursive_mutex m;
    return m;
}

JASCriticalSection::JASCriticalSection() {
    audioMutex().lock();
}

JASCriticalSection::~JASCriticalSection() {
    audioMutex().unlock();
}

// ---- Output ---------------------------------------------------------------------------------------------------

static OutputSubframe sOutBuffer;
static std::array< f32, DSP_SUBFRAME_SIZE * OutputSubframe::NUM_CHANNELS > sInterleaved;
static SDL_AudioStream* sStream;
static bool sInitialized;

static int renderSubframe();

static void SDLCALL getNewAudio(void*, SDL_AudioStream*, int needed, int) {
    while (needed > 0) {
        JASCriticalSection section;
        const u32 subFrames = JASDriver::getSubFrames();
        JASAudioThread::setDSPSyncCount(subFrames);
        for (u32 i = 0; i < subFrames; i++) {
            needed -= renderSubframe();
            JASAudioThread::snIntCount -= 1;
        }
    }
}

namespace {
    constexpr int kExtChannels = 2;
    constexpr int kExtSrcInSampleCount = DSP_SUBFRAME_SIZE * 2;
    constexpr int kExtSrcOutSampleCount = DSP_SUBFRAME_SIZE * 3;

    // 32 kHz -> 48 kHz for streams mixed in through JASDriver's external mix callback.
    struct LinearResampler {
        f32 prev = 0.0f;
        int phase = 0;

        void resample(std::span< const s16 > in, std::span< f32 > out, int skip, int offset) {
            const int inSize = in.size() / skip;
            const int outSize = out.size() / skip;
            int idx = -1;
            int phi = phase;
            int n = 0;
            while (n < outSize) {
                f32 s0 = (idx < 0) ? prev : in[idx * skip + offset] / 32768.0f;
                f32 s1 = in[idx * skip + skip + offset] / 32768.0f;
                f32 frac = phi * (1.0f / 3.0f);
                out[n++ * skip + offset] = s0 + frac * (s1 - s0);
                phi += 2;
                if (phi >= 3) {
                    phi -= 3;
                    idx++;
                }
            }
            prev = in[(inSize - 1) * skip + offset] / 32768.0f;
            phase = phi;
        }
    };

    struct ExtResampler {
        std::array< f32, kExtSrcOutSampleCount * kExtChannels > buf;
        int available = 0;
        LinearResampler left;
        LinearResampler right;

        bool hungry() const { return available <= 0; }

        void feed(std::span< const s16 > in) {
            std::span< f32 > out{buf};
            left.resample(in, out, kExtChannels, 0);
            right.resample(in, out, kExtChannels, 1);
            available = buf.size();
        }

        std::span< const f32 > drain() {
            constexpr int kDrain = DSP_SUBFRAME_SIZE * kExtChannels;
            std::span< const f32 > rv{&buf[buf.size() - available], kDrain};
            available -= kDrain;
            return rv;
        }
    } sExt;
}  // namespace

static int renderSubframe() {
    sOutBuffer = {};

    JASDriver::updateDSP();
    DspRender(sOutBuffer);

    std::span< f32 > out{sInterleaved.data(), static_cast< size_t >(DSP_SUBFRAME_SIZE * OutChannelCount)};
    size_t o = 0;
    for (int s = 0; s < DSP_SUBFRAME_SIZE; s++) {
        for (int c = 0; c < OutChannelCount; c++) {
            out[o++] = sOutBuffer.channels[c][s];
        }
    }

    if (JASDriver::extMixCallback != nullptr && JASDriver::sMixMode == MIX_MODE_INTERLEAVE) {
        if (sExt.hungry()) {
            const s16* mix = JASDriver::extMixCallback(kExtSrcInSampleCount);
            if (mix != nullptr) {
                sExt.feed(std::span< const s16 >{mix, static_cast< size_t >(kExtSrcInSampleCount * kExtChannels)});
            }
        }
        if (!sExt.hungry()) {
            auto in = sExt.drain();
            for (int i = 0; i < DSP_SUBFRAME_SIZE; i++) {
                out[i * OutChannelCount] += in[i * kExtChannels];
                out[i * OutChannelCount + 1] += in[i * kExtChannels + 1];
            }
        }
    }

    const int bytes = static_cast< int >(out.size_bytes());
    if (sStream != nullptr) {
        SDL_PutAudioStreamData(sStream, out.data(), bytes);
    }
    return bytes;
}

void pc::audio::Initialize() {
    if (sInitialized) {
        return;
    }
    sInitialized = true;

    // 48 kHz output: JAudio2 scales voice pitch and sequence tempo to match.
    JASDriver::setOutputRate(static_cast< JASOutputRate >(-1));

    OutChannelCount = 2;
    DspInit();
    JASDsp::initBuffer();
    JASDSPChannel::initAll();
    JASPoolAllocObject_MultiThreaded< JASChannel >::newMemPool(0x48);
    JASChannel::initBankDisposeMsgQueue();

    if (!SDL_WasInit(SDL_INIT_AUDIO) && !SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        std::fprintf(stderr, "[audio] SDL audio init failed: %s\n", SDL_GetError());
        return;
    }
    const SDL_AudioSpec spec = {SDL_AUDIO_F32, OutChannelCount, SampleRate};
    sStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, &getNewAudio, nullptr);
    if (sStream == nullptr) {
        std::fprintf(stderr, "[audio] no playback device: %s\n", SDL_GetError());
        return;
    }
    SDL_ResumeAudioStreamDevice(sStream);
}

void pc::audio::Shutdown() {
    if (sStream != nullptr) {
        SDL_DestroyAudioStream(sStream);
        sStream = nullptr;
    }
}

void pc::audio::SetMasterVolume(f32 value) {
    JASCriticalSection section;
    MasterVolume = value <= 0.0f ? 0.0f : std::pow(10.0f, (value - 1.0f) * 2.0f);
}

void pc::audio::SetEnableReverb(bool value) {
    JASCriticalSection section;
    EnableReverb = value;
}

void pc::audio::SetPaused(bool paused) {
    if (sStream == nullptr) {
        return;
    }
    if (paused) {
        SDL_PauseAudioStreamDevice(sStream);
    } else {
        SDL_ResumeAudioStreamDevice(sStream);
    }
}

f32 pc::audio::VolumeFromU16(u16 value) {
    return static_cast< f32 >(value) / static_cast< f32 >(JASDriver::getChannelLevel_dsp());
}
