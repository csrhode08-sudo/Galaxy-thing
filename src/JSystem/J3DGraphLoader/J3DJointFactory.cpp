#include "JSystem/J3DGraphLoader/J3DJointFactory.hpp"
#include "JSystem/J3DGraphAnimator/J3DJoint.hpp"
#include "JSystem/J3DGraphLoader/J3DModelLoader.hpp"
#include "JSystem/JSupport/JSupport.hpp"
#include <stdint.h>
#ifdef TARGET_PC
#include "pc/j3d.h"
#endif

J3DJointFactory::J3DJointFactory(J3DJointBlock const& block) {
    mJointInitData = JSUConvertOffsetToPtr< J3DJointInitData >(&block, (uintptr_t)block.mpJointInitData);
    mIndexTable = JSUConvertOffsetToPtr< u16 >(&block, (uintptr_t)block.mpIndexTable);

#ifdef TARGET_PC
    if (pc::j3d::gConvert) {
        u16 maxIndex = 0;
        for (int i = 0; i < block.mJointNum; i++) {
            be_swap(mIndexTable[i]);
            if (mIndexTable[i] > maxIndex) {
                maxIndex = mIndexTable[i];
            }
        }
        for (int index = 0; index <= maxIndex; index++) {  // each init data entry once
            J3DJointInitData* initData = &mJointInitData[index];
            be_swap(initData->mKind);
            be_swap(initData->mTransformInfo.mScale);
            be_swap(initData->mTransformInfo.mRotation);
            be_swap(initData->mTransformInfo.mTranslate);
            be_swap(initData->mRadius);
            be_swap(initData->mMin);
            be_swap(initData->mMax);
        }
    }
#endif
}

J3DJoint* J3DJointFactory::create(int no) {
    J3DJoint* joint = new (JKRHeapToken::Dummy) J3DJoint();
    joint->mJntNo = no;
    joint->mKind = getKind(no);
    joint->mScaleCompensate = getScaleCompensate(no);
    joint->mTransformInfo = getTransformInfo(no);
    joint->mBoundingSphereRadius = getRadius(no);
    joint->mMin = getMin(no);
    joint->mMax = getMax(no);
    joint->mMtxCalc = NULL;

    if (joint->mScaleCompensate == 0xFF)
        joint->mScaleCompensate = 0;

    return joint;
}
