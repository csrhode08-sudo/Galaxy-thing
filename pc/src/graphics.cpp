#include "pc/graphics.h"
#include "pc/platform.h"

#include <aurora/aurora.h>
#include <aurora/gfx.h>
#include <revolution/gx.h>
#include <aurora/texture.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
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
std::string sDir;

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
    sDir = configDir;
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
        else if (k == "msaa") sSettings.msaa = v > 1 ? 4 : 1;  // WebGPU supports only 1 or 4 samples
        else if (k == "anisotropy") sSettings.anisotropy = clampChoice(v, {1, 2, 4, 8, 16}, 16);
        else if (k == "vsync") sSettings.vsync = v != 0;
        else if (k == "fullscreen") sSettings.fullscreen = v != 0;
        else if (k == "dump_textures") sSettings.dumpTextures = v != 0;
    }
    sStartup = sSettings;
}

void applyToConfig(AuroraConfig& config) {
    config.msaa = sSettings.msaa;
    config.maxTextureAnisotropy = static_cast<uint16_t>(sSettings.anisotropy);
    config.vsync = sSettings.vsync;
    config.startFullscreen = sSettings.fullscreen;
    config.allowTextureDumps = sSettings.dumpTextures;
}

void applyRuntime() {
    aurora::window::set_frame_buffer_scale(static_cast<float>(sSettings.resolution));
    aurora_enable_vsync(sSettings.vsync);
}

std::string dumpFolder() {
    return sDir + "/texture_dumps";
}

std::string replacementFolder() {
    return sDir + "/textures";
}

void loadReplacements() {
    // Replacement images (.png or .dds, named like the dumped files) in the "textures" folder, including
    // subfolders. The same names as Dolphin's texture packs.
    std::error_code ec;
    std::filesystem::create_directories(replacementFolder(), ec);
    const auto group = aurora::texture::load_replacement_directory(std::filesystem::u8path(replacementFolder()));
    pc_log("[graphics] %zu replacement textures from %s\n", group.registrations.size(), replacementFolder().c_str());
    if (sSettings.dumpTextures) {
        pc_log("[graphics] Dumping textures to %s\n", dumpFolder().c_str());
    }
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
    f << "dump_textures=" << (sSettings.dumpTextures ? 1 : 0) << '\n';
}

}  // namespace pc::graphics
