#include "core/runtime.h"
#include "mods/packages.h"
#include "mods/sdk/nsmbu.h"
#include "guest_addr.h"

#include <cstdio>
#include <cstring>
#include <string>

extern "C" void f_02994164_orig(Cpu* c);
extern "C" void nsmbu_note_scene_src(uint32_t src, uint32_t factory);
extern "C" void f_024BAD4C_orig(Cpu* c);
extern "C" void f_024BD86C_orig(Cpu* c);
extern "C" void f_0226FBC4_orig(Cpu* c);
extern "C" void f_025D8B28_orig(Cpu* c);
extern "C" void f_02672C90_orig(Cpu* c);
extern "C" void f_02681BFC_orig(Cpu* c);
extern "C" void f_0267CB3C_orig(Cpu* c);
extern "C" void f_0252F900_orig(Cpu* c);
extern "C" void f_025CF020_orig(Cpu* c);

namespace threads {
Cpu* current();
}

static const uint32_t kAreaTask = GD(0x101D1598);
static const uint32_t kCourseInfo = GD(0x101D18EC);
static const uint32_t kCourseSelectMgr = GD(0x101CBE14);
static const uint32_t kCourseTask = GD(0x101D15F0);
static const uint32_t kFieldGame = GD(0x101D1604);
static const uint32_t kMainGame = GD(0x101E7D7C);
static const uint32_t kSaveMgr = GD(0x101E7DA4);
static const uint32_t kSaveSlotSize = 0x204u;
static const uint32_t kCreatorMap = GC(0x020292B8);
static const uint32_t kCreatorCourseSelect = GC(0x0202B86C);
static const uint32_t kPathNodeSize = 0x68;

static bool g_banner_armed = false;

static uint32_t g_world_course_textbox = 0;
static uint32_t g_world_course_multi2d = 0;

static void write_three_glyphs(uint32_t base, const uint8_t g[3]) {
    if (base == 0) return;
    for (int i = 0; i < 3; i++) st8(base + (uint32_t)i, g[i]);
}

static bool course_pair_ok(uint8_t world, uint8_t course) {
    return world != 0xFFu && world <= 8u && course != 0xFFu && course <= 40u;
}

static bool read_course_info(uint8_t* world_out, uint8_t* course_out) {
    uint32_t info = ld32(kCourseInfo);
    if (info == 0) return false;
    uint8_t world = ld8(info + 0x10);
    uint8_t course = ld8(info + 0x11);
    uint8_t next_world = ld8(info + 0x1C);
    uint8_t next_course = ld8(info + 0x1D);
    if (course_pair_ok(next_world, next_course) && (next_world != world || next_course != course)) {
        world = next_world;
        course = next_course;
    }
    if (world == 0xFFu || course == 0xFFu || course > 40u) {
        const uint8_t now_world = ld8(info + 0x00);
        const uint8_t now_course = ld8(info + 0x01);
        if ((world == 0xFFu || world > 8u) && now_world != 0xFFu) world = now_world;
        if ((course == 0xFFu || course > 40u) && now_course != 0xFFu) course = now_course;
    }
    if (world_out) *world_out = world;
    if (course_out) *course_out = course;
    return true;
}

static bool course_banner_state(uint8_t g[3], uint8_t* world_out, uint8_t* course_out) {
    uint8_t world = 0;
    uint8_t course = 0;
    if (!read_course_info(&world, &course)) return false;
    int display = (int)course + 1;
    if (display < 1 || display > 999) return false;
    char buf[4];
    int n = snprintf(buf, sizeof(buf), "%d", display);
    if (n < 1 || n > 3) return false;
    g[0] = g[1] = g[2] = 0xFF;
    for (int i = 0; i < n; i++) {
        int d = buf[i] - '0';
        g[i] = (d >= 1 && d <= 5) ? (uint8_t)(d - 1) : 0xFF;
    }
    if (world_out) *world_out = world;
    if (course_out) *course_out = course;
    return true;
}

