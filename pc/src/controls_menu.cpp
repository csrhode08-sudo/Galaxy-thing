// Settings menu (Dear ImGui) styled after Super Mario Galaxy's own menus: a starry navy panel, rounded pill
// buttons, gold highlights and outlined lettering. Shown over loading screens (as a small prompt) and with F1.
#include "pc/controls_menu.h"
#include "pc/graphics.h"
#include "pc/input.h"

#include <imgui.h>

#include <SDL3/SDL.h>

#include <cfloat>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>

#ifndef IM_PI
#define IM_PI 3.14159265358979323846f
#endif

namespace pc::ui {
namespace {

bool sOpen = false;
bool sLoadingScreen = false;
int sCaptureAction = -1;
int sCaptureSlot = -1;
float sCaptureTimeout = 0.0f;
int sPage = 0;
float sTime = 0.0f;

ImFont* sFontBody = nullptr;
ImFont* sFontTitle = nullptr;

const char* const kSlotHeaders[input::kBindingsPerAction] = {"Primary", "Secondary", "Extra"};

// ---- Palette ------------------------------------------------------------------------------------------------
constexpr ImU32 rgba(int r, int g, int b, int a = 255) {
    return IM_COL32(r, g, b, a);
}
const ImVec4 kGold = ImVec4(1.00f, 0.81f, 0.19f, 1.0f);
const ImU32 kGoldU = rgba(255, 206, 48);
const ImU32 kNavyU = rgba(10, 22, 70);
const ImU32 kOutlineU = rgba(8, 20, 70);

// ---- Drawing helpers ----------------------------------------------------------------------------------------
void drawStar(ImDrawList* dl, ImVec2 c, float rOuter, ImU32 fill, ImU32 outline, float angle = -IM_PI / 2) {
    ImVec2 pts[10];
    const float rInner = rOuter * 0.47f;
    for (int i = 0; i < 10; i++) {
        const float r = (i & 1) ? rInner : rOuter;
        const float a = angle + i * IM_PI / 5.0f;
        pts[i] = ImVec2(c.x + std::cos(a) * r, c.y + std::sin(a) * r);
    }
    dl->AddConcavePolyFilled(pts, 10, fill);
    if (outline != 0) {
        dl->AddPolyline(pts, 10, outline, ImDrawFlags_Closed, rOuter * 0.12f);
    }
}

// Text with a dark outline, like the game's lettering.
void outlinedText(ImDrawList* dl, ImFont* font, float size, ImVec2 pos, ImU32 col, const char* text, float thick = 2.0f) {
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx != 0 || dy != 0) {
                dl->AddText(font, size, ImVec2(pos.x + dx * thick, pos.y + dy * thick), kOutlineU, text);
            }
        }
    }
    dl->AddText(font, size, ImVec2(pos.x, pos.y + thick * 0.8f), rgba(0, 0, 0, 90), text);
    dl->AddText(font, size, pos, col, text);
}

// Little twinkling stars scattered over a rectangle (deterministic positions).
void drawStarfield(ImDrawList* dl, ImVec2 min, ImVec2 max, int count, unsigned seed) {
    unsigned s = seed;
    auto rnd = [&s] {
        s = s * 1664525u + 1013904223u;
        return (s >> 8) / float(1 << 24);
    };
    for (int i = 0; i < count; i++) {
        const ImVec2 p(min.x + rnd() * (max.x - min.x), min.y + rnd() * (max.y - min.y));
        const float phase = rnd() * 6.28f, speed = 0.8f + rnd() * 2.0f, size = 0.8f + rnd() * 1.6f;
        const float tw = 0.35f + 0.65f * (0.5f + 0.5f * std::sin(sTime * speed + phase));
        dl->AddCircleFilled(p, size, rgba(220, 235, 255, int(200 * tw)));
        if (size > 2.0f) {
            dl->AddLine(ImVec2(p.x - size * 2.5f, p.y), ImVec2(p.x + size * 2.5f, p.y), rgba(200, 225, 255, int(90 * tw)));
            dl->AddLine(ImVec2(p.x, p.y - size * 2.5f), ImVec2(p.x, p.y + size * 2.5f), rgba(200, 225, 255, int(90 * tw)));
        }
    }
}

