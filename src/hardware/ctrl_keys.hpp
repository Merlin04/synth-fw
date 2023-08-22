#pragma once

#include <cstdint>
#include <util/event.hpp>

enum CtrlKey : uint8_t {
    KEY_BACK = 2,
    KEY_SELECT = 3,
    KEY_OSC = 4,
    KEY_RIGHT_1 = 5,
    KEY_RIGHT_2 = 6,
    KEY_RIGHT_3 = 7,
    KEY_RIGHT_4 = 8
};

inline auto ctrl_keys_evt = BubblingEvent<CtrlKey>();