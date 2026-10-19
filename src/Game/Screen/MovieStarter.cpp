#include "Game/Screen/MovieStarter.hpp"
#include "Game/Screen/MoviePlayingSequence.hpp"
#include "Game/Util/ActorSwitchUtil.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ObjUtil.hpp"

#ifdef TARGET_PC
#include <cstdlib>
// Test aid: PC_TEST_MOVIE=<frames> starts the stage's first movie that many frames after it loads.
namespace {
    MovieStarter* sTestStarter = nullptr;
    int sTestCountdown = -1;
}  // namespace

void MovieStarter_pcTestTick() {
    if (sTestStarter != nullptr && sTestCountdown > 0 && --sTestCountdown == 0) {
        sTestStarter->appear();
        sTestStarter = nullptr;
    }
}
#endif

MovieStarter::MovieStarter(const char* pName) : LiveActor(pName), mMovieType(-1) {
}

void MovieStarter::init(const JMapInfoIter& rIter) {
    MR::connectToSceneLayoutMovement(this);
    MR::invalidateClipping(this);

    if (MR::useStageSwitchReadAppear(this, rIter)) {
        MR::listenStageSwitchOnAppear(this, MR::Functor(this, &MovieStarter::appear));
    }

    MR::useStageSwitchWriteDead(this, rIter);
    MR::createMoviePlayingSequence();
    MR::getJMapInfoArg0WithInit(rIter, &mMovieType);
    makeActorDead();
#ifdef TARGET_PC
    if (const char* env = std::getenv("PC_TEST_MOVIE"); env != nullptr && sTestStarter == nullptr) {
        sTestStarter = this;
        sTestCountdown = std::atoi(env);
    }
#endif
}

void MovieStarter::appear() {
    LiveActor::appear();

    if (mMovieType == 0) {
        if (MR::isOnGameEventFlagPlayMoviePrologueA()) {
            kill();
            return;
        }

        MR::onGameEventFlagPlayMoviePrologueA();
    }

    if (mMovieType == 1) {
        MR::onGameEventFlagPlayMoviePrologueB();
    }

    MR::startMovie(mMovieType);
}

void MovieStarter::kill() {
    if (MR::isValidSwitchDead(this)) {
        MR::onSwitchDead(this);
    }

    LiveActor::kill();
}

void MovieStarter::control() {
    if (MR::isEndMovie(mMovieType)) {
        kill();
    }
}
