#include "test/script_frame.h"

#include <atomic>
#include <cstdlib>

#include "core/env.h"

namespace script_frame {
namespace {
std::atomic<int64_t> g_align_tv{-1};
}

bool enabled() {
    static const bool on = enabled_value(env::TEST_FIXED_SEED());
    return on;
}

void set_align_frame(uint64_t tv) {
    int64_t none = -1;
    g_align_tv.compare_exchange_strong(none, int64_t(tv), std::memory_order_relaxed);
}

bool aligned() { return g_align_tv.load(std::memory_order_relaxed) >= 0; }

uint64_t frame(uint64_t tv) {
    if (!enabled()) return tv;
    return rebase(tv, g_align_tv.load(std::memory_order_relaxed));
}

}
