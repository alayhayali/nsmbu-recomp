#include "core/runtime.h"
#include "guest_addr.h"
#include "input/input.h"

#include <cstdio>
#include <string>
#include <sys/stat.h>

extern "C" void f_029D9E30_orig(Cpu* c);
extern "C" void f_029872BC_orig(Cpu* c);
extern "C" void f_029D7648_orig(Cpu* c);
extern "C" void f_029D8270_orig(Cpu* c);
extern "C" void f_029D8304_orig(Cpu* c);
extern "C" void f_029D82D4_orig(Cpu* c);
extern "C" void f_029D8394_orig(Cpu* c);

static const uint32_t kSaveMgr = GD(0x101E7DA4);
static const uint32_t kSaveSlotSize = 0x204u;
static const uint32_t kMainRecordBase = 0x10u;
static const uint32_t kBackupRecordBase = 0xC28u;

static const char* read_error_name(int32_t e) {
    if (e == 0) return "ok";
    if (e == 1) return "device unavailable";
    if (e == -1) return "file unavailable";
    if (e == -2) return "file invalid";
    return "unknown";
}

static const char* write_error_name(int32_t e) {
    if (e == 0) return "ok";
    if (e == -1) return "device unavailable";
    if (e == -2) return "write failed";
    return "unknown";
}

static const char* state_name(uint32_t state) {
    switch (state) {
    case 1: return "load rp_savedata.dat";
    case 2: return "write rp_savedata.dat";
    case 3: return "load challenge replay";
    case 4: return "write challenge replay";
    case 5: return "load rp_personal_savedata.dat";
    case 6: return "write rp_personal_savedata.dat";
    default: return "unknown state";
    }
}

static uint32_t save_mgr() {
    return ld32(kSaveMgr);
}

static uint32_t savedata_buf(uint32_t mgr) {
    return mgr ? ld32(mgr + 0x24u) : 0;
}

static bool in_mem2(uint32_t ea) {
    return ea >= mem::kMem2Start && ea < mem::kMem2End;
}

static void format_exists(char* out, size_t n, uint32_t savedata) {
    if (!in_mem2(savedata) || savedata + kMainRecordBase + 3u * kSaveSlotSize >= mem::kMem2End) {
        snprintf(out, n, "?");
        return;
    }
    char* p = out;
    char* end = out + n;
    for (uint32_t i = 0; i < 3u && p + 8 < end; i++) {
        uint8_t main = ld8(savedata + kMainRecordBase + i * kSaveSlotSize);
        uint8_t backup = ld8(savedata + kBackupRecordBase + i * kSaveSlotSize);
        int wrote = snprintf(p, (size_t)(end - p), "%s%d%s%s", i ? " " : "", i, main ? "+" : "-", backup ? "b" : "");
        if (wrote < 0) break;
        p += wrote;
    }
}

static long host_save_size(const char* rel) {
    std::string path = config::save_dir + rel;
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return -1;
    return (long)st.st_size;
}

static void log_savedata_summary(const char* what, bool writing) {
    uint32_t mgr = save_mgr();
    int32_t err = mgr ? (int32_t)ld32(mgr + (writing ? 0x1Cu : 0x18u)) : 0x7fffffff;
    const char* (*err_name)(int32_t) = writing ? write_error_name : read_error_name;
    uint32_t buf = savedata_buf(mgr);
    char exists[48];
    format_exists(exists, sizeof exists, buf);
    uint8_t last = 0xFF;
    uint32_t magic = 0;
    uint8_t ver[3] = {};
    if (in_mem2(buf) && buf + 0x10u < mem::kMem2End) {
        magic = ld32(buf);
        ver[0] = ld8(buf + 4);
        ver[1] = ld8(buf + 5);
        ver[2] = ld8(buf + 6);
        last = ld8(buf + 7);
    }
    LOG("[save] %s: %s (%d), last_file=%u, magic=%08X ver=%u.%u.%u, files %s, dir %s", what, err_name(err), err,
        (unsigned)last, magic, ver[0], ver[1], ver[2], exists, config::save_dir.c_str());
}

