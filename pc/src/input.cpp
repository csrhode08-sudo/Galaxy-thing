// Galaxy PC input layer: Wii Remote + Nunchuk emulation over SDL3 keyboard, mouse and gamepads.
#include <cstdlib>
#include <string>
#include <vector>
#include "pc/input.h"
#include "pc/platform.h"

#include <revolution/kpad.h>

#include <SDL3/SDL.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <fstream>
#include <mutex>
#include <sstream>

namespace pc::input {
namespace {

std::recursive_mutex sMutex;
Settings sSettings;
std::string sConfigPath;

// Live device state
bool sKeys[SDL_SCANCODE_COUNT];
bool sMouseButtons[8];
float sMouseX = 0.5f, sMouseY = 0.5f;  // normalized window coordinates
bool sMouseInWindow = false;
SDL_Gamepad* sPad = nullptr;
float sPadPointerX = 0.0f, sPadPointerY = 0.0f;  // KPAD space (-1..1)
bool sPadPointerActive = false;                    // last pointer input came from the gamepad

// Per-frame derived state
u32 sPrevHold = 0;
int sShakeFrames = 0;
bool sPrevSpin = false;
bool sRumble = false;

// Remap capture
bool sCapturing = false;
bool sCaptured = false;
Binding sCaptureResult;

constexpr int kStatusesPerFrame = 3;  // the Wii samples the remote at ~200 Hz; emulate ~180 Hz

const char* const kActionNames[(int)Action::Count] = {
    "Move Up",        "Move Down",      "Move Left",      "Move Right",     "Crouch / Ground Pound (Z)",
    "Reset Camera (C)", "Jump (A)",     "Shoot Star Bits (B)", "Spin (Shake)", "First-Person View (D-Pad Up)",
    "D-Pad Down",     "D-Pad Left",     "D-Pad Right",    "Pause (+)",      "Star List (-)",
    "1",              "2",              "Home Menu",      "Pointer Up (gamepad)", "Pointer Down (gamepad)",
    "Pointer Left (gamepad)", "Pointer Right (gamepad)", "Center Pointer (gamepad)",
};

Binding key(SDL_Scancode s) { return {BindKind::Key, (int)s}; }
Binding mouse(int b) { return {BindKind::MouseButton, b}; }
Binding pad(SDL_GamepadButton b) { return {BindKind::PadButton, (int)b}; }
Binding axisPos(SDL_GamepadAxis a) { return {BindKind::PadAxisPos, (int)a}; }
Binding axisNeg(SDL_GamepadAxis a) { return {BindKind::PadAxisNeg, (int)a}; }

void setBinds(Settings& s, Action a, Binding b0, Binding b1 = {}, Binding b2 = {}) {
    s.binds[(int)a][0] = b0;
    s.binds[(int)a][1] = b1;
    s.binds[(int)a][2] = b2;
}

float axisValue(SDL_GamepadAxis a) {
    if (!sPad) {
        return 0.0f;
    }
    float v = SDL_GetGamepadAxis(sPad, a) / 32767.0f;
    return std::clamp(v, -1.0f, 1.0f);
}

// Analog strength (0..1) of one binding.
float bindingValue(const Binding& b) {
    switch (b.kind) {
    case BindKind::Key:
        return (b.code >= 0 && b.code < SDL_SCANCODE_COUNT && sKeys[b.code]) ? 1.0f : 0.0f;
    case BindKind::MouseButton:
        return (b.code >= 0 && b.code < 8 && sMouseButtons[b.code]) ? 1.0f : 0.0f;
    case BindKind::PadButton:
        return (sPad && SDL_GetGamepadButton(sPad, (SDL_GamepadButton)b.code)) ? 1.0f : 0.0f;
    case BindKind::PadAxisPos: {
        float v = axisValue((SDL_GamepadAxis)b.code);
        return v > 0.0f ? v : 0.0f;
    }
    case BindKind::PadAxisNeg: {
        float v = axisValue((SDL_GamepadAxis)b.code);
        return v < 0.0f ? -v : 0.0f;
    }
    default:
        return 0.0f;
    }
}

float actionValue(Action a) {
    float v = 0.0f;
    for (const Binding& b : sSettings.binds[(int)a]) {
        v = std::max(v, bindingValue(b));
    }
    return v;
}

bool actionHeld(Action a) {
    // Triggers/axes count as pressed past half travel.
    return actionValue(a) >= 0.5f;
}

float actionAxis(Action neg, Action pos) {
    return actionValue(pos) - actionValue(neg);
}

void openFirstGamepad() {
    if (sPad) {
        return;
    }
    int count = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&count);
    if (ids) {
        if (count > 0) {
            sPad = SDL_OpenGamepad(ids[0]);
            if (sPad) {
                pc_log("[input] Using gamepad: %s\n", SDL_GetGamepadName(sPad));
            }
        }
        SDL_free(ids);
    }
}

// ---- config file -------------------------------------------------------------------------

std::string encodeBinding(const Binding& b) {
    switch (b.kind) {
    case BindKind::Key: return std::string("key:") + SDL_GetScancodeName((SDL_Scancode)b.code);
    case BindKind::MouseButton: return "mouse:" + std::to_string(b.code);
    case BindKind::PadButton: return std::string("pad:") + SDL_GetGamepadStringForButton((SDL_GamepadButton)b.code);
    case BindKind::PadAxisPos: return std::string("axis+:") + SDL_GetGamepadStringForAxis((SDL_GamepadAxis)b.code);
    case BindKind::PadAxisNeg: return std::string("axis-:") + SDL_GetGamepadStringForAxis((SDL_GamepadAxis)b.code);
    default: return "none";
    }
}

Binding decodeBinding(const std::string& s) {
    size_t colon = s.find(':');
    if (colon == std::string::npos) {
        return {};
    }
    std::string tag = s.substr(0, colon), val = s.substr(colon + 1);
    Binding b;
    if (tag == "key") {
        SDL_Scancode sc = SDL_GetScancodeFromName(val.c_str());
        if (sc != SDL_SCANCODE_UNKNOWN) b = key(sc);
    } else if (tag == "mouse") {
        b = mouse(std::atoi(val.c_str()));
    } else if (tag == "pad") {
        SDL_GamepadButton gb = SDL_GetGamepadButtonFromString(val.c_str());
        if (gb != SDL_GAMEPAD_BUTTON_INVALID) b = pad(gb);
    } else if (tag == "axis+" || tag == "axis-") {
        SDL_GamepadAxis ga = SDL_GetGamepadAxisFromString(val.c_str());
        if (ga != SDL_GAMEPAD_AXIS_INVALID) b = tag == "axis+" ? axisPos(ga) : axisNeg(ga);
    }
    return b;
}

std::string actionKey(Action a) {
    std::string n = actionName(a);
    std::string out;
    for (char c : n) {
        if (std::isalnum((unsigned char)c)) out += (char)std::tolower((unsigned char)c);
        else if (!out.empty() && out.back() != '_') out += '_';
    }
    while (!out.empty() && out.back() == '_') out.pop_back();
    return out;
}

void load() {
    std::ifstream f(sConfigPath);
    if (!f) {
        return;
    }
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = line.substr(0, eq), v = line.substr(eq + 1);
        auto asBool = [&] { return v == "1" || v == "true"; };
        if (k == "mouse_pointer") sSettings.mousePointer = asBool();
        else if (k == "hide_cursor") sSettings.hideCursor = asBool();
        else if (k == "stick_deadzone") sSettings.stickDeadzone = std::clamp((float)std::atof(v.c_str()), 0.0f, 0.9f);
        else if (k == "pad_pointer_speed") sSettings.padPointerSpeed = std::clamp((float)std::atof(v.c_str()), 0.1f, 10.0f);
        else if (k == "invert_pad_pointer_y") sSettings.invertPadPointerY = asBool();
        else if (k == "rumble") sSettings.rumble = asBool();
        else {
            for (int a = 0; a < (int)Action::Count; a++) {
                if (k == actionKey((Action)a)) {
                    std::stringstream ss(v);
                    std::string item;
                    for (int i = 0; i < kBindingsPerAction; i++) {
                        sSettings.binds[a][i] = std::getline(ss, item, ',') ? decodeBinding(item) : Binding{};
                    }
                }
            }
        }
    }
}

}  // namespace

