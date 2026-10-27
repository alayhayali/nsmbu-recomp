#pragma once

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

}