extern "C" void hook_029D9E30(Cpu* c) {
    uint32_t state = c->r[5];
    if (state >= 1 && state <= 6) LOG("[save] %s starting", state_name(state));
    f_029D9E30_orig(c);
}

extern "C" void site_029D94C0(Cpu*) {
    log_savedata_summary("load rp_savedata.dat", false);
    LOG("[save] host common/rp_savedata.dat %ld bytes", host_save_size("/common/rp_savedata.dat"));
}

extern "C" void site_029D97B8(Cpu*) {
    log_savedata_summary("write rp_savedata.dat", true);
    LOG("[save] host common/rp_savedata.dat %ld bytes after write", host_save_size("/common/rp_savedata.dat"));
}

extern "C" void site_029D9CB8(Cpu*) {
    uint32_t mgr = save_mgr();
    int32_t err = mgr ? (int32_t)ld32(mgr + 0x18u) : 0x7fffffff;
    LOG("[save] load rp_personal_savedata.dat: %s (%d), host %ld bytes, dir %s", read_error_name(err), err,
        host_save_size("/user/rp_personal_savedata.dat"), config::save_dir.c_str());
}

extern "C" void site_029D9E2C(Cpu*) {
    uint32_t mgr = save_mgr();
    int32_t err = mgr ? (int32_t)ld32(mgr + 0x1Cu) : 0x7fffffff;
    LOG("[save] write rp_personal_savedata.dat: %s (%d), host %ld bytes, dir %s", write_error_name(err), err,
        host_save_size("/user/rp_personal_savedata.dat"), config::save_dir.c_str());
}

extern "C" void hook_029D8270(Cpu* c) {
    LOG("[save] startReadGame requested");
    f_029D8270_orig(c);
}

extern "C" void hook_029D8304(Cpu* c) {
    uint32_t mgr = save_mgr();
    uint32_t buf = savedata_buf(mgr);
    char exists[48];
    format_exists(exists, sizeof exists, buf);
    uint8_t last = in_mem2(buf) && buf + 8u < mem::kMem2End ? ld8(buf + 7) : 0xFF;
    LOG("[save] startWriteGame requested (last_file=%u, files %s)", (unsigned)last, exists);
    f_029D8304_orig(c);
}

extern "C" void hook_029D82D4(Cpu* c) {
    LOG("[save] startReadPersonalSavedata requested");
    f_029D82D4_orig(c);
}

extern "C" void hook_029D8394(Cpu* c) {
    LOG("[save] startWritePersonalSavedata requested");
    f_029D8394_orig(c);
}

extern "C" void hook_029872BC(Cpu* c) {
    uint32_t mgr = save_mgr();
    uint32_t buf = savedata_buf(mgr);
    uint8_t file = 0;
    uint8_t exists = 0;
    if (in_mem2(buf) && buf + kMainRecordBase + 3u * kSaveSlotSize < mem::kMem2End) {
        file = ld8(buf + 7);
        if (file < 6u) exists = ld8(buf + kMainRecordBase + (uint32_t)file * kSaveSlotSize);
    }
    LOG("[save] continue: file %u %s -> %s (controller %s)", (unsigned)file, exists ? "has record" : "empty",
        exists ? "CourseSelect" : "Begin Story Mode", input::pro_controller() ? "Pro" : "GamePad");
    f_029872BC_orig(c);
}

extern "C" void hook_029D7648(Cpu* c) {
    uint8_t file = (uint8_t)c->r[4];
    uint32_t mgr = c->r[3] ? c->r[3] : save_mgr();
    uint32_t buf = savedata_buf(mgr);
    uint8_t main = 0;
    uint8_t backup = 0;
    if (in_mem2(buf) && file < 6u && buf + kBackupRecordBase + ((uint32_t)file + 1u) * kSaveSlotSize < mem::kMem2End) {
        main = ld8(buf + kMainRecordBase + (uint32_t)file * kSaveSlotSize);
        backup = ld8(buf + kBackupRecordBase + (uint32_t)file * kSaveSlotSize);
    }
    LOG("[save] apply file %u (main %s, backup %s)", (unsigned)file, main ? "set" : "empty", backup ? "set" : "empty");
    f_029D7648_orig(c);
}
