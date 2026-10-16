#pragma once

#include <revolution/os/OSInterrupt.h>

#ifdef TARGET_PC
// Interrupt masking does nothing on PC, and the audio mixer runs on the SDL audio thread,
// so critical sections take a global recursive lock instead.
class JASCriticalSection {
public:
    JASCriticalSection();
    ~JASCriticalSection();
};
#else
class JASCriticalSection {
public:
    JASCriticalSection() : mInterruptState(OSDisableInterrupts()) {};
    ~JASCriticalSection() {
        OSRestoreInterrupts(mInterruptState);
    }
    /* 0x00 */ BOOL mInterruptState;
};
#endif
