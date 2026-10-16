#ifndef PC_REVOLUTION_DVD_H
#define PC_REVOLUTION_DVD_H
#include <revolution/types.h>
#include <dolphin/dvd.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*DVDCBCallback)(s32 result, struct DVDCommandBlock* block);
BOOL DVDCheckDiskAsync(DVDCommandBlock* block, DVDCBCallback callback);
#ifdef __cplusplus
}
#endif
#endif
