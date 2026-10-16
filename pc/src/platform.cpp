// Galaxy PC platform glue: logging, emulated MEM2 arena, Wii-only OS calls, per-frame hooks.
#include "pc/platform.h"
#include "pc/controls_menu.h"
#include "pc/frame.h"
#include "pc/input.h"

#include <revolution/gx.h>
#include <revolution/os.h>
#include <revolution/vi.h>

#include <aurora/aurora.h>
#include <aurora/event.h>
#include <imgui.h>

#include <SDL3/SDL.h>

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern void* MEM1Start;
extern void* MEM1End;

namespace pc {
bool IsRunning = true;
bool IsShuttingDown = false;
}  // namespace pc

extern "C" void pc_log(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fflush(stderr);
}

// ---- Memory ------------------------------------------------------------------------------
// MEM1 is Aurora's arena. MEM2 (the Wii's 64 MB GDDR3) is a second block we allocate here.
// Both are larger than on the console because 64-bit pointers make many structures bigger.

static u8* sMEM2Start = nullptr;
static u8* sMEM2End = nullptr;
static u8* sMEM2ArenaLo = nullptr;
static u8* sMEM2ArenaHi = nullptr;

void pc_initMEM2(u32 size) {
    sMEM2Start = static_cast<u8*>(std::calloc(1, size));
    if (sMEM2Start == nullptr) {
        pc_log("Fatal: unable to allocate %u bytes for MEM2\n", size);
        std::abort();
    }
    sMEM2End = sMEM2Start + size;
    sMEM2ArenaLo = sMEM2Start;
    sMEM2ArenaHi = sMEM2End;
}

bool pc_isGameHeapPtr(const void* p) {
    const u8* b = static_cast<const u8*>(p);
    if (b == nullptr) {
        return false;
    }
    if (b >= static_cast<const u8*>(MEM1Start) && b < static_cast<const u8*>(MEM1End)) {
        return true;
    }
    return b >= sMEM2Start && b < sMEM2End;
}

extern "C" {

void* OSGetMEM1ArenaLo(void) { return OSGetArenaLo(); }
void* OSGetMEM1ArenaHi(void) { return OSGetArenaHi(); }
void OSSetMEM1ArenaLo(void* p) { OSSetArenaLo(p); }
void OSSetMEM1ArenaHi(void* p) { OSSetArenaHi(p); }
void* OSGetMEM2ArenaLo(void) { return sMEM2ArenaLo; }
void* OSGetMEM2ArenaHi(void) { return sMEM2ArenaHi; }
void OSSetMEM2ArenaLo(void* p) { sMEM2ArenaLo = static_cast<u8*>(p); }
void OSSetMEM2ArenaHi(void* p) { sMEM2ArenaHi = static_cast<u8*>(p); }

void* OSAllocFromMEM1ArenaLo(u32 size, u32 align) {
    return OSAllocFromArenaLo(size, align);
}

void* OSAllocFromMEM2ArenaLo(u32 size, u32 align) {
    uintptr_t p = (reinterpret_cast<uintptr_t>(sMEM2ArenaLo) + align - 1) & ~(uintptr_t)(align - 1);
    sMEM2ArenaLo = reinterpret_cast<u8*>(p + size);
    return reinterpret_cast<void*>(p);
}

u32 OSGetPhysicalMem2Size(void) { return (u32)(sMEM2End - sMEM2Start); }
u32 OSGetPhysicalMem1Size(void) { return (u32)(static_cast<u8*>(MEM1End) - static_cast<u8*>(MEM1Start)); }

const u8 OSGetAppType(void) { return 0x80; }
const char* OSGetAppGamename(void) { return "RMGK01"; }

void OSShutdownSystem(void) {
    pc::IsShuttingDown = true;
    pc::IsRunning = false;
    std::exit(0);
}

void OSRestart(u32) { OSShutdownSystem(); }
void OSReturnToMenu(void) { OSShutdownSystem(); }
void OSReturnToDataManager(void) { OSShutdownSystem(); }
BOOL OSIsRestart(void) { return FALSE; }

u8 OSGetLanguage(void) { return 0; }
u32 OSGetEuRgb60Mode(void) { return 0; }

}  // extern "C"

// ---- Frame hooks -------------------------------------------------------------------------

namespace {
bool sFrameBegun = false;
auto sLastFrame = std::chrono::steady_clock::now();
float sDt = 1.0f / 60.0f;
bool sCursorHidden = false;
}  // namespace

namespace pc {

void frameBegin() {
    auto now = std::chrono::steady_clock::now();
    sDt = std::chrono::duration<float>(now - sLastFrame).count();
    sDt = sDt > 0.25f ? 0.25f : sDt;
    sLastFrame = now;

    const AuroraEvent* ev = aurora_update();
    while (ev != nullptr && ev->type != AURORA_NONE) {
        switch (ev->type) {
        case AURORA_EXIT:
            IsRunning = false;
            IsShuttingDown = true;
            pc::input::save();
            aurora_shutdown();
            std::exit(0);
        case AURORA_SDL_EVENT:
            if (ev->sdl.type == SDL_EVENT_KEY_DOWN && !ev->sdl.key.repeat && ev->sdl.key.scancode == SDL_SCANCODE_F1 &&
                !pc::input::isCapturing()) {
                pc::ui::toggleMenu();
            } else if (ev->sdl.type == SDL_EVENT_KEY_DOWN && !ev->sdl.key.repeat && ev->sdl.key.scancode == SDL_SCANCODE_F11) {
                SDL_Window* w = SDL_GetWindowFromID(ev->sdl.key.windowID);
                if (w) SDL_SetWindowFullscreen(w, !(SDL_GetWindowFlags(w) & SDL_WINDOW_FULLSCREEN));
            }
            // While the menu is open (and not capturing a rebind), the game doesn't see input.
            if (!pc::ui::isMenuOpen() || pc::input::isCapturing()) {
                pc::input::handleEvent(ev->sdl);
            } else if (ev->sdl.type == SDL_EVENT_KEY_UP || ev->sdl.type == SDL_EVENT_MOUSE_BUTTON_UP ||
                       ev->sdl.type == SDL_EVENT_MOUSE_MOTION || ev->sdl.type == SDL_EVENT_GAMEPAD_ADDED ||
                       ev->sdl.type == SDL_EVENT_GAMEPAD_REMOVED) {
                pc::input::handleEvent(ev->sdl);  // keep releases/connection state consistent
            }
            break;
        default:
            break;
        }
        ++ev;
    }

    // Hide the OS cursor while the mouse is the in-game pointer and no menu needs it.
    bool wantHidden = pc::input::settings().mousePointer && pc::input::settings().hideCursor && !pc::ui::isMenuOpen();
    if (wantHidden != sCursorHidden) {
        sCursorHidden = wantHidden;
        if (wantHidden) SDL_HideCursor();
        else SDL_ShowCursor();
    }

    pc::input::newFrame(sDt);
    sFrameBegun = aurora_begin_frame();
}

void frameEnd() {
    if (sFrameBegun) {
        pc::ui::draw(sDt);
        aurora_end_frame();
        sFrameBegun = false;
    }
    // Stand in for the console's vertical retrace interrupt.
    VIWaitForRetrace();
}

}  // namespace pc
