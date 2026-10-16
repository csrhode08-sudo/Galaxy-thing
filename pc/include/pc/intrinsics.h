// PC replacements for PowerPC / CodeWarrior compiler intrinsics.
#ifndef PC_INTRINSICS_H
#define PC_INTRINSICS_H

#include <math.h>
#include <string.h>
#include <stdint.h>

#ifdef __cplusplus
#define PC_INLINE inline
#else
#define PC_INLINE static inline
#endif

PC_INLINE float __frsqrte(float x) { return 1.0f / sqrtf(x); }
PC_INLINE float __fres(float x) { return 1.0f / x; }
PC_INLINE double __fabs(double x) { return fabs(x); }
PC_INLINE float __fabsf(float x) { return fabsf(x); }
PC_INLINE double __fnabs(double x) { return -fabs(x); }
PC_INLINE float __fnabsf(float x) { return -fabsf(x); }
PC_INLINE double __fsel(double a, double b, double c) { return a >= 0.0 ? b : c; }
PC_INLINE float __fsels(float a, float b, float c) { return a >= 0.0f ? b : c; }
PC_INLINE int __abs(int x) { return x < 0 ? -x : x; }
PC_INLINE uint32_t __cntlzw(uint32_t x) {
#if defined(_MSC_VER) && !defined(__clang__)
    unsigned long idx;
    return _BitScanReverse(&idx, x) ? 31 - idx : 32;
#else
    return x == 0 ? 32 : (uint32_t)__builtin_clz(x);
#endif
}
PC_INLINE uint32_t __rlwinm(uint32_t v, int sh, int mb, int me) {
    uint32_t r = (v << sh) | (v >> ((32 - sh) & 31));
    uint32_t mask = (mb <= me) ? ((0xFFFFFFFFu >> mb) & (0xFFFFFFFFu << (31 - me)))
                               : ((0xFFFFFFFFu >> mb) | (0xFFFFFFFFu << (31 - me)));
    return r & mask;
}
PC_INLINE uint32_t __rlwimi(uint32_t dst, uint32_t v, int sh, int mb, int me) {
    uint32_t r = (v << sh) | (v >> ((32 - sh) & 31));
    uint32_t mask = (mb <= me) ? ((0xFFFFFFFFu >> mb) & (0xFFFFFFFFu << (31 - me)))
                               : ((0xFFFFFFFFu >> mb) | (0xFFFFFFFFu << (31 - me)));
    return (r & mask) | (dst & ~mask);
}
PC_INLINE void* __memcpy(void* d, const void* s, size_t n) { return memcpy(d, s, n); }
PC_INLINE void __dcbz(void* p, int off) { memset((char*)p + off, 0, 32); }
PC_INLINE void __dcbf(void* p, int off) { (void)p; (void)off; }
PC_INLINE void __dcbst(void* p, int off) { (void)p; (void)off; }
PC_INLINE void __dcbi(void* p, int off) { (void)p; (void)off; }
PC_INLINE void __dcbt(void* p, int off) { (void)p; (void)off; }
PC_INLINE void __sync(void) {}
PC_INLINE void __eieio(void) {}

#endif
