#pragma once

#include <cmath>
#include <cstddef>

namespace gfx {

struct ResPreset {
    float scale;
    const char* label;
};

inline constexpr float kResScaleMin = 0.5f;
inline constexpr float kResScaleMax = 4.0f;

inline constexpr ResPreset kResPresets[] = {
    {0.5f, "360p"},
    {0.75f, "540p"},
    {1.0f, "720p"},
    {1.5f, "1080p"},
    {2.0f, "1440p"},
    {3.0f, "2160p/4K"},
};

inline constexpr std::size_t res_preset_count() {
    return sizeof kResPresets / sizeof kResPresets[0];
}

inline float clamp_res_scale(float f) {
    if (!std::isfinite(f) || f <= 0.0f) f = 1.0f;
    if (f < kResScaleMin) return kResScaleMin;
    if (f > kResScaleMax) return kResScaleMax;
    return f;
}

inline float next_res_preset(float cur) {
    for (const ResPreset& p : kResPresets)
        if (cur < p.scale - 0.01f) return p.scale;
    return kResPresets[0].scale;
}

}
