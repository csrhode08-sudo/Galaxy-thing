// Graphics settings (F1 menu, "Graphics" tab), saved to graphics.ini in the user folder.
#pragma once

#include <string>

#include <aurora/aurora.h>

namespace pc::graphics {

struct Settings {
    int resolution = 0;     // 0 = match the window; 1..6 = that many times the Wii's 640x456
    int msaa = 1;           // anti-aliasing samples: 1 (off) or 4 (WebGPU's only other count) — applied at startup
    int anisotropy = 16;    // texture filtering: 1, 2, 4, 8, 16 — applied at startup
    bool vsync = true;
    bool fullscreen = false;  // start in fullscreen
    bool dumpTextures = false;  // save every texture the game uses (for making replacements) — applied at startup
};

void load(const std::string& configDir);  // before aurora_initialize
void applyToConfig(AuroraConfig& config);  // startup-only settings
void applyRuntime();                       // after aurora_initialize, and whenever a runtime setting changes
void save();
Settings& settings();
const Settings& startupSettings();
std::string dumpFolder();         // where dumped textures go
std::string replacementFolder();
std::string uiDumpFolder();       // where "Save all menu & HUD images" writes  // where replacement textures are loaded from
void loadReplacements();
size_t reloadReplacements();       // F5: pick up new or edited replacement images; returns how many are loaded
          // after aurora_initialize  // the values the game was started with (for "restart to apply")

}  // namespace pc::graphics
