#include "Game/System/DrawBufferExecuter.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/System/DrawBuffer.hpp"
#include "Game/Util/LightUtil.hpp"
#include <algorithm>

DrawBufferExecuter::DrawBufferExecuter(const char* pName, J3DModel* pModel, s32 a1)
    : mActors(nullptr), mMaxNumActors(0), mNumActors(0), mName(pName), mDrawBuffer(nullptr), mLightType(-1), mDrawBufferCount(0) {
    mDrawBuffer = new (JKRHeapToken::Dummy) DrawBuffer(pModel);
}

void DrawBufferExecuter::allocateActorListBuffer() {
    s32 count = mDrawBufferCount;
    if (count != 0) {
        mActors = new (JKRHeapToken::Dummy) LiveActor*[count];
        mMaxNumActors = count;
        mDrawBuffer->init(mDrawBufferCount);
    }
}

void DrawBufferExecuter::add(LiveActor* pActor) {
    s32 lastActor = mNumActors;
    mNumActors++;
    mActors[lastActor] = pActor;
    mDrawBuffer->add(pActor);
}

void DrawBufferExecuter::remove(LiveActor* pActor) {
    LiveActor** it = msl::find(mActors, mActors + mNumActors, pActor);
    mActors[it - mActors] = getActor(mNumActors - 1);
    mNumActors--;
    mDrawBuffer->remove(pActor);
}

void DrawBufferExecuter::findLightInfo(LiveActor* pActor) {
    MR::initActorLightInfoDrawBuffer(pActor, mDrawBuffer);
}

void DrawBufferExecuter::onExecuteLight(s32 lightType) {
    mLightType = lightType;
}
void DrawBufferExecuter::offExecuteLight() {
    mLightType = -1;
}

void DrawBufferExecuter::calcViewAndEntry() {
    msl::for_each(&mActors[0], &mActors[mNumActors], msl::mem_func(&LiveActor::calcViewAndEntry));
}

void DrawBufferExecuter::drawOpa() const {
    if (mLightType != -1) {
        MR::loadLight(mLightType);
    }
    mDrawBuffer->drawOpa();
}

void DrawBufferExecuter::drawXlu() const {
    if (mLightType != -1) {
        MR::loadLight(mLightType);
    }
    mDrawBuffer->drawXlu();
}
