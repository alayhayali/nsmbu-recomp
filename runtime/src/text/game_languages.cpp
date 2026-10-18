

#include "text/game_languages.h"

#include <atomic>
#include <mutex>

namespace game_lang {
namespace {

const char* const kNames[kLanguages] = {"Japanese", "English", "French", "German", "Italian", "Spanish",
                                        "Chinese", "Korean", "Dutch", "Portuguese", "Russian", "Chinese (Taiwan)"};

std::mutex g_mu;
Start g_start;
std::atomic<bool> g_started{false};

}

const char* name(int language) { return language >= 0 && language < kLanguages ? kNames[language] : "?"; }

int usable(int language) { return language >= 0 && language < kLanguages ? language : 1; }

Start choose(int language) {
    Start s;
    s.language = usable(language);
    return s;
}

Start current() {
    std::lock_guard<std::mutex> lk(g_mu);
    return g_start;
}

void begin(const Start& start) {
    std::lock_guard<std::mutex> lk(g_mu);
    g_start = start;
    g_started.store(true, std::memory_order_release);
}

int started() { return g_started.load(std::memory_order_acquire) ? current().language : -1; }
void set_started(int language) {
    Start s;
    s.language = language;
    begin(s);
}

}
