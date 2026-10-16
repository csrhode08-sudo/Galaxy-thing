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
#ifndef GEKKO
#define PSMTX44Identity C_MTX44Identity
#define PSMTX44Copy C_MTX44Copy
#define PSMTX44Concat C_MTX44Concat
#define PSMTX44Transpose C_MTX44Transpose
#endif
#ifdef __cplusplus
}
#endif
#endif
