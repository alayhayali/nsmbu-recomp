

#pragma once

#include <cstring>

namespace build {
const char* version();
const char* commit();

inline bool dev() {
    const char* v = version();
    return v && (std::strncmp(v, "dev", 3) == 0 || std::strchr(v, '+'));
}
}
