#include <revolution/dvd.h>
#include "JSystem/JAudio2/JASWaveArcLoader.hpp"
#include "JSystem/JAudio2/JASDvdThread.hpp"
#include "JSystem/JAudio2/JASMutex.hpp"
#include "JSystem/JAudio2/JASTaskThread.hpp"
#include "JSystem/JKernel/JKRDvdAramRipper.hpp"
#include <cstring>
#include <stdint.h>

JASHeap* JASWaveArcLoader::sAramHeap;

JASHeap* JASWaveArcLoader::getRootHeap() {
    if (JASWaveArcLoader::sAramHeap) {
        return JASWaveArcLoader::sAramHeap;
    }
    return JASKernel::getAramHeap();
}

char JASWaveArcLoader::sCurrentDir[DIR_MAX] = "/AudioRes/Waves/";

void JASWaveArcLoader::setCurrentDir(char const* dir) {
    strcpy(sCurrentDir, dir);
    u32 len = strlen(sCurrentDir);
    if (sCurrentDir[len - 1] != '/') {
        sCurrentDir[len] = '/';
        sCurrentDir[len + 1] = '\0';
    }
}

char* JASWaveArcLoader::getCurrentDir() {
    return sCurrentDir;
}

JASWaveArc::JASWaveArc() : mHeap(this) {
    _48 = 0;
    mStatus = 0;
    _58 = 0;
    mEntryNum = -1;
    mFileLength = 0;
    _5A = 0;
    OSInitMutex(&mMutex);
}

JASWaveArc::~JASWaveArc() {
}

bool JASWaveArc::loadSetup(u32 param_0) {
    JASMutexLock mutexLock(&mMutex);
    if (_58 != param_0) {
        return false;
    }
    if (mStatus != 1) {
        return false;
    }
    _48 = 1;
    mStatus = 2;
    return true;
}

bool JASWaveArc::eraseSetup() {
    JASMutexLock mutexLock(&mMutex);
    if (mStatus == 0) {
        return false;
    }
    if (mStatus == 1) {
        mStatus = 0;
        return false;
    }
    _48 = 0;
    mStatus = 0;
    return true;
}

void JASWaveArc::loadToAramCallback(void* this_) {
    loadToAramCallbackParams* tmp = (loadToAramCallbackParams*)this_;
    JASWaveArc* wavArc = tmp->mWavArc;
#ifdef TARGET_PC
    // Wave memory is host memory on PC; read the .aw file straight into it.
    {
        DVDFileInfo info;
        if (!DVDFastOpen(tmp->mEntryNum, &info)) {
            OSReport("[audio] wave archive entry %d could not be opened\n", tmp->mEntryNum);
            return;
        }
        const s32 length = (s32)info.length;
        const s32 read = DVDReadPrio(&info, reinterpret_cast< void* >(tmp->mBase), length, 0, 2);
        DVDClose(&info);
        if (read < 0) {
            OSReport("[audio] wave archive entry %d read failed\n", tmp->mEntryNum);
            return;
        }
    }
#else
    if (JKRDvdAramRipper::loadToAram(tmp->mEntryNum, tmp->mBase, EXPAND_SWITCH_UNKNOWN0, 0, 0, nullptr) == nullptr) {
        return;
    }
#endif
    wavArc->_5A--;
    if (wavArc->loadSetup(tmp->_C)) {
        wavArc->onLoadDone();
    }
}

bool JASWaveArc::sendLoadCmd() {
    JASMutexLock mutexLock(&mMutex);
    _48 = 0;
    mStatus = 1;
    loadToAramCallbackParams commandInfo;
    commandInfo.mWavArc = this;
    commandInfo.mEntryNum = mEntryNum;
    commandInfo.mBase = (uintptr_t)mHeap.getBase();
    commandInfo._C = ++_58;

    _5A++;

    if (JASDvd::getThreadPointer()->sendCmdMsg(loadToAramCallback, &commandInfo, sizeof(commandInfo)) == 0) {  // 0x10 on the Wii
        mHeap.free();
        return false;
    }
    return true;
}

#ifndef TARGET_PC  // the game overrides this (Game/System/Overwrite*.cpp)
bool JASWaveArc::load(JASHeap* heap) {
    if (mEntryNum < 0) {
        return false;
    }
    JASMutexLock mutexLock(&mMutex);
    if (mStatus != 0) {
        return false;
    }
    if (heap == nullptr) {
        heap = JASWaveArcLoader::getRootHeap();
    }
    if (mHeap.alloc(heap, mFileLength) == false) {
        OSReport("[audio] no audio memory for wave archive %d (0x%x bytes)\n", mEntryNum, mFileLength);
        return false;
    }
    return sendLoadCmd();
}
#endif

bool JASWaveArc::loadTail(JASHeap* heap) {
    if (mEntryNum < 0) {
        return false;
    }
    JASMutexLock mutexLock(&mMutex);
    if (mStatus != 0) {
        return false;
    }
    if (heap == nullptr) {
        heap = JASWaveArcLoader::getRootHeap();
    }
    if (mHeap.allocTail(heap, mFileLength) == false) {
        return false;
    }
    return sendLoadCmd();
}

bool JASWaveArc::erase() {
    return mHeap.free();
}

void JASWaveArc::onDispose() {
    if (eraseSetup()) {
        onEraseDone();
    }
}

void JASWaveArc::setEntryNum(s32 entryNum) {
    DVDFileInfo fileInfo;
    if (entryNum < 0) {
        return;
    }
    if (DVDFastOpen(entryNum, &fileInfo) == FALSE) {
        return;
    }
    mFileLength = fileInfo.length;
    DVDClose(&fileInfo);
    mEntryNum = entryNum;
}

#ifndef TARGET_PC  // the game overrides this (Game/System/Overwrite*.cpp)
void JASWaveArc::setFileName(char const* fileName) {
    char* currentDir = JASWaveArcLoader::getCurrentDir();
    size_t length = strlen(currentDir);
    length = length + strlen(fileName);
    char* path = new (JKRHeapToken::Dummy, JASKernel::getSystemHeap(), -4) char[length + 1];
    strcpy(path, currentDir);
    strcat(path, fileName);
    path[length] = '\0';
    int entryNum = DVDConvertPathToEntrynum(path);
    if (entryNum < 0) {
        OSReport("[audio] wave archive not found: %s\n", path);
    }
    delete[] path;
    if (entryNum < 0) {
        return;
    }
    setEntryNum(entryNum);
}
#endif
