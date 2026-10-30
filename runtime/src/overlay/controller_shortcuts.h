#pragma once

#include "input/input_map.h"

namespace overlay_shortcuts {

inline bool combo_edge(bool a, bool b, bool prev_a, bool prev_b) {
    const bool now = a && b;
    const bool prev = prev_a && prev_b;
    return now && !prev;
}

inline bool action_down(const input_map::Mapping& m, int action, const bool keys[256]) {
    if (action < 0 || action >= input_map::kActionCount) return false;
    for (int k : m.keys[action])
        if (k >= 0 && k < 256 && keys[k]) return true;
    return false;
}

inline bool keyboard_start_select(const input_map::Mapping& m, const bool keys[256]) {
    return action_down(m, input_map::kPlus, keys) && action_down(m, input_map::kMinus, keys);
}

inline bool settings_controller_press(bool home_edge, bool r3_edge) { return home_edge || r3_edge; }

}
