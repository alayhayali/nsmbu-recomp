#pragma once

#include <cstddef>

namespace window_title {
constexpr size_t kCap = 160;
void format_game_label(char* out, size_t cap);
void format_base(char* out, size_t cap);
}
