#pragma once
#include <atomic>
#include <cmath>
#include <cstring>

#include "core/env.h"

namespace heat_guard {
enum DeviceClass { kPhone = 0, kFanHandheld = 1 };
enum Setting { kNormal = 0, kRelaxed = 1, kOff = 2 };

struct Limits {
    bool active;
    float pause;
    float resume;
};

inline Limits limits(DeviceClass device, Setting setting) {
    if (setting == kOff) return {false, 0.0f, 0.0f};
    if (device == kFanHandheld) return setting == kRelaxed ? Limits{true, 1.10f, 1.00f} : Limits{true, 1.00f, 0.90f};
    if (setting == kRelaxed) return {true, 0.90f, 0.80f};
    return {true, 0.72f, 0.60f};
}

inline bool next_paused(bool paused, float headroom, Limits l) {
    if (!l.active) return false;
    if (std::isnan(headroom)) return paused;
    if (!paused) return headroom >= l.pause;
    return !(headroom < l.resume);
}

inline Setting parse(const char* v) {
    if (!v) return kNormal;
    if (!std::strcmp(v, "relaxed") || !std::strcmp(v, "1")) return kRelaxed;
    if (!std::strcmp(v, "off") || !std::strcmp(v, "2")) return kOff;
    return kNormal;
}
inline const char* name(Setting s) { return s == kRelaxed ? "relaxed" : s == kOff ? "off" : "normal"; }
inline const char* device_name(DeviceClass d) { return d == kFanHandheld ? "handheld (fan)" : "phone"; }

inline DeviceClass to_device(int v) { return v == kFanHandheld ? kFanHandheld : kPhone; }
inline Setting to_setting(int v) { return v == kRelaxed ? kRelaxed : v == kOff ? kOff : kNormal; }

inline std::atomic<int> device{kPhone};
inline std::atomic<int> choice{parse(env::HEAT_GUARD())};
inline bool env_override() { return env::HEAT_GUARD.present(); }
}
