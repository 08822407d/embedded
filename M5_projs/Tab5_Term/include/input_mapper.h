#pragma once

#include <stddef.h>
#include <stdint.h>

namespace input {

// Shared input vocabulary. Device drivers translate hardware reports into
// KeyEvent; input_mapper translates KeyEvent into terminal bytes using the
// current terminal modes. This keeps A164, USB-A, and future transports from
// duplicating xterm sequence logic.
enum Modifier : uint8_t {
    ModifierNone = 0,
    ModifierShift = 1 << 0,
    ModifierCtrl = 1 << 1,
    ModifierAlt = 1 << 2,
    ModifierMeta = 1 << 3,
};

enum class KeyAction : uint8_t {
    Press,
    Release,
    Repeat,
};

enum class KeyCode : uint8_t {
    Unknown,
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    Digit1,
    Digit2,
    Digit3,
    Digit4,
    Digit5,
    Digit6,
    Digit7,
    Digit8,
    Digit9,
    Digit0,
    Enter,
    Escape,
    Backspace,
    Tab,
    Space,
    Minus,
    Equal,
    LeftBracket,
    RightBracket,
    Backslash,
    Semicolon,
    Apostrophe,
    Grave,
    Comma,
    Period,
    Slash,
    Insert,
    Home,
    PageUp,
    Delete,
    End,
    PageDown,
    ArrowRight,
    ArrowLeft,
    ArrowDown,
    ArrowUp,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    KeypadDivide,
    KeypadMultiply,
    KeypadMinus,
    KeypadPlus,
    KeypadEnter,
    Keypad1,
    Keypad2,
    Keypad3,
    Keypad4,
    Keypad5,
    Keypad6,
    Keypad7,
    Keypad8,
    Keypad9,
    Keypad0,
    KeypadDecimal,
    KeypadEqual,
};

struct KeyEvent {
    KeyCode key = KeyCode::Unknown;
    uint8_t modifiers = ModifierNone;
    KeyAction action = KeyAction::Press;
};

struct TerminalModes {
    bool application_cursor = false;
    bool application_keypad = false;
};

constexpr size_t kMaxEncodedInputBytes = 16;

struct EncodedInput {
    uint8_t bytes[kMaxEncodedInputBytes] = {};
    uint8_t length = 0;
};

// Release events intentionally encode to zero bytes; they are present so
// drivers can share press/release/repeat state without transport-specific
// special cases.
EncodedInput mapKeyEvent(const KeyEvent& event, const TerminalModes& modes);

} // namespace input
