#pragma once
#include <cstdint>

namespace Midi {
    void init();
    void velocity_handler(uint8_t row, uint8_t column, uint8_t velocity, bool pressed);
}