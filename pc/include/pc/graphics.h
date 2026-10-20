// Graphics settings (F1 menu, "Graphics" tab), saved to graphics.ini in the user folder.
#pragma once

#include <string>

#include <aurora/aurora.h>

namespace pc::graphics {

struct Settings {
    int resolution = 0;     // 0 = match the window; 1..6 = that many times the Wii's 640x456
    int msaa = 1;           // anti-aliasing samples: 1 (off), 2, 4, 8 — applied at startup
    int anisotropy = 16;    // texture filtering: 1, 2, 4, 8, 16 — applied at startup
    bool vsync = true;
    bool fullscreen = false;  // start in fullscreen
};

void load(const std::string& configDir);  // before aurora_initialize
void applyToConfig(AuroraConfig& config);  // startup-only settings
void applyRuntime();                       // after aurora_initialize, and whenever a runtime setting changes
void save();
Settings& settings();
const Settings& startupSettings();  // the values the game was started with (for "restart to apply")

}  // namespace pc::graphics
