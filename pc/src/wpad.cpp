// WPAD / KPAD (Wii Remote) SDK replacement backed by pc::input.
#include "pc/input.h"
#include "pc/platform.h"

#include <revolution/kpad.h>
#include <revolution/wpad.h>

namespace {
WPADConnectCallback sConnectCallback[WPAD_MAX_CONTROLLERS];
WPADExtensionCallback sExtensionCallback[WPAD_MAX_CONTROLLERS];
bool sAnnounced = false;

// Remote 0 (with a Nunchuk) is always connected; tell the game once callbacks exist.
void announceConnection() {
    if (sAnnounced) {
        return;
    }
    if (sConnectCallback[0] != nullptr) {
        sAnnounced = true;
        sConnectCallback[0](0, WPAD_ERR_NONE);
        if (sExtensionCallback[0] != nullptr) {
            sExtensionCallback[0](0, WPAD_DEV_FREESTYLE);
        }
    }
}
}  // namespace

extern "C" {

void KPADInit() {}
void KPADReset(void) {}
void KPADSetBtnRepeat(s32, f32, f32) {}
void KPADSetSensorHeight(s32, f32) {}
void KPADSetAccParam(s32, f32, f32) {}
void KPADSetPosParam(s32, f32, f32) {}
void KPADSetHoriParam(s32, f32, f32) {}
void KPADSetDistParam(s32, f32, f32) {}
void KPADEnableDPD(s32) {}
void KPADDisableDPD(s32) {}

s32 KPADRead(s32 chan, KPADStatus status[], u32 count) {
    announceConnection();
    return pc::input::readKPAD(chan, status, (int)count);
}

void WPADRegisterAllocator(WPADAlloc, WPADFree) {}

u32 WPADGetWorkMemorySize(void) {
    return 0x1000;
}

void WPADDisconnect(s32) {}

s32 WPADProbe(s32 chan, u32* type) {
    if (chan != 0) {
        if (type) *type = WPAD_DEV_NOT_FOUND;
        return WPAD_ERR_NO_CONTROLLER;
    }
    if (type) *type = WPAD_DEV_FREESTYLE;
    return WPAD_ERR_NONE;
}

s32 WPADGetInfoAsync(s32 chan, WPADInfo* info, WPADCallback cb) {
    if (chan != 0) {
        if (cb) cb(chan, WPAD_ERR_NO_CONTROLLER);
        return WPAD_ERR_NO_CONTROLLER;
    }
    if (info) {
        info->dpd = TRUE;
        info->speaker = FALSE;
        info->attach = TRUE;
        info->lowBat = FALSE;
        info->nearempty = FALSE;
        info->battery = 4;
        info->led = 1;
        info->protocol = 0;
        info->firmware = 0;
    }
    if (cb) cb(chan, WPAD_ERR_NONE);
    return WPAD_ERR_NONE;
}

// The remote's speaker isn't emulated; the game plays those sounds through the TV instead.
BOOL WPADIsSpeakerEnabled(s32) {
    return FALSE;
}

s32 WPADControlSpeaker(s32 chan, u32, WPADCallback cb) {
    if (cb) cb(chan, WPAD_ERR_NONE);
    return WPAD_ERR_NONE;
}

u8 WPADGetSpeakerVolume(void) {
    return 0;
}

s32 WPADSendStreamData(s32, void*, u16) {
    return WPAD_ERR_NONE;
}

BOOL WPADCanSendStreamData(s32) {
    return FALSE;
}

void WPADControlMotor(s32 chan, u32 command) {
    pc::input::setRumble(chan, command != 0);
}

void WPADSetAutoSleepTime(u8) {}

u8 WPADGetSensorBarPosition(void) {
    return 0;
}

WPADExtensionCallback WPADSetExtensionCallback(s32 chan, WPADExtensionCallback cb) {
    if (chan < 0 || chan >= WPAD_MAX_CONTROLLERS) return nullptr;
    WPADExtensionCallback old = sExtensionCallback[chan];
    sExtensionCallback[chan] = cb;
    return old;
}

WPADConnectCallback WPADSetConnectCallback(s32 chan, WPADConnectCallback cb) {
    if (chan < 0 || chan >= WPAD_MAX_CONTROLLERS) return nullptr;
    WPADConnectCallback old = sConnectCallback[chan];
    sConnectCallback[chan] = cb;
    return old;
}

}  // extern "C"
