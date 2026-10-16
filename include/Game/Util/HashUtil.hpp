#pragma once

#include <revolution/types.h>

class HashSortTable {
public:
    HashSortTable(u32);

    // Values are pointer-sized on PC: several users store object pointers in them.
    bool add(const char*, uintptr_t, bool);
    bool add(u32, uintptr_t);
    bool addOrSkip(u32, uintptr_t);
    void sort();
    bool search(u32, uintptr_t*);
    bool search(const char*, uintptr_t*);
    bool search(const char*, const char*, uintptr_t*);
    void swap(const char*, const char*);

    /* 0x00 */ bool mHasBeenSorted;
    /* 0x04 */ u32* mHashCodes;
    /* 0x08 */ uintptr_t* _8;
    /* 0x0C */ u16* _C;
    /* 0x10 */ u16* _10;
    /* 0x14 */ u32 mCurrentLength;
    /* 0x18 */ u32 mMaxLength;
};

namespace MR {
    u32 getHashCode(const char*);
    u32 getHashCodeLower(const char*);
};  // namespace MR