const char* actionName(Action a) {
    return (int)a >= 0 && a < Action::Count ? kActionNames[(int)a] : "?";
}

std::string bindingName(const Binding& b) {
    switch (b.kind) {
    case BindKind::Key: {
        const char* n = SDL_GetScancodeName((SDL_Scancode)b.code);
        return n && *n ? n : "Key?";
    }
    case BindKind::MouseButton:
        switch (b.code) {
        case SDL_BUTTON_LEFT: return "Mouse Left";
        case SDL_BUTTON_RIGHT: return "Mouse Right";
        case SDL_BUTTON_MIDDLE: return "Mouse Middle";
        case SDL_BUTTON_X1: return "Mouse Back";
        case SDL_BUTTON_X2: return "Mouse Forward";
        default: return "Mouse " + std::to_string(b.code);
        }
    case BindKind::PadButton:
        switch ((SDL_GamepadButton)b.code) {
        case SDL_GAMEPAD_BUTTON_SOUTH: return "Pad A";
        case SDL_GAMEPAD_BUTTON_EAST: return "Pad B";
        case SDL_GAMEPAD_BUTTON_WEST: return "Pad X";
        case SDL_GAMEPAD_BUTTON_NORTH: return "Pad Y";
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return "Pad LB";
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return "Pad RB";
        case SDL_GAMEPAD_BUTTON_LEFT_STICK: return "Pad L3";
        case SDL_GAMEPAD_BUTTON_RIGHT_STICK: return "Pad R3";
        case SDL_GAMEPAD_BUTTON_START: return "Pad Start";
        case SDL_GAMEPAD_BUTTON_BACK: return "Pad Back";
        case SDL_GAMEPAD_BUTTON_GUIDE: return "Pad Guide";
        case SDL_GAMEPAD_BUTTON_DPAD_UP: return "Pad D-Up";
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return "Pad D-Down";
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT: return "Pad D-Left";
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return "Pad D-Right";
        default: return std::string("Pad ") + SDL_GetGamepadStringForButton((SDL_GamepadButton)b.code);
        }
    case BindKind::PadAxisPos:
    case BindKind::PadAxisNeg: {
        bool pos = b.kind == BindKind::PadAxisPos;
        switch ((SDL_GamepadAxis)b.code) {
        case SDL_GAMEPAD_AXIS_LEFTX: return pos ? "Left Stick Right" : "Left Stick Left";
        case SDL_GAMEPAD_AXIS_LEFTY: return pos ? "Left Stick Down" : "Left Stick Up";
        case SDL_GAMEPAD_AXIS_RIGHTX: return pos ? "Right Stick Right" : "Right Stick Left";
        case SDL_GAMEPAD_AXIS_RIGHTY: return pos ? "Right Stick Down" : "Right Stick Up";
        case SDL_GAMEPAD_AXIS_LEFT_TRIGGER: return "Left Trigger";
        case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER: return "Right Trigger";
        default: return "Axis";
        }
    }
    default:
        return "";
    }
}

