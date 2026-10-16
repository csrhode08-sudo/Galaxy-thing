// Galaxy PC input layer: emulates a Wii Remote + Nunchuk from keyboard, mouse and gamepads.
#pragma once

#include <SDL3/SDL_events.h>
#include <string>

struct KPADStatus;

namespace pc::input {

// Every remappable thing the player can do. Order is the order shown in the controls menu.
enum class Action : int {
    // Nunchuk
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    Z,  // crouch / ground pound
    C,  // camera reset
    // Wii Remote buttons
    A,      // jump
    B,      // shoot Star Bits
    Spin,   // shake the Wii Remote
    DUp,    // first-person view
    DDown,
    DLeft,
    DRight,
    Plus,   // pause
    Minus,  // star list
    One,
    Two,
    Home,
    // Pointer (for gamepads; the mouse moves the pointer directly)
    PointerUp,
    PointerDown,
    PointerLeft,
    PointerRight,
    PointerCenter,
    Count
};

enum class BindKind : int { None, Key, MouseButton, PadButton, PadAxisPos, PadAxisNeg };

struct Binding {
    BindKind kind = BindKind::None;
    int code = 0;  // SDL_Scancode, mouse button index, SDL_GamepadButton or SDL_GamepadAxis

    bool operator==(const Binding& o) const { return kind == o.kind && code == o.code; }
};

constexpr int kBindingsPerAction = 3;

struct Settings {
    Binding binds[(int)Action::Count][kBindingsPerAction];
    bool mousePointer = true;        // the mouse cursor is the Wii Remote pointer
    bool hideCursor = true;          // hide the OS cursor over the game window
    float stickDeadzone = 0.20f;     // gamepad stick deadzone (0..1)
    float padPointerSpeed = 1.6f;    // gamepad pointer speed (screen widths per second)
    bool invertPadPointerY = false;
    bool rumble = true;
};

void init(const std::string& configDir);
void shutdown();
void handleEvent(const SDL_Event& ev);  // feed every SDL event
void newFrame(float dt);               // once per game frame, before the game reads input

// Fills KPAD statuses for a channel; returns how many were written (0 if nothing connected).
int readKPAD(int chan, KPADStatus* out, int count);
void setRumble(int chan, bool on);

Settings& settings();
Settings defaultSettings();
void save();

const char* actionName(Action a);
std::string bindingName(const Binding& b);

// Remap support: while capturing, the next key/button/axis press is returned instead of driving the game.
void beginCapture();
bool pollCapture(Binding* out);  // true once something was pressed
void cancelCapture();
bool isCapturing();

}  // namespace pc::input
