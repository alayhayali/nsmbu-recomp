#pragma once
#include <cstdint>

namespace script_frame {

constexpr uint64_t kAlignFrame = 1800;

inline bool enabled_value(const char* v) { return v && *v && !(v[0] == '0' && v[1] == 0); }

inline uint64_t rebase(uint64_t tv, int64_t align_tv) {
    if (align_tv < 0 || tv < uint64_t(align_tv)) return tv;
    return tv - uint64_t(align_tv) + kAlignFrame;
}

bool enabled();
void set_align_frame(uint64_t tv);
bool aligned();
uint64_t frame(uint64_t tv);

}
