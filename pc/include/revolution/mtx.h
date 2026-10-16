#ifndef PC_REVOLUTION_MTX_H
#define PC_REVOLUTION_MTX_H
#include <revolution/types.h>
#include <dolphin/mtx.h>
#include <dolphin/mtx/mtx44ext.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef f32 Mtx23[2][3];
typedef f32 (*Mtx23P)[3];
typedef f32 Mtx33[3][3];
typedef f32 (*Mtx3P)[3];
#ifdef __cplusplus
}
#endif
#endif
