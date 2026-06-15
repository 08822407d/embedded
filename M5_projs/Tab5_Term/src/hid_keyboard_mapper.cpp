#include "hid_keyboard_mapper.h"

namespace input {
namespace {
constexpr uint8_t kHidLeftCtrl = 0x01;
constexpr uint8_t kHidLeftShift = 0x02;
constexpr uint8_t kHidLeftAlt = 0x04;
constexpr uint8_t kHidLeftGui = 0x08;
constexpr uint8_t kHidRightCtrl = 0x10;
constexpr uint8_t kHidRightShift = 0x20;
constexpr uint8_t kHidRightAlt = 0x40;
constexpr uint8_t kHidRightGui = 0x80;
} // namespace

uint8_t normalizeHidModifiers(uint8_t hid_modifiers)
{
    uint8_t modifiers = ModifierNone;
    if ((hid_modifiers & (kHidLeftShift | kHidRightShift)) != 0) {
        modifiers |= ModifierShift;
    }
    if ((hid_modifiers & (kHidLeftCtrl | kHidRightCtrl)) != 0) {
        modifiers |= ModifierCtrl;
    }
    if ((hid_modifiers & (kHidLeftAlt | kHidRightAlt)) != 0) {
        modifiers |= ModifierAlt;
    }
    if ((hid_modifiers & (kHidLeftGui | kHidRightGui)) != 0) {
        modifiers |= ModifierMeta;
    }
    return modifiers;
}

KeyCode hidUsageToKeyCode(uint8_t usage)
{
    if (usage >= 0x04 && usage <= 0x1D) {
        return static_cast<KeyCode>(
            static_cast<uint8_t>(KeyCode::A) + usage - 0x04);
    }
    if (usage >= 0x1E && usage <= 0x27) {
        return static_cast<KeyCode>(
            static_cast<uint8_t>(KeyCode::Digit1) + usage - 0x1E);
    }
    if (usage >= 0x3A && usage <= 0x45) {
        return static_cast<KeyCode>(
            static_cast<uint8_t>(KeyCode::F1) + usage - 0x3A);
    }
    if (usage >= 0x54 && usage <= 0x63) {
        return static_cast<KeyCode>(
            static_cast<uint8_t>(KeyCode::KeypadDivide) + usage - 0x54);
    }

    switch (usage) {
    case 0x28: return KeyCode::Enter;
    case 0x29: return KeyCode::Escape;
    case 0x2A: return KeyCode::Backspace;
    case 0x2B: return KeyCode::Tab;
    case 0x2C: return KeyCode::Space;
    case 0x2D: return KeyCode::Minus;
    case 0x2E: return KeyCode::Equal;
    case 0x2F: return KeyCode::LeftBracket;
    case 0x30: return KeyCode::RightBracket;
    case 0x31:
    case 0x32:
        return KeyCode::Backslash;
    case 0x33: return KeyCode::Semicolon;
    case 0x34: return KeyCode::Apostrophe;
    case 0x35: return KeyCode::Grave;
    case 0x36: return KeyCode::Comma;
    case 0x37: return KeyCode::Period;
    case 0x38: return KeyCode::Slash;
    case 0x49: return KeyCode::Insert;
    case 0x4A: return KeyCode::Home;
    case 0x4B: return KeyCode::PageUp;
    case 0x4C: return KeyCode::Delete;
    case 0x4D: return KeyCode::End;
    case 0x4E: return KeyCode::PageDown;
    case 0x4F: return KeyCode::ArrowRight;
    case 0x50: return KeyCode::ArrowLeft;
    case 0x51: return KeyCode::ArrowDown;
    case 0x52: return KeyCode::ArrowUp;
    case 0x67: return KeyCode::KeypadEqual;
    default: return KeyCode::Unknown;
    }
}

} // namespace input
