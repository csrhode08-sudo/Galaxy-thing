#pragma once
// Force-included on Windows builds only: POSIX names the decomp and the platform layer use.
#ifdef _WIN32
#include <string.h>
#include <malloc.h>
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif
