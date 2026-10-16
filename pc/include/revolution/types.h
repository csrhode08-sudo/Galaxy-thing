// PC shim: Wii SDK basic types, mapped onto Aurora's dolphin types.
#ifndef TYPES_H
#define TYPES_H

#include <dolphin/types.h>
#include <stdint.h>
#include <stddef.h>

#ifndef __REGISTER
#define __REGISTER
#endif
#ifndef AT_ADDRESS
#define AT_ADDRESS(x)
#endif
#ifndef ALWAYS_INLINE
#define ALWAYS_INLINE inline
#endif
#ifndef NO_INLINE
#define NO_INLINE
#endif
#ifndef ATTRIBUTE_ALIGN
#ifdef _MSC_VER
#define ATTRIBUTE_ALIGN(num) __declspec(align(num))
#else
#define ATTRIBUTE_ALIGN(num) __attribute__((aligned(num)))
#endif
#endif
#ifndef ATTRIBUTE_PACKED
#define ATTRIBUTE_PACKED
#endif
#ifndef ATTRIBUTE_WEAK
// Objects defined in headers (e.g. nerve singletons) rely on weak linkage, as with CodeWarrior.
#define ATTRIBUTE_WEAK __attribute__((weak))
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define ROUND_UP(x, align) (((x) + (align) - 1) & (-(align)))
#define ROUND_UP_PTR(x, align) ((void*)((((uintptr_t)(x)) + (align) - 1) & (~((uintptr_t)(align) - 1))))

#define ALIGN_PREV(X, N) ((X) & ~((N) - 1))
#define ALIGN_NEXT(X, N) ALIGN_PREV(((X) + (N) - 1), N)

#define ARRAY_SIZE(o) (s32)(sizeof(o) / sizeof(o[0]))
#define ARRAY_SIZEU(o) (sizeof(o) / sizeof(o[0]))

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif
#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#define FOURCC(c0, c1, c2, c3) (u32)((c0 & 0xFF) << 24 | (c1 & 0xFF) << 16 | (c2 & 0xFF) << 8 | (c3 & 0xFF))

#define IS_ALIGNED(x, align) (((uintptr_t)(x) & ((align) - 1)) == 0)
#define IS_NOT_ALIGNED(X, N) (((X) & ((N) - 1)) != 0)

#define IS_REF_NULL(r) (0)
#define IS_REF_NONNULL(r) (1)

#include "pc/intrinsics.h"

#endif  // TYPES_H
