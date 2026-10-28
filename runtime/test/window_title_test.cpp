#include "core/build_info.h"
#include "core/window_title.h"
#include "guest_addr.h"

#include <cstdio>
#include <cstring>

static int failures = 0;

static void check(bool ok, const char* what) {
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        failures++;
    }
}

int main() {
    char buf[window_title::kCap];
    window_title::format_base(buf, sizeof buf);
    check(std::strstr(buf, "New Super Mario Bros. U") != nullptr, "app name");
    check(std::strstr(buf, build::version()) != nullptr, "release version");
    check(std::strstr(buf, g_guest_build_label) != nullptr, "game label");
    return failures ? 1 : 0;
}