Settings defaultSettings() {
    Settings s;
    setBinds(s, Action::MoveUp, key(SDL_SCANCODE_W), axisNeg(SDL_GAMEPAD_AXIS_LEFTY));
    setBinds(s, Action::MoveDown, key(SDL_SCANCODE_S), axisPos(SDL_GAMEPAD_AXIS_LEFTY));
    setBinds(s, Action::MoveLeft, key(SDL_SCANCODE_A), axisNeg(SDL_GAMEPAD_AXIS_LEFTX));
    setBinds(s, Action::MoveRight, key(SDL_SCANCODE_D), axisPos(SDL_GAMEPAD_AXIS_LEFTX));
    setBinds(s, Action::Z, key(SDL_SCANCODE_LSHIFT), axisPos(SDL_GAMEPAD_AXIS_LEFT_TRIGGER));
    setBinds(s, Action::C, key(SDL_SCANCODE_Q), pad(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER));
    setBinds(s, Action::A, key(SDL_SCANCODE_SPACE), pad(SDL_GAMEPAD_BUTTON_SOUTH));
    setBinds(s, Action::B, mouse(SDL_BUTTON_LEFT), axisPos(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER));
    setBinds(s, Action::Spin, mouse(SDL_BUTTON_RIGHT), pad(SDL_GAMEPAD_BUTTON_WEST), key(SDL_SCANCODE_E));
    setBinds(s, Action::DUp, key(SDL_SCANCODE_UP), pad(SDL_GAMEPAD_BUTTON_DPAD_UP));
    setBinds(s, Action::DDown, key(SDL_SCANCODE_DOWN), pad(SDL_GAMEPAD_BUTTON_DPAD_DOWN));
    setBinds(s, Action::DLeft, key(SDL_SCANCODE_LEFT), pad(SDL_GAMEPAD_BUTTON_DPAD_LEFT));
    setBinds(s, Action::DRight, key(SDL_SCANCODE_RIGHT), pad(SDL_GAMEPAD_BUTTON_DPAD_RIGHT));
    setBinds(s, Action::Plus, key(SDL_SCANCODE_RETURN), pad(SDL_GAMEPAD_BUTTON_START));
    setBinds(s, Action::Minus, key(SDL_SCANCODE_TAB), pad(SDL_GAMEPAD_BUTTON_BACK));
    setBinds(s, Action::One, key(SDL_SCANCODE_1), pad(SDL_GAMEPAD_BUTTON_NORTH));
    setBinds(s, Action::Two, key(SDL_SCANCODE_2), pad(SDL_GAMEPAD_BUTTON_EAST));
    setBinds(s, Action::Home, key(SDL_SCANCODE_H), pad(SDL_GAMEPAD_BUTTON_GUIDE));
    setBinds(s, Action::PointerUp, axisNeg(SDL_GAMEPAD_AXIS_RIGHTY));
    setBinds(s, Action::PointerDown, axisPos(SDL_GAMEPAD_AXIS_RIGHTY));
    setBinds(s, Action::PointerLeft, axisNeg(SDL_GAMEPAD_AXIS_RIGHTX));
    setBinds(s, Action::PointerRight, axisPos(SDL_GAMEPAD_AXIS_RIGHTX));
    setBinds(s, Action::PointerCenter, pad(SDL_GAMEPAD_BUTTON_RIGHT_STICK), key(SDL_SCANCODE_R));
    return s;
}

