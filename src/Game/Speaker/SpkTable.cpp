#include "Game/Speaker/SpkTable.hpp"

SpkTable::SpkTable() {
    mInitialized = false;
    mResourceCount = 0;
    mParameters = nullptr;
    mNames = nullptr;
}

void SpkTable::setResource(void* pRes) {
    mInitialized = false;

#ifdef TARGET_PC
    // File data is big-endian and stores 32-bit name offsets; build a separate pointer table.
    const BE(s32)* cursor = (const BE(s32)*)pRes;
    mResourceCount = cursor[0];
    mParameters = (SpkParameters*)((u8*)pRes + (s32)cursor[1]);
    const BE(u32)* nameOffsets = (const BE(u32)*)((u8*)pRes + (s32)cursor[2]);
    const char** names = new (JKRHeapToken::Dummy) const char*[mResourceCount];
    for (s32 i = 0; i < mResourceCount; i++) {
        names[i] = (const char*)pRes + (u32)nameOffsets[i];
    }
    mNames = names;
#else

    s32* cursor = (s32*)pRes;

    s32 resourceCount = *cursor++;
    s32 entryOff = *cursor++;
    s32 dataOffsetsStartOff = *cursor++;
    s32* pIsDataOffsetsInitialized = cursor;
    BOOL isDataOffsetsInitialized = *cursor++;

    mResourceCount = resourceCount;

    SpkParameters* entryOffset = (SpkParameters*)((s32)pRes + entryOff);
    mParameters = entryOffset;
    const char** names = (const char**)((s32)pRes + dataOffsetsStartOff);
    if (!isDataOffsetsInitialized) {
        for (s32 i = 0; i < mResourceCount; i++) {
            names[i] += (s32)pRes;
        }
    }

    mNames = names;
    *pIsDataOffsetsInitialized = TRUE;
#endif
    mInitialized = true;
}
