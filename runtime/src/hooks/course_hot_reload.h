#pragma once

#include <string>

namespace course_hot_reload {

struct Info {
    bool enabled = false;
    bool env_locked = false;
    std::string pack;
    std::string path;
    std::string status;
};

bool enabled();
void set_enabled(bool on);
bool env_locked();
void request_reload();
void tick();
Info info();

}
