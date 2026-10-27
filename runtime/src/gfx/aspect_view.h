#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace aspect::view {
constexpr float kBase = 16.0f / 9.0f;
constexpr float kMinScreen = 160.0f;

inline bool near_base(float aspect) {
    return std::fabs(aspect - kBase) <= 0.01f * kBase;
}

inline bool gamepad_target(uint32_t width, uint32_t height) {
    for (uint32_t w = 854, h = 480; w >= 32; w >>= 1, h >>= 1)
        if ((width == w || width == w + 1) && height == h) return true;
    return false;
}

inline bool near_px(float value, float size) { return std::fabs(value - size) <= 1.0f; }

inline bool framebuffer_ortho(float top, float bottom, float left, float right) {
    float w = std::fabs(right - left), h = std::fabs(top - bottom);
    if (near_px(w, 1280.0f) && near_px(h, 720.0f)) return true;
    if (near_px(w, 640.0f) && near_px(h, 360.0f)) return true;
    if ((near_px(w, 854.0f) || near_px(w, 855.0f)) && near_px(h, 480.0f)) return true;
    if ((near_px(w, 427.0f) || near_px(w, 428.0f)) && near_px(h, 240.0f)) return true;
    return false;
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
    if (framebuffer_ortho(top, bottom, left, right)) return o;
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

inline bool fullscreen_quad(const float* values, size_t count, float width, float height) {
    if (!values || count < 4 || !(width >= 32.0f && height >= 32.0f)) return false;
    bool wide = false;
    bool inset = false;
    auto rect = [](float w, float h, float x, float y) {
        if (!(w > 32.0f && h > 16.0f && w < 4000.0f && h < 2000.0f)) return false;
        if (std::fabs(x) > 2500.0f || std::fabs(y) > 2500.0f) return false;
        if (w <= 255.0f && h <= 255.0f && x >= 0.0f && y >= 0.0f && x <= 255.0f && y <= 255.0f) return false;
        return true;
    };
    for (size_t i = 0; i + 3 < count; i += 4) {
        if (!rect(values[i], values[i + 1], values[i + 2], values[i + 3])) continue;
        if (values[i] >= width * 0.85f && values[i + 1] >= 64.0f) wide = true;
        else inset = true;
    }
    if (count >= 6 && std::fabs(values[0] - width) < 8.0f && std::fabs(values[1]) < 1.0f && std::fabs(values[4]) < 1.0f &&
        std::fabs(values[5] - height) < 8.0f)
        wide = true;
    return wide && !inset;
}

inline bool screen_ortho_scale(const float* values, size_t count, float width, float height) {
    if (!values || !(width >= 32.0f && height >= 32.0f) || count < 8) return false;
    float sx = 2.0f / width, sy = 2.0f / height;
    float tx = 0.02f * sx;
    for (size_t i = 0; i + 6 < count; ++i) {
        if (std::fabs(values[i + 1]) > tx || std::fabs(values[i + 4]) > tx) continue;
        float zx = values[i] / sx;
        float zy = std::fabs(values[i + 5]) / sy;
        if (zx < 0.2f || zx > 5.0f) continue;
        if (std::fabs(zx - zy) <= 0.02f * zx) return true;
    }
    return false;
}

inline bool screen_ortho_ranges(const float* base, const uint32_t* begin, const uint32_t* end, size_t count, float width,
                                float height) {
    if (!base || !begin || !end || !count) return false;
    uint32_t order[64];
    size_t n = count > 64 ? 64 : count;
    for (size_t i = 0; i < n; ++i) order[i] = uint32_t(i);
    std::sort(order, order + n, [&](uint32_t a, uint32_t b) { return begin[a] < begin[b]; });
    uint32_t start = begin[order[0]], stop = end[order[0]];
    auto hit = [&](uint32_t a, uint32_t b) {
        return b > a && screen_ortho_scale(base + a, b - a, width, height);
    };
    for (size_t i = 1; i < n; ++i) {
        uint32_t next = begin[order[i]], next_end = end[order[i]];
        if (next <= stop) stop = std::max(stop, next_end);
        else {
            if (hit(start, stop)) return true;
            start = next;
            stop = next_end;
        }
    }
    return hit(start, stop);
}

struct OrthoPlace {
    float x, y, w, h;
    float pad_x, pad_y, scale_x, scale_y;
};

inline uint32_t cover_min(float value, uint32_t limit) {
    return uint32_t(std::clamp(std::floor(double(value) + 1e-3), 0.0, double(limit)));
}

inline uint32_t cover_max(float value, uint32_t limit) {
    return uint32_t(std::clamp(std::ceil(double(value) - 1e-3), 0.0, double(limit)));
}

inline bool center_framebuffer_draw(bool wide_target, bool screen_ortho, bool cover) {
    return wide_target && screen_ortho && !cover;
}

inline void scale_guest_ortho(float* values, size_t count, float guest_w, float guest_h, float kx, float ky) {
    if (!values || count == 0 || !(guest_w >= 32.0f && guest_h >= 32.0f)) return;
    if (kx == 1.0f && ky == 1.0f) return;
    float sx = 2.0f / guest_w, sy = 2.0f / guest_h;
    float ax = 2.0f / (guest_w * kx), ay = 2.0f / (guest_h * ky);
    float tx = 0.02f * sx, ty = 0.02f * sy;
    for (size_t i = 0; i < count; ++i) {
        float v = values[i];
        float a = std::fabs(v);
        if (std::fabs(a - sx) <= tx && std::fabs(a - ax) > tx) values[i] = v / kx;
        else if (std::fabs(a - sy) <= ty && std::fabs(a - ay) > ty) values[i] = v / ky;
    }
}

inline OrthoPlace place_ortho(float xo, float xs, float yo, float ys, float extent_w, float extent_h, float logical_w,
                               float logical_h, float sx, float sy, float ax, float ay, bool center) {
    float scale_x = center && ax > 0.0f ? sx / ax : sx;
    float scale_y = center && ay > 0.0f ? sy / ay : sy;
    float pad_x = center ? (extent_w - logical_w * scale_x) * 0.5f : 0.0f;
    float pad_y = center ? (extent_h - logical_h * scale_y) * 0.5f : 0.0f;
    return {pad_x + (xo - xs) * scale_x, pad_y + (yo - ys) * scale_y, xs * 2.0f * scale_x, ys * 2.0f * scale_y,
            pad_x, pad_y, scale_x, scale_y};
}
}
