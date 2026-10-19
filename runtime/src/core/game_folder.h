#pragma once

#include <string>

namespace game_folder {

std::string unusable_reason(const std::string& root);

[[noreturn]] void fail(const std::string& reason);

}
