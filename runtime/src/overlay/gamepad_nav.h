#pragma once
#include "input/input_map.h"

struct ImGuiIO;

namespace overlay {

class GamepadNavigation {
    bool mouse_owned_ = false;

public:
    void reset() { mouse_owned_ = false; }
    bool controller_active() const { return !mouse_owned_; }
    bool pressed(const float* values, const float* previous, int input) const {
        return controller_active() && values[input] > 0.5f && previous[input] <= 0.5f;
    }
    void feed(ImGuiIO& io, const float* values, const float* previous, bool enabled, bool mouse_used,
              const int* face = nullptr);
};

}
