// Force-included into every C++ translation unit of the game on PC.
#pragma once
#include <cstddef>
#include <cstring>
#include <cfloat>
#include <cmath>
#include <algorithm>
#include <new>
#include <string>
#include <cwchar>
#include <cwctype>
#include <cstdarg>
#include <cstdlib>
#include <limits>
#include <bitset>
#include <utility>
#include <type_traits>
#include <pc/msl_algorithm.hpp>
#include <pc/endian.h>
#include <macros.h>
#include <pc/wide.h>

// Tag type used by game new-expressions so they allocate from JKRHeap (see tools/rewrite_new.py).
enum class JKRHeapToken { Dummy };
inline void* operator new(size_t, JKRHeapToken, void* where) noexcept { return where; }
inline void* operator new[](size_t, JKRHeapToken, void* where) noexcept { return where; }
class JKRHeap;
void* operator new(size_t size, JKRHeapToken) noexcept;
void* operator new(size_t size, JKRHeapToken, int align) noexcept;
void* operator new(size_t size, JKRHeapToken, JKRHeap* pHeap, int align) noexcept;
void* operator new[](size_t size, JKRHeapToken) noexcept;
void* operator new[](size_t size, JKRHeapToken, int align) noexcept;
void* operator new[](size_t size, JKRHeapToken, JKRHeap* pHeap, int align) noexcept;

// True if p lies inside the emulated console memory arenas that back the game's JKRHeaps.
bool pc_isGameHeapPtr(const void* p);

// CodeWarrior __declspec forms used by the decomp.
#ifndef _MSC_VER
#define __declspec(x) PC_DECLSPEC_##x
#define PC_DECLSPEC_weak __attribute__((weak))
#define PC_DECLSPEC_force_export
#endif

// Functions whose bodies are PowerPC assembly on the console and C on PC.
#define ASM

// CodeWarrior runtime helper used by decompiled code.
inline unsigned long long __cvt_dbl_usll(double d) {
    return (unsigned long long)d;
}
