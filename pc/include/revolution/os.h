// PC shim: Wii OS -> Aurora OS, plus Wii-only declarations used by Galaxy.
#ifndef PC_REVOLUTION_OS_H
#define PC_REVOLUTION_OS_H

#include <revolution/types.h>
#include <stdarg.h>
#include <dolphin/os.h>
#include <dolphin/base/PPCArch.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OSHalt(msg) OSPanic(__FILE__, __LINE__, msg)
#define OSDiffTick(tick1, tick0) ((s32)(tick1) - (s32)(tick0))
#define OS_TICKS_DELTA(x, y) ((s32)x - (s32)y)
#define OSIsMEM1Region(addr) (1)
#define OSIsMEM2Region(addr) (0)
#define OSAssert_Line(line_, exp_)
#ifndef ASSERT
#define ASSERT(exp) ((void)0)
#endif
#ifndef ASSERTMSG
#define ASSERTMSG(exp, msg) ((void)0)
#endif
#ifndef OS_ERROR_FPE
#define OS_ERROR_FPE 16
#endif

typedef struct OSIOSRev {
    u8 reserved;
    u8 major;
    u8 minor;
    u8 micro;
    u8 month;
    u8 date;
    u16 year;
} OSIOSRev;

void OSSetMEM2ArenaHi(void*);
void OSSetMEM2ArenaLo(void*);
void* OSGetMEM1ArenaLo(void);
void* OSGetMEM1ArenaHi(void);
void* OSGetMEM2ArenaLo(void);
void* OSGetMEM2ArenaHi(void);
void OSSetMEM1ArenaLo(void*);
void OSSetMEM1ArenaHi(void*);
void* OSAllocFromMEM1ArenaLo(u32, u32);
void* OSAllocFromMEM2ArenaLo(u32, u32);
u32 OSGetPhysicalMem2Size(void);
u32 OSGetPhysicalMem1Size(void);
void OSProtectRange(u32, void*, u32, u32);
const u8 OSGetAppType(void);
const char* OSGetAppGamename(void);
void OSShutdownSystem(void);
void OSRestart(u32);
void OSReturnToMenu(void);
void OSReturnToDataManager(void);
s32 OSGetResetButtonState(void);
BOOL OSIsRestart(void);
u32 OSGetResetCode(void);
void OSResetSystem(int reset, u32 resetCode, BOOL forceMenu);
u32 OSGetProgressiveMode(void);
void OSSetProgressiveMode(u32);
u8 OSGetLanguage(void);
u32 OSGetEuRgb60Mode(void);
typedef void (*OSPowerCallback)(void);
OSPowerCallback OSSetPowerCallback(OSPowerCallback callback);
void OSRebootSystem(void);
void OSSleepTicks(OSTime ticks);

#ifdef __cplusplus
}
#endif

#endif
