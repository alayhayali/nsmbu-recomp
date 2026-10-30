#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>

namespace touch_buttons {
enum Mode { Automatic, AlwaysShow, Hide };
inline bool visible(int mode, bool controller, bool revealed) {
    return revealed || mode == AlwaysShow || (mode == Automatic && !controller);
}

inline std::atomic<int> mode{Automatic};
inline std::atomic<bool> controller{false}, controls{true};
inline std::atomic<int64_t> reveal_until{0};

inline int64_t now_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
inline bool visible_now() { return visible(mode, controller, now_ms() < reveal_until); }
inline void reveal() { reveal_until = now_ms() + 5000; }
}
