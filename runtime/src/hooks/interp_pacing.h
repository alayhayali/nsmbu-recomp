
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace interp {
namespace pacing {

inline float lerp_f(float a, float b, float t) { return t == 0.5f ? 0.5f * (a + b) : (1.0f - t) * a + t * b; }

inline int16_t lerp_s16(int16_t a, int16_t b, float t) {
    const int16_t d = (int16_t)(b - a);
    return t == 0.5f ? (int16_t)(a + d / 2) : (int16_t)(a + (int)std::lround(d * t));
}
inline uint8_t lerp_u8(uint8_t a, uint8_t b, float t) {
    return t == 0.5f ? (uint8_t)((a + b + 1) / 2) : (uint8_t)std::lround(a + (b - a) * t);
}

inline void slerp_weights(float cosang, float t, float& wa, float& wb) {
    if (t == 0.5f) {
        wa = wb = 1.0f;
        return;
    }
    const float ang = std::acos(std::min(1.0f, std::max(-1.0f, cosang)));
    if (ang > 1e-4f) {
        const float s = std::sin(ang);
        wa = std::sin((1 - t) * ang) / s;
        wb = std::sin(t * ang) / s;
    } else {
        wa = 1 - t;
        wb = t;
    }
}

inline const int* fps_ladder(int& n) {
    static const int rates[] = {60, 120, 144, 165, 240};
    n = int(sizeof rates / sizeof rates[0]);
    return rates;
}

inline int valid_fps(int f) {
    int n = 0;
    const int* rates = fps_ladder(n);
    int best = rates[0];
    for (int i = 0; i < n; i++)
        if (std::abs(rates[i] - f) < std::abs(best - f)) best = rates[i];
    return best;
}

inline int display_fps(int hz) {
    int n = 0;
    const int* rates = fps_ladder(n);
    int shown = rates[0];
    for (int i = 0; i < n; i++)
        if (rates[i] <= hz + 2) shown = rates[i];
    return shown;
}

inline int cap_fps(int fps, int hz, bool vsync) {
    fps = valid_fps(fps);
    if (!vsync || hz <= 0) return fps;
    return std::min(fps, display_fps(hz));
}

constexpr int64_t kLogicStepNs = 1'000'000'000LL / 60;
constexpr int64_t kMaxDtNs = 100'000'000LL;
constexpr int kMaxLogicStepsPerWake = 4;

inline int64_t clamp_dt_ns(int64_t dt_ns) { return std::clamp(dt_ns, int64_t{0}, kMaxDtNs); }

inline float blend_t(int64_t acc_ns) {
    if (acc_ns <= 0) return 0.f;
    if (acc_ns >= kLogicStepNs) return 1.f;
    return float(double(acc_ns) / double(kLogicStepNs));
}

inline float present_t(int logic_steps, float open_t) { return logic_steps > 0 ? 1.f : open_t; }

struct WakePlan {
    int logic_steps = 0;
    float t = 1.f;
    int64_t acc_ns = 0;
};

inline WakePlan advance_accumulator(int64_t acc_ns, int64_t dt_ns, bool interpolate, int max_steps = kMaxLogicStepsPerWake) {
    WakePlan out;
    out.acc_ns = acc_ns;
    if (!interpolate) {
        out.logic_steps = 1;
        out.t = 1.f;
        out.acc_ns = 0;
        return out;
    }
    out.acc_ns += clamp_dt_ns(dt_ns);
    max_steps = std::max(1, max_steps);
    while (out.acc_ns >= kLogicStepNs && out.logic_steps < max_steps) {
        out.acc_ns -= kLogicStepNs;
        out.logic_steps++;
    }
    out.t = present_t(out.logic_steps, blend_t(out.acc_ns));
    return out;
}

inline int64_t present_period_ns(int present_cap) {
    present_cap = std::max(1, valid_fps(present_cap));
    return 1'000'000'000LL / present_cap;
}

inline int present_granule(int present_cap) {
    present_cap = valid_fps(present_cap);
    if (present_cap <= 60) return 4;
    return std::max(1, (4 * 60 + present_cap / 2) / present_cap);
}

inline int64_t repeat_period_ns(bool course_play, int output_fps) {
    if (course_play) return 0;
    const int fps = valid_fps(output_fps);
    if (fps <= 60) return 0;
    return present_period_ns(fps);
}

inline int64_t bunched_present_delay_ns(bool course_play, int output_fps, int64_t since_ns) {
    const int64_t period = repeat_period_ns(course_play, output_fps);
    if (period <= 0 || since_ns < 0 || since_ns * 2 >= period) return 0;
    return period - since_ns;
}
inline bool skip_bunched_present(bool course_play, int output_fps, int64_t since_ns) {
    return bunched_present_delay_ns(course_play, output_fps, since_ns) > 0;
}

inline bool collapse_duplicate_vsync(bool presented_since_wake, bool wake_valid, int64_t since_wake_ns) {
    if (presented_since_wake || !wake_valid) return false;
    return since_wake_ns < 2'000'000;
}

inline int64_t absorb_late_wake(int64_t last_ns, int64_t now_ns, int64_t period_ns) {
    if (period_ns <= 0 || now_ns <= last_ns) return last_ns;
    const int64_t slots = (now_ns - last_ns) / period_ns;
    return last_ns + slots * period_ns;
}

}
}
