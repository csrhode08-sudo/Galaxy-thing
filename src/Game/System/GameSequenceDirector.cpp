#include "Game/System/GameSequenceDirector.hpp"
#include "Game/NWC24/NWC24Messenger.hpp"
#include "Game/System/FindingLuigiEventScheduler.hpp"
#include "Game/System/GameDataTemporaryInGalaxy.hpp"
#include "Game/System/GameSequenceProgress.hpp"
#include "Game/System/SaveDataHandleSequence.hpp"
#include <JSystem/JKernel/JKRHeap.hpp>

GameSequenceDirector::GameSequenceDirector()
    : mGameDataTemporaryInGalaxy(nullptr), mGameSequenceProgress(nullptr), mSaveDataHandleSequence(nullptr), mNWC24Messenger(nullptr) {
    mSaveDataHandleSequence = new (JKRHeapToken::Dummy) SaveDataHandleSequence();
    mSaveDataHandleSequence->registerFunctorOnSaveSuccess(MR::Functor(this, &GameSequenceDirector::executeOnSaveSuccess));
    mSaveDataHandleSequence->registerFunctorJustBeforeSave(MR::Functor(this, &GameSequenceDirector::executeJustBeforeSave));

    mGameSequenceProgress = new (JKRHeapToken::Dummy) GameSequenceProgress();
    mGameDataTemporaryInGalaxy = new (JKRHeapToken::Dummy) GameDataTemporaryInGalaxy();
    mNWC24Messenger = new (JKRHeapToken::Dummy) NWC24Messenger("NWC24Messenger");
}

void GameSequenceDirector::initAfterResourceLoaded() {
    mSaveDataHandleSequence->initAfterResourceLoaded();
    mGameSequenceProgress->initAfterResourceLoaded();
    mNWC24Messenger->initAfterResourceLoaded();
}

void GameSequenceDirector::update() {
    mSaveDataHandleSequence->update();
    mGameSequenceProgress->update();
    mNWC24Messenger->movement();
    mNWC24Messenger->calcAnim();
}

void GameSequenceDirector::draw() const {
    mSaveDataHandleSequence->draw();
    mGameSequenceProgress->draw();
    mNWC24Messenger->draw();
}

bool GameSequenceDirector::isInitializedGameDataHolder() const {
    return mSaveDataHandleSequence->isInitializedGameDataHolder();
}

GameDataHolder* GameSequenceDirector::getGameDataHolder() {
    return mSaveDataHandleSequence->getHolder();
}

void GameSequenceDirector::executeOnSaveSuccess() {
    mGameSequenceProgress->mFindingLuigiEventScheduler->sendMail();
}

void GameSequenceDirector::executeJustBeforeSave() {
    mGameSequenceProgress->mFindingLuigiEventScheduler->writeSendSize();
}
