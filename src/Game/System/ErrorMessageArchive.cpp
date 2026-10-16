#include "Game/System/ErrorMessageArchive.hpp"
#ifdef TARGET_PC
alignas(32) u8 cErrorMessageArchive[0x1CAC0];
#else
#include "assets/ErrorMessageArchive.hpp"
#endif
