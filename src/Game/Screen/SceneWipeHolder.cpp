#include "Game/Screen/SceneWipeHolder.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Screen/WipeFade.hpp"
#include "Game/Screen/WipeGameOver.hpp"
#include "Game/Screen/WipeKoopa.hpp"
#include "Game/Screen/WipeRing.hpp"
#include "Game/Util/ObjUtil.hpp"

SceneWipeHolder::SceneWipeHolder() : WipeHolderBase(8, "シーンワイプ保持") {
    WipeLayoutBase* pWipeLayout;

    pWipeLayout = new (JKRHeapToken::Dummy) WipeRing(1, "円ワイプ");
    addWipeLayout(pWipeLayout);
    MR::connectToSceneWipeLayout(pWipeLayout);
    MR::joinToNameObjGroup(pWipeLayout, "IgnorePauseNameObj");

    pWipeLayout = new (JKRHeapToken::Dummy) WipeFade("フェードワイプ", Color8(0, 0, 0, 255));
    addWipeLayout(pWipeLayout);
    MR::connectToSceneWipeLayout(pWipeLayout);
    MR::joinToNameObjGroup(pWipeLayout, "IgnorePauseNameObj");

    pWipeLayout = new (JKRHeapToken::Dummy) WipeFade("白フェードワイプ", Color8(255, 255, 255, 255));
    addWipeLayout(pWipeLayout);
    MR::connectToSceneWipeLayout(pWipeLayout);
    MR::joinToNameObjGroup(pWipeLayout, "IgnorePauseNameObj");

    pWipeLayout = new (JKRHeapToken::Dummy) WipeGameOver();
    addWipeLayout(pWipeLayout);
    MR::connectToSceneWipeLayout(pWipeLayout);
    MR::joinToNameObjGroup(pWipeLayout, "IgnorePauseNameObj");

    pWipeLayout = new (JKRHeapToken::Dummy) WipeKoopa();
    addWipeLayout(pWipeLayout);
    MR::connectToSceneWipeLayout(pWipeLayout);
    MR::joinToNameObjGroup(pWipeLayout, "IgnorePauseNameObj");
}

namespace SceneWipeHolderFunction {
    SceneWipeHolder* getSceneWipeHolder() {
        return MR::getSceneObj< SceneWipeHolder >(SceneObj_SceneWipeHolder);
    }

    void openWipe(const char* pWipeName, s32 frame) {
        getSceneWipeHolder()->forceClose(pWipeName);
        getSceneWipeHolder()->wipe(nullptr, frame);
    }

    void closeWipe(const char* pWipeName, s32 frame) {
        getSceneWipeHolder()->forceOpen(pWipeName);
        getSceneWipeHolder()->wipe(nullptr, frame);
    }

    void forceOpenWipe(const char* pWipeName) {
        getSceneWipeHolder()->forceOpen(pWipeName);
    }

    void forceCloseWipe(const char* pWipeName) {
        getSceneWipeHolder()->forceClose(pWipeName);
    }
};  // namespace SceneWipeHolderFunction
