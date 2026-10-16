#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/AreaObj/AreaObjContainer.hpp"
#include "Game/Boss/BossAccessor.hpp"
#include "Game/Boss/SkeletalFishBabyRailHolder.hpp"
#include "Game/Boss/SkeletalFishBossRailHolder.hpp"
#include "Game/Boss/TripodBossAccesser.hpp"
#include "Game/Camera/CameraContext.hpp"
#include "Game/Camera/CameraDirector.hpp"
#include "Game/Demo/DemoDirector.hpp"
#include "Game/Demo/PrologueDirector.hpp"
#include "Game/Effect/EffectSystem.hpp"
#include "Game/Enemy/BegomanBase.hpp"
#include "Game/Enemy/KabokuriFireHolder.hpp"
#include "Game/Enemy/KameckBeamHolder.hpp"
#include "Game/Enemy/KarikariDirector.hpp"
#include "Game/Enemy/TakoHeiInkHolder.hpp"
#include "Game/GameAudio/AudBgmConductor.hpp"
#include "Game/GameAudio/AudCameraWatcher.hpp"
#include "Game/GameAudio/AudEffectDirector.hpp"
#include "Game/Gravity/PlanetGravityManager.hpp"
#include "Game/LiveActor/AllLiveActorGroup.hpp"
#include "Game/LiveActor/ClippingDirector.hpp"
#include "Game/LiveActor/LiveActorGroupArray.hpp"
#include "Game/LiveActor/MessageSensorHolder.hpp"
#include "Game/LiveActor/MirrorCamera.hpp"
#include "Game/LiveActor/SensorHitChecker.hpp"
#include "Game/LiveActor/ShadowController.hpp"
#include "Game/LiveActor/ShadowSurfaceDrawer.hpp"
#include "Game/LiveActor/ShadowVolumeDrawer.hpp"
#include "Game/LiveActor/VolumeModelDrawer.hpp"
#include "Game/Map/Air.hpp"
#include "Game/Map/CollisionDirector.hpp"
#include "Game/Map/LightDirector.hpp"
#include "Game/Map/NamePosHolder.hpp"
#include "Game/Map/OceanHomeMapCtrl.hpp"
#include "Game/Map/PlanetMapCreator.hpp"
#include "Game/Map/QuakeEffectGenerator.hpp"
#include "Game/Map/RaceManager.hpp"
#include "Game/Map/SleepControllerHolder.hpp"
#include "Game/Map/SphereSelector.hpp"
#include "Game/Map/StageSwitch.hpp"
#include "Game/Map/SunshadeMapHolder.hpp"
#include "Game/Map/SwitchWatcherHolder.hpp"
#include "Game/Map/WaterAreaHolder.hpp"
#include "Game/Map/WaterPlant.hpp"
#include "Game/MapObj/AirBubbleHolder.hpp"
#include "Game/MapObj/ArrowSwitchMultiHolder.hpp"
#include "Game/MapObj/BigBubbleHolder.hpp"
#include "Game/MapObj/BigFanHolder.hpp"
#include "Game/MapObj/ChipHolder.hpp"
#include "Game/MapObj/ClipAreaDropHolder.hpp"
#include "Game/MapObj/ClipAreaHolder.hpp"
#include "Game/MapObj/ClipFieldFillDraw.hpp"
#include "Game/MapObj/CoinHolder.hpp"
#include "Game/MapObj/CoinRotater.hpp"
#include "Game/MapObj/EarthenPipe.hpp"
#include "Game/MapObj/ElectricRailHolder.hpp"
#include "Game/MapObj/FallOutFieldDraw.hpp"
#include "Game/MapObj/FirePressureBulletHolder.hpp"
#include "Game/MapObj/GCapture.hpp"
#include "Game/MapObj/MapPartsRailGuideHolder.hpp"
#include "Game/MapObj/MiniatureGalaxyHolder.hpp"
#include "Game/MapObj/Note.hpp"
#include "Game/MapObj/PurpleCoinHolder.hpp"
#include "Game/MapObj/SpiderThread.hpp"
#include "Game/MapObj/SpinDriverPathDrawer.hpp"
#include "Game/MapObj/StarPieceDirector.hpp"
#include "Game/MapObj/WarpPod.hpp"
#include "Game/MapObj/WaterPressureBulletHolder.hpp"
#include "Game/NPC/EventDirector.hpp"
#include "Game/NPC/MiiFaceIconHolder.hpp"
#include "Game/NPC/MiiFacePartsHolder.hpp"
#include "Game/NPC/NPCDirector.hpp"
#include "Game/NPC/TalkDirector.hpp"
#include "Game/NameObj/MovementOnOffGroupHolder.hpp"
#include "Game/NameObj/NameObjExecuteHolder.hpp"
#include "Game/NameObj/NameObjGroup.hpp"
#include "Game/Player/GroupChecker.hpp"
#include "Game/Player/MarioHolder.hpp"
#include "Game/Player/PlayerEvent.hpp"
#include "Game/Ride/FluffWind.hpp"
#include "Game/Ride/PlantLeaf.hpp"
#include "Game/Ride/PlantStalk.hpp"
#include "Game/Ride/SwingRope.hpp"
#include "Game/Ride/Trapeze.hpp"
#include "Game/Scene/PlacementStateChecker.hpp"
#include "Game/Scene/SceneDataInitializer.hpp"
#include "Game/Scene/SceneNameObjMovementController.hpp"
#include "Game/Scene/ScenePlayingResult.hpp"
#include "Game/Scene/StageDataHolder.hpp"
#include "Game/Scene/StopSceneController.hpp"
#include "Game/Screen/BloomEffect.hpp"
#include "Game/Screen/BloomEffectSimple.hpp"
#include "Game/Screen/CaptureScreenDirector.hpp"
#include "Game/Screen/CenterScreenBlur.hpp"
#include "Game/Screen/CinemaFrame.hpp"
#include "Game/Screen/CometRetryButton.hpp"
#include "Game/Screen/DepthOfFieldBlur.hpp"
#include "Game/Screen/GalaxyMapController.hpp"
#include "Game/Screen/GalaxyNamePlateDrawer.hpp"
#include "Game/Screen/GameSceneLayoutHolder.hpp"
#include "Game/Screen/HeatHazeEffect.hpp"
#include "Game/Screen/ImageEffectSystemHolder.hpp"
#include "Game/Screen/InformationObserver.hpp"
#include "Game/Screen/LensFlare.hpp"
#include "Game/Screen/MoviePlayerSimple.hpp"
#include "Game/Screen/MoviePlayingSequence.hpp"
#include "Game/Screen/OdhConverter.hpp"
#include "Game/Screen/PlayerActionGuidance.hpp"
#include "Game/Screen/SceneWipeHolder.hpp"
#include "Game/Screen/ScreenAlphaCapture.hpp"
#include "Game/Screen/ScreenBlurEffect.hpp"
#include "Game/Screen/StaffRoll.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/BaseMatrixFollowTargetHolder.hpp"
#include "Game/Util/FurCtrl.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/ShareUtil.hpp"
#include "Game/Util/SingletonHolder.hpp"

