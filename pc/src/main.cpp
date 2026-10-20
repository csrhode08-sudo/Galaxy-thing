#include <cstdlib>
// Super Mario Galaxy PC: program entry point.
#include "pc/frame.h"
#include "pc/input.h"
#include "pc/platform.h"
#include "pc/graphics.h"
#include "pc/controls_menu.h"

#include <revolution/os.h>

#include <aurora/aurora.h>
#include <aurora/dvd.h>
#include <revolution/dvd.h>
#include <aurora/main.h>

#include <SDL3/SDL.h>

#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

void smg_main(void);  // GameSystem.cpp
void pc_initMEM2(u32 size);
namespace pc {
    bool loadDolAssets(const std::string& discPath);
}

namespace {

void logCallback(AuroraLogLevel level, const char* module, const char* message, unsigned int len) {
    static const char* const kLevels[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
    if (level == LOG_DEBUG) {
        return;
    }
    pc::log("[%s %s] %.*s\n", kLevels[level], module, (int)len, message);
    if (level == LOG_FATAL) {
        std::fflush(stderr);
        std::abort();
    }
}

bool hasDiscExtension(const std::filesystem::path& p) {
    std::string e = p.extension().string();
    for (char& c : e) c = (char)std::tolower((unsigned char)c);
    return e == ".wbfs" || e == ".iso" || e == ".rvz" || e == ".wia" || e == ".ciso" || e == ".gcz" || e == ".nfs";
}

// Finds the Korean Super Mario Galaxy disc (RMGK01) next to the executable or in the working directory.
std::string findDisc(const char* exeDir) {
    std::vector<std::filesystem::path> dirs = {std::filesystem::current_path()};
    if (exeDir) dirs.emplace_back(exeDir);
    std::string fallback;
    for (const auto& d : dirs) {
        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(d, ec)) {
            if (!entry.is_regular_file() || !hasDiscExtension(entry.path())) continue;
            std::string name = entry.path().filename().string();
            if (name.find("RMGK01") != std::string::npos || name.find("Korea") != std::string::npos) {
                return entry.path().string();
            }
            if (fallback.empty()) fallback = entry.path().string();
        }
    }
    return fallback;
}

}  // namespace