// A pill-shaped button; `selected` draws it gold.
bool pillButton(const char* label, bool selected, ImVec2 size = ImVec2(0, 0)) {
    if (selected) {
        ImGui::PushStyleColor(ImGuiCol_Button, kGold);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.88f, 0.40f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.92f, 0.55f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.04f, 0.09f, 0.28f, 1.0f));
    }
    const bool pressed = ImGui::Button(label, size);
    if (selected) {
        ImGui::PopStyleColor(4);
    }
    return pressed;
}

void sectionHeading(const char* text) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float h = ImGui::GetFontSize();
    drawStar(dl, ImVec2(p.x + h * 0.5f, p.y + h * 0.55f), h * 0.5f, kGoldU, kOutlineU);
    ImGui::SetCursorScreenPos(ImVec2(p.x + h * 1.3f, p.y));
    outlinedText(dl, ImGui::GetFont(), h, ImGui::GetCursorScreenPos(), kGoldU, text, 1.5f);
    ImGui::Dummy(ImVec2(ImGui::CalcTextSize(text).x, h));
    ImGui::Spacing();
}

// ---- Bindings -----------------------------------------------------------------------------------------------
void drawBindingButton(int action, int slot) {
    auto& s = input::settings();
    input::Binding& b = s.binds[action][slot];
    ImGui::PushID(action * 16 + slot);
    std::string label;
    const bool capturing = sCaptureAction == action && sCaptureSlot == slot;
    if (capturing) {
        label = "Press a key or button...";
    } else {
        label = input::bindingName(b);
        if (label.empty()) {
            label = "-";
        }
    }
    if (pillButton(label.c_str(), capturing, ImVec2(-FLT_MIN, 0))) {
        sCaptureAction = action;
        sCaptureSlot = slot;
        sCaptureTimeout = 5.0f;
        input::beginCapture();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Click to change, right-click to clear");
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

void drawControlsPage() {
    auto& s = input::settings();
    ImGui::TextColored(ImVec4(0.75f, 0.85f, 1.0f, 1.0f),
                       "Click a button, then press a key, mouse button, or controller button. Right-click clears. Esc cancels.");
    ImGui::Spacing();
    if (ImGui::BeginTable("binds", 1 + input::kBindingsPerAction,
                          ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_PadOuterX,
                          ImVec2(0, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() - 8.0f))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch, 1.3f);
        for (const char* h : kSlotHeaders) {
            ImGui::TableSetupColumn(h, ImGuiTableColumnFlags_WidthStretch, 1.0f);
        }
        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
        for (int c = 0; c < 1 + input::kBindingsPerAction; c++) {
            ImGui::TableSetColumnIndex(c);
            ImGui::TextColored(kGold, "%s", ImGui::TableGetColumnName(c));
        }
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
    if (pillButton("Reset to defaults", false)) {
        input::Settings d = input::defaultSettings();
        for (int a = 0; a < (int)input::Action::Count; a++) {
            for (int i = 0; i < input::kBindingsPerAction; i++) {
                s.binds[a][i] = d.binds[a][i];
            }
        }
        input::save();
    }
}

void drawPointerPage() {
    auto& s = input::settings();
    bool changed = false;
    sectionHeading("Wii Remote pointer");
    changed |= ImGui::Checkbox("Use the mouse as the pointer", &s.mousePointer);
    changed |= ImGui::Checkbox("Hide the system cursor over the game", &s.hideCursor);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
    changed |= ImGui::SliderFloat("Controller pointer speed", &s.padPointerSpeed, 0.2f, 5.0f, "%.1f");
    changed |= ImGui::Checkbox("Invert controller pointer up/down", &s.invertPadPointerY);
    ImGui::Spacing();
    sectionHeading("Controller");
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
    changed |= ImGui::SliderFloat("Stick deadzone", &s.stickDeadzone, 0.0f, 0.6f, "%.2f");
    changed |= ImGui::Checkbox("Rumble", &s.rumble);
    if (changed) {
        input::save();
    }
}

// A row of pill buttons choosing one of `count` values; returns true if the choice changed.
bool pillChoice(const char* id, int* index, const char* const* names, int count) {
    bool changed = false;
    ImGui::PushID(id);
    for (int i = 0; i < count; i++) {
        if (i > 0) {
            ImGui::SameLine();
        }
        ImGui::PushID(i);
        if (pillButton(names[i], *index == i) && *index != i) {
            *index = i;
            changed = true;
        }
        ImGui::PopID();
    }
    ImGui::PopID();
    return changed;
}

void drawGraphicsPage() {
    auto& g = graphics::settings();
    const auto& start = graphics::startupSettings();
    bool changed = false;

    sectionHeading("Resolution");
    static const char* kResolutions[] = {"Window", "1x", "2x", "3x", "4x", "5x", "6x"};
    if (pillChoice("res", &g.resolution, kResolutions, IM_ARRAYSIZE(kResolutions))) {
        graphics::applyRuntime();
        changed = true;
    }
    ImGui::TextColored(ImVec4(0.75f, 0.85f, 1.0f, 1.0f), g.resolution == 0
                                                             ? "Sharpest: renders at your window or screen size."
                                                             : "Renders at %dx the Wii's 640x456, scaled to the window.",
                       g.resolution);
    ImGui::Spacing();

    sectionHeading("Anti-aliasing");
    static const int kMsaa[] = {1, 4};
    static const char* kMsaaNames[] = {"Off", "4x"};
    int msaaIdx = g.msaa > 1 ? 1 : 0;
    if (pillChoice("msaa", &msaaIdx, kMsaaNames, 2)) {
        g.msaa = kMsaa[msaaIdx];
        changed = true;
    }
    ImGui::Spacing();

    sectionHeading("Texture filtering");
    static const int kAniso[] = {1, 2, 4, 8, 16};
    static const char* kAnisoNames[] = {"Off", "2x", "4x", "8x", "16x"};
    int anisoIdx = 0;
    for (int i = 0; i < 5; i++) {
        if (kAniso[i] == g.anisotropy) anisoIdx = i;
    }
    if (pillChoice("aniso", &anisoIdx, kAnisoNames, 5)) {
        g.anisotropy = kAniso[anisoIdx];
        changed = true;
    }
    if (g.msaa != start.msaa || g.anisotropy != start.anisotropy) {
        ImGui::TextColored(kGold, "Restart the game to apply anti-aliasing and texture filtering.");
    }
    ImGui::Spacing();

    sectionHeading("Display");
    if (ImGui::Checkbox("VSync", &g.vsync)) {
        graphics::applyRuntime();
        changed = true;
    }
    ImGui::SameLine(0.0f, 40.0f);
    int nWindows = 0;
    SDL_Window** windows = SDL_GetWindows(&nWindows);
    SDL_Window* window = (windows != nullptr && nWindows > 0) ? windows[0] : nullptr;
    SDL_free(windows);
    bool fullscreen = window != nullptr && (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0;
    if (ImGui::Checkbox("Fullscreen (F11)", &fullscreen) && window != nullptr) {
        SDL_SetWindowFullscreen(window, fullscreen);
        g.fullscreen = fullscreen;
        changed = true;
    }
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.75f, 0.85f, 1.0f, 1.0f), "%.0f frames per second. The game runs at 60, like the Wii.",
                       ImGui::GetIO().Framerate);
    ImGui::Spacing();

    sectionHeading("Texture packs");
    if (ImGui::Checkbox("Save the game's textures as you play (for making HD replacements)", &g.dumpTextures)) {
        changed = true;
    }
    if (g.dumpTextures != start.dumpTextures) {
        ImGui::TextColored(kGold, "Restart the game to apply.");
    }
    ImGui::TextColored(ImVec4(0.75f, 0.85f, 1.0f, 1.0f),
                       "Edit a saved image, keep its file name (as .png or .dds, any size), and put it in the\n"
                       "replacements folder; it is used the next time the game starts.");
    auto openFolder = [](const std::string& dir) {
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::u8path(dir), ec);
        std::string url = "file:///" + dir;
        for (char& ch : url) {
            if (ch == '\\') ch = '/';
        }
        SDL_OpenURL(url.c_str());
    };
    if (pillButton("Open saved textures", false)) {
        openFolder(graphics::dumpFolder());
    }
    ImGui::SameLine();
    if (pillButton("Open replacements folder", false)) {
        openFolder(graphics::replacementFolder());
    }

    if (changed) {
        graphics::save();
    }
}

