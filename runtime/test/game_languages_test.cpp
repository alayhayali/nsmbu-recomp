

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <string>
#include "text/game_languages.h"
namespace config { std::string game_dir, save_dir; }
int main() {
    assert(std::string(game_lang::name(1)) == "English" && std::string(game_lang::name(11)) == "Chinese (Taiwan)" &&
           std::string(game_lang::name(12)) == "?" && std::string(game_lang::name(-1)) == "?");
    assert(game_lang::usable(2) == 2 && game_lang::usable(3) == 3 && game_lang::usable(11) == 11 &&
           game_lang::usable(12) == 1 && game_lang::usable(-1) == 1);
    assert(game_lang::started() == -1);
    game_lang::set_started(5);
    assert(game_lang::started() == 5);
    assert(game_lang::current().language == 5);
    assert(game_lang::choose(7).language == 7);
    puts("game_languages_test: language names, fallback to English, started state passed");
}
