#include "tab5_keyboard_input.h"

#include "app_config.h"

#if ENABLE_TAB5_KEYBOARD

#include <Arduino.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedKEYBOARD.h>
#include <Wire.h>

#include "hid_keyboard_mapper.h"
#include "input_event_queue.h"
#include "input_mapper.h"

namespace {
m5::unit::UnitUnified units;
m5::unit::UnitTab5Keyboard keyboard;
bool keyboard_enabled = false;
bool key_down = false;
uint8_t last_hid_keycode = 0;
input::KeyCode last_key = input::KeyCode::Unknown;

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
    const uint8_t modifiers = input::normalizeHidModifiers(event.modifier);
    const uint8_t hid_keycode = event.hid.keycode;

    if (hid_keycode == 0) {
        submitRelease(modifiers);
        return;
    }

    const input::KeyCode key = input::hidUsageToKeyCode(hid_keycode);
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
