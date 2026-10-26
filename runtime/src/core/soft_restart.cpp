
#include "core/soft_restart.h"

#include "input/rumble.h"
#include "mods/packages.h"
#include "recomp_table.h"
#include "guest_addr.h"

#include <atomic>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <string>

void mem_setup_heaps(uint32_t data_end);
void mem_reset_heaps();
void mem_init_data_imports(uint32_t alloc_slot, uint32_t alloc_ex_slot, uint32_t free_slot);
void fs_reset_for_reboot();
void ax_stop_for_reboot();
void ax_reset_for_reboot();
void gx2_ss_drain();
void gx2_reset_for_reboot();
void console_language_reset();

namespace interp {
void ss_reset();
}
namespace aspect {
void ss_reset();
}

namespace soft_restart {
namespace {

enum State { Idle, Requested, Killing, MainExited, Rebooting };

std::mutex g_mu;
std::condition_variable g_cv;
std::atomic<State> g_state{Idle};
std::string g_why;
std::atomic<bool> g_ui_refresh{false};
LoadedModule g_mod{};
uint32_t g_argv = 0;

void init_data_imports() {
    uint32_t alloc = 0, alloc_ex = 0, free_ = 0;
    for (unsigned i = 0; i < g_recomp_import_count; i++) {
        const RecompImport& im = g_recomp_imports[i];
        if (im.is_func) continue;
        std::string n = im.name;
        if (n == "MEMAllocFromDefaultHeap") alloc = im.addr;
        else if (n == "MEMAllocFromDefaultHeapEx") alloc_ex = im.addr;
        else if (n == "MEMFreeToDefaultHeap") free_ = im.addr;
        else if (n == "__gh_FOPEN_MAX") st32(im.addr, 20);
        else if (n == "environ") st32(im.addr, im.addr + 0x10);
    }
    mem_init_data_imports(alloc, alloc_ex, free_);
}

void clear_cafe_workspace() {
    memset(mem::ptr(mem::kFgBucket), 0, mem::kFgBucketSize);
    memset(mem::ptr(mem::kMem1), 0, mem::kMem1Size);
    if (g_mod.data_end > mem::kMem2Start && g_mod.data_end <= mem::kMem2End) {
        uint32_t heap = (g_mod.data_end + 0xFFF) & ~0xFFFu;
        if (heap < mem::kMem2End) memset(mem::ptr(heap), 0, mem::kMem2End - heap);
    }
}

void do_reboot() {
    LOG("[soft_restart] rebooting guest (%s)", g_why.c_str());
    threads::reset_for_reboot();
    ax_reset_for_reboot();
    fs_reset_for_reboot();
    gx2_reset_for_reboot();
    interp::ss_reset();
    aspect::ss_reset();
    rumble::reset();
    mem::reset_allocators();
    mem_reset_heaps();
    clear_cafe_workspace();
    mods::packages::reinitialize();
    console_language_reset();

    LoadedModule m{};
    std::string rpx = config::rpx_path();
    if (!load_rpx(rpx, m)) fatal("soft restart: cannot load %s", rpx.c_str());
    if (m.entry != g_recomp_entry_point) fatal("soft restart: %s does not match the recompiled code", rpx.c_str());
    LOG("[soft_restart] loaded %s entry %08X", rpx.c_str(), m.entry);
    init_data_imports();
    mem_setup_heaps(m.data_end);
    threads::init(m);
    uint32_t argv_arr = mem::runtime_alloc(16);
    uint32_t arg0 = mem::runtime_alloc(16);
    mem::write_cstr(arg0, config::kRpxName, 16);
    st32(argv_arr, arg0);
    g_mod = m;
    g_argv = argv_arr;
    LOG("[soft_restart] guest ready");
}

}

void request(const char* why) {
    std::lock_guard<std::mutex> lk(g_mu);
    if (g_state.load() != Idle) return;
    g_why = why && *why ? why : "settings";
    g_state = Requested;
    LOG("[soft_restart] requested (%s)", g_why.c_str());
}

bool pending() {
    State s = g_state.load();
    return s != Idle;
}

const char* reason() { return g_why.c_str(); }

void prepare() {
    if (g_state.load() != Requested) return;
    std::string busy;
    if (!threads::quiesce(250, busy, 2, 0)) {
        threads::thaw();
        if (!busy.empty()) LOG("[soft_restart] waiting for guest (%s)", busy.c_str());
        return;
    }
    gx2_ss_drain();
    ax_stop_for_reboot();
    g_state = Killing;
    LOG("[soft_restart] stopping guest threads (%s)", g_why.c_str());
    threads::kill_all_for_reboot();
}

void run_if_ready() {
    if (g_state.load() != MainExited) return;
    g_state = Rebooting;
    do_reboot();
    g_ui_refresh.store(true);
    {
        std::lock_guard<std::mutex> lk(g_mu);
        g_state = Idle;
        g_why.clear();
    }
    g_cv.notify_all();
}

bool on_guest_main_exit() {
    State s = g_state.load();
    if (s != Killing && s != Requested) return false;
    g_state = MainExited;
    g_cv.notify_all();
    return true;
}

void wait_until_idle() {
    std::unique_lock<std::mutex> lk(g_mu);
    g_cv.wait(lk, [] { return g_state.load() == Idle; });
}

bool process_exit_blocked() {
    State s = g_state.load();
    return s == Killing || s == MainExited || s == Rebooting || threads::reboot_killing();
}

bool take_ui_refresh() { return g_ui_refresh.exchange(false); }

void set_module(const LoadedModule& m) { g_mod = m; }
LoadedModule& module() { return g_mod; }
uint32_t argv_ea() { return g_argv; }
void set_argv_ea(uint32_t ea) { g_argv = ea; }

}
