// PC TPL (texture palette) API. The structs below are native, built from the big-endian file by
// TPLGet(); texture/palette pixel data still points into the file (the GPU layer reads it big-endian).
#ifndef TPL_H
#define TPL_H

#include "revolution/gx.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    u16 numEntries;
    u8 unpacked;
    u8 _4;
    GXTlutFmt format;
    char* data;
} TPLClutHeader, *TPLClutHeaderPtr;

typedef struct {
    u16 height;
    u16 width;
    u32 format;
    char* data;
    GXTexWrapMode wrapS;
    GXTexWrapMode wrapT;
    GXTexFilter minFilter;
    GXTexFilter magFilter;
    f32 LODBias;
    u8 edgeLODEnable;
    u8 minLOD;
    u8 maxLOD;
    u8 unpacked;
} TPLHeader, *TPLHeaderPtr;

typedef struct {
    TPLHeaderPtr textureHeader;
    TPLClutHeaderPtr CLUTHeader;
} TPLDescriptor, *TPLDescriptorPtr;

// Opaque on PC: points at the raw file data. Only pass it to TPLBind/TPLGet.
typedef struct TPLPalette TPLPalette, *TPLPalettePtr;

void TPLBind(TPLPalettePtr);
TPLDescriptorPtr TPLGet(TPLPalettePtr, u32);

#ifdef __cplusplus
}
#endif

#endif  // TPL_H