Settings& settings() {
    return sSettings;
}

void save() {
    std::lock_guard lock(sMutex);
    std::ofstream f(sConfigPath, std::ios::trunc);
    if (!f) {
        pc_log("[input] Could not write %s\n", sConfigPath.c_str());
        return;
    }
    f << "# Super Mario Galaxy PC controls. Edit in game with F1, or by hand.\n";
    f << "# Bindings: key:<SDL key name>, mouse:<1=left 2=middle 3=right 4,5=side>, pad:<button>, axis+:<axis>, axis-:<axis>\n";
    for (int a = 0; a < (int)Action::Count; a++) {
        f << actionKey((Action)a) << '=';
        for (int i = 0; i < kBindingsPerAction; i++) {
            if (i) f << ',';
            f << encodeBinding(sSettings.binds[a][i]);
        }
        f << '\n';
    }
    f << "mouse_pointer=" << (sSettings.mousePointer ? 1 : 0) << '\n';
    f << "hide_cursor=" << (sSettings.hideCursor ? 1 : 0) << '\n';
    f << "stick_deadzone=" << sSettings.stickDeadzone << '\n';
    f << "pad_pointer_speed=" << sSettings.padPointerSpeed << '\n';
    f << "invert_pad_pointer_y=" << (sSettings.invertPadPointerY ? 1 : 0) << '\n';
    f << "rumble=" << (sSettings.rumble ? 1 : 0) << '\n';
}