static void seed_maingame_course_tags(uint8_t world, uint8_t course) {
    uint32_t game = ld32(kMainGame);
    if (game == 0) return;
    st32(game + 0x19Cu, world);
    st32(game + 0x1A8u, course);
}

static bool in_mem2(uint32_t ea) {
    return ea >= mem::kMem2Start && ea < mem::kMem2End;
}

static bool checkpoint_missing() {
    uint32_t field_game = ld32(kFieldGame);
    if (field_game == 0) return true;
    return (int32_t)ld32(field_game + 0x10) < 0;
}

static uint32_t cs_get_path(uint32_t mgr, uint32_t world, uint32_t section) {
    uint32_t idx = (world << 1) + section;
    if (idx >= 0x12u) return 0;
    return ld32(mgr + 0x60u + idx * 4u);
}

static uint32_t cs_get_node(uint32_t path, uint32_t node_idx) {
    if (path == 0 || node_idx >= 0x100u) return 0;
    return path + 0x10u + node_idx * kPathNodeSize;
}

static bool node_name_matches(uint32_t node, const char* want) {
    if (!in_mem2(node)) return false;
    uint32_t p = ld32(node);
    if (!in_mem2(p) || ld8(p) < 0x20 || ld8(p) >= 0x7F) return false;
    return mem::read_cstr(p) == want;
}

static bool find_course_map_node(uint8_t world, uint8_t course, uint32_t* section_out,
                                 uint32_t* node_idx_out, uint32_t* cp_index_out) {
    uint32_t mgr = ld32(kCourseSelectMgr);
    if (mgr == 0) return false;
    char want[8];
    snprintf(want, sizeof(want), "W%u%02u", (unsigned)world + 1u, (unsigned)course + 1u);
    for (uint32_t section = 0; section < 2u; section++) {
        uint32_t path = cs_get_path(mgr, world, section);
        if (path == 0 || !in_mem2(path)) continue;
        for (uint32_t node_idx = 0; node_idx < 0x100u; node_idx++) {
            uint32_t node = cs_get_node(path, node_idx);
            if (!in_mem2(node) || node + kPathNodeSize > mem::kMem2End) break;
            if (!node_name_matches(node, want)) continue;
            if (section_out) *section_out = section;
            if (node_idx_out) *node_idx_out = node_idx;
            if (cp_index_out) *cp_index_out = node_idx;
            return true;
        }
    }
    return false;
}

static void write_checkpoint(uint32_t data, int32_t index, uint8_t world, uint8_t course,
                             uint8_t file, const uint8_t banner[3]) {
    st32(data + 0x00u, (uint32_t)index);
    st8(data + 0x04u, world);
    st8(data + 0x05u, course);
    st8(data + 0x06u, file);
    st8(data + 0x07u, 0);
    st32(data + 0x08u, 0);
    st8(data + 0x0Cu, 0xFF);
    st8(data + 0x0Du, 0xFF);
    st8(data + 0x0Eu, 0xFF);
    st8(data + 0x0Fu, 0xFF);
    for (int i = 0; i < 3; i++) st8(data + 0x10u + (uint32_t)i, banner[i]);
}

static void seed_player_map_fields(uint32_t player, uint32_t node, uint8_t world, uint32_t section,
                                   uint32_t course_slot, bool pack_create_pos) {
    if (player == 0 || !in_mem2(player)) return;
    st32(player + 0xF0u, world);
    st32(player + 0xF4u, section);
    st32(player + 0xF8u, 0);
    st32(player + 0xFCu, course_slot);
    if (node == 0 || !in_mem2(node) || node + 0x4Cu >= mem::kMem2End) return;
    const double x = ldf32(node + 0x44u);
    const double y = ldf32(node + 0x48u);
    const double z = ldf32(node + 0x4Cu);
    if (player + 0x58u < mem::kMem2End) {
        stf32(player + 0x50u, x);
        stf32(player + 0x54u, y);
        stf32(player + 0x58u, z);
    }
    if (pack_create_pos && player + 0x118u < mem::kMem2End) {
        stf32(player + 0x110u, x);
        stf32(player + 0x114u, y);
        stf32(player + 0x118u, z);
    }
}

