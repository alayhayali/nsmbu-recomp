#pragma once
#include <atomic>
#include <cstdint>
#include <cstring>

#include "core/env.h"

namespace phone_rumble {
enum Mode { kOff, kTouch, kFallback };
inline Mode parse(const char* v) {
    if (!v || !std::strcmp(v, "touch") || !std::strcmp(v, "1")) return kTouch;
    if (!std::strcmp(v, "fallback") || !std::strcmp(v, "2")) return kFallback;
    return kOff;
}
inline bool touch_selected(bool controlsVisible, bool lastTouch) {
    return controlsVisible && lastTouch;
}
inline bool allowed(Mode mode, bool touchActive, bool hasControllerRumble) {
    return mode != kOff && (touchActive || (mode == kFallback && !hasControllerRumble));
}
inline std::atomic<Mode> choice{parse(env::PHONE_RUMBLE())};
inline std::atomic<bool> last_input_touch{false};
inline bool env_override() { return env::PHONE_RUMBLE.present(); }

/** Sustained levels go out at most every 100 ms (the phone's lease lasts 150 ms); a stop or a new level goes at once. */
class Refresh {
    uint16_t last_ = 0;
    uint64_t time_ = 0;
public:
    bool send(uint16_t level, uint64_t now) {
        if (level == last_ && (!level || now - time_ < 100)) return false;
        last_ = level; time_ = now; return true;
    }
};
}
