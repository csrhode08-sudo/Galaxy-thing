#include "Game/System/StoryProgressTable.hpp"
#ifdef TARGET_PC
alignas(32) u8 cStoryProgressTable[0x1E0];
#else
#include "assets/StoryProgressTable.hpp"
#endif
