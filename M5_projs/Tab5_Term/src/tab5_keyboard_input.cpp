#include "tab5_keyboard_input.h"

#include "app_config.h"

#if ENABLE_TAB5_KEYBOARD

#include <Arduino.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedKEYBOARD.h>
#include <Wire.h>

#include "input_event_queue.h"
#include "input_mapper.h"

namespace {
constexpr uint8_t kHidLeftCtrl = 0x01;
constexpr uint8_t kHidLeftShift = 0x02;
constexpr uint8_t kHidLeftAlt = 0x04;
constexpr uint8_t kHidLeftGui = 0x08;
constexpr uint8_t kHidRightCtrl = 0x10;
constexpr uint8_t kHidRightShift = 0x20;
constexpr uint8_t kHidRightAlt = 0x40;
constexpr uint8_t kHidRightGui = 0x80;

m5::unit::UnitUnified units;
m5::unit::UnitTab5Keyboard keyboard;
bool keyboard_enabled = false;
bool key_down = false;
uint8_t last_hid_keycode = 0;
input::KeyCode last_key = input::KeyCode::Unknown;

uint8_t normalizeModifiers(uint8_t hid_modifiers)
{
    uint8_t modifiers = input::ModifierNone;
    if ((hid_modifiers & (kHidLeftShift | kHidRightShift)) != 0) {
        modifiers |= input::ModifierShift;
    }
    if ((hid_modifiers & (kHidLeftCtrl | kHidRightCtrl)) != 0) {
        modifiers |= input::ModifierCtrl;
    }
    if ((hid_modifiers & (kHidLeftAlt | kHidRightAlt)) != 0) {
        modifiers |= input::ModifierAlt;
    }
    if ((hid_modifiers & (kHidLeftGui | kHidRightGui)) != 0) {
        modifiers |= input::ModifierMeta;
    }
    return modifiers;
}

input::KeyCode hidUsageToKeyCode(uint8_t usage)
{
    if (usage >= 0x04 && usage <= 0x1D) {
        return static_cast<input::KeyCode>(
            static_cast<uint8_t>(input::KeyCode::A) + usage - 0x04);
    }
    if (usage >= 0x1E && usage <= 0x27) {
        return static_cast<input::KeyCode>(
            static_cast<uint8_t>(input::KeyCode::Digit1) + usage - 0x1E);
    }
    if (usage >= 0x3A && usage <= 0x45) {
        return static_cast<input::KeyCode>(
            static_cast<uint8_t>(input::KeyCode::F1) + usage - 0x3A);
    }
    if (usage >= 0x54 && usage <= 0x63) {
        return static_cast<input::KeyCode>(
            static_cast<uint8_t>(input::KeyCode::KeypadDivide) + usage - 0x54);
    }

    switch (usage) {
    case 0x28: return input::KeyCode::Enter;
    case 0x29: return input::KeyCode::Escape;
    case 0x2A: return input::KeyCode::Backspace;
    case 0x2B: return input::KeyCode::Tab;
    case 0x2C: return input::KeyCode::Space;
    case 0x2D: return input::KeyCode::Minus;
    case 0x2E: return input::KeyCode::Equal;
    case 0x2F: return input::KeyCode::LeftBracket;
    case 0x30: return input::KeyCode::RightBracket;
    case 0x31:
    case 0x32:
        return input::KeyCode::Backslash;
    case 0x33: return input::KeyCode::Semicolon;
    case 0x34: return input::KeyCode::Apostrophe;
    case 0x35: return input::KeyCode::Grave;
    case 0x36: return input::KeyCode::Comma;
    case 0x37: return input::KeyCode::Period;
    case 0x38: return input::KeyCode::Slash;
    case 0x49: return input::KeyCode::Insert;
    case 0x4A: return input::KeyCode::Home;
    case 0x4B: return input::KeyCode::PageUp;
    case 0x4C: return input::KeyCode::Delete;
    case 0x4D: return input::KeyCode::End;
    case 0x4E: return input::KeyCode::PageDown;
    case 0x4F: return input::KeyCode::ArrowRight;
    case 0x50: return input::KeyCode::ArrowLeft;
    case 0x51: return input::KeyCode::ArrowDown;
    case 0x52: return input::KeyCode::ArrowUp;
    case 0x67: return input::KeyCode::KeypadEqual;
    default: return input::KeyCode::Unknown;
    }
}

void submitRelease(uint8_t modifiers)
{
    if (!key_down || last_key == input::KeyCode::Unknown) {
        key_down = false;
        last_hid_keycode = 0;
        last_key = input::KeyCode::Unknown;
        return;
    }

    input::submitKeyEvent({
        .key = last_key,
        .modifiers = modifiers,
        .action = input::KeyAction::Release,
    });
    key_down = false;
    last_hid_keycode = 0;
    last_key = input::KeyCode::Unknown;
}

void processHidEvent(const m5::unit::tab5_keyboard::Event& event)
{
    const uint8_t modifiers = normalizeModifiers(event.modifier);
    const uint8_t hid_keycode = event.hid.keycode;

    if (hid_keycode == 0) {
        submitRelease(modifiers);
        return;
    }

    const input::KeyCode key = hidUsageToKeyCode(hid_keycode);
    if (key == input::KeyCode::Unknown) {
        return;
    }

    const input::KeyAction action =
        key_down && hid_keycode == last_hid_keycode
        ? input::KeyAction::Repeat
        : input::KeyAction::Press;

    if (key_down && hid_keycode != last_hid_keycode) {
        submitRelease(modifiers);
    }

    if (!input::submitKeyEvent({
            .key = key,
            .modifiers = modifiers,
            .action = action,
        })) {
        Serial.println("[tab5-kbd] input event queue full");
    }

    key_down = true;
    last_hid_keycode = hid_keycode;
    last_key = key;
}
} // namespace

bool tab5KeyboardInputBegin()
{
    if (keyboard_enabled) {
        return true;
    }

    auto config = keyboard.config();
    config.mode = m5::unit::tab5_keyboard::Mode::HID;
    config.irq_pin = TAB5_KEYBOARD_INT_PIN;
    config.start_periodic = true;
    config.software_repeat = false;
    keyboard.config(config);

    Wire.end();
    if (!Wire.begin(
            TAB5_KEYBOARD_SDA_PIN,
            TAB5_KEYBOARD_SCL_PIN,
            keyboard.component_config().clock)) {
        Serial.println("[tab5-kbd] failed to start Ext.Port1 I2C");
        return false;
    }

    if (!units.add(keyboard, Wire) || !units.begin()) {
        Serial.println("[tab5-kbd] keyboard not found");
        return false;
    }

    keyboard_enabled = true;
    Serial.printf(
        "[tab5-kbd] ready addr=0x%02X fw=0x%02X mode=HID irq=G%d\r\n",
        keyboard.address(),
        keyboard.firmwareVersion(),
        TAB5_KEYBOARD_INT_PIN);
    return true;
}

void tab5KeyboardInputUpdate()
{
    if (!keyboard_enabled) {
        return;
    }

    units.update();
    while (!keyboard.empty()) {
        const auto event = keyboard.oldest();
        if (event.type == m5::unit::tab5_keyboard::EventType::Hid) {
            processHidEvent(event);
        }
        keyboard.discard();
    }
}

bool tab5KeyboardInputIsEnabled()
{
    return keyboard_enabled;
}

#else

bool tab5KeyboardInputBegin()
{
    return false;
}

void tab5KeyboardInputUpdate() {}

bool tab5KeyboardInputIsEnabled()
{
    return false;
}

#endif
