// Wii GX / VI / misc SDK functions that Aurora doesn't implement.
#include "pc/platform.h"

#include <revolution/gx.h>
#include <revolution/os.h>
#include <revolution/vi.h>
#include <revolution/dvd.h>
#include <revolution/thp.h>

#include <cctype>

namespace pc_gx {
volatile WGPipe gWGPipe;
}

extern "C" {

// The console uses draw-sync tokens and FIFO breakpoints to pace the CPU against the GPU.
// Aurora submits whole frames, so these complete immediately.
static GXDrawSyncCallback sDrawSyncCallback = nullptr;
static u16 sLastDrawSync = 0;

void GXSetDrawSync(u16 token) {
    sLastDrawSync = token;
    if (sDrawSyncCallback != nullptr) {
        sDrawSyncCallback(token);
    }
}

GXDrawSyncCallback GXSetDrawSyncCallback(GXDrawSyncCallback cb) {
    GXDrawSyncCallback old = sDrawSyncCallback;
    sDrawSyncCallback = cb;
    return old;
}

u16 GXReadDrawSync(void) {
    return sLastDrawSync;
}

void GXEnableBreakPt(void*) {}
void GXDisableBreakPt(void) {}

void GXSetZScaleOffset(f32, f32) {
    STUB_LOG();
}

void GXLoadTexMtxIndx(u16, u32, GXTexMtxType) {
    STUB_LOG();
}

void GXGetTexObjAll(const GXTexObj* obj, void** image_ptr, u16* width, u16* height, GXTexFmt* format, GXTexWrapMode* wrap_s,
                    GXTexWrapMode* wrap_t, GXBool* mipmap) {
    if (image_ptr) *image_ptr = GXGetTexObjData(obj);
    if (width) *width = GXGetTexObjWidth(obj);
    if (height) *height = GXGetTexObjHeight(obj);
    if (format) *format = GXGetTexObjFmt(obj);
    if (wrap_s) *wrap_s = GXGetTexObjWrapS(obj);
    if (wrap_t) *wrap_t = GXGetTexObjWrapT(obj);
    if (mipmap) *mipmap = GXGetTexObjMipMap(obj);
}

void GXGetTexObjLODAll(const GXTexObj* obj, GXTexFilter* min_filt, GXTexFilter* mag_filt, f32* min_lod, f32* max_lod, f32* lod_bias,
                       GXBool* bias_clamp, GXBool* do_edge_lod, GXAnisotropy* max_aniso) {
    (void)obj;
    if (min_filt) *min_filt = GX_LINEAR;
    if (mag_filt) *mag_filt = GX_LINEAR;
    if (min_lod) *min_lod = 0.0f;
    if (max_lod) *max_lod = 0.0f;
    if (lod_bias) *lod_bias = 0.0f;
    if (bias_clamp) *bias_clamp = GX_FALSE;
    if (do_edge_lod) *do_edge_lod = GX_FALSE;
    if (max_aniso) *max_aniso = GX_ANISO_1;
    STUB_LOG();
}

u32 VIGetDimmingCount(void) {
    return 0;
}

BOOL VIEnableDimming(BOOL) {
    return FALSE;
}

void VISetTrapFilter(VIBool) {}
void VISetGamma(u32) {}

static OSPowerCallback sPowerCallback = nullptr;
OSPowerCallback OSSetPowerCallback(OSPowerCallback cb) {
    OSPowerCallback old = sPowerCallback;
    sPowerCallback = cb;
    return old;
}

void OSRebootSystem(void) {
    OSShutdownSystem();
}

BOOL DVDCheckDiskAsync(DVDCommandBlock* block, DVDCBCallback callback) {
    if (callback != nullptr) {
        callback(1, block);
    }
    return TRUE;
}

int stricmp(const char* a, const char* b) {
    for (;; a++, b++) {
        int ca = std::tolower((unsigned char)*a), cb = std::tolower((unsigned char)*b);
        if (ca != cb || ca == 0) {
            return ca - cb;
        }
    }
}

// Movies (THP) are not drawn yet.
void THPGXRestore(void) {}
void THPGXYuv2RgbSetup(const GXRenderModeObj*) { STUB_LOG(); }
void THPGXYuv2RgbDraw(u8*, u8*, u8*, s16, s16, s16, s16, s16, s16) { STUB_LOG(); }
void THPGXSetTexObjFilter(GXTexFilter) {}
void THPGXGetTexObjFilter(GXTexFilter* minFilter, GXTexFilter* magFilter) {
    if (minFilter) *minFilter = GX_LINEAR;
    if (magFilter) *magFilter = GX_LINEAR;
}

}  // extern "C"