SceneObjHolder::SceneObjHolder() {
    for (int i = 0; i < SceneObj_NumMax; i++) {
        mObj[i] = nullptr;
    }
}

NameObj* SceneObjHolder::create(int id) {
    NameObj* pObj = mObj[id];

    if (pObj != nullptr) {
        return pObj;
    }

    pObj = newEachObj(id);
    pObj->initWithoutIter();

    mObj[id] = pObj;

    return pObj;
}

NameObj* SceneObjHolder::getObj(int id) const {
    return mObj[id];
}

bool SceneObjHolder::isExist(int id) const {
    return mObj[id] != nullptr;
}

NameObj* SceneObjHolder::newEachObj(int id) {
    switch (id) {
    case SceneObj_SensorHitChecker:
        return new (JKRHeapToken::Dummy) SensorHitChecker("センサー当たり");
    case SceneObj_CollisionDirector:
        return new (JKRHeapToken::Dummy) CollisionDirector();
    case SceneObj_ClippingDirector:
        return new (JKRHeapToken::Dummy) ClippingDirector();
    case SceneObj_DemoDirector:
        return new (JKRHeapToken::Dummy) DemoDirector("デモ指揮");
    case SceneObj_EventDirector:
        return new (JKRHeapToken::Dummy) EventDirector();
    case SceneObj_EffectSystem:
        return new (JKRHeapToken::Dummy) EffectSystem("エフェクトシステム", true);
    case SceneObj_LightDirector:
        return new (JKRHeapToken::Dummy) LightDirector();
    case SceneObj_SceneDataInitializer:
        return new (JKRHeapToken::Dummy) SceneDataInitializer();
    case SceneObj_StageDataHolder:
        return new (JKRHeapToken::Dummy) StageDataHolder(MR::getCurrentStageName(), 0, true);
    case SceneObj_MessageSensorHolder:
        return new (JKRHeapToken::Dummy) MessageSensorHolder("システム汎用センサー");
    case SceneObj_StageSwitchContainer:
        return new (JKRHeapToken::Dummy) StageSwitchContainer();
    case SceneObj_SwitchWatcherHolder:
        return new (JKRHeapToken::Dummy) SwitchWatcherHolder();
    case SceneObj_SleepControllerHolder:
        return new (JKRHeapToken::Dummy) SleepControllerHolder();
    case SceneObj_AreaObjContainer:
        return new (JKRHeapToken::Dummy) AreaObjContainer("エリアオブジェクトコンテナ管理");
    case SceneObj_LiveActorGroupArray:
        return new (JKRHeapToken::Dummy) LiveActorGroupArray("オブジェクトグループ");
    case SceneObj_MovementOnOffGroupHolder:
        return new (JKRHeapToken::Dummy) MovementOnOffGroupHolder("Movementグループ管理");
    case SceneObj_CaptureScreenActor:
        return new (JKRHeapToken::Dummy) CaptureScreenActor(MR::DrawType_CaptureScreenIndirect, "Indirect");
    case SceneObj_AudCameraWatcher:
        return new (JKRHeapToken::Dummy) AudCameraWatcher();
    case SceneObj_AudEffectDirector:
        return new (JKRHeapToken::Dummy) AudEffectDirector();
    case SceneObj_AudBgmConductor:
        return new (JKRHeapToken::Dummy) AudBgmConductor();
    case SceneObj_MarioHolder:
        return new (JKRHeapToken::Dummy) MarioHolder();
    case SceneObj_MirrorCamera:
        return new (JKRHeapToken::Dummy) MirrorCamera("鏡用カメラ");
    case SceneObj_CameraContext:
        return new (JKRHeapToken::Dummy) CameraContext();
    case SceneObj_NameObjGroup:
        return new (JKRHeapToken::Dummy) NameObjGroup("IgnorePauseNameObj", 16);
    case SceneObj_TalkDirector:
        return new (JKRHeapToken::Dummy) TalkDirector("会話ディレクター");
    case SceneObj_EventSequencer:
        return new (JKRHeapToken::Dummy) EventSequencer();
    case SceneObj_StopSceneController:
        return new (JKRHeapToken::Dummy) StopSceneController();
    case SceneObj_SceneNameObjMovementController:
        return new (JKRHeapToken::Dummy) SceneNameObjMovementController();
    case SceneObj_ImageEffectSystemHolder:
        return new (JKRHeapToken::Dummy) ImageEffectSystemHolder();
    case SceneObj_BloomEffect:
        return new (JKRHeapToken::Dummy) BloomEffect("ブルーム");
    case SceneObj_BloomEffectSimple:
        return new (JKRHeapToken::Dummy) BloomEffectSimple();
    case SceneObj_ScreenBlurEffect:
        return new (JKRHeapToken::Dummy) ScreenBlurEffect("画面ブラー");
    case SceneObj_DepthOfFieldBlur:
        return new (JKRHeapToken::Dummy) DepthOfFieldBlur("被写界深度ブラー");
    case SceneObj_SceneWipeHolder:
        return new (JKRHeapToken::Dummy) SceneWipeHolder();
    case SceneObj_PlayerActionGuidance:
        return new (JKRHeapToken::Dummy) PlayerActionGuidance();
    case SceneObj_ScenePlayingResult:
        return new (JKRHeapToken::Dummy) ScenePlayingResult();
    case SceneObj_LensFlareDirector:
        return new (JKRHeapToken::Dummy) LensFlareDirector();
    case SceneObj_FurDrawManager:
        return new (JKRHeapToken::Dummy) FurDrawManager(64);
    case SceneObj_PlacementStateChecker:
        return new (JKRHeapToken::Dummy) PlacementStateChecker("オブジェクト配置状態の監視");
    case SceneObj_NamePosHolder:
        return new (JKRHeapToken::Dummy) NamePosHolder();
    case SceneObj_NPCDirector:
        return new (JKRHeapToken::Dummy) NPCDirector();
    case SceneObj_ResourceShare:
        return new (JKRHeapToken::Dummy) ResourceShare();
    case SceneObj_MoviePlayerSimple:
        return new (JKRHeapToken::Dummy) MoviePlayerSimple();
    case SceneObj_InformationObserver:
        return new (JKRHeapToken::Dummy) InformationObserver();
    case SceneObj_CenterScreenBlur:
        return new (JKRHeapToken::Dummy) CenterScreenBlur();
    case SceneObj_OdhConverter:
        return new (JKRHeapToken::Dummy) OdhConverter();
    case SceneObj_CometRetryButton:
        return new (JKRHeapToken::Dummy) CometRetryButton("コメットリトライボタン");
    case SceneObj_AllLiveActorGroup:
        return new (JKRHeapToken::Dummy) AllLiveActorGroup();
    case SceneObj_CameraDirector:
        return new (JKRHeapToken::Dummy) CameraDirector("カメラ管理");
    case SceneObj_PlanetGravityManager:
        return new (JKRHeapToken::Dummy) PlanetGravityManager("重力");
    case SceneObj_BaseMatrixFollowTargetHolder:
        return new (JKRHeapToken::Dummy) BaseMatrixFollowTargetHolder("行列追随先リスト", 256, 256);
    case SceneObj_GameSceneLayoutHolder:
        return new (JKRHeapToken::Dummy) GameSceneLayoutHolder();
    case SceneObj_TripodBossAccesser:
        return new (JKRHeapToken::Dummy) TripodBossAccesser("三脚ボスアクセサ");
    case SceneObj_KameckBeamHolder:
        return new (JKRHeapToken::Dummy) KameckBeamHolder();
    case SceneObj_KameckFireBallHolder:
        return new (JKRHeapToken::Dummy) KameckFireBallHolder();
    case SceneObj_KameckBeamTurtleHolder:
        return new (JKRHeapToken::Dummy) KameckBeamTurtleHolder();
    case SceneObj_KabokuriFireHolder:
        return new (JKRHeapToken::Dummy) KabokuriFireHolder();
    case SceneObj_TakoHeiInkHolder:
        return new (JKRHeapToken::Dummy) TakoHeiInkHolder();
    case SceneObj_SwingRopeGroup:
        return new (JKRHeapToken::Dummy) SwingRopeGroup("スイングロープ描画");
    case SceneObj_CoinHolder:
        return new (JKRHeapToken::Dummy) CoinHolder("コイン管理");
    case SceneObj_PurpleCoinHolder:
        return new (JKRHeapToken::Dummy) PurpleCoinHolder();
    case SceneObj_CoinRotater:
        return new (JKRHeapToken::Dummy) CoinRotater("コイン回転管理");
    case SceneObj_AirBubbleHolder:
        return new (JKRHeapToken::Dummy) AirBubbleHolder("空気アワ管理");
    case SceneObj_StarPieceDirector:
        return new (JKRHeapToken::Dummy) StarPieceDirector("スターピース指揮");
    case SceneObj_BegomanAttackPermitter:
        return new (JKRHeapToken::Dummy) BegomanAttackPermitter("ベーゴマン攻撃許可者");
    case SceneObj_BigFanHolder:
        return new (JKRHeapToken::Dummy) BigFanHolder();
    case SceneObj_KarikariDirector:
        return new (JKRHeapToken::Dummy) KarikariDirector("カリカリディレクター");
    case SceneObj_ShadowControllerHolder:
        return new (JKRHeapToken::Dummy) ShadowControllerHolder();
    case SceneObj_ShadowVolumeDrawInit:
        return new (JKRHeapToken::Dummy) ShadowVolumeDrawInit();
    case SceneObj_ShadowSurfaceDrawInit:
        return new (JKRHeapToken::Dummy) ShadowSurfaceDrawInit("水面影描画初期化");
    case SceneObj_PlantStalkDrawInit:
        return new (JKRHeapToken::Dummy) PlantStalkDrawInit("植物の茎描画初期化");
    case SceneObj_PlantLeafDrawInit:
        return new (JKRHeapToken::Dummy) PlantLeafDrawInit("描画初期化[植物の葉]");
    case SceneObj_TrapezeRopeDrawInit:
        return new (JKRHeapToken::Dummy) TrapezeRopeDrawInit("空中ブランコロープ描画");
    case SceneObj_VolumeModelDrawInit:
        return new (JKRHeapToken::Dummy) VolumeModelDrawInit();
    case SceneObj_SpinDriverPathDrawInit:
        return new (JKRHeapToken::Dummy) SpinDriverPathDrawInit();
    case SceneObj_NoteGroup:
        return new (JKRHeapToken::Dummy) NoteGroup();
    case SceneObj_ClipAreaHolder:
        return new (JKRHeapToken::Dummy) ClipAreaHolder("クリップエリアホルダー");
    case SceneObj_ArrowSwitchMultiHolder:
        return new (JKRHeapToken::Dummy) ArrowSwitchMultiHolder();
    case SceneObj_ClipAreaDropHolder:
        return new (JKRHeapToken::Dummy) ClipAreaDropHolder();
    case SceneObj_FallOutFieldDraw:
        return new (JKRHeapToken::Dummy) FallOutFieldDraw("クリップエリア描画[抜き]");
    case SceneObj_ClipFieldFillDraw:
        return new (JKRHeapToken::Dummy) ClipFieldFillDraw("クリップエリア描画[塗りつぶし]");
    case SceneObj_ScreenAlphaCapture:
        return new (JKRHeapToken::Dummy) ScreenAlphaCapture("アルファテクスチャ取り込み");
    case SceneObj_MapPartsRailGuideHolder:
        return new (JKRHeapToken::Dummy) MapPartsRailGuideHolder();
    case SceneObj_GCapture:
        return new (JKRHeapToken::Dummy) GCapture("Gキャプチャー");
    case SceneObj_NameObjExecuteHolder:
        return new (JKRHeapToken::Dummy) NameObjExecuteHolder(4096);
    case SceneObj_ElectricRailHolder:
        return new (JKRHeapToken::Dummy) ElectricRailHolder("電撃レール保持");
    case SceneObj_SpiderThread:
        return new (JKRHeapToken::Dummy) SpiderThread("クモの巣");
    case SceneObj_QuakeEffectGenerator:
        return new (JKRHeapToken::Dummy) QuakeEffectGenerator();
    case SceneObj_HeatHazeDirector:
        return new (JKRHeapToken::Dummy) HeatHazeDirector("陽炎制御");
    case SceneObj_BlueChipHolder:
        return new (JKRHeapToken::Dummy) ChipHolder("ブルーチップホルダー", 0);
    case SceneObj_YellowChipHolder:
        return new (JKRHeapToken::Dummy) ChipHolder("イエローーチップホルダー", 1);
    case SceneObj_BigBubbleHolder:
        return new (JKRHeapToken::Dummy) BigBubbleHolder("オオアワホルダー");
    case SceneObj_EarthenPipeMediator:
        return new (JKRHeapToken::Dummy) EarthenPipeMediator();
    case SceneObj_WaterAreaHolder:
        return new (JKRHeapToken::Dummy) WaterAreaHolder();
    case SceneObj_WaterPlantDrawInit:
        return new (JKRHeapToken::Dummy) WaterPlantDrawInit();
    case SceneObj_OceanHomeMapCtrl:
        return new (JKRHeapToken::Dummy) OceanHomeMapCtrl();
    case SceneObj_RaceManager:
        return new (JKRHeapToken::Dummy) RaceManager();
    case SceneObj_GroupCheckManager:
        return new (JKRHeapToken::Dummy) GroupCheckManager("属性グループマネージャー");
    case SceneObj_SkeletalFishBabyRailHolder:
        return new (JKRHeapToken::Dummy) SkeletalFishBabyRailHolder("スカルシャークベビーレール管理");
    case SceneObj_SkeletalFishBossRailHolder:
        return new (JKRHeapToken::Dummy) SkeletalFishBossRailHolder("スカルシャークボスレール管理");
    case SceneObj_WaterPressureBulletHolder:
        return new (JKRHeapToken::Dummy) WaterPressureBulletHolder("ウォータープレッシャー玉ホルダ−");
    case SceneObj_FirePressureBulletHolder:
        return new (JKRHeapToken::Dummy) FirePressureBulletHolder("ファイアプレッシャー玉ホルダ−");
    case SceneObj_SunshadeMapHolder:
        return new (JKRHeapToken::Dummy) SunshadeMapHolder();
    case SceneObj_MiiFacePartsHolder:
        return new (JKRHeapToken::Dummy) MiiFacePartsHolder(128);
    case SceneObj_MiiFaceIconHolder:
        return new (JKRHeapToken::Dummy) MiiFaceIconHolder(16, "Miiアイコン保持管理");
    case SceneObj_FluffWindHolder:
        return new (JKRHeapToken::Dummy) FluffWindHolder();
    case SceneObj_SphereSelector:
        return new (JKRHeapToken::Dummy) SphereSelector();
    case SceneObj_GalaxyNamePlateDrawer:
        return new (JKRHeapToken::Dummy) GalaxyNamePlateDrawer();
    case SceneObj_CinemaFrame:
        return new (JKRHeapToken::Dummy) CinemaFrame(true);
    case SceneObj_BossAccessor:
        return new (JKRHeapToken::Dummy) BossAccessor();
    case SceneObj_MiniatureGalaxyHolder:
        return new (JKRHeapToken::Dummy) MiniatureGalaxyHolder();
    case SceneObj_PlanetMapCreator:
        return new (JKRHeapToken::Dummy) PlanetMapCreator("惑星クリエイタ");
    case SceneObj_WarpPodMgr:
        return new (JKRHeapToken::Dummy) WarpPodMgr("ワープポッド管理局");
    case SceneObj_PriorDrawAirHolder:
        return new (JKRHeapToken::Dummy) PriorDrawAirHolder();
    case SceneObj_GalaxyMapController:
        return new (JKRHeapToken::Dummy) GalaxyMapController();
    case SceneObj_MoviePlayingSequenceHolder:
        return new (JKRHeapToken::Dummy) MoviePlayingSequenceHolder("ムービー管理保持");
    case SceneObj_PrologueHolder:
        return new (JKRHeapToken::Dummy) PrologueHolder("プロローグ保持");
    case SceneObj_StaffRoll:
        return new (JKRHeapToken::Dummy) StaffRoll("スタッフロール");
    default:
        return nullptr;
    }
}

namespace MR {
    NameObj* createSceneObj(int id) {
        return getSceneObjHolder()->create(id);
    }

    SceneObjHolder* getSceneObjHolder() {
        return SingletonHolder< GameSystem >::get()->mSceneController->getSceneObjHolder();
    }

    bool isExistSceneObj(int id) {
        GameSystemSceneController* pSceneController = SingletonHolder< GameSystem >::get()->mSceneController;

        if (pSceneController == nullptr) {
            return false;
        }

        if (!pSceneController->isExistSceneObjHolder()) {
            return false;
        }

        return MR::getSceneObjHolder()->isExist(id);
    }
};  // namespace MR
