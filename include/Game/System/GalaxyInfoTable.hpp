#pragma once

#include <revolution/types.h>

#ifdef TARGET_PC
// Copied out of the disc's main.dol at startup (pc/src/dol_assets.cpp).
extern u8 cGalaxyInfoTable[];
#else
extern const u8 cGalaxyInfoTable[];
#endif
