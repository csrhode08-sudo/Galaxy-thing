// Wii SDK services that have no counterpart in Aurora: NAND (save files) on the host filesystem, system
// configuration, and inert stand-ins for features that do not exist on PC (Mii channel data, WiiConnect24
// messages, the HOME menu module loader, the Wii Speaker encoder, the DSP).
#include <revolution/dvd.h>
#include "pc/platform.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>
#include <system_error>

typedef int32_t s32;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef float f32;
typedef int BOOL;

namespace fs = std::filesystem;

// ---- NAND -------------------------------------------------------------------------------------------------
// Save files live in <user dir>/save/. The game's NAND home directory maps onto that folder.

namespace {

constexpr s32 NAND_RESULT_OK = 0;
constexpr s32 NAND_RESULT_ACCESS = -1;
constexpr s32 NAND_RESULT_EXISTS = -6;
constexpr s32 NAND_RESULT_INVALID = -8;
constexpr s32 NAND_RESULT_MAXFD = -10;
constexpr s32 NAND_RESULT_NOEXISTS = -12;
constexpr s32 NAND_RESULT_UNKNOWN = -64;
constexpr int NAND_MAX_PATH = 64;
constexpr const char* kHomeDir = "/title/00010000/524d474b/data";

struct NANDFileInfo {
    s32 fileDescriptor;
    s32 origFd;
    char origPath[NAND_MAX_PATH];
    char tmpPath[NAND_MAX_PATH];
    u8 accType;
    u8 stage;
    u8 mark;
};

std::mutex sNandLock;
FILE* sFiles[32];

fs::path saveRoot() {
    fs::path p = fs::path(pc::userDir()) / "save";
    std::error_code ec;
    fs::create_directories(p, ec);
    return p;
}

fs::path hostPath(const char* nandPath) {
    std::string p = nandPath ? nandPath : "";
    const std::string home = kHomeDir;
    if (p.compare(0, home.size(), home) == 0) {
        p = p.substr(home.size());
    }
    while (!p.empty() && p[0] == '/') {
        p.erase(0, 1);
    }
    return saveRoot() / p;
}

}  // namespace

