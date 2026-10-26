#pragma once
#include <algorithm>
#include <cmath>

namespace aspect::view {
constexpr float kBase = 16.0f / 9.0f;
constexpr float kMinScreen = 160.0f;

inline bool near_base(float aspect) {
    return std::fabs(aspect - kBase) <= 0.01f * kBase;
}

inline void factors(float aspect, float& kx, float& ky) {
    kx = aspect >= kBase ? aspect / kBase : 1.0f;
    ky = aspect >= kBase ? 1.0f : kBase / aspect;
}

struct Ortho {
    float top, bottom, left, right;
};

inline Ortho expand_ortho(float top, float bottom, float left, float right, float aspect) {
    Ortho o{top, bottom, left, right};
    float w = right - left, h = top - bottom;
    float aw = std::fabs(w), ah = std::fabs(h);
    if (!(aw > 1.0f && ah > 1.0f)) return o;
    if (std::fabs(aw / ah - kBase) > 0.01f * kBase) return o;
    if (std::min(aw, ah) < kMinScreen) return o;
    float kx, ky;
    factors(aspect, kx, ky);
    if (kx == 1.0f && ky == 1.0f) return o;
    float cx = (left + right) * 0.5f, cy = (top + bottom) * 0.5f;
    o.left = cx + (left - cx) * kx;
    o.right = cx + (right - cx) * kx;
    o.top = cy + (top - cy) * ky;
    o.bottom = cy + (bottom - cy) * ky;
    return o;
}

struct Persp {
    float fovy, aspect;
};

inline Persp expand_perspective(float fovy, float aspect, float requested) {
    Persp p{fovy, aspect};
    if (!(fovy > 0.0f) || !near_base(aspect)) return p;
    float kx, ky;
    factors(requested, kx, ky);
    (void)kx;
    if (ky != 1.0f) p.fovy = 2.0f * std::atan(std::tan(fovy * 0.5f) * ky);
    p.aspect = requested;
    return p;
}
}
