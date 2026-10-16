// PC shim: Wii GX -> Aurora GX.
#ifndef PC_REVOLUTION_GX_H
#define PC_REVOLUTION_GX_H

#include <revolution/types.h>
#include <dolphin/gx.h>
#include <dolphin/gd.h>

#ifdef __cplusplus
extern "C++" {
// The original code writes GPU commands straight to the write-gather pipe at 0xCC008000.
// On PC, route each typed store into Aurora's command FIFO interpreter instead.
namespace pc_gx {
template <typename T, void (*Fn)(T)> struct FifoPort {
    inline void operator=(T v) volatile { Fn(v); }
};
inline void wr_u8(::u8 v) { GXCmd1u8(v); }
inline void wr_u16(::u16 v) { GXCmd1u16(v); }
inline void wr_u32(::u32 v) { GXCmd1u32(v); }
inline void wr_s8(::s8 v) { GXCmd1u8((::u8)v); }
inline void wr_s16(::s16 v) { GXCmd1u16((::u16)v); }
inline void wr_s32(::s32 v) { GXCmd1u32((::u32)v); }
inline void wr_f32(::f32 v) { ::u32 b; memcpy(&b, &v, 4); GXCmd1u32(b); }
struct WGPipe {
    FifoPort<::u8, wr_u8> u8;
    FifoPort<::u16, wr_u16> u16;
    FifoPort<::u32, wr_u32> u32;
    FifoPort<::s8, wr_s8> s8;
    FifoPort<::s16, wr_s16> s16;
    FifoPort<::s32, wr_s32> s32;
    FifoPort<::f32, wr_f32> f32;
};
extern volatile WGPipe gWGPipe;
}  // namespace pc_gx
#undef GXWGFifo
#define GXWGFifo pc_gx::gWGPipe

inline void GXSetCopyFilter(GXBool aa, const u8 sample_pattern[12][2], GXBool vf, const u8 vfilter[7]) {
    GXSetCopyFilter(aa, const_cast<u8(*)[2]>(sample_pattern), vf, const_cast<u8*>(vfilter));
}

// Typed matrix overloads so classes convertible to Mtx (e.g. nw4r::math::MTX34) resolve unambiguously.
inline void GXLoadPosMtxImm(const f32 (*m)[4], u32 id) { GXLoadPosMtxImm(static_cast<const void*>(m), id); }
inline void GXLoadPosMtxImm(f32 (*m)[4], u32 id) { GXLoadPosMtxImm(static_cast<const void*>(m), id); }
inline void GXLoadNrmMtxImm(const f32 (*m)[4], u32 id) { GXLoadNrmMtxImm(static_cast<const void*>(m), id); }
inline void GXLoadNrmMtxImm(f32 (*m)[4], u32 id) { GXLoadNrmMtxImm(static_cast<const void*>(m), id); }
inline void GXLoadTexMtxImm(const f32 (*m)[4], u32 id, GXTexMtxType t) { GXLoadTexMtxImm(static_cast<const void*>(m), id, t); }
inline void GXLoadTexMtxImm(f32 (*m)[4], u32 id, GXTexMtxType t) { GXLoadTexMtxImm(static_cast<const void*>(m), id, t); }

// Wii signature: GXGetCPUFifo(GXFifoObj* out).
inline void GXGetCPUFifo(GXFifoObj* out) {
    GXFifoObj* cur = ::GXGetCPUFifo();
    if (out != nullptr && cur != nullptr) {
        memcpy(out, cur, sizeof(GXFifoObj));
    }
}
}  // extern "C++"
#endif

#ifdef __cplusplus
extern "C" {
#endif
// Wii GX functions not provided by Aurora; implemented in pc/src/gx_compat.cpp.
void GXSetDrawSync(u16 token);
u16 GXReadDrawSync(void);
void GXEnableBreakPt(void* breakPt);
void GXDisableBreakPt(void);
void GXSetZScaleOffset(f32 scale, f32 offset);
void GXInitSpecularDirHA(GXLightObj* lt_obj, f32 nx, f32 ny, f32 nz, f32 hx, f32 hy, f32 hz);
void GXLoadTexMtxIndx(u16 mtxIndx, u32 id, GXTexMtxType type);
void GXGetTexObjAll(const GXTexObj* obj, void** image_ptr, u16* width, u16* height, GXTexFmt* format, GXTexWrapMode* wrap_s,
                    GXTexWrapMode* wrap_t, GXBool* mipmap);
void GXGetTexObjLODAll(const GXTexObj* obj, GXTexFilter* min_filt, GXTexFilter* mag_filt, f32* min_lod, f32* max_lod, f32* lod_bias,
                       GXBool* bias_clamp, GXBool* do_edge_lod, GXAnisotropy* max_aniso);
static inline void GXCmd1f32(f32 v) {
    u32 b;
    memcpy(&b, &v, sizeof(b));
    GXCmd1u32(b);
}
#ifdef __cplusplus
}
#endif

#define GX_WRITE_U8(v) GXCmd1u8((u8)(v))
#define GX_WRITE_U16(v) GXCmd1u16((u16)(v))
#define GX_WRITE_U32(v) GXCmd1u32((u32)(v))
#define GX_WRITE_F32(v) do { f32 _f = (v); u32 _u; memcpy(&_u, &_f, 4); GXCmd1u32(_u); } while (0)

#endif
