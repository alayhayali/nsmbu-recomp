

#pragma once
#include <string>

namespace game_lang {

constexpr int kLanguages = 12;
const char* name(int language);

int usable(int language);

struct Start {
    int language = -1;
};

Start choose(int language);

Start current();
void begin(const Start& start);

int started();
void set_started(int language);

}
