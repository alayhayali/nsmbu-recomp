#include "overlay/controller_shortcuts.h"

#include <cstdio>

static int g_pass = 0, g_fail = 0;

static void check(bool ok, const char* msg) {
    if (ok) ++g_pass;
    else {
        ++g_fail;
        std::printf("FAIL: %s\n", msg);
    }
}

int main() {
    using namespace overlay_shortcuts;
    check(!settings_controller_press(false, false), "idle");
    check(settings_controller_press(true, false), "home");
    check(settings_controller_press(false, true), "r3");
    check(settings_controller_press(true, true), "both");
    std::printf("controller_shortcuts_test: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
