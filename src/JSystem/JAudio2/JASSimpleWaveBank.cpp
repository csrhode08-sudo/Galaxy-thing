#include "JSystem/JAudio2/JASSimpleWaveBank.hpp"
#include <stdint.h>

JASSimpleWaveBank::JASSimpleWaveBank() {
    mWaveTable = nullptr;
    mWaveTableSize = 0;
}

JASSimpleWaveBank::~JASSimpleWaveBank() {
    delete[] mWaveTable;
}

void JASSimpleWaveBank::setWaveTableSize(u32 size, JKRHeap* heap) {
    delete[] mWaveTable;
    mWaveTable = new (JKRHeapToken::Dummy, heap, 0) TWaveHandle[size];
    mWaveTableSize = size;
}

JASWaveHandle* JASSimpleWaveBank::getWaveHandle(u32 no) const {
    if (no >= mWaveTableSize) {
        return nullptr;
    }
    return &mWaveTable[no];
}

void JASSimpleWaveBank::setWaveInfo(u32 no, JASWaveInfo const& waveInfo) {
    mWaveTable[no].mWaveInfo = waveInfo;
    mWaveTable[no].mWaveInfo._24 = &_48;
    mWaveTable[no].mHeap = &mHeap;
}

JASWaveArc* JASSimpleWaveBank::getWaveArc(u32 no) {
    if (no != 0) {
        return nullptr;
    }
    return this;
}

int JASSimpleWaveBank::TWaveHandle::getWavePtr() const {
    void* base = mHeap->getBase();
    if (base == nullptr) {
        return 0;
    }
    #ifdef TARGET_PC
    // DSP addresses are offsets into the emulated audio memory (see JASKernel::sAramBase).
    return (int)((uintptr_t)base - JASKernel::sAramBase) + mWaveInfo.mAWStartOffs;
#else
    return (intptr_t)base + mWaveInfo.mAWStartOffs;
#endif
}
