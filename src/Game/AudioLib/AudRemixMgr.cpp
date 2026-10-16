#include "Game/AudioLib/AudRemixMgr.hpp"
#include <JSystem/JKernel/JKRHeap.hpp>

AudRemixMgr::AudRemixMgr(JKRHeap* pHeap) {
    mHeap = pHeap;
    mGroups = nullptr;
    mSoundObj = nullptr;
    mRemixSeq = nullptr;
}

void AudRemixMgr::init() {
    mSoundObj = new (JKRHeapToken::Dummy, mHeap, 0) AudSoundObject(nullptr, 0x10, mHeap);
    mRemixSeq = new (JKRHeapToken::Dummy, mHeap, 0) AudRemixSequencer();
}

void AudRemixMgr::update() {
    mRemixSeq->update();
}

#ifdef TARGET_PC
// The remix table is big-endian 32-bit words; swap each word the parser below consumes, once.
static void swapRemixSeqResource(void* ptr) {
    u32* p = static_cast< u32* >(ptr);
    auto swap = [](u32* w, u32 n) {
        for (u32 i = 0; i < n; i++) {
            w[i] = __builtin_bswap32(w[i]);
        }
    };
    swap(p, 1);
    const u32 groupCount = *p++;
    swap(p, groupCount);
    p += groupCount;
    for (u32 g = 0; g < groupCount; g++) {
        swap(p, 2);
        const u32 trackCount = p[0];
        const u32 noteCount = p[1];
        p += 2;
        swap(p, noteCount);
        p += noteCount;
        for (u32 t = 0; t < trackCount; t++) {
            swap(p, 1 + noteCount * 4);
            p += 1 + noteCount * 4;
        }
    }
}
#endif

void AudRemixMgr::setRemixSeqResource(void* ptr) {
#ifdef TARGET_PC
    swapRemixSeqResource(ptr);
#endif
    u32* newPtr = (u32*)ptr;
    int groupCount = *(newPtr);
    newPtr++;
    mGroupCount = groupCount;
    newPtr += mGroupCount;
    mGroups = new (JKRHeapToken::Dummy, mHeap, 0) RemixNoteGroupData[mGroupCount];
    for (int i = 0; i < mGroupCount; i++) {
        RemixNoteGroupData* pGroupData = &mGroups[i];
        pGroupData->mIndex = i;
        pGroupData->mTrackCount = *newPtr++;
        pGroupData->mNoteCount = *newPtr++;
        pGroupData->_C = newPtr;
        newPtr += pGroupData->mNoteCount;
        pGroupData->mRemixTracks = new (JKRHeapToken::Dummy, mHeap, 0) RemixNoteTrackData[pGroupData->mTrackCount];
        for (int j = 0; j < pGroupData->mTrackCount; j++) {
            pGroupData->mRemixTracks[j]._0 = *newPtr++;
            pGroupData->mRemixTracks[j]._4 = reinterpret_cast< RemixNoteData* >(newPtr);
            newPtr += pGroupData->mNoteCount * 4;
        }
    }
}

RemixNoteGroupData* AudRemixMgr::getRemixNoteGroupDataFromMelodyNo(s32 melodyNo) const {
    for (int i = 0; i < mGroupCount; i++) {
        if (melodyNo == mGroups[i].mIndex) {
            return &mGroups[i];
        }
    }
    return nullptr;
}
