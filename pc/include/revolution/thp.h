// PC shim: THP movie types from the Wii SDK + Aurora's THP decoder.
#ifndef PC_REVOLUTION_THP_H
#define PC_REVOLUTION_THP_H
#include <revolution/types.h>
#include <dolphin/thp.h>
#include <revolution/gx.h>
#include <revolution/dvd.h>

#ifdef __cplusplus
extern "C" {
#endif

// These headers are read straight from .thp files (big-endian).
typedef struct {
    char magic[4];
    BE(u32) version;
    BE(u32) bufSize;
    BE(u32) audioMaxSamples;
    BE(f32) frameRate;
    BE(u32) numFrames;
    BE(u32) firstFrameSize;
    BE(u32) movieDataSize;
    BE(u32) compInfoDataOffsets;
    BE(u32) offsetDataOffsets;
    BE(u32) movieDataOffsets;
    BE(u32) finalFrameDataOffsets;
} THPHeader;

typedef struct {
    BE(u32) xSize;
    BE(u32) ySize;
    BE(u32) videoType;
} THPVideoInfo;

typedef struct {
    BE(u32) numComponents;
    u8 frameComp[16];
} THPFrameCompInfo;

typedef struct {
    BE(u32) sndChannels;
    BE(u32) sndFrequency;
    BE(u32) sndNumSamples;
    BE(u32) sndNumTracks;
} THPAudioInfo;

typedef struct {
    u8* ptr;
    s32 frameNumber;
    volatile BOOL isValid;
} THPReadBuffer;

typedef struct {
    u8* ytexture;
    u8* utexture;
    u8* vtexture;
    s32 frameNumber;
} THPTextureSet;

typedef struct {
    s16* buffer;
    s16* curPtr;
    u32 validSample;
} THPAudioBuffer;

void THPGXRestore(void);
void THPGXYuv2RgbSetup(const GXRenderModeObj* rmode);
void THPGXYuv2RgbDraw(u8* y_data, u8* u_data, u8* v_data, s16 x, s16 y, s16 textureWidth, s16 textureHeight, s16 polygonWidth,
                      s16 polygonHeight);
void THPGXSetTexObjFilter(GXTexFilter minFilter, GXTexFilter magFilter);
void THPGXGetTexObjFilter(GXTexFilter* minFilter, GXTexFilter* magFilter);

#ifdef __cplusplus
}
#endif
#endif
