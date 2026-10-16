#include "Game/System/GalaxyInfoTable.hpp"
#ifdef TARGET_PC
alignas(32) u8 cGalaxyInfoTable[0xD20];
#else
#include "assets/GalaxyInfoTable.hpp"
#endif
