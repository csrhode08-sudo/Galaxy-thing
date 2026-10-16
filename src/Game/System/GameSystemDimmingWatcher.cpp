#include "Game/System/GameSystemDimmingWatcher.hpp"
#include "Game/Util/TriggerChecker.hpp"
#include <revolution.h>

GameSystemDimmingWatcher::GameSystemDimmingWatcher() : mChecker(nullptr), _4(true), _5(false) {
    mChecker = new (JKRHeapToken::Dummy) TriggerChecker();
}

void GameSystemDimmingWatcher::update() {
    mChecker->update(_4 || _5);

    if (mChecker->getOnTrigger()) {
        VIEnableDimming(true);
    } else if (mChecker->getOffTrigger()) {
        VIEnableDimming(false);
    }
}
