#pragma once

#include <stdint.h>

namespace terminal {
namespace unicode {

constexpr const char *kVersion = "15.0.0";

enum class ColumnWidth : int8_t {
    Control = -1,
    Zero = 0,
    Narrow = 1,
    Wide = 2,
};

ColumnWidth columnWidth(uint32_t codepoint);
bool isValidScalar(uint32_t codepoint);

} // namespace unicode
} // namespace terminal
