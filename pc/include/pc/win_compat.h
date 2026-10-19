#pragma once
// Force-included on Windows builds only: POSIX names the decomp and the platform layer use.
#ifdef _WIN32
#define _USE_MATH_DEFINES
#include <string.h>
#include <malloc.h>
#include <math.h>
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
// clang-cl defines _MSC_VER, but the decomp places alignment after declarators (GNU attribute syntax).
#define ATTRIBUTE_ALIGN(num) __attribute__((aligned(num)))
#endif