static void seed_map_return_pose(uint32_t player, bool pack_create_pos) {
    uint8_t world = 0;
    uint8_t course = 0;
    if (!read_course_info(&world, &course)) return;
    if (!course_pair_ok(world, course)) return;
    uint32_t section = 0;
    uint32_t node_idx = 0;
    if (!find_course_map_node(world, course, &section, &node_idx, nullptr)) return;
    uint32_t course_slot = 0;
    uint32_t game = ld32(kMainGame);
    if (game == 0) return;
    st32(game + 0x19Cu, world);
    st32(game + 0x1A0u, section);
    st32(game + 0x1A4u, node_idx);
    st32(game + 0x1A8u, course);
    st32(game + 0x2Cu, ld32(game + 0x2Cu) | 8u);
    if (checkpoint_missing()) {
        uint8_t banner[3];
        uint8_t bw = 0;
        uint8_t bc = 0;
        if (!course_banner_state(banner, &bw, &bc)) {
            banner[0] = banner[1] = banner[2] = 0xFF;
        }
        uint32_t field_game = ld32(kFieldGame);
        if (field_game != 0) {
            uint32_t info = ld32(kCourseInfo);
            uint8_t file = info != 0 ? ld8(info + 0x12) : 0;
            write_checkpoint(field_game + 0x10, (int32_t)node_idx, world, course, file, banner);
        }
    }
    uint32_t mgr = ld32(kCourseSelectMgr);
    uint32_t path = mgr != 0 ? cs_get_path(mgr, world, section) : 0;
    uint32_t node = cs_get_node(path, node_idx);
    uint32_t save_mgr = ld32(kSaveMgr);
    uint32_t savedata = save_mgr != 0 ? ld32(save_mgr + 0x24u) : 0;
    if (in_mem2(savedata) && savedata + 0x10u + 3u * kSaveSlotSize < mem::kMem2End) {
        uint8_t file = ld8(savedata + 0x07u);
        if (file >= 3u) file = 0;
        uint32_t slot = savedata + 0x10u + (uint32_t)file * kSaveSlotSize;
        st8(slot + 0x04u, (uint8_t)(world + 1u));
        st8(slot + 0x05u, (uint8_t)section);
        st8(slot + 0x06u, (uint8_t)node_idx);
        st8(slot + 0x43u, (uint8_t)node_idx);
    }
    if (node != 0 && in_mem2(node)) {
        int32_t slot = (int32_t)ld32(node + 0x58u);
        if (slot >= 0) course_slot = (uint32_t)slot;
    }
    seed_player_map_fields(player, node, world, section, course_slot, pack_create_pos);
}

static std::string textbox_name_string(uint32_t textbox) {
    if (!in_mem2(textbox) || textbox + 0xACu > mem::kMem2End) return {};
    uint32_t name_p = ld32(textbox + 0xA8);
    if (!in_mem2(name_p)) return {};
    uint8_t b0 = ld8(name_p);
    if (b0 >= 0x20 && b0 < 0x7F) return mem::read_cstr(name_p);
    if (name_p + 8u > mem::kMem2End) return {};
    uint32_t q = ld32(name_p + 4);
    if (in_mem2(q) && ld8(q) >= 0x20 && ld8(q) < 0x7F) return mem::read_cstr(q);
    q = ld32(name_p);
    if (in_mem2(q) && ld8(q) >= 0x20 && ld8(q) < 0x7F) return mem::read_cstr(q);
    return {};
}

