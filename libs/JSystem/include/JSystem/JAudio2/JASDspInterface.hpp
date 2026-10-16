#pragma once

#include <cstddef>
#include <revolution/dsp.h>

struct JASWaveInfo;

/// Number of voices the DSP mixes.
#define DSP_CHANNELS 64
/// Output channels the DSP calculates volumes for (5.1).
#define DSP_OUTPUT_CHANNELS 6
/// Samples the DSP renders per sub frame.
#define DSP_SUBFRAME_SIZE 0x50

namespace JASDsp {
    struct FxlineConfig_ {
        u8 _0;
        u8 _1;
        u16 _2;
        s16 _4;
        u16 _6;
        s16 _8;
        u16 _A;
        u32 _C;
        s16 _10[8];
    };

    typedef struct {
        u16 _0;
        u16 _2;
        s16* _4;
        u16 _8;
        s16 _A;
        u16 _C;
        s16 _E;
        u16 _10[8];
    } FxBuf;

    struct OutputChannelConfig {
        u16 mBusConnect;
        u16 mTargetVolume;
        u16 mCurrentVolume;
        u16 mVolumeProgress;
    };

    struct TChannel {
        void init();
        void playStart();
        void playStop();
        void replyFinishRequest();
        void forceStop();
        bool isActive() const;
        bool isFinish() const;
        void setWaveInfo(JASWaveInfo const&, u32, u32);
        void setOscInfo(u32);
        void initAutoMixer();
        void setAutoMixer(u16, u8, u8, u8, u8);
        void setPitch(u16);
        void setMixerInitVolume(u8, s16);
        void setMixerVolume(u8, s16);
        void setPauseFlag(u8);
        void flush();
        void initFilter();
        void setFilterMode(u16);
        void setIIRFilterParam(s16*);
        void setFIR8FilterParam(s16*);
        void setDistFilter(s16);
        void setBusConnect(u8, u8);

        /* 0x000 */ u16 mIsActive;
        /* 0x002 */ u16 mIsFinished;      // written by the DSP when playback ends
        /* 0x004 */ u16 mPitch;
        /* 0x006 */ short _unused1;
        /* 0x008 */ u16 mResetFlag;       // set on playStart, cleared by the DSP after resetting its state
        /* 0x00A */ u8 _unused2[0x00C - 0x00A];
        /* 0x00C */ s16 mPauseFlag;
        /* 0x00E */ short _unused3;
        /* 0x010 */ OutputChannelConfig mOutputChannels[DSP_OUTPUT_CHANNELS];
        /* 0x040 */ u8 _unused4[0x050 - 0x040];
        /* 0x050 */ u16 mAutoMixerPanDolby;  // pan in the upper 8 bits, dolby in the lower 8
        /* 0x052 */ u16 mAutoMixerFxMix;
        /* 0x054 */ u16 mAutoMixerInitVolume;
        /* 0x056 */ u16 mAutoMixerVolume;
        /* 0x058 */ u16 mAutoMixerBeenSet;
        /* 0x05A */ u8 _unused5[0x060 - 0x05A];
        /* 0x060 */ short field_0x060;
        /* 0x062 */ u8 _unused6[0x064 - 0x062];
        /* 0x064 */ u16 mSamplesPerBlock;
        /* 0x066 */ short field_0x066;
        /* 0x068 */ u32 mSamplePosition;
        /* 0x06C */ u8 _unused7[0x070 - 0x06C];
        /* 0x070 */ u32 mAramStreamPosition;  // current read position, updated by the DSP
        /* 0x074 */ u32 mSamplesLeft;
        /* 0x078 */ short field_0x078[4];
        /* 0x080 */ short field_0x080[20];
        /* 0x0A8 */ short field_0x0a8[4];
        /* 0x0B0 */ u16 field_0x0b0[16];
        /* 0x0D0 */ u8 _unused8[0x100 - 0x0D0];
        /* 0x100 */ u16 mBytesPerBlock;
        /* 0x102 */ u16 mLoopFlag;
        /* 0x104 */ s16 mpLast;
        /* 0x106 */ s16 mpPenult;
        /* 0x108 */ u16 mFilterMode;
        /* 0x10A */ u16 mForcedStop;
        /* 0x10C */ int field_0x10c;
        /* 0x110 */ u32 mLoopStartSample;
        /* 0x114 */ u32 mEndSample;
        /* 0x118 */ u32 mWaveAramAddress;
        /* 0x11C */ int mSampleCount;
        /* 0x120 */ s16 fir_filter_params[8];
        /* 0x130 */ u8 _unused9[0x148 - 0x130];
        /* 0x148 */ s16 iir_filter_params[8];
        /* 0x158 */ u8 _unused10[0x180 - 0x158];
#ifdef TARGET_PC
        // Base that mWaveAramAddress is relative to; nullptr means the audio memory (JASKernel::sAramBase).
        void const* mAramBaseAddress;
#endif
    };

    void boot(void (*)(void*));
    void releaseHalt(u32);
    void finishWork(u16);
    void syncFrame(u32, u32, u32);
    void setDSPMixerLevel(f32);
    f32 getDSPMixerLevel();
    TChannel* getDSPHandle(int);
    TChannel* getDSPHandleNc(int);
    void setFilterTable(s16*, s16*, u32);
    void flushBuffer();
    void invalChannelAll();
    void initBuffer();
    int setFXLine(u8, s16*, JASDsp::FxlineConfig_*);
    BOOL changeFXLineParam(u8, u8, uintptr_t);

    extern u8 const DSPADPCM_FILTER[64];
    extern u32 const DSPRES_FILTER[320];
    extern u16 SEND_TABLE[];
    extern TChannel* CH_BUF;
    extern FxBuf* FX_BUF;
    extern f32 sDSPVolume;
};  // namespace JASDsp

u16 DSP_CreateMap2(u32 msg);
