#include "input_mapper.h"

namespace input {
namespace {
constexpr uint8_t kEscape = 0x1B;

constexpr bool hasModifier(uint8_t modifiers, Modifier modifier)
{
    return (modifiers & static_cast<uint8_t>(modifier)) != 0;
}

constexpr bool appendByte(EncodedInput& output, uint8_t byte)
{
    if (output.length >= kMaxEncodedInputBytes) {
        return false;
    }
    output.bytes[output.length++] = byte;
    return true;
}

constexpr bool appendText(EncodedInput& output, const char *text)
{
    while (*text != '\0') {
        if (!appendByte(output, static_cast<uint8_t>(*text++))) {
            return false;
        }
    }
    return true;
}

constexpr bool appendNumber(EncodedInput& output, uint8_t value)
{
    if (value >= 10 && !appendByte(output, static_cast<uint8_t>('0' + value / 10))) {
        return false;
    }
    return appendByte(output, static_cast<uint8_t>('0' + value % 10));
}

constexpr uint8_t xtermModifier(uint8_t modifiers)
{
    uint8_t value = 1;
    if (hasModifier(modifiers, ModifierShift)) {
        value += 1;
    }
    if (hasModifier(modifiers, ModifierAlt)) {
        value += 2;
    }
    if (hasModifier(modifiers, ModifierCtrl)) {
        value += 4;
    }
    return value;
}

constexpr uint8_t printableByte(KeyCode key, bool shifted)
{
    if (key >= KeyCode::A && key <= KeyCode::Z) {
        const uint8_t offset =
            static_cast<uint8_t>(key) - static_cast<uint8_t>(KeyCode::A);
        return static_cast<uint8_t>((shifted ? 'A' : 'a') + offset);
    }

    switch (key) {
    case KeyCode::Digit1: return shifted ? '!' : '1';
    case KeyCode::Digit2: return shifted ? '@' : '2';
    case KeyCode::Digit3: return shifted ? '#' : '3';
    case KeyCode::Digit4: return shifted ? '$' : '4';
    case KeyCode::Digit5: return shifted ? '%' : '5';
    case KeyCode::Digit6: return shifted ? '^' : '6';
    case KeyCode::Digit7: return shifted ? '&' : '7';
    case KeyCode::Digit8: return shifted ? '*' : '8';
    case KeyCode::Digit9: return shifted ? '(' : '9';
    case KeyCode::Digit0: return shifted ? ')' : '0';
    case KeyCode::Enter: return '\r';
    case KeyCode::Escape: return kEscape;
    case KeyCode::Backspace: return 0x7F;
    case KeyCode::Tab: return '\t';
    case KeyCode::Space: return ' ';
    case KeyCode::Minus: return shifted ? '_' : '-';
    case KeyCode::Equal: return shifted ? '+' : '=';
    case KeyCode::LeftBracket: return shifted ? '{' : '[';
    case KeyCode::RightBracket: return shifted ? '}' : ']';
    case KeyCode::Backslash: return shifted ? '|' : '\\';
    case KeyCode::Semicolon: return shifted ? ':' : ';';
    case KeyCode::Apostrophe: return shifted ? '"' : '\'';
    case KeyCode::Grave: return shifted ? '~' : '`';
    case KeyCode::Comma: return shifted ? '<' : ',';
    case KeyCode::Period: return shifted ? '>' : '.';
    case KeyCode::Slash: return shifted ? '?' : '/';
    default: return 0;
    }
}

constexpr uint8_t controlByte(KeyCode key)
{
    if (key >= KeyCode::A && key <= KeyCode::Z) {
        return static_cast<uint8_t>(
            static_cast<uint8_t>(key) - static_cast<uint8_t>(KeyCode::A) + 1);
    }

    switch (key) {
    case KeyCode::Digit2: return 0x00;
    case KeyCode::LeftBracket: return 0x1B;
    case KeyCode::Backslash: return 0x1C;
    case KeyCode::RightBracket: return 0x1D;
    case KeyCode::Digit6: return 0x1E;
    case KeyCode::Minus: return 0x1F;
    case KeyCode::Slash: return 0x7F;
    default: return 0xFF;
    }
}

constexpr void appendAltPrefix(EncodedInput& output, uint8_t modifiers)
{
    if (hasModifier(modifiers, ModifierAlt)) {
        appendByte(output, kEscape);
    }
}

constexpr EncodedInput encodeCursorKey(
    char final_byte,
    uint8_t modifiers,
    bool application_mode)
{
    EncodedInput output;
    const uint8_t modifier = xtermModifier(modifiers);

    if (modifier != 1) {
        appendText(output, "\x1B[1;");
        appendNumber(output, modifier);
        appendByte(output, static_cast<uint8_t>(final_byte));
    } else if (application_mode) {
        appendText(output, "\x1BO");
        appendByte(output, static_cast<uint8_t>(final_byte));
    } else {
        appendText(output, "\x1B[");
        appendByte(output, static_cast<uint8_t>(final_byte));
    }
    return output;
}

constexpr EncodedInput encodeTildeKey(uint8_t number, uint8_t modifiers)
{
    EncodedInput output;
    appendText(output, "\x1B[");
    appendNumber(output, number);
    const uint8_t modifier = xtermModifier(modifiers);
    if (modifier != 1) {
        appendByte(output, ';');
        appendNumber(output, modifier);
    }
    appendByte(output, '~');
    return output;
}

constexpr EncodedInput encodeFunctionKey(
    KeyCode key,
    uint8_t modifiers)
{
    const uint8_t modifier = xtermModifier(modifiers);
    if (key >= KeyCode::F1 && key <= KeyCode::F4) {
        EncodedInput output;
        const char final_byte = static_cast<char>(
            'P' + static_cast<uint8_t>(key) - static_cast<uint8_t>(KeyCode::F1));
        if (modifier == 1) {
            appendText(output, "\x1BO");
        } else {
            appendText(output, "\x1B[1;");
            appendNumber(output, modifier);
        }
        appendByte(output, static_cast<uint8_t>(final_byte));
        return output;
    }

    constexpr uint8_t function_numbers[] = {15, 17, 18, 19, 20, 21, 23, 24};
    const uint8_t index =
        static_cast<uint8_t>(key) - static_cast<uint8_t>(KeyCode::F5);
    return encodeTildeKey(function_numbers[index], modifiers);
}

constexpr uint8_t keypadNumericByte(KeyCode key)
{
    switch (key) {
    case KeyCode::KeypadDivide: return '/';
    case KeyCode::KeypadMultiply: return '*';
    case KeyCode::KeypadMinus: return '-';
    case KeyCode::KeypadPlus: return '+';
    case KeyCode::KeypadEnter: return '\r';
    case KeyCode::Keypad1: return '1';
    case KeyCode::Keypad2: return '2';
    case KeyCode::Keypad3: return '3';
    case KeyCode::Keypad4: return '4';
    case KeyCode::Keypad5: return '5';
    case KeyCode::Keypad6: return '6';
    case KeyCode::Keypad7: return '7';
    case KeyCode::Keypad8: return '8';
    case KeyCode::Keypad9: return '9';
    case KeyCode::Keypad0: return '0';
    case KeyCode::KeypadDecimal: return '.';
    case KeyCode::KeypadEqual: return '=';
    default: return 0;
    }
}

constexpr uint8_t keypadApplicationByte(KeyCode key)
{
    switch (key) {
    case KeyCode::KeypadDivide: return 'o';
    case KeyCode::KeypadMultiply: return 'j';
    case KeyCode::KeypadMinus: return 'm';
    case KeyCode::KeypadPlus: return 'k';
    case KeyCode::KeypadEnter: return 'M';
    case KeyCode::Keypad1: return 'q';
    case KeyCode::Keypad2: return 'r';
    case KeyCode::Keypad3: return 's';
    case KeyCode::Keypad4: return 't';
    case KeyCode::Keypad5: return 'u';
    case KeyCode::Keypad6: return 'v';
    case KeyCode::Keypad7: return 'w';
    case KeyCode::Keypad8: return 'x';
    case KeyCode::Keypad9: return 'y';
    case KeyCode::Keypad0: return 'p';
    case KeyCode::KeypadDecimal: return 'n';
    case KeyCode::KeypadEqual: return 'X';
    default: return 0;
    }
}

constexpr bool isKeypadKey(KeyCode key)
{
    return key >= KeyCode::KeypadDivide && key <= KeyCode::KeypadEqual;
}

constexpr EncodedInput mapKeyEventConstexpr(
    const KeyEvent& event,
    const TerminalModes& modes)
{
    EncodedInput output;
    if (event.action == KeyAction::Release || event.key == KeyCode::Unknown) {
        return output;
    }

    if (event.key == KeyCode::Tab && hasModifier(event.modifiers, ModifierShift)) {
        appendAltPrefix(output, event.modifiers);
        appendText(output, "\x1B[Z");
        return output;
    }

    if (hasModifier(event.modifiers, ModifierCtrl)) {
        const uint8_t control = controlByte(event.key);
        if (control != 0xFF) {
            appendAltPrefix(output, event.modifiers);
            appendByte(output, control);
            return output;
        }
    }

    const uint8_t printable =
        printableByte(event.key, hasModifier(event.modifiers, ModifierShift));
    if (printable != 0) {
        appendAltPrefix(output, event.modifiers);
        appendByte(output, printable);
        return output;
    }

    switch (event.key) {
    case KeyCode::ArrowUp:
        return encodeCursorKey('A', event.modifiers, modes.application_cursor);
    case KeyCode::ArrowDown:
        return encodeCursorKey('B', event.modifiers, modes.application_cursor);
    case KeyCode::ArrowRight:
        return encodeCursorKey('C', event.modifiers, modes.application_cursor);
    case KeyCode::ArrowLeft:
        return encodeCursorKey('D', event.modifiers, modes.application_cursor);
    case KeyCode::Home:
        return encodeCursorKey('H', event.modifiers, modes.application_cursor);
    case KeyCode::End:
        return encodeCursorKey('F', event.modifiers, modes.application_cursor);
    case KeyCode::Insert:
        return encodeTildeKey(2, event.modifiers);
    case KeyCode::Delete:
        return encodeTildeKey(3, event.modifiers);
    case KeyCode::PageUp:
        return encodeTildeKey(5, event.modifiers);
    case KeyCode::PageDown:
        return encodeTildeKey(6, event.modifiers);
    default:
        break;
    }

    if (event.key >= KeyCode::F1 && event.key <= KeyCode::F12) {
        return encodeFunctionKey(event.key, event.modifiers);
    }

    if (isKeypadKey(event.key)) {
        if (modes.application_keypad) {
            appendText(output, "\x1BO");
            appendByte(output, keypadApplicationByte(event.key));
        } else {
            appendAltPrefix(output, event.modifiers);
            appendByte(output, keypadNumericByte(event.key));
        }
    }

    return output;
}

constexpr bool equals(const EncodedInput& output, const char *expected)
{
    size_t index = 0;
    while (expected[index] != '\0') {
        if (index >= output.length
            || output.bytes[index] != static_cast<uint8_t>(expected[index])) {
            return false;
        }
        ++index;
    }
    return index == output.length;
}

static_assert(equals(
    mapKeyEventConstexpr({KeyCode::A, ModifierNone, KeyAction::Press}, {}),
    "a"));
static_assert(equals(
    mapKeyEventConstexpr({KeyCode::A, ModifierShift, KeyAction::Press}, {}),
    "A"));
static_assert(
    mapKeyEventConstexpr({KeyCode::C, ModifierCtrl, KeyAction::Press}, {}).bytes[0]
    == 0x03);
static_assert(equals(
    mapKeyEventConstexpr({KeyCode::X, ModifierAlt, KeyAction::Press}, {}),
    "\x1Bx"));
static_assert(equals(
    mapKeyEventConstexpr({KeyCode::Tab, ModifierShift, KeyAction::Press}, {}),
    "\x1B[Z"));
static_assert(equals(
    mapKeyEventConstexpr({KeyCode::ArrowUp, ModifierNone, KeyAction::Press}, {}),
    "\x1B[A"));
static_assert(equals(
    mapKeyEventConstexpr(
        {KeyCode::ArrowUp, ModifierNone, KeyAction::Press},
        {true, false}),
    "\x1BOA"));
static_assert(equals(
    mapKeyEventConstexpr({KeyCode::ArrowUp, ModifierCtrl, KeyAction::Press}, {}),
    "\x1B[1;5A"));
static_assert(equals(
    mapKeyEventConstexpr({KeyCode::F1, ModifierNone, KeyAction::Press}, {}),
    "\x1BOP"));
static_assert(equals(
    mapKeyEventConstexpr({KeyCode::F5, ModifierShift, KeyAction::Press}, {}),
    "\x1B[15;2~"));
static_assert(equals(
    mapKeyEventConstexpr({KeyCode::Keypad1, ModifierNone, KeyAction::Press}, {}),
    "1"));
static_assert(equals(
    mapKeyEventConstexpr(
        {KeyCode::Keypad1, ModifierNone, KeyAction::Press},
        {false, true}),
    "\x1BOq"));
static_assert(
    mapKeyEventConstexpr({KeyCode::A, ModifierNone, KeyAction::Release}, {}).length
    == 0);
} // namespace

EncodedInput mapKeyEvent(const KeyEvent& event, const TerminalModes& modes)
{
    return mapKeyEventConstexpr(event, modes);
}

} // namespace input
