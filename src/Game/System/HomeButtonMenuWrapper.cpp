#include "Game/System/HomeButtonMenuWrapper.hpp"
#include "Game/Util/FileUtil.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include <JSystem/JKernel/JKRExpHeap.hpp>
#include <revolution/rso.h>

void (*HBMCreateRSO)(const HBMDataInfo*);
void (*HBMInitRSO)(void);
void (*HBMCalcRSO)(const HBMControllerData*);
void (*HBMDrawRSO)(void);
HBMSelectBtnNum (*HBMGetSelectBtnNumRSO)(void);
void (*HBMSetAdjustFlagRSO)(int);
void (*HBMStartBlackOutRSO)(void);

static RSOExportFuncTable exp_tbl[] = {{"HBMCreateRSO", (u32*)&HBMCreateRSO},
                                       {"HBMInitRSO", (u32*)&HBMInitRSO},
                                       {"HBMCalcRSO", (u32*)&HBMCalcRSO},
                                       {"HBMDrawRSO", (u32*)&HBMDrawRSO},
                                       {"HBMGetSelectBtnNumRSO", (u32*)&HBMGetSelectBtnNumRSO},
                                       {"HBMSetAdjustFlagRSO", (u32*)&HBMSetAdjustFlagRSO},
                                       {"HBMStartBlackOutRSO", (u32*)&HBMStartBlackOutRSO}};

typedef void (*ProloguePtr)(BOOL);

#ifdef TARGET_PC
// The HOME button menu is a PowerPC relocatable module (RSO); it can't run here. The PC build uses its own
// menu (F1), so the HOME menu calls do nothing and report "back to the game" straight away.
void RSO::setupRsoHomeButtonMenu() {
}
void RSO::HBMCreate(const HBMDataInfo*) {
}
void RSO::HBMInit() {
}
void RSO::HBMCalc(const HBMControllerData*) {
}
void RSO::HBMDraw() {
}
HBMSelectBtnNum RSO::HBMGetSelectBtnNum() {
    return HBM_SELECT_HOMEBTN;
}
void RSO::HBMSetAdjustFlag(int) {
}
void RSO::HBMStartBlackOut() {
}
#else
void RSO::setupRsoHomeButtonMenu() {
    u32 i;
    RSOObjectHeader* rsoPtr;
    RSOExportFuncTable* pTbl;
    const RSOObjectHeader* symbolTable = reinterpret_cast< const RSOObjectHeader* >(MR::receiveFile("/ModuleData/product.sel"));
    int jumpCodeSize;
    void* bss;
    void* jumps;

    RSOListInit((void*)symbolTable);
    if (symbolTable != nullptr) {
        jumpCodeSize = RSOGetJumpCodeSize(symbolTable);

        if (jumpCodeSize == 0) {
            jumps = nullptr;
        } else {
            jumps = new (JKRHeapToken::Dummy, MR::getStationedHeapGDDR3(), 0) char[jumpCodeSize];
            RSOMakeJumpCode(symbolTable, jumps);
        }
        rsoPtr = reinterpret_cast< RSOObjectHeader* >(MR::receiveFile("/ModuleData/HomeButtonMenuWrapperRSO.rso"));
        if (rsoPtr->mBssSize) {
            bss = new (JKRHeapToken::Dummy, MR::getStationedHeapGDDR3(), 0) char[rsoPtr->mBssSize];
        }

        RSOLinkList((void*)rsoPtr, bss);

        if (rsoPtr != nullptr) {
            RSOLinkJump(rsoPtr, symbolTable, jumps);
            reinterpret_cast< ProloguePtr >(rsoPtr->mProlog)(RSOIsImportSymbolResolvedAll(rsoPtr));
            for (i = 0; i < ARRAY_SIZE(exp_tbl); i++) {
                pTbl = &exp_tbl[i];
                RSOFindExportSymbolAddr(rsoPtr, pTbl->symbol_name);
                *(pTbl->symbol_ptr) = (u32)(uintptr_t)RSOFindExportSymbolAddr(rsoPtr, pTbl->symbol_name);
            }
        }
    }
}

void RSO::HBMCreate(const HBMDataInfo* pHBInfo) {
    (*HBMCreateRSO)(pHBInfo);
}

void RSO::HBMInit() {
    (*HBMInitRSO)();
}

void RSO::HBMCalc(const HBMControllerData* pController) {
    (*HBMCalcRSO)(pController);
}

void RSO::HBMDraw() {
    (*HBMDrawRSO)();
}

HBMSelectBtnNum RSO::HBMGetSelectBtnNum() {
    return (*HBMGetSelectBtnNumRSO)();
}

void RSO::HBMSetAdjustFlag(int flag) {
    (*HBMSetAdjustFlagRSO)(flag);
}

void RSO::HBMStartBlackOut() {
    (*HBMStartBlackOutRSO)();
}

#endif

void _unresolved(void) {
}