static void reapply_world_course_textbox(Cpu* c) {
    if (c == nullptr) c = threads::current();
    if (c == nullptr) return;
    uint8_t world = 0;
    uint8_t course = 0;
    if (read_course_info(&world, &course)) seed_maingame_course_tags(world, course);
    uint32_t task = ld32(kCourseTask);
    uint32_t pregame = task != 0 ? ld32(task + 0xCC) : 0;
    uint32_t multi2d = in_mem2(pregame) ? ld32(pregame) : g_world_course_multi2d;
    uint32_t textbox = g_world_course_textbox;
    if (textbox == 0 && in_mem2(pregame) && pregame + 0x800u <= mem::kMem2End) {
        for (uint32_t off = 0; off < 0x800; off += 4) {
            uint32_t cand = ld32(pregame + off);
            if (!in_mem2(cand)) continue;
            std::string name = textbox_name_string(cand);
            if (name.find("WorldCourse") == std::string::npos) continue;
            textbox = cand;
            g_world_course_textbox = cand;
            g_world_course_multi2d = multi2d;
            break;
        }
    }
    if (textbox == 0) return;
    uint32_t saved[6] = {c->r[3], c->r[4], c->r[5], c->r[6], c->r[7], c->r[8]};
    c->r[3] = textbox;
    c->r[4] = 0;
    f_02681BFC_orig(c);
    c->r[3] = saved[0];
    c->r[4] = saved[1];
    c->r[5] = saved[2];
    c->r[6] = saved[3];
    c->r[7] = saved[4];
    c->r[8] = saved[5];
}

static void fill_course_banner_digits(Cpu* c, uint32_t glyph_base, bool refresh) {
    if (!g_banner_armed) return;
    if (mods::lua_sdk::story_leave_pending()) return;
    uint8_t g[3];
    uint8_t world = 0;
    uint8_t course = 0;
    if (!course_banner_state(g, &world, &course)) return;
    seed_maingame_course_tags(world, course);
    if (glyph_base != 0) {
        write_three_glyphs(glyph_base, g);
        write_three_glyphs(glyph_base + 0x12D, g);
    }
    uint32_t task = ld32(kCourseTask);
    if (task != 0) {
        write_three_glyphs(task + 0x134, g);
        write_three_glyphs(task + 0x261, g);
    }
    uint32_t field_game = ld32(kFieldGame);
    if (field_game != 0) {
        uint32_t data = field_game + 0x10;
        write_three_glyphs(data + 0x10, g);
        write_three_glyphs(data + 0x13D, g);
    }
    if (refresh) reapply_world_course_textbox(c != nullptr ? c : threads::current());
}

extern "C" void nsmbu_banner_entry_arm(void) {
    g_banner_armed = true;
}

extern "C" void nsmbu_fill_course_banner_digits(void) {
    fill_course_banner_digits(threads::current(), 0, true);
}

extern "C" void hook_025CF020(Cpu* c) {
    if (ld32(kAreaTask) == 0) {
        c->r[3] = 0;
        return;
    }
    f_025CF020_orig(c);
}

extern "C" void hook_0252F900(Cpu* c) {
    if (ld32(kAreaTask) == 0) return;
    f_0252F900_orig(c);
}

extern "C" void hook_02994164(Cpu* c) {
    uint32_t factory = ld32(c->r[5] + 4);
    nsmbu_note_scene_src(c->r[4], factory);
    if (g_banner_armed && (factory == kCreatorMap || factory == kCreatorCourseSelect))
        g_banner_armed = false;
    f_02994164_orig(c);
    mods::packages::scene_change(factory);
}

extern "C" void hook_029CBBB4(Cpu*) {}

extern "C" void hook_024BD86C(Cpu* c) {
    if (mods::lua_sdk::story_leave_pending() && ld32(kCourseTask) != 0) return;
    f_024BD86C_orig(c);
}

extern "C" void site_024B8324(Cpu* c) {
    seed_map_return_pose(0, false);
}

extern "C" void hook_0226FBC4(Cpu* c) {
    uint32_t player = c->r[3];
    seed_map_return_pose(player, true);
    f_0226FBC4_orig(c);
    seed_map_return_pose(player, false);
}

extern "C" void site_0226FD14(Cpu* c) {
    seed_map_return_pose(c->r[30], true);
}

extern "C" void site_0226FECC(Cpu* c) {
    seed_map_return_pose(c->r[30], true);
}

extern "C" void site_02270310(Cpu* c) {
    seed_map_return_pose(c->r[30], true);
}

extern "C" void site_022703A4(Cpu* c) {
    seed_map_return_pose(c->r[30], true);
}

