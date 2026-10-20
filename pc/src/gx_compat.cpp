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

// Movies (THP): the SDK's THPDraw. The decoder writes Y/U/V as tiled I8 textures and three TEV stages turn them
// into RGB.
static GXTexFilter sThpFilter = GX_LINEAR;

void THPGXRestore(void) {
    GXSetZMode(GX_ENABLE, GX_ALWAYS, GX_DISABLE);
    GXSetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_SET);
    GXSetNumTexGens(1);
    GXSetNumChans(0);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
    GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GXSetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);
    GXSetTevSwapMode(GX_TEVSTAGE1, GX_TEV_SWAP0, GX_TEV_SWAP0);
    GXSetTevSwapMode(GX_TEVSTAGE2, GX_TEV_SWAP0, GX_TEV_SWAP0);
    GXSetTevSwapMode(GX_TEVSTAGE3, GX_TEV_SWAP0, GX_TEV_SWAP0);
    GXSetTevSwapModeTable(GX_TEV_SWAP0, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_ALPHA);
    GXSetTevSwapModeTable(GX_TEV_SWAP1, GX_CH_RED, GX_CH_RED, GX_CH_RED, GX_CH_ALPHA);
    GXSetTevSwapModeTable(GX_TEV_SWAP2, GX_CH_GREEN, GX_CH_GREEN, GX_CH_GREEN, GX_CH_ALPHA);
    GXSetTevSwapModeTable(GX_TEV_SWAP3, GX_CH_BLUE, GX_CH_BLUE, GX_CH_BLUE, GX_CH_ALPHA);
}

void THPGXYuv2RgbSetup(const GXRenderModeObj* rmode) {
    const s32 scrWidth = rmode->fbWidth;
    const s32 scrHeight = rmode->efbHeight;
    Mtx44 pMtx;
    Mtx mMtx;

    C_MTXOrtho(pMtx, 0.0f, scrHeight, 0.0f, scrWidth, 0.0f, -1.0f);
    GXSetProjection(pMtx, GX_ORTHOGRAPHIC);
    GXSetViewport(0.0f, 0.0f, scrWidth, scrHeight, 0.0f, 1.0f);
    GXSetScissor(0, 0, scrWidth, scrHeight);
    PSMTXIdentity(mMtx);
    GXLoadPosMtxImm(mMtx, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
    GXSetZMode(GX_ENABLE, GX_ALWAYS, GX_DISABLE);
    GXSetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR);
    GXSetColorUpdate(GX_TRUE);
    GXSetAlphaUpdate(GX_FALSE);
    GXSetNumChans(0);
    GXSetNumTexGens(2);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
    GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
    GXInvalidateTexAll();
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT7, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
    GXSetVtxAttrFmt(GX_VTXFMT7, GX_VA_TEX0, GX_TEX_ST, GX_U16, 0);
    GXSetNumTevStages(4);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR_NULL);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_KONST, GX_CC_C0);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_KONST, GX_CA_A0);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
    GXSetTevKColorSel(GX_TEVSTAGE0, GX_TEV_KCSEL_K0);
    GXSetTevKAlphaSel(GX_TEVSTAGE0, GX_TEV_KASEL_K0_A);
    GXSetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);
    GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD1, GX_TEXMAP2, GX_COLOR_NULL);
    GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_TEXC, GX_CC_KONST, GX_CC_CPREV);
    GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_FALSE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_TEXA, GX_CA_KONST, GX_CA_APREV);
    GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
    GXSetTevKColorSel(GX_TEVSTAGE1, GX_TEV_KCSEL_K1);
    GXSetTevKAlphaSel(GX_TEVSTAGE1, GX_TEV_KASEL_K1_A);
    GXSetTevSwapMode(GX_TEVSTAGE1, GX_TEV_SWAP0, GX_TEV_SWAP0);
    GXSetTevOrder(GX_TEVSTAGE2, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
    GXSetTevColorIn(GX_TEVSTAGE2, GX_CC_ZERO, GX_CC_TEXC, GX_CC_ONE, GX_CC_CPREV);
    GXSetTevColorOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE2, GX_CA_TEXA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV);
    GXSetTevAlphaOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevSwapMode(GX_TEVSTAGE2, GX_TEV_SWAP0, GX_TEV_SWAP0);
    GXSetTevOrder(GX_TEVSTAGE3, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevColorIn(GX_TEVSTAGE3, GX_CC_APREV, GX_CC_CPREV, GX_CC_KONST, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE3, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE3, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE3, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevSwapMode(GX_TEVSTAGE3, GX_TEV_SWAP0, GX_TEV_SWAP0);
    GXSetTevKColorSel(GX_TEVSTAGE3, GX_TEV_KCSEL_K2);
    GXSetTevColorS10(GX_TEVREG0, GXColorS10{-90, 0, -114, 135});
    GXSetTevKColor(GX_KCOLOR0, GXColor{0, 0, 226, 88});
    GXSetTevKColor(GX_KCOLOR1, GXColor{179, 0, 0, 182});
    GXSetTevKColor(GX_KCOLOR2, GXColor{255, 0, 255, 128});
    GXSetTevSwapModeTable(GX_TEV_SWAP0, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_ALPHA);
}

void THPGXYuv2RgbDraw(u8* y_data, u8* u_data, u8* v_data, s16 x, s16 y, s16 textureWidth, s16 textureHeight, s16 polygonWidth,
                      s16 polygonHeight) {
    GXTexObj tobj0, tobj1, tobj2;
    GXInitTexObj(&tobj0, y_data, textureWidth, textureHeight, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXInitTexObjLOD(&tobj0, sThpFilter, sThpFilter, 0, 0, 0, GX_FALSE, GX_FALSE, GX_ANISO_1);
    GXLoadTexObj(&tobj0, GX_TEXMAP0);
    GXInitTexObj(&tobj1, u_data, textureWidth >> 1, textureHeight >> 1, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXInitTexObjLOD(&tobj1, sThpFilter, sThpFilter, 0, 0, 0, GX_FALSE, GX_FALSE, GX_ANISO_1);
    GXLoadTexObj(&tobj1, GX_TEXMAP1);
    GXInitTexObj(&tobj2, v_data, textureWidth >> 1, textureHeight >> 1, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXInitTexObjLOD(&tobj2, sThpFilter, sThpFilter, 0, 0, 0, GX_FALSE, GX_FALSE, GX_ANISO_1);
    GXLoadTexObj(&tobj2, GX_TEXMAP2);

    GXBegin(GX_QUADS, GX_VTXFMT7, 4);
    GXPosition3s16(x, y, 0);
    GXTexCoord2u16(0, 0);
    GXPosition3s16(x + polygonWidth, y, 0);
    GXTexCoord2u16(1, 0);
    GXPosition3s16(x + polygonWidth, y + polygonHeight, 0);
    GXTexCoord2u16(1, 1);
    GXPosition3s16(x, y + polygonHeight, 0);
    GXTexCoord2u16(0, 1);
    GXEnd();
}

void THPGXSetTexObjFilter(GXTexFilter filter) {
    sThpFilter = filter;
}

void THPGXGetTexObjFilter(GXTexFilter* minFilter, GXTexFilter* magFilter) {
    if (minFilter) *minFilter = sThpFilter;
    if (magFilter) *magFilter = sThpFilter;
}

}  // extern "C"
