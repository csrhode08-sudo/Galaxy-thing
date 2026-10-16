// Controls remapping menu (Dear ImGui), shown over loading screens and on demand with F1.
#include "pc/controls_menu.h"
#include "pc/input.h"

#include <imgui.h>

#include <SDL3/SDL.h>

#include <cfloat>
#include <string>

namespace pc::ui {
namespace {

bool sOpen = false;
bool sLoadingScreen = false;
int sCaptureAction = -1;
int sCaptureSlot = -1;
float sCaptureTimeout = 0.0f;

const char* const kSlotHeaders[input::kBindingsPerAction] = {"Primary", "Secondary", "Extra"};

void drawBindingButton(int action, int slot) {
    auto& s = input::settings();
    input::Binding& b = s.binds[action][slot];
    ImGui::PushID(action * 16 + slot);
    std::string label;
    if (sCaptureAction == action && sCaptureSlot == slot) {
        label = "Press a key / button...";
    } else {
        label = input::bindingName(b);
        if (label.empty()) {
            label = "-";
        }
    }
    if (ImGui::Button(label.c_str(), ImVec2(-FLT_MIN, 0))) {
        sCaptureAction = action;
        sCaptureSlot = slot;
        sCaptureTimeout = 5.0f;
        input::beginCapture();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Click to rebind, right-click to clear");
    }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        b = {};
        input::save();
    }
    ImGui::PopID();
}

void finishCapture() {
    input::Binding b;
    if (sCaptureAction >= 0 && input::pollCapture(&b)) {
        auto& s = input::settings();
        // Escape cancels instead of binding.
        if (!(b.kind == input::BindKind::Key && b.code == SDL_SCANCODE_ESCAPE)) {
            // A binding may only drive one action: remove it elsewhere first.
            for (int a = 0; a < (int)input::Action::Count; a++) {
                for (int i = 0; i < input::kBindingsPerAction; i++) {
                    if (s.binds[a][i] == b) {
                        s.binds[a][i] = {};
                    }
                }
            }
            s.binds[sCaptureAction][sCaptureSlot] = b;
            input::save();
        }
        sCaptureAction = sCaptureSlot = -1;
    }
}

}  // namespace

void setLoadingScreen(bool loading) {
    sLoadingScreen = loading;
}

bool isMenuOpen() {
    return sOpen;
}

void toggleMenu() {
    sOpen = !sOpen;
    if (!sOpen && sCaptureAction >= 0) {
        input::cancelCapture();
        sCaptureAction = sCaptureSlot = -1;
    }
}

void draw(float dt) {
    if (sCaptureAction >= 0) {
        sCaptureTimeout -= dt;
        if (sCaptureTimeout <= 0.0f) {
            input::cancelCapture();
            sCaptureAction = sCaptureSlot = -1;
        } else {
            finishCapture();
        }
    }

    ImGuiIO& io = ImGui::GetIO();
    if (!sOpen && sLoadingScreen) {
        // Small prompt in the corner of loading screens.
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 16.0f, io.DisplaySize.y - 16.0f), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
        ImGui::SetNextWindowBgAlpha(0.55f);
        if (ImGui::Begin("##controls_hint", nullptr,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav)) {
            if (ImGui::Button("Controls (F1)")) {
                toggleMenu();
            }
        }
        ImGui::End();
    }

    if (!sOpen) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(720.0f, 560.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    if (!ImGui::Begin("Controls", &sOpen, ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }

    auto& s = input::settings();
    if (ImGui::BeginTabBar("controls_tabs")) {
        if (ImGui::BeginTabItem("Bindings")) {
            ImGui::TextWrapped("Click a binding, then press a key, mouse button, or controller button/stick. "
                               "Right-click a binding to clear it. Esc cancels.");
            if (ImGui::BeginTable("binds", 1 + input::kBindingsPerAction,
                                  ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY,
                                  ImVec2(0, ImGui::GetContentRegionAvail().y - 40.0f))) {
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch, 1.4f);
                for (const char* h : kSlotHeaders) {
                    ImGui::TableSetupColumn(h, ImGuiTableColumnFlags_WidthStretch, 1.0f);
                }
                ImGui::TableHeadersRow();
                for (int a = 0; a < (int)input::Action::Count; a++) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted(input::actionName((input::Action)a));
                    for (int i = 0; i < input::kBindingsPerAction; i++) {
                        ImGui::TableSetColumnIndex(1 + i);
                        drawBindingButton(a, i);
                    }
                }
                ImGui::EndTable();
            }
            if (ImGui::Button("Reset to defaults")) {
                input::Settings d = input::defaultSettings();
                for (int a = 0; a < (int)input::Action::Count; a++) {
                    for (int i = 0; i < input::kBindingsPerAction; i++) {
                        s.binds[a][i] = d.binds[a][i];
                    }
                }
                input::save();
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Pointer & Sticks")) {
            bool changed = false;
            changed |= ImGui::Checkbox("Use the mouse as the Wii Remote pointer", &s.mousePointer);
            changed |= ImGui::Checkbox("Hide the system cursor over the game", &s.hideCursor);
            changed |= ImGui::SliderFloat("Controller pointer speed", &s.padPointerSpeed, 0.2f, 5.0f, "%.1f");
            changed |= ImGui::Checkbox("Invert controller pointer Y", &s.invertPadPointerY);
            changed |= ImGui::SliderFloat("Stick deadzone", &s.stickDeadzone, 0.0f, 0.6f, "%.2f");
            changed |= ImGui::Checkbox("Controller rumble", &s.rumble);
            if (changed) {
                input::save();
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();

    if (!sOpen && sCaptureAction >= 0) {
        input::cancelCapture();
        sCaptureAction = sCaptureSlot = -1;
    }
}

}  // namespace pc::ui
