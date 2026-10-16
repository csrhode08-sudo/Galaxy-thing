#pragma once

#include <revolution/types.h>

// Host audio output: a CPU implementation of the Wii DSP mixer feeding an SDL3 audio stream.
// Based on Dusklight's audio backend (CC0): https://github.com/TwilitRealm/dusklight
namespace pc::audio {
    /// Starts mixing and playback. Called once JAudio2's heaps exist (JAU_JASInitializer::initJASystem).
    void Initialize();
    void Shutdown();

    /// 0..1 master volume, applied on a -4 dB per 10% curve.
    void SetMasterVolume(f32 value);
    void SetEnableReverb(bool value);
    void SetPaused(bool paused);
}
