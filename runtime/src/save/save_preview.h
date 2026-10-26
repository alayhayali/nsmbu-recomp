#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace save_preview {

struct FileInfo {
    bool used = false;
    bool from_quick = false;
    uint8_t lives = 0;
    uint8_t coins = 0;
    uint8_t world = 0;
    uint8_t section = 0;
    uint8_t node = 0;
    int courses = 0;
    uint32_t play_seconds = 0;
};

struct Info {
    bool present = false;
    bool readable = false;
    uint8_t version[3] = {};
    uint8_t last_file = 0;
    int used_files = 0;
    FileInfo current;
};

Info inspect_bytes(const uint8_t* data, size_t n);
Info inspect_file(const std::string& path);
Info inspect_dir(const std::string& save_dir);
std::string summary(const Info& info);

}
