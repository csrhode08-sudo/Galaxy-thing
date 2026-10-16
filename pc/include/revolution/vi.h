#ifndef PC_REVOLUTION_VI_H
#define PC_REVOLUTION_VI_H
#include <revolution/types.h>
#include <dolphin/vi.h>

typedef u8 VIBool;
#define VI_FALSE ((VIBool)0)
#define VI_TRUE ((VIBool)1)

#define VI_MAX_WIDTH_NTSC 720
#define VI_MAX_HEIGHT_NTSC 480
#define VI_MAX_WIDTH_PAL 720
#define VI_MAX_HEIGHT_PAL 574
#define VI_MAX_WIDTH_MPAL 720
#define VI_MAX_HEIGHT_MPAL 480
#define VI_MAX_WIDTH_EURGB60 VI_MAX_WIDTH_NTSC
#define VI_MAX_HEIGHT_EURGB60 VI_MAX_HEIGHT_NTSC
#ifndef VI_TVMODE_EURGB60_PROG
#define VI_TVMODE_EURGB60_PROG ((VITVMode)VI_TVMODE(VI_EURGB60, 2))
#endif

#ifdef __cplusplus
extern "C" {
#endif
u32 VIGetDimmingCount(void);
BOOL VIEnableDimming(BOOL enable);
void VISetTrapFilter(VIBool filter);
void VISetGamma(u32 gamma);
#ifdef __cplusplus
}
#endif
#endif
