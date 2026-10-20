// Controls remapping menu shown over loading screens (and anywhere with F1).
#pragma once

namespace pc::ui {
void initImGui(float displayScale);  // fonts and theme; from Aurora's ImGui init callback
void draw(float dt);              // call once per frame inside the ImGui frame
void toggleMenu();
bool isMenuOpen();
void setLoadingScreen(bool loading);  // the game reports when a loading screen is up
}  // namespace pc::ui
