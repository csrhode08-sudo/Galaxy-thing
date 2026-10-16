#pragma once
// PC: byte-order conversion of J3D material structures that are read from files but are also runtime types,
// so they cannot be declared big-endian. The file data is copied and the copy converted.
#ifdef TARGET_PC
#include "JSystem/J3DGraphBase/J3DStruct.hpp"

inline void J3DSwapTexMtxInfo(J3DTexMtxInfo& info) {
    be_swap(info.mCenter);
    be_swap(info.mSRT.mScaleX);
    be_swap(info.mSRT.mScaleY);
    be_swap(info.mSRT.mRotation);
    be_swap(info.mSRT.mTranslationX);
    be_swap(info.mSRT.mTranslationY);
    for (int i = 0; i < 16; i++) {
        be_swap(info.mEffectMtx[i / 4][i % 4]);
    }
}

inline void J3DSwapIndTexMtxInfo(J3DIndTexMtxInfo& info) {
    for (int i = 0; i < 6; i++) {
        be_swap(info.field_0x0[i / 3][i % 3]);
    }
}

inline void J3DSwapFogInfo(J3DFogInfo& info) {
    be_swap(info.mCenter);
    be_swap(info.mStartZ);
    be_swap(info.mEndZ);
    be_swap(info.mNearZ);
    be_swap(info.mFarZ);
    for (int i = 0; i < 10; i++) {
        be_swap(info.mFogAdjTable.r[i]);
    }
}
#endif
