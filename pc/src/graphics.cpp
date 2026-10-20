#include "pc/graphics.h"
#include "pc/platform.h"

#include <aurora/aurora.h>
#include <aurora/gfx.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>

// Aurora's frame-buffer scale (0 = match the window); not in its public headers.
namespace aurora::window {
void set_frame_buffer_scale(float scale);
}

namespace pc::graphics {

namespace {
Settings sSettings;
Settings sStartup;
std::string sPath;

int clampChoice(int v, std::initializer_list<int> allowed, int fallback) {
    for (int a : allowed) {
        if (v == a) return v;
    }
    return fallback;
}
}  // namespace

Settings& settings() {
    return sSettings;
}

const Settings& startupSettings() {
    return sStartup;
}

void load(const std::string& configDir) {
    sPath = configDir + "/graphics.ini";
    std::ifstream f(sPath);
    std::string line;
    while (f && std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        const size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string k = line.substr(0, eq);
        const int v = std::atoi(line.c_str() + eq + 1);
        if (k == "resolution") sSettings.resolution = std::clamp(v, 0, 6);
        else if (k == "msaa") sSettings.msaa = clampChoice(v, {1, 2, 4, 8}, 1);
        else if (k == "anisotropy") sSettings.anisotropy = clampChoice(v, {1, 2, 4, 8, 16}, 16);
        else if (k == "vsync") sSettings.vsync = v != 0;
        else if (k == "fullscreen") sSettings.fullscreen = v != 0;
    }
    sStartup = sSettings;
}

void applyToConfig(AuroraConfig& config) {
    config.msaa = sSettings.msaa;
    config.maxTextureAnisotropy = static_cast<uint16_t>(sSettings.anisotropy);
    config.vsync = sSettings.vsync;
    config.startFullscreen = sSettings.fullscreen;
}

void applyRuntime() {
    aurora::window::set_frame_buffer_scale(static_cast<float>(sSettings.resolution));
    aurora_enable_vsync(sSettings.vsync);
}

void save() {
    std::ofstream f(sPath, std::ios::trunc);
    if (!f) {
        pc_log("[graphics] Could not write %s\n", sPath.c_str());
        return;
    }
    f << "# Super Mario Galaxy PC graphics. Edit in game with F1, or by hand.\n";
    f << "# resolution: 0 = match the window, 1-6 = multiple of the Wii's 640x456\n";
    f << "resolution=" << sSettings.resolution << '\n';
    f << "msaa=" << sSettings.msaa << '\n';
    f << "anisotropy=" << sSettings.anisotropy << '\n';
    f << "vsync=" << (sSettings.vsync ? 1 : 0) << '\n';
    f << "fullscreen=" << (sSettings.fullscreen ? 1 : 0) << '\n';
}

}  // namespace pc::graphics