// ---- Theme --------------------------------------------------------------------------------------------------
void applyTheme(float scale) {
    ImGuiStyle& st = ImGui::GetStyle();
    st = ImGuiStyle();
    st.WindowRounding = 26.0f;
    st.ChildRounding = 18.0f;
    st.FrameRounding = 18.0f;
    st.PopupRounding = 14.0f;
    st.GrabRounding = 18.0f;
    st.ScrollbarRounding = 12.0f;
    st.TabRounding = 14.0f;
    st.WindowBorderSize = 3.0f;
    st.FrameBorderSize = 1.5f;
    st.PopupBorderSize = 2.0f;
    st.WindowPadding = ImVec2(28.0f, 22.0f);
    st.FramePadding = ImVec2(16.0f, 7.0f);
    st.ItemSpacing = ImVec2(12.0f, 10.0f);
    st.ItemInnerSpacing = ImVec2(10.0f, 6.0f);
    st.CellPadding = ImVec2(8.0f, 5.0f);
    st.ScrollbarSize = 16.0f;
    st.GrabMinSize = 22.0f;
    st.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    st.ScaleAllSizes(scale);

    ImVec4* c = st.Colors;
    auto col = [](int r, int g, int b, int a = 255) { return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f); };
    c[ImGuiCol_Text] = col(255, 255, 255);
    c[ImGuiCol_TextDisabled] = col(150, 175, 225);
    c[ImGuiCol_WindowBg] = col(12, 26, 82, 238);
    c[ImGuiCol_ChildBg] = col(0, 0, 0, 0);
    c[ImGuiCol_PopupBg] = col(16, 34, 100, 248);
    c[ImGuiCol_Border] = col(150, 215, 255, 220);
    c[ImGuiCol_BorderShadow] = col(0, 0, 0, 0);
    c[ImGuiCol_FrameBg] = col(24, 56, 140, 235);
    c[ImGuiCol_FrameBgHovered] = col(44, 88, 186);
    c[ImGuiCol_FrameBgActive] = col(60, 112, 216);
    c[ImGuiCol_Button] = col(36, 96, 212);
    c[ImGuiCol_ButtonHovered] = col(72, 142, 255);
    c[ImGuiCol_ButtonActive] = col(255, 206, 48);
    c[ImGuiCol_Header] = col(40, 90, 200);
    c[ImGuiCol_HeaderHovered] = col(72, 132, 240);
    c[ImGuiCol_HeaderActive] = col(255, 206, 48);
    c[ImGuiCol_CheckMark] = col(255, 214, 60);
    c[ImGuiCol_SliderGrab] = col(255, 206, 48);
    c[ImGuiCol_SliderGrabActive] = col(255, 232, 140);
    c[ImGuiCol_Separator] = col(150, 215, 255, 90);
    c[ImGuiCol_ScrollbarBg] = col(0, 0, 0, 0);
    c[ImGuiCol_ScrollbarGrab] = col(80, 130, 230);
    c[ImGuiCol_ScrollbarGrabHovered] = col(110, 160, 255);
    c[ImGuiCol_ScrollbarGrabActive] = col(255, 206, 48);
    c[ImGuiCol_TableHeaderBg] = col(20, 46, 124);
    c[ImGuiCol_TableRowBg] = col(0, 0, 0, 0);
    c[ImGuiCol_TableRowBgAlt] = col(255, 255, 255, 12);
    c[ImGuiCol_TableBorderLight] = col(150, 215, 255, 40);
    c[ImGuiCol_TableBorderStrong] = col(150, 215, 255, 80);
    c[ImGuiCol_Tab] = col(36, 96, 212);
    c[ImGuiCol_TabHovered] = col(72, 142, 255);
    c[ImGuiCol_TabSelected] = col(255, 206, 48);
    c[ImGuiCol_PlotHistogram] = col(255, 206, 48);
    c[ImGuiCol_NavHighlight] = col(255, 206, 48);
    c[ImGuiCol_ModalWindowDimBg] = col(0, 0, 20, 140);
}