extern "C" {

s32 NANDGetHomeDir(char* path) {
    std::strncpy(path, kHomeDir, NAND_MAX_PATH);
    return NAND_RESULT_OK;
}

s32 NANDCheck(u32, u32, u32* answer) {
    if (answer) *answer = 0;
    return NAND_RESULT_OK;
}

s32 NANDCreate(const char* path, u8, u8) {
    std::lock_guard<std::mutex> lk(sNandLock);
    fs::path p = hostPath(path);
    std::error_code ec;
    if (fs::exists(p, ec)) {
        return NAND_RESULT_EXISTS;
    }
    fs::create_directories(p.parent_path(), ec);
    FILE* f = std::fopen(p.string().c_str(), "wb");
    if (!f) return NAND_RESULT_ACCESS;
    std::fclose(f);
    return NAND_RESULT_OK;
}

s32 NANDDelete(const char* path) {
    std::lock_guard<std::mutex> lk(sNandLock);
    std::error_code ec;
    fs::path p = hostPath(path);
    if (!fs::exists(p, ec)) return NAND_RESULT_NOEXISTS;
    fs::remove_all(p, ec);
    return ec ? NAND_RESULT_ACCESS : NAND_RESULT_OK;
}

s32 NANDMove(const char* path, const char* destDir) {
    std::lock_guard<std::mutex> lk(sNandLock);
    std::error_code ec;
    fs::path from = hostPath(path);
    fs::path to = hostPath(destDir) / from.filename();
    if (!fs::exists(from, ec)) return NAND_RESULT_NOEXISTS;
    fs::create_directories(to.parent_path(), ec);
    fs::rename(from, to, ec);
    if (ec) {  // e.g. the destination exists
        fs::remove(to, ec);
        fs::rename(from, to, ec);
    }
    return ec ? NAND_RESULT_ACCESS : NAND_RESULT_OK;
}

s32 NANDOpen(const char* path, NANDFileInfo* info, u8 mode) {
    std::lock_guard<std::mutex> lk(sNandLock);
    fs::path p = hostPath(path);
    std::error_code ec;
    if (!fs::exists(p, ec)) return NAND_RESULT_NOEXISTS;
    const char* m = mode == 1 ? "rb" : "r+b";  // 1 = read, 2 = write, 3 = read/write
    int fd = -1;
    for (int i = 0; i < 32; i++) {
        if (!sFiles[i]) {
            fd = i;
            break;
        }
    }
    if (fd < 0) return NAND_RESULT_MAXFD;
    FILE* f = std::fopen(p.string().c_str(), m);
    if (!f) return NAND_RESULT_ACCESS;
    sFiles[fd] = f;
    std::memset(info, 0, sizeof(*info));
    info->fileDescriptor = fd;
    info->accType = mode;
    std::strncpy(info->origPath, path ? path : "", NAND_MAX_PATH - 1);
    return NAND_RESULT_OK;
}

s32 NANDClose(NANDFileInfo* info) {
    std::lock_guard<std::mutex> lk(sNandLock);
    if (!info || info->fileDescriptor < 0 || info->fileDescriptor >= 32 || !sFiles[info->fileDescriptor]) {
        return NAND_RESULT_INVALID;
    }
    std::fclose(sFiles[info->fileDescriptor]);
    sFiles[info->fileDescriptor] = nullptr;
    info->fileDescriptor = -1;
    return NAND_RESULT_OK;
}

s32 NANDRead(NANDFileInfo* info, void* buf, u32 len) {
    std::lock_guard<std::mutex> lk(sNandLock);
    if (!info || info->fileDescriptor < 0 || info->fileDescriptor >= 32 || !sFiles[info->fileDescriptor]) {
        return NAND_RESULT_INVALID;
    }
    return (s32)std::fread(buf, 1, len, sFiles[info->fileDescriptor]);
}

s32 NANDWrite(NANDFileInfo* info, const void* buf, u32 len) {
    std::lock_guard<std::mutex> lk(sNandLock);
    if (!info || info->fileDescriptor < 0 || info->fileDescriptor >= 32 || !sFiles[info->fileDescriptor]) {
        return NAND_RESULT_INVALID;
    }
    s32 n = (s32)std::fwrite(buf, 1, len, sFiles[info->fileDescriptor]);
    std::fflush(sFiles[info->fileDescriptor]);
    return n;
}

s32 NANDGetLength(NANDFileInfo* info, u32* length) {
    std::lock_guard<std::mutex> lk(sNandLock);
    if (!info || info->fileDescriptor < 0 || info->fileDescriptor >= 32 || !sFiles[info->fileDescriptor]) {
        return NAND_RESULT_INVALID;
    }
    FILE* f = sFiles[info->fileDescriptor];
    long pos = std::ftell(f);
    std::fseek(f, 0, SEEK_END);
    *length = (u32)std::ftell(f);
    std::fseek(f, pos, SEEK_SET);
    return NAND_RESULT_OK;
}

// The banner shown in the Wii's data management screen; only its header matters here.
void NANDInitBanner(void* banner, u32 flag, const u16* title, const u16* comment) {
    u8* b = static_cast<u8*>(banner);
    std::memset(b, 0, 0x60);
    const u32 sig = 0x5749424E;  // 'WIBN', big-endian like the console
    b[0] = sig >> 24;
    b[1] = sig >> 16;
    b[2] = sig >> 8;
    b[3] = sig;
    b[4] = flag >> 24;
    b[5] = flag >> 16;
    b[6] = flag >> 8;
    b[7] = flag;
    u16* c = reinterpret_cast<u16*>(b + 0x20);
    for (int i = 0; title && i < 32 && title[i]; i++) c[i] = title[i];
    for (int i = 0; comment && i < 32 && comment[i]; i++) c[32 + i] = comment[i];
}

// ---- System configuration ---------------------------------------------------------------------------------

u8 SCGetLanguage(void) {
    // English when its text is available (the UsEnglish folder from the US disc, added as an overlay),
    // otherwise Korean, the language of the RMGK01 data this build targets.
    static int sLanguage = -1;
    if (sLanguage < 0) {
        sLanguage = DVDConvertPathToEntrynum("/UsEnglish/MessageData/Message.arc") >= 0 ? 1 : 9;
    }
    return (u8)sLanguage;
}
u8 SCGetAspectRatio(void) {
    return 1;  // 16:9
}
u8 SCGetProgressiveMode(void) {
    return 1;
}
u8 SCGetEuRgb60Mode(void) {
    return 0;
}
u8 SCGetSoundMode(void) {
    return 1;  // stereo
}

// ---- Misc OS / GX -----------------------------------------------------------------------------------------

void OSRegisterVersion(const char*) {}
void PPCSync(void) {}
void GXPeekARGB(u16, u16, u32* color) {
    if (color) *color = 0;
}
void GXPokeAlphaRead(int) {}
void GXSetScissorBoxOffset(s32, s32) {}

// Generic MEM allocator dispatch (MEMAllocator = { funcs*, heap, param1, param2 }).
struct MEMAllocatorFunc;
struct MEMAllocator {
    const MEMAllocatorFunc* pFunc;
    void* pHeap;
    u32 heapParam1;
    u32 heapParam2;
};
struct MEMAllocatorFunc {
    void* (*pfAlloc)(MEMAllocator*, u32);
    void (*pfFree)(MEMAllocator*, void*);
};
void* MEMAllocFromAllocator(MEMAllocator* a, u32 size) {
    return a->pFunc->pfAlloc(a, size);
}
void MEMFreeToAllocator(MEMAllocator* a, void* p) {
    a->pFunc->pfFree(a, p);
}

// ---- ARAM (the Wii emulates it in MEM2) -------------------------------------------------------------------

void* ARGetStorageAddress(void);
u32 ARGetBaseAddress(void) {
    return 0x4000;
}
void ARStartDMA(u32 type, u32 mainmem, u32 aram, u32 len) {
    // Only reachable with 32-bit addresses, which host pointers are not; JKRAramPiece::startDMA handles DMA on PC.
    (void)type;
    (void)mainmem;
    (void)aram;
    (void)len;
    STUB_LOG();
}

// ---- Mii (RVLFaceLib): no Mii data on PC; the game falls back to its own icons ------------------------------

enum { RFLErrcode_Success = 0, RFLErrcode_NotAvailable = 1 };
s32 RFLInitResAsync(void*, void*, u32, BOOL) {
    return RFLErrcode_NotAvailable;
}
s32 RFLGetAsyncStatus(void) {
    return RFLErrcode_NotAvailable;
}
s32 RFLGetLastReason(void) {
    return 0;
}
void RFLExit(void) {}
u32 RFLGetWorkSize(BOOL) {
    return 32;
}
u32 RFLGetModelBufferSize(int, u32) {
    return 32;
}
BOOL RFLIsAvailableOfficialData(u16) {
    return 0;
}
BOOL RFLSearchOfficialData(const void*, u16*) {
    return 0;
}
s32 RFLGetAdditionalInfo(void*, int, const void*, u16) {
    return RFLErrcode_NotAvailable;
}
s32 RFLInitCharModel(void*, int, void*, u16, void*, int, u32) {
    return RFLErrcode_NotAvailable;
}
s32 RFLMakeIcon(void*, int, void*, u16, int, const void*) {
    return RFLErrcode_NotAvailable;
}
void RFLSetExpression(void*, int) {}
void RFLSetMtx(void*, const void*) {}
void RFLDrawOpa(const void*) {}
void RFLDrawXlu(const void*) {}
void RFLDrawOpaCore(const void*, const void*) {}
void RFLDrawXluCore(const void*, const void*) {}
void RFLLoadVertexSetting(const void*) {}
void RFLLoadMaterialSetting(const void*) {}

// ---- WiiConnect24 (letters to the Wii Message Board): unavailable ------------------------------------------

constexpr s32 NWC24_ERR_FATAL = -1;
s32 NWC24OpenLib(void*) {
    return NWC24_ERR_FATAL;
}
s32 NWC24CloseLib(void) {
    return 0;
}
s32 NWC24GetErrorCode(void) {
    return 0;
}
s32 NWC24GetMyUserId(uint64_t* id) {
    if (id) *id = 0;
    return NWC24_ERR_FATAL;
}
s32 NWC24InitMsgObj(void*, int) {
    return NWC24_ERR_FATAL;
}
s32 NWC24CommitMsg(void*) {
    return NWC24_ERR_FATAL;
}
s32 NWC24GetMsgSize(const void*, u32* size) {
    if (size) *size = 0;
    return NWC24_ERR_FATAL;
}
s32 NWC24SetMsgAltName(void*, const u16*, u32) {
    return NWC24_ERR_FATAL;
}
s32 NWC24SetMsgAttached(void*, const char*, u32, int) {
    return NWC24_ERR_FATAL;
}
s32 NWC24SetMsgLedPattern(void*, u16) {
    return NWC24_ERR_FATAL;
}
s32 NWC24SetMsgMBDelay(void*, u8) {
    return NWC24_ERR_FATAL;
}
s32 NWC24SetMsgMBNoReply(void*, BOOL) {
    return NWC24_ERR_FATAL;
}
s32 NWC24SetMsgTag(void*, u16) {
    return NWC24_ERR_FATAL;
}
s32 NWC24SetMsgText(void*, const char*, u32, int, int) {
    return NWC24_ERR_FATAL;
}
s32 NWC24SetMsgToId(void*, uint64_t) {
    return NWC24_ERR_FATAL;
}
void VFInitEx(void*, u32) {}

// ---- RSO (the HOME button menu is a PowerPC module and cannot be loaded) -----------------------------------

BOOL RSOListInit(void*) {
    return 0;
}
BOOL RSOLinkList(void*, void*) {
    return 0;
}
int RSOLinkJump(void*, const void*, void*) {
    return 0;
}
BOOL RSOIsImportSymbolResolvedAll(const void*) {
    return 0;
}
int RSOGetJumpCodeSize(const void*) {
    return 0;
}
void RSOMakeJumpCode(const void*, void*) {}
void* RSOFindExportSymbolAddr(const void*, const char*) {
    return nullptr;
}

// ---- Wii Speaker ADPCM encoder ----------------------------------------------------------------------------

s32 WENCGetEncodeData(void*, u32, const int16_t*, s32 samples, u8* out) {
    if (out && samples > 0) std::memset(out, 0, (size_t)(samples + 1) / 2);
    return samples > 0 ? (samples + 1) / 2 : 0;
}

// ---- DSP (JAudio2's DSP microcode); replaced later by a host mixer -----------------------------------------

u32 DSPCheckMailFromDSP(void) {
    return 0;
}
u32 DSPReadMailFromDSP(void) {
    return 0;
}

}  // extern "C"

// JAudio2's DSP interface (C++ linkage).
void DSPReleaseHalt2(u32) {}
void DsetMixerLevel(f32) {}
void DsetVARAM(u32) {}
void DsetupTable(u32, u32, u32, u32, u32) {}
void DspBoot(void (*callback)(void*)) {
    if (callback) callback(nullptr);
}
void DspFinishWork(u16) {}
void DsyncFrame2(u32, u32, u32) {}

// 4x4 matrix helpers missing from Aurora's MTX library.
extern "C" {
void C_MTX44Identity(f32 m[4][4]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            m[i][j] = i == j ? 1.0f : 0.0f;
        }
    }
}

void C_MTX44Copy(const f32 src[4][4], f32 dst[4][4]) {
    if (src != dst) {
        std::memcpy(dst, src, sizeof(f32) * 16);
    }
}
}
