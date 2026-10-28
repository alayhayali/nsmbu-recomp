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

void format_game_label(char* out, size_t cap) {
    const char* name = g_guest_build_name;
    if (!out || cap <= 1 || !name || !*name) {
        if (out && cap)
            out[0] = 0;
        return;
    }
    const char* v0 = std::strstr(name, "_V0");
    if (v0 && v0[3] == '\0') {
        std::snprintf(out, cap, "%.*s v0", (int)(v0 - name), name);
        return;
    }
    if (std::strncmp(name, "NSMBU_NSLU_", 11) == 0) {
        std::snprintf(out, cap, "NSMBU+NSLU %s v%d", name + 11, g_guest_build_title_version);
        return;
    }
    std::snprintf(out, cap, "%s v%d", name, g_guest_build_title_version);
}

void format_base(char* out, size_t cap) {
    if (!out || !cap)
        return;
    char release[48];
    char game[48];
    release_label(release, sizeof release);
    format_game_label(game, sizeof game);
    std::snprintf(out, cap, "%s - %s - %s", app_title::kName, release, game);
}

}