std::string findFont() {
    std::vector<std::filesystem::path> dirs;
#ifdef _WIN32
    const char* win = std::getenv("WINDIR");
    dirs.emplace_back(std::filesystem::path(win ? win : "C:\\Windows") / "Fonts");
    if (const char* local = std::getenv("LOCALAPPDATA")) {
        dirs.emplace_back(std::filesystem::path(local) / "Microsoft" / "Windows" / "Fonts");
    }
#else
    dirs.emplace_back("/usr/share/fonts/truetype/dejavu");
    dirs.emplace_back("/usr/share/fonts/truetype/liberation");
    dirs.emplace_back("/usr/share/fonts/TTF");
#endif
    // Rounded, heavy faces first: closest to the game's lettering.
    static const char* kNames[] = {"arlrdbd.ttf",  "seguibl.ttf",           "segoeuib.ttf",
                                   "arialbd.ttf",  "DejaVuSans-Bold.ttf",   "LiberationSans-Bold.ttf"};
    for (const char* name : kNames) {
        for (const auto& d : dirs) {
            std::error_code ec;
            const auto p = d / name;
            if (std::filesystem::exists(p, ec)) {
                return p.string();
            }
        }
    }
    return {};
}

}  // namespace

void initImGui(float displayScale) {
    const float scale = displayScale > 0.0f ? displayScale : 1.0f;
    ImGuiIO& io = ImGui::GetIO();
    const std::string font = findFont();
    ImFontConfig cfg;
    cfg.OversampleH = 2;
    if (!font.empty()) {
        sFontBody = io.Fonts->AddFontFromFileTTF(font.c_str(), 21.0f * scale, &cfg);
        sFontTitle = io.Fonts->AddFontFromFileTTF(font.c_str(), 40.0f * scale, &cfg);
    }
    if (sFontBody == nullptr) {
        ImFontConfig def;
        def.SizePixels = 20.0f * scale;
        sFontBody = io.Fonts->AddFontDefault(&def);
        def.SizePixels = 34.0f * scale;
        sFontTitle = io.Fonts->AddFontDefault(&def);
    }
    io.FontDefault = sFontBody;
    io.FontGlobalScale = 1.0f / scale;
    applyTheme(1.0f);
}

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
    sTime += dt;
    // Test aid: GALAXY_OPEN_MENU=<page 0-2> opens the menu at startup.
    static bool sCheckedEnv = false;
    if (!sCheckedEnv) {
        sCheckedEnv = true;
        if (const char* env = std::getenv("GALAXY_OPEN_MENU")) {
            sOpen = true;
            sPage = std::clamp(std::atoi(env), 0, 2);
        }
    }
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
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 20.0f, io.DisplaySize.y - 20.0f), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 40.0f);
        if (ImGui::Begin("##controls_hint", nullptr,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav)) {
            const float h = ImGui::GetFrameHeight();
            ImGui::Dummy(ImVec2(h, h));
            const ImVec2 starPos = ImGui::GetItemRectMin();
            drawStar(ImGui::GetWindowDrawList(), ImVec2(starPos.x + h * 0.5f, starPos.y + h * 0.5f),
                     h * (0.44f + 0.04f * std::sin(sTime * 3.0f)), kGoldU, kOutlineU, -IM_PI / 2 + 0.15f * std::sin(sTime));
            ImGui::SameLine();
            if (pillButton("Settings  (F1)", false)) {
                toggleMenu();
            }
        }
        ImGui::End();
        ImGui::PopStyleVar(2);
    }

    if (!sOpen) {
        return;
    }

    // Dim the game behind the menu.
    ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0, 0), io.DisplaySize, rgba(0, 4, 24, 110));

    const ImVec2 size(std::min(1000.0f, io.DisplaySize.x * 0.9f), std::min(720.0f, io.DisplaySize.y * 0.9f));
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (!ImGui::Begin("##settings", nullptr,
                      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                          ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar)) {
        ImGui::End();
        return;
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 wmin = ImGui::GetWindowPos();
    const ImVec2 wmax(wmin.x + ImGui::GetWindowWidth(), wmin.y + ImGui::GetWindowHeight());
    const float rounding = ImGui::GetStyle().WindowRounding;

    // Panel decoration: a lighter band at the top, a starfield, an inner glow line.
    dl->AddRectFilled(ImVec2(wmin.x + 3, wmin.y + 3), ImVec2(wmax.x - 3, wmin.y + 92), rgba(60, 120, 230, 70), rounding,
                      ImDrawFlags_RoundCornersTop);
    drawStarfield(dl, ImVec2(wmin.x + 20, wmin.y + 12), ImVec2(wmax.x - 20, wmax.y - 12), 70, 0x5EED);
    dl->AddRect(ImVec2(wmin.x + 7, wmin.y + 7), ImVec2(wmax.x - 7, wmax.y - 7), rgba(120, 190, 255, 60), rounding - 5, 0, 2.0f);

    // Title.
    const float titleSize = sFontTitle != nullptr ? sFontTitle->FontSize * io.FontGlobalScale : 34.0f;
    const ImVec2 titlePos(wmin.x + 76, wmin.y + 22);
    drawStar(dl, ImVec2(wmin.x + 46, titlePos.y + titleSize * 0.55f), titleSize * 0.55f, kGoldU, kOutlineU,
             -IM_PI / 2 + 0.12f * std::sin(sTime * 1.5f));
    outlinedText(dl, sFontTitle, titleSize, titlePos, rgba(255, 255, 255), "Settings", 2.5f);
    ImGui::SetCursorScreenPos(ImVec2(wmax.x - 190, wmin.y + 30));
    if (pillButton("Close  (F1)", false, ImVec2(160, 0))) {
        toggleMenu();
    }

    // Page selector.
    ImGui::SetCursorScreenPos(ImVec2(wmin.x + 28, wmin.y + 104));
    static const char* kPages[] = {"Controls", "Pointer & Sticks", "Graphics"};
    for (int i = 0; i < 3; i++) {
        if (i > 0) {
            ImGui::SameLine();
        }
        if (pillButton(kPages[i], sPage == i, ImVec2(0, ImGui::GetFrameHeight() * 1.15f))) {
            sPage = i;
        }
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::BeginChild("##page", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground)) {
        switch (sPage) {
        case 0:
            drawControlsPage();
            break;
        case 1:
            drawPointerPage();
            break;
        default:
            drawGraphicsPage();
            break;
        }
    }
    ImGui::EndChild();
    ImGui::End();

    if (!sOpen && sCaptureAction >= 0) {
        input::cancelCapture();
        sCaptureAction = sCaptureSlot = -1;
    }
}

}  // namespace pc::ui
