#pragma once

#include <string>

namespace pc {
// Saves every menu/HUD image from the layout archives (in the current language) as replacement-ready PNGs below
// `outRoot`. Returns a short summary for the menu.
std::string dumpUiTextures(const std::string& outRoot);
}  // namespace pc
