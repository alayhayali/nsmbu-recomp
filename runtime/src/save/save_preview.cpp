#include "save/save_preview.h"

#include "save/portable_state.h"

#include <cstdio>

namespace save_preview {
namespace {

constexpr size_t kStoryFiles = 3;
constexpr size_t kLevelFlagCount = 123;
constexpr size_t kLevelFlagOff = 0x6B;
constexpr size_t kPlaySecondsOff = 0x30;

uint32_t be32(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

FileInfo view_record(const uint8_t* rec) {
    FileInfo v;
    v.used = rec[0] != 0;
    if (!v.used) return v;
    v.coins = rec[2];
    v.world = rec[4];
    v.section = rec[5];
    v.node = rec[6];
    v.lives = rec[0x0C];
    if (v.lives == 0) v.lives = 5;
    v.play_seconds = be32(rec + kPlaySecondsOff);
    for (size_t i = 0; i < kLevelFlagCount; i++)
        if (rec[kLevelFlagOff + i]) v.courses++;
    return v;
}

FileInfo active_record(const uint8_t* data, size_t file) {
    const uint8_t* main = data + pstate::kNsmbuMainOff + file * pstate::kNsmbuSlotSize;
    const uint8_t* quick = data + pstate::kNsmbuQuickOff + file * pstate::kNsmbuSlotSize;
    if (quick[0]) {
        FileInfo v = view_record(quick);
        v.from_quick = true;
        return v;
    }
    return view_record(main);
}

std::string play_time(uint32_t seconds) {
    char buf[32];
    if (seconds >= 3600)
        snprintf(buf, sizeof buf, "%uh %um", seconds / 3600, (seconds % 3600) / 60);
    else if (seconds >= 60)
        snprintf(buf, sizeof buf, "%um %us", seconds / 60, seconds % 60);
    else
        snprintf(buf, sizeof buf, "%us", seconds);
    return buf;
}

}

Info inspect_bytes(const uint8_t* data, size_t n) {
    Info out;
    if (!data || n < pstate::kNsmbuSaveDataSize) return out;
    out.present = true;
    if (data[0] != 'R' || data[1] != 'P' || data[2] != 'S' || data[3] != 'D') return out;
    out.readable = true;
    out.version[0] = data[4];
    out.version[1] = data[5];
    out.version[2] = data[6];
    out.last_file = data[7];
    if (out.last_file > 5) out.last_file = 5;
    for (size_t i = 0; i < kStoryFiles; i++) {
        if (data[pstate::kNsmbuMainOff + i * pstate::kNsmbuSlotSize]) out.used_files++;
    }
    out.current = active_record(data, out.last_file);
    return out;
}

Info inspect_file(const std::string& path) {
    Info out;
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return out;
    out.present = true;
    std::vector<uint8_t> buf(pstate::kNsmbuSaveDataSize);
    size_t got = fread(buf.data(), 1, buf.size(), f);
    fclose(f);
    if (got != buf.size()) return out;
    return inspect_bytes(buf.data(), buf.size());
}

Info inspect_dir(const std::string& save_dir) {
    if (save_dir.empty()) return {};
    return inspect_file(save_dir + "/common/rp_savedata.dat");
}

std::string summary(const Info& info) {
    if (!info.present) return "No game save yet.";
    if (!info.readable) return "Game save could not be read.";
    char head[96];
    snprintf(head, sizeof head, "%d file%s · last file %u · save %u.%u.%u", info.used_files,
             info.used_files == 1 ? "" : "s", (unsigned)info.last_file + 1, info.version[0], info.version[1],
             info.version[2]);
    if (!info.current.used) return std::string(head) + " · empty";
    const FileInfo& f = info.current;
    char body[160];
    snprintf(body, sizeof body, " · World %u · %u lives · %u coins · %d courses · %s", (unsigned)f.world + 1,
             (unsigned)f.lives, (unsigned)f.coins, f.courses, play_time(f.play_seconds).c_str());
    return std::string(head) + body;
}

}