void init(const std::string& configDir) {
    std::lock_guard lock(sMutex);
    sSettings = defaultSettings();
    sConfigPath = configDir + "/controls.ini";
    load();
    std::memset(sKeys, 0, sizeof(sKeys));
    std::memset(sMouseButtons, 0, sizeof(sMouseButtons));
    openFirstGamepad();
}

void shutdown() {
    std::lock_guard lock(sMutex);
    if (sPad) {
        SDL_CloseGamepad(sPad);
        sPad = nullptr;
    }
}

void handleEvent(const SDL_Event& ev) {
    std::lock_guard lock(sMutex);
    switch (ev.type) {
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        if (ev.key.scancode < SDL_SCANCODE_COUNT) {
            sKeys[ev.key.scancode] = ev.type == SDL_EVENT_KEY_DOWN;
            if (sCapturing && !sCaptured && ev.type == SDL_EVENT_KEY_DOWN && !ev.key.repeat) {
                sCaptureResult = key(ev.key.scancode);
                sCaptured = true;
            }
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (ev.button.button < 8) {
            sMouseButtons[ev.button.button] = ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
            if (sCapturing && !sCaptured && ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                sCaptureResult = mouse(ev.button.button);
                sCaptured = true;
            }
        }
        break;
    case SDL_EVENT_MOUSE_MOTION: {
        SDL_Window* w = SDL_GetWindowFromID(ev.motion.windowID);
        int ww = 1, wh = 1;
        if (w) SDL_GetWindowSize(w, &ww, &wh);
        sMouseX = ev.motion.x / std::max(1, ww);
        sMouseY = ev.motion.y / std::max(1, wh);
        sMouseInWindow = true;
        sPadPointerActive = false;
        break;
    }
    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        sMouseInWindow = false;
        break;
    case SDL_EVENT_WINDOW_MOUSE_ENTER:
        sMouseInWindow = true;
        break;
    case SDL_EVENT_GAMEPAD_ADDED:
        openFirstGamepad();
        break;
    case SDL_EVENT_GAMEPAD_REMOVED:
        if (sPad && SDL_GetGamepadID(sPad) == ev.gdevice.which) {
            SDL_CloseGamepad(sPad);
            sPad = nullptr;
            openFirstGamepad();
        }
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        if (sCapturing && !sCaptured) {
            sCaptureResult = pad((SDL_GamepadButton)ev.gbutton.button);
            sCaptured = true;
        }
        break;
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        if (sCapturing && !sCaptured && std::abs(ev.gaxis.value) > 24000) {
            sCaptureResult = ev.gaxis.value > 0 ? axisPos((SDL_GamepadAxis)ev.gaxis.axis) : axisNeg((SDL_GamepadAxis)ev.gaxis.axis);
            sCaptured = true;
        }
        break;
    default:
        break;
    }
}

// Test automation: GALAXY_AUTOINPUT="A@600:10,STICKU@900:120,..." holds a button or stick direction from a frame
// for a number of frames. Used to drive the game in unattended test runs.
namespace {
struct AutoInput {
    std::string name;
    u32 start;
    u32 frames;
};
std::vector<AutoInput> sAutoInputs;
u32 sFrameCounter;
bool sAutoParsed;

void parseAutoInput() {
    sAutoParsed = true;
    const char* env = std::getenv("GALAXY_AUTOINPUT");
    if (env == nullptr) {
        return;
    }
    std::string spec(env);
    size_t pos = 0;
    while (pos < spec.size()) {
        size_t end = spec.find(',', pos);
        if (end == std::string::npos) end = spec.size();
        std::string tok = spec.substr(pos, end - pos);
        size_t at = tok.find('@');
        if (at != std::string::npos) {
            AutoInput in;
            in.name = tok.substr(0, at);
            size_t colon = tok.find(':', at);
            in.start = (u32)std::strtoul(tok.c_str() + at + 1, nullptr, 10);
            in.frames = colon != std::string::npos ? (u32)std::strtoul(tok.c_str() + colon + 1, nullptr, 10) : 6;
            sAutoInputs.push_back(in);
        }
        pos = end + 1;
    }
}

bool autoActive(const char* name) {
    for (const AutoInput& in : sAutoInputs) {
        if (in.name == name && sFrameCounter >= in.start && sFrameCounter < in.start + in.frames) {
            return true;
        }
    }
    return false;
}
}  // namespace

void newFrame(float dt) {
    std::lock_guard lock(sMutex);
    sFrameCounter++;
    if (sCapturing) {
        return;
    }

    // Gamepad pointer: velocity-controlled cursor on the right stick (or whatever is bound).
    float px = actionAxis(Action::PointerLeft, Action::PointerRight);
    float py = actionAxis(Action::PointerUp, Action::PointerDown);
    float mag = std::sqrt(px * px + py * py);
    if (mag > sSettings.stickDeadzone) {
        float scale = (mag - sSettings.stickDeadzone) / (1.0f - sSettings.stickDeadzone) / mag;
        px *= scale;
        py *= scale;
        if (sSettings.invertPadPointerY) py = -py;
        if (!sPadPointerActive) {
            sPadPointerX = sMouseX * 2.0f - 1.0f;
            sPadPointerY = sMouseY * 2.0f - 1.0f;
            sPadPointerActive = true;
        }
        sPadPointerX = std::clamp(sPadPointerX + px * sSettings.padPointerSpeed * 2.0f * dt, -1.0f, 1.0f);
        sPadPointerY = std::clamp(sPadPointerY + py * sSettings.padPointerSpeed * 2.0f * dt, -1.0f, 1.0f);
    }
    if (actionHeld(Action::PointerCenter)) {
        sPadPointerX = sPadPointerY = 0.0f;
        sPadPointerActive = true;
    }

    // Spin: a press starts a short burst of acceleration, like flicking the remote.
    bool spin = actionHeld(Action::Spin);
    if (spin && !sPrevSpin) {
        sShakeFrames = 8;
    }
    sPrevSpin = spin;
}

int readKPAD(int chan, KPADStatus* out, int count) {
    std::lock_guard lock(sMutex);
    if (chan != 0 || count <= 0 || out == nullptr) {
        return 0;
    }

    u32 hold = 0;
    if (!sCapturing) {
        struct {
            Action a;
            u32 bit;
        } const map[] = {
            {Action::A, WPAD_BUTTON_A},       {Action::B, WPAD_BUTTON_B},        {Action::Z, WPAD_BUTTON_Z},
            {Action::C, WPAD_BUTTON_C},       {Action::DUp, WPAD_BUTTON_UP},     {Action::DDown, WPAD_BUTTON_DOWN},
            {Action::DLeft, WPAD_BUTTON_LEFT}, {Action::DRight, WPAD_BUTTON_RIGHT}, {Action::Plus, WPAD_BUTTON_PLUS},
            {Action::Minus, WPAD_BUTTON_MINUS}, {Action::One, WPAD_BUTTON_1},     {Action::Two, WPAD_BUTTON_2},
            {Action::Home, WPAD_BUTTON_HOME},
        };
        for (const auto& m : map) {
            if (actionHeld(m.a)) hold |= m.bit;
        }
        if (!sAutoParsed) parseAutoInput();
        if (!sAutoInputs.empty()) {
            static const struct { const char* n; u32 bit; } kAuto[] = {
                {"A", WPAD_BUTTON_A}, {"B", WPAD_BUTTON_B}, {"Z", WPAD_BUTTON_Z}, {"C", WPAD_BUTTON_C},
                {"UP", WPAD_BUTTON_UP}, {"DOWN", WPAD_BUTTON_DOWN}, {"LEFT", WPAD_BUTTON_LEFT}, {"RIGHT", WPAD_BUTTON_RIGHT},
                {"PLUS", WPAD_BUTTON_PLUS}, {"MINUS", WPAD_BUTTON_MINUS}, {"1", WPAD_BUTTON_1}, {"2", WPAD_BUTTON_2},
            };
            for (const auto& a : kAuto) {
                if (autoActive(a.n)) hold |= a.bit;
            }
        }
    }

    // Nunchuk stick, radial deadzone, clamped to the unit circle.
    float sx = 0.0f, sy = 0.0f;
    if (!sCapturing) {
        sx = actionAxis(Action::MoveLeft, Action::MoveRight);
        sy = actionAxis(Action::MoveDown, Action::MoveUp);  // KPAD stick: +y is up
        if (autoActive("STICKU")) sy = 1.0f;
        if (autoActive("STICKD")) sy = -1.0f;
        if (autoActive("STICKL")) sx = -1.0f;
        if (autoActive("STICKR")) sx = 1.0f;
        float m = std::sqrt(sx * sx + sy * sy);
        if (m < sSettings.stickDeadzone) {
            sx = sy = 0.0f;
        } else {
            float scaled = std::min(1.0f, (m - sSettings.stickDeadzone) / (1.0f - sSettings.stickDeadzone));
            sx = sx / m * scaled;
            sy = sy / m * scaled;
        }
    }

    float posX, posY;
    bool pointerValid;
    if (sPadPointerActive) {
        posX = sPadPointerX;
        posY = sPadPointerY;
        pointerValid = true;
    } else {
        posX = sMouseX * 2.0f - 1.0f;
        posY = sMouseY * 2.0f - 1.0f;
        pointerValid = sSettings.mousePointer && sMouseInWindow;
    }

    int n = std::min(count, kStatusesPerFrame);
    for (int i = 0; i < n; i++) {
        KPADStatus& s = out[i];
        std::memset(&s, 0, sizeof(s));
        s.hold = hold;
        if (i == 0) {
            s.trig = hold & ~sPrevHold;
            s.release = sPrevHold & ~hold;
        }

        // Remote held pointing at the screen, at rest: gravity along -y in KPAD space.
        Vec acc = {0.0f, -1.0f, 0.0f};
        if (sShakeFrames > 0) {
            float phase = (sShakeFrames & 1) ? 1.0f : -1.0f;
            acc.x = 2.5f * phase;
            acc.y = -1.0f + 1.5f;
            acc.z = 1.5f * phase;
        }
        s.acc = acc;
        s.acc_value = std::sqrt(acc.x * acc.x + acc.y * acc.y + acc.z * acc.z);
        s.acc_speed = 0.0f;
        s.acc_vertical.x = 0.0f;
        s.acc_vertical.y = -1.0f;

        s.pos.x = posX;
        s.pos.y = posY;
        s.horizon.x = 1.0f;
        s.horizon.y = 0.0f;
        s.dist = 1.0f;
        s.dpd_valid_fg = pointerValid ? 2 : 0;

        s.dev_type = WPAD_DEV_FREESTYLE;
        s.wpad_err = WPAD_ERR_NONE;
        s.data_format = 0;
        s.ex_status.fs.stick.x = sx;
        s.ex_status.fs.stick.y = sy;
        s.ex_status.fs.acc.x = 0.0f;
        s.ex_status.fs.acc.y = -1.0f;
        s.ex_status.fs.acc.z = 0.0f;
        s.ex_status.fs.acc_value = 1.0f;
        s.ex_status.fs.acc_speed = 0.0f;
    }
    if (sShakeFrames > 0) {
        sShakeFrames--;
    }
    sPrevHold = hold;
    return n;
}

void setRumble(int chan, bool on) {
    std::lock_guard lock(sMutex);
    if (chan != 0 || !sPad) {
        return;
    }
    if (on != sRumble) {
        sRumble = on;
        SDL_RumbleGamepad(sPad, on && sSettings.rumble ? 0x6000 : 0, on && sSettings.rumble ? 0x9000 : 0, on ? 1000 : 0);
    }
}

void beginCapture() {
    std::lock_guard lock(sMutex);
    sCapturing = true;
    sCaptured = false;
}

bool pollCapture(Binding* out) {
    std::lock_guard lock(sMutex);
    if (sCapturing && sCaptured) {
        *out = sCaptureResult;
        sCapturing = false;
        sCaptured = false;
        return true;
    }
    return false;
}

void cancelCapture() {
    std::lock_guard lock(sMutex);
    sCapturing = false;
    sCaptured = false;
}

bool isCapturing() {
    std::lock_guard lock(sMutex);
    return sCapturing;
}

}  // namespace pc::input
