#include "core/window_title.h"

#include "app_title.h"
#include "core/build_info.h"
#include "guest_addr.h"

#include <cstdio>

namespace window_title {

void format_base(char* out, size_t cap) {
    if (!out || !cap)
        return;
    std::snprintf(out, cap, "%s — %s · %s", app_title::kName, build::version(), g_guest_build_label);
}

}
