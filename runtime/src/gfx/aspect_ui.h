#pragma once
#include <cmath>
#include <cstddef>

namespace aspect::ui {

inline bool screen_ortho_scale(const float* values, size_t count, float width, float height) {
    if (!values || !(width >= 32.0f && height >= 32.0f) || count < 8) return false;
    float sx = 2.0f / width, sy = 2.0f / height;
    float tx = 0.02f * sx, ty = 0.02f * sy;
    for (size_t i = 0; i + 6 < count; ++i) {
        if (std::fabs(values[i] - sx) > tx || std::fabs(values[i + 1]) > tx) continue;
        if (std::fabs(values[i + 4]) <= tx && std::fabs(values[i + 5] - sy) <= ty) return true;
    }
    return false;
}

inline bool screen_ortho_vec2(const float* values, size_t count, float width, float height) {
    if (!values || !(width >= 32.0f && height >= 32.0f) || count < 2) return false;
    float sx = 2.0f / width, sy = 2.0f / height;
    float tx = 0.02f * sx, ty = 0.02f * sy;
    for (size_t i = 0; i + 1 < count; ++i) {
        if (std::fabs(values[i] - sx) > tx || std::fabs(values[i + 1] - sy) > ty) continue;
        return true;
    }
    return false;
}

inline bool screen_ortho_packed(const float* values, size_t count, float width, float height) {
    if (!values || !(width >= 32.0f && height >= 32.0f) || count < 7) return false;
    float sx = 2.0f / width, sy = 2.0f / height;
    float tx = 0.02f * sx, ty = 0.02f * sy;
    for (size_t i = 0; i + 6 < count; ++i) {
        if (std::fabs(values[i] - sx) > tx || std::fabs(values[i + 5] - sy) > ty) continue;
        if (std::fabs(values[i + 2]) > tx || std::fabs(values[i + 3]) > tx || std::fabs(values[i + 4]) > tx) continue;
        if (std::fabs(values[i + 1]) > 1.0f + tx || std::fabs(values[i + 6]) > 1.0f + ty) continue;
        return true;
    }
    return false;
}

inline bool screen_matrix(const float* values, size_t count, float width, float height) {
    return screen_ortho_scale(values, count, width, height) || screen_ortho_vec2(values, count, width, height) ||
           screen_ortho_packed(values, count, width, height);
}

}