static void retarget_focus_node(Cpu* c, int copied_reg) {
    seed_map_return_pose(0, false);
    uint8_t world = 0;
    uint8_t course = 0;
    if (!read_course_info(&world, &course) || !course_pair_ok(world, course)) return;
    uint32_t section = 0;
    uint32_t node_idx = 0;
    if (!find_course_map_node(world, course, &section, &node_idx, nullptr)) return;
    uint32_t mgr = ld32(kCourseSelectMgr);
    uint32_t path = mgr != 0 ? cs_get_path(mgr, world, section) : 0;
    uint32_t node = cs_get_node(path, node_idx);
    if (node == 0 || !in_mem2(node)) return;
    c->r[3] = node;
    if (copied_reg >= 0) c->r[copied_reg] = ld32(node + 0x50u);
}

extern "C" void site_0227CCF4(Cpu* c) {
    retarget_focus_node(c, 8);
}

extern "C" void site_0227E2A8(Cpu* c) {
    retarget_focus_node(c, 11);
}

extern "C" void hook_024BAD4C(Cpu* c) {
    uint32_t dest = c->r[3];
    f_024BAD4C_orig(c);
    fill_course_banner_digits(c, dest + 0x10, true);
}

extern "C" void site_024BD96C(Cpu* c) {
    uint32_t task = ld32(kCourseTask);
    uint32_t base = task != 0 ? task + 0x134 : c->r[31] + 0x134;
    fill_course_banner_digits(c, base, true);
}

extern "C" void site_024BE280(Cpu* c) {
    fill_course_banner_digits(c, 0, true);
}

extern "C" void hook_025D8B28(Cpu* c) {
    if (!g_banner_armed) {
        f_025D8B28_orig(c);
        return;
    }
    fill_course_banner_digits(c, 0, false);
    f_025D8B28_orig(c);
    reapply_world_course_textbox(c);
}

extern "C" void hook_0267CB3C(Cpu* c) {
    if (mods::lua_sdk::story_leave_pending()) return;
    if (!g_banner_armed) {
        f_0267CB3C_orig(c);
        return;
    }
    uint8_t course = 0;
    uint32_t before = c->r[4];
    if (read_course_info(nullptr, &course)) {
        if (before == 0xFFFFFFFFu || (int32_t)before < 0)
            c->r[4] = course;
        else if (course <= 11)
            c->r[4] = course;
    }
    static int logged = 0;
    if (logged++ < 40)
        LOG("[banner-cb3c] in=%u out=%u course=%u lr=%08X", before, c->r[4], course, c->lr);
    f_0267CB3C_orig(c);
}

extern "C" void hook_02681BFC(Cpu* c) {
    if (mods::lua_sdk::story_leave_pending()) return;
    if (!g_banner_armed) {
        f_02681BFC_orig(c);
        return;
    }
    uint32_t textbox = c->r[3];
    uint8_t world = 0;
    uint8_t course = 0;
    if (read_course_info(&world, &course)) seed_maingame_course_tags(world, course);
    f_02681BFC_orig(c);
    std::string name = textbox_name_string(textbox);
    if (name.find("WorldCourse") != std::string::npos) {
        g_world_course_textbox = textbox;
        uint32_t task = ld32(kCourseTask);
        if (task != 0) {
            uint32_t pregame = ld32(task + 0xCC);
            if (pregame != 0) g_world_course_multi2d = ld32(pregame);
        }
    }
}

extern "C" void hook_02672C90(Cpu* c) {
    if (!g_banner_armed) {
        f_02672C90_orig(c);
        return;
    }
    uint32_t multi2d = c->r[3];
    uint32_t textbox = c->r[4];
    uint8_t world = 0;
    uint8_t course = 0;
    if (read_course_info(&world, &course)) seed_maingame_course_tags(world, course);
    f_02672C90_orig(c);
    if (textbox_name_string(textbox).find("WorldCourse") != std::string::npos) {
        g_world_course_textbox = textbox;
        g_world_course_multi2d = multi2d;
    }
}
