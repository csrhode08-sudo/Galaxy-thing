#pragma once

#include <revolution/types.h>

class JASSeqReader {
public:
    JASSeqReader() {
        init();
    }
    void init();
    void init(void*);
    bool call(u32);
    bool loopStart(u32);
    bool loopEnd();
    bool ret();
    u32 readMidiValue();
    void* getStackPtr(u32 idx) const;

    void jump(u32 param_1) {
        mSeqCursor = mSeqBuff + param_1;
    }

    void jump(void* param_1) {
        mSeqCursor = (u8*)param_1;
    }

    u32 get24(u32 param_0) const {
        const u8* p = mSeqBuff + param_0;
        return ((u32)p[0] << 16) | ((u32)p[1] << 8) | p[2];
    }

    u32* getBase() {
        return (u32*)mSeqBuff;
    }
    u32 getOffset() {
        return (u32)((u8*)mSeqCursor - (u8*)mSeqBuff);
    }
    u8* getAddr(u32 param_0) {
        return mSeqBuff + param_0;
    }
    u8 getByte(u32 param_0) const {
        return *(mSeqBuff + param_0);
    }
    // Sequence data is big-endian.
    u16 get16(u32 param_0) const {
        const u8* p = mSeqBuff + param_0;
        return (u16)((p[0] << 8) | p[1]);
    }
    u32 get32(u32 param_0) const {
        const u8* p = mSeqBuff + param_0;
        return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | p[3];
    }
    u8* getCur() {
        return mSeqCursor;
    }
    u32 readByte() {
        return *mSeqCursor++;
    }
    u32 read16() {
#ifdef __MWERKS__
        return *((u16*)mSeqCursor)++;
#else
        u32 value = (mSeqCursor[0] << 8) | mSeqCursor[1];
        mSeqCursor += 2;
        return value;
#endif
    }
    u32 read24() {
        mSeqCursor--;
#ifdef __MWERKS__
        return (*((u32*)mSeqCursor)++) & 0x00ffffff;
#else
        u32 value = ((u32)mSeqCursor[1] << 16) | ((u32)mSeqCursor[2] << 8) | mSeqCursor[3];
        mSeqCursor += 4;
        return value;
#endif
    }
    u16 getLoopCount() const {
        if (mNumStacks == 0) {
            return 0;
        }
        return mLoopCounts[mNumStacks - 1];
    }

    /* 0x00 */ u8* mSeqBuff;
    /* 0x04 */ u8* mSeqCursor;
    /* 0x08 */ u32 mNumStacks;
    /* 0x0C */ u8* mStackPtrs[8];
    /* 0x2C */ u16 mLoopCounts[8];
};
