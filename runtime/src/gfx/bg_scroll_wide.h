#pragma once

#include <cmath>

namespace aspect::scroll {

struct Edges {
    float left, right, width;
};

inline Edges widen(float left, float right, float center, float kx) {
    Edges e{left, right, right - left};
    if (kx <= 1.0f)
        return e;
    e.left = center + (left - center) * kx;
    e.right = center + (right - center) * kx;
    e.width = e.right - e.left;
    return e;
}

inline bool should_widen(float width, float height, float kx) {
    if (!(height > 1.0f) || !(width > 1.0f) || !(kx > 1.0f))
        return false;
    float aspect = width / height;
    float base = 16.0f / 9.0f;
    return std::fabs(aspect - base) <= 0.02f * base;
}

}
