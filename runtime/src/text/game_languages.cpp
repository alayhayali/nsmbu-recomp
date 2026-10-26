
#include "text/game_languages.h"

#include <atomic>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>

namespace config {
extern std::string game_dir;
}

namespace game_lang {
namespace {

const char* const kNames[kLanguages] = {"Japanese", "English", "French", "German", "Italian", "Spanish",
                                        "Chinese", "Korean", "Dutch", "Portuguese", "Russian", "Chinese (Taiwan)"};

std::mutex g_mu;
Start g_start;
std::atomic<bool> g_started{false};

std::string g_scanned_dir;
bool g_carried[kLanguages];

void ensure_scan() {
    if (g_scanned_dir == config::game_dir) return;
    g_scanned_dir = config::game_dir;
    std::memset(g_carried, 0, sizeof(g_carried));
    namespace fs = std::filesystem;
    std::error_code ec;
    bool any = false;
    for (const char* region : {"US", "EU", "JP"}) {
        const fs::path dir = fs::path(config::game_dir) / "content" / region;
        if (!fs::is_directory(dir, ec)) continue;
        for (fs::directory_iterator it(dir, ec), end; it != end; it.increment(ec)) {
            if (ec || !it->is_directory(ec)) continue;
            const std::string folder = it->path().filename().string();
            if (folder == "font" || folder == "timestamp") continue;
            for (int i = 0; i < kLanguages; i++) {
                if (folder == kNames[i]) {
                    g_carried[i] = true;
                    any = true;
                    break;
                }
            }
        }
    }
    if (!any) g_carried[1] = true;
}

}

const char* name(int language) { return language >= 0 && language < kLanguages ? kNames[language] : "?"; }

bool carried(int language) {
    if (language < 0 || language >= kLanguages) return false;
    ensure_scan();
    return g_carried[language];
}

int usable(int language) { return carried(language) ? language : 1; }

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