int main(int argc, char* argv[]) {
    pc::installCrashHandler();
    std::string discPath;
    std::string overlayPath;
    for (int i = 1; i < argc; i++) {
        if ((std::strcmp(argv[i], "--disc") == 0 || std::strcmp(argv[i], "-d") == 0) && i + 1 < argc) {
            discPath = argv[++i];
        } else if (std::strcmp(argv[i], "--overlay") == 0 && i + 1 < argc) {
            overlayPath = argv[++i];
        } else if (argv[i][0] != '-') {
            discPath = argv[i];
        }
    }

    const char* base = SDL_GetBasePath();
    if (discPath.empty()) {
        discPath = findDisc(base);
    }

    char* prefPath = SDL_GetPrefPath("GalaxyPC", "SuperMarioGalaxy");
    std::string userDir = prefPath ? prefPath : ".";
    if (prefPath) SDL_free(prefPath);
    pc::setUserDir(userDir);
    pc::openLogFile(base ? base : "", userDir);

    AuroraConfig config{};
    config.appName = "Super Mario Galaxy";
    config.userPath = userDir.c_str();
    config.cachePath = userDir.c_str();
    config.desiredBackend = BACKEND_AUTO;
    config.vsync = true;
    config.windowPosX = -1;
    config.windowPosY = -1;
    config.windowWidth = 1280;
    config.windowHeight = 960;
    if (const char* size = std::getenv("GALAXY_WINDOW")) {  // e.g. GALAXY_WINDOW=640x480
        int w = 0, h = 0;
        if (std::sscanf(size, "%dx%d", &w, &h) == 2 && w > 0 && h > 0) {
            config.windowWidth = w;
            config.windowHeight = h;
        }
    }
    config.logCallback = &logCallback;
    config.logLevel = LOG_INFO;
    config.allowCpuAdapter = true;
    config.imGuiInitCallback = [](const AuroraWindowSize* size) { pc::ui::initImGui(size != nullptr ? size->scale : 1.0f); };
    pc::graphics::load(userDir);
    pc::graphics::applyToConfig(config);
    // The Wii has 24 MB MEM1 + 64 MB MEM2; 64-bit pointers make structures bigger, so give headroom.
    config.mem1Size = 96 * 1024 * 1024;
    config.mem2Size = 32 * 1024 * 1024;  // emulated ARAM
    AuroraInfo info = aurora_initialize(argc, argv, &config);
    (void)info;
    pc::graphics::applyRuntime();
    pc::graphics::loadReplacements();

    std::error_code dirEc;
    const bool isDir = !discPath.empty() && std::filesystem::is_directory(discPath, dirEc);
    // A directory of extracted disc files (its "files" folder) also works, e.g. for development.
    const bool opened = isDir ? aurora_dvd_open_directory(discPath.c_str(), "RMGK01") : (!discPath.empty() && aurora_dvd_open(discPath.c_str()));
    if (!opened) {
        std::string msg = discPath.empty()
                              ? "Couldn't find the game disc.\n\nPut your Super Mario Galaxy disc image (the Korean version, RMGK01, as "
                                ".wbfs/.iso/.rvz) next to SuperMarioGalaxy.exe, or pass its path on the command line."
                              : "Couldn't open the disc image:\n" + discPath;
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Super Mario Galaxy", msg.c_str(), nullptr);
        aurora_shutdown();
        return 1;
    }
    pc_log("[main] Disc: %s\n", discPath.c_str());

    // English text and menus: the US release (RMGE01) next to the Korean disc provides its UsEnglish folder.
    if (std::getenv("GALAXY_KOREAN") == nullptr) {
        std::vector<std::filesystem::path> dirs = {std::filesystem::path(discPath).parent_path(), std::filesystem::current_path()};
        if (base) dirs.emplace_back(base);
        bool found = false;
        for (const auto& d : dirs) {
            std::error_code ec;
            for (const auto& entry : std::filesystem::directory_iterator(d, ec)) {
                if (found) break;
                if (!entry.is_regular_file(ec) || !hasDiscExtension(entry.path())) continue;
                if (std::filesystem::equivalent(entry.path(), discPath, ec)) continue;
                const std::string p = entry.path().string();
                if (aurora_dvd_overlay_disc(p.c_str(), "RMGE01", "/UsEnglish/", "/KrKorean/")) {
                    pc_log("[main] English from: %s\n", p.c_str());
                    found = true;
                }
            }
            if (found) break;
        }
    }

    // Test aid: PC_DVD_SELFTEST=<disc path> prints the start of that file from the (overlaid) disc and exits.
    if (const char* testPath = std::getenv("PC_DVD_SELFTEST")) {
        DVDFileInfo fi;
        if (DVDOpen(testPath, &fi)) {
            static u8 buf[64] __attribute__((aligned(32)));
            const s32 n = DVDReadPrio(&fi, buf, 32, 0, 2);
            pc_log("[dvdtest] %s: size %u, read %d: %.32s\n", testPath, (unsigned)fi.length, (int)n, (const char*)buf);
            DVDClose(&fi);
        } else {
            pc_log("[dvdtest] %s: not found\n", testPath);
        }
        std::exit(0);
    }

    // Extra files layered over the disc, e.g. the UsEnglish folder taken from the US release for English text.
    if (overlayPath.empty()) {
        for (const std::filesystem::path& d : {std::filesystem::current_path(), std::filesystem::path(base ? base : ".")}) {
            std::error_code ec;
            if (std::filesystem::is_directory(d / "overlay", ec)) {
                overlayPath = (d / "overlay").string();
                break;
            }
        }
    }
    if (!overlayPath.empty() && aurora_dvd_overlay_directory(overlayPath.c_str())) {
        pc_log("[main] Overlay: %s\n", overlayPath.c_str());
    }

    if (!pc::loadDolAssets(discPath)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Super Mario Galaxy",
                                 "Couldn't read the game's data tables from the disc. Use the Korean disc (RMGK01).", nullptr);
        aurora_shutdown();
        return 1;
    }

    pc::input::init(userDir);

    OSInit();
    pc_initMEM2(160 * 1024 * 1024);

    smg_main();  // never returns
    return 0;
}
