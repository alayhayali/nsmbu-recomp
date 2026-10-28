#include "core/window_title.h"

#include "app_title.h"
#include "core/build_info.h"
#include "guest_addr.h"

#include <cstdio>
#include <cstring>

namespace window_title {

static void release_label(char* out, size_t cap) {
    const char* v = build::version();
    if (!out || cap <= 1 || !v || !*v) {
        if (out && cap)
            out[0] = 0;
        return;
    }
    if (std::strncmp(v, "dev", 3) == 0) {
        std::snprintf(out, cap, "dev git");
        return;
    }
    const char* plus = std::strchr(v, '+');
    if (plus) {
        const char* base = v;
        size_t n = (size_t)(plus - v);
        if (n > 0 && *base == 'v') {
            base++;
            n--;
        }
        std::snprintf(out, cap, "%.*s git", (int)n, base);
        return;
    }
    std::snprintf(out, cap, "%s", v);
}

void format_base(char* out, size_t cap) {
    if (!out || !cap)
        return;
    char release[48];
    release_label(release, sizeof release);
    std::snprintf(out, cap, "%s - %s - %s", app_title::kName, release, g_guest_build_label);
}

}
