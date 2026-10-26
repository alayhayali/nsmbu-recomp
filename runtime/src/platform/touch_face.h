#pragma once
#include <cstdint>

#include "input/input_map.h"

namespace touch_face {

inline int sdl_button(int pad) {
    using namespace input_map;
    switch (pad) {
    case kPadA: return 0;
    case kPadB: return 1;
    case kPadX: return 2;
    case kPadY: return 3;
    case kPadOptions: return 4;
    case kPadHome: return 5;
    case kPadMenu: return 6;
    case kPadL3: return 7;
    case kPadR3: return 8;
    case kPadLB: return 9;
    case kPadRB: return 10;
    case kPadDUp: return 11;
    case kPadDDown: return 12;
    case kPadDLeft: return 13;
    case kPadDRight: return 14;
    default: return -1;
    }
}

/** drawn: bit 0..3 = the on-screen A, B, X, Y held; returns the SDL_GamepadButton bits the mapping binds to them. */
inline uint32_t buttons(uint32_t drawn, const input_map::Mapping& m) {
    static const int kWiiU[4] = {input_map::kA, input_map::kB, input_map::kX, input_map::kY};
    uint32_t out = 0;
    for (int i = 0; i < 4; i++) {
        if (!(drawn & (1u << i))) continue;
        const int b = sdl_button(m.pad[kWiiU[i]]);
        if (b >= 0) out |= 1u << b;
    }
    return out;
}

}
