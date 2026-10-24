#include "collision_wireframe.h"

#include "core/env.h"
#include "core/runtime.h"
#include "overlay/game_rect.h"
#include "guest_addr.h"
#include "ppc.h"

#include "imgui.h"

#include <atomic>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <unordered_set>
#include <vector>

namespace collision_wireframe {
namespace {

const uint32_t kBgScrollMgr = GD(0x101E7C54);
const uint32_t kActorCcMgr = GD(0x101C95D8);
const uint32_t kActorBgMgr = GD(0x101C9360);
const uint32_t kActorMgr = GD(0x101C30C8);
const uint32_t kBg = GD(0x101DAB94);
const uint32_t kGetBgCheckDataXy = GC(0x02685148);
const uint32_t kGetLeftPos = GC(0x0219B49C);
const uint32_t kGetRightPos = GC(0x0219B480);
const uint32_t kGetTopPos = GC(0x0219B434);
const uint32_t kGetUnderPos = GC(0x0219B450);

constexpr uint32_t kScreenLeft = 0x70;
constexpr uint32_t kScreenRight = 0x78;
constexpr uint32_t kScreenTop = 0x7C;
constexpr uint32_t kScreenBottom = 0x84;
constexpr uint32_t kScreenPrevLeft = 0x38;
constexpr uint32_t kScreenPrevBottom = 0x3C;
constexpr uint32_t kScreenPrevRight = 0x40;
constexpr uint32_t kScreenPrevTop = 0x44;

constexpr uint32_t kCcActiveHead = 0x14;
constexpr uint32_t kCcCreateHead = 0x20;
constexpr uint32_t kCcOwner = 0x40;
constexpr uint32_t kCcMask = 0x6C;
constexpr uint32_t kCcShape = 0x84;

constexpr uint32_t kActorMgrPtrCache = 0x6A20;
constexpr uint32_t kCacheArray = 0x4;
constexpr uint32_t kCacheLiveEnd = 0xC;
constexpr uint32_t kActorDeleteFlag = 0xF;
constexpr uint32_t kActorCc = 0xBC;

constexpr uint32_t kBgListHeads[] = {0x14, 0x20, 0x2C, 0x38, 0x44};
constexpr uint32_t kBgAffected = 0x70;
constexpr uint32_t kBgOwner = 0x90;
constexpr uint32_t kBgFollowPos = 0x98;
constexpr uint32_t kBgPosOffX = 0xAC;
constexpr uint32_t kBgPosOffY = 0xB0;
constexpr uint32_t kBgPointCount = 0x158;
constexpr uint32_t kBgPointPtr = 0x15C;
constexpr uint32_t kBgRideCount = 0x160;
constexpr uint32_t kBgRidePtr = 0x164;
constexpr uint32_t kBgInactive = 0xDC;
constexpr uint32_t kRideSeg = 0x4;
constexpr uint32_t kRideStride = 0x20;
constexpr uint32_t kActorPos = 0x6C;
constexpr uint32_t kBlockBoxBg = 0x17D8;

constexpr uint32_t kShapeCircle = 1;
constexpr float kTile = 16.0f;
constexpr size_t kMaxActors = 512;
constexpr size_t kMaxTiles = 4096;

struct Box {
    float x0, y0, x1, y1;
    bool tile;
    bool circle;
};

struct Snapshot {
    float left = 0, right = 0, top = 0, bottom = 0;
    std::vector<Box> boxes;
    bool valid = false;
};

std::mutex g_mu;
Snapshot g_snap;
std::atomic<bool> g_enabled{env::COLLISION_VIEWER.enabled()};

bool read_f32(uint32_t ea, float& out) {
    if (!ea) return false;
    out = (float)ldf32(ea);
    return std::isfinite(out);
}

bool read_u32(uint32_t ea, uint32_t& out) {
    if (!ea) return false;
    out = ld32(ea);
    return true;
}

bool read_u8(uint32_t ea, uint8_t& out) {
    if (!ea) return false;
    out = ld8(ea);
    return true;
}

float guest_f1(Cpu* c, uint32_t fn, uint32_t self) {
    guest_call(c, fn, {self});
    return (float)c->f[1].ps0;
}

uint64_t bg_check(Cpu* c, uint32_t bg, uint16_t x, uint16_t y, uint8_t layer) {
    guest_call(c, kGetBgCheckDataXy, {bg, x, y, layer, 1});
    return (uint64_t(c->r[3]) << 32) | c->r[4];
}

bool tile_solid(uint64_t data) {
    return uint32_t((data >> 32) & 0xF) != 0;
}

void push_box(Snapshot& s, float x0, float y0, float x1, float y1, bool tile, bool circle) {
    if (!(x1 > x0) || !(y1 > y0)) return;
    if (s.boxes.size() >= kMaxActors + kMaxTiles) return;
    s.boxes.push_back({x0, y0, x1, y1, tile, circle});
}

void capture_tiles(Cpu* c, Snapshot& s, uint32_t bg) {
    const int unit = (int)kTile;
    const int mask = ~(unit - 1);
    const int left = (int)std::floor(s.left) & mask;
    const int right = ((int)std::ceil(s.right) + unit - 1) & mask;
    const int bottom = ((int)std::ceil(-s.bottom) + unit - 1) & mask;
    const int top = (int)std::floor(-s.top) & mask;
    if (right <= left || bottom <= top) return;
    size_t tiles = 0;
    for (int ty = bottom - unit; ty >= top && tiles < kMaxTiles; ty -= unit) {
        for (int tx = left; tx < right && tiles < kMaxTiles; tx += unit) {
            bool hit = false;
            for (uint8_t layer = 0; layer < 3 && !hit; layer++) {
                if (tile_solid(bg_check(c, bg, (uint16_t)tx, (uint16_t)ty, layer))) hit = true;
            }
            if (!hit) continue;
            push_box(s, (float)tx, (float)-(ty + unit), (float)(tx + unit), (float)-ty, true, false);
            tiles++;
        }
    }
}

bool read_screen(uint32_t scroll, float& left, float& right, float& top, float& bottom) {
    if (!read_f32(scroll + kScreenLeft, left) || !read_f32(scroll + kScreenRight, right) ||
        !read_f32(scroll + kScreenTop, top) || !read_f32(scroll + kScreenBottom, bottom))
        return false;
    return (right > left) && (top > bottom);
}

bool read_screen_prev(uint32_t scroll, float& left, float& right, float& top, float& bottom) {
    float b = 0, t = 0;
    if (!read_f32(scroll + kScreenPrevLeft, left) || !read_f32(scroll + kScreenPrevBottom, b) ||
        !read_f32(scroll + kScreenPrevRight, right) || !read_f32(scroll + kScreenPrevTop, t))
        return false;
    bottom = b;
    top = t;
    return (right > left) && (top > bottom);
}

bool capture_one_cc(Cpu* c, Snapshot& s, uint32_t cc) {
    if (!cc) return false;
    uint32_t owner = 0, shape = 0;
    uint8_t mask = 0;
    if (!read_u32(cc + kCcOwner, owner) || !owner) return false;
    if (!read_u8(cc + kCcMask, mask) || mask == 0) return false;
    if (!read_u32(cc + kCcShape, shape)) return false;
    const float x0 = guest_f1(c, kGetLeftPos, cc);
    const float x1 = guest_f1(c, kGetRightPos, cc);
    const float y0 = guest_f1(c, kGetUnderPos, cc);
    const float y1 = guest_f1(c, kGetTopPos, cc);
    if (!std::isfinite(x0) || !std::isfinite(x1) || !std::isfinite(y0) || !std::isfinite(y1)) return false;
    push_box(s, x0, y0, x1, y1, false, shape == kShapeCircle);
    return true;
}

void capture_cc_list(Cpu* c, Snapshot& s, uint32_t head) {
    uint32_t node = head;
    size_t n = 0;
    while (node && n++ < kMaxActors && s.boxes.size() < kMaxActors + kMaxTiles) {
        uint32_t cc = 0, next = 0;
        if (!read_u32(node, cc) || !read_u32(node + 4, next)) break;
        node = next;
        capture_one_cc(c, s, cc);
    }
}

void capture_hitboxes_mgr(Cpu* c, Snapshot& s) {
    uint32_t mgr = 0;
    if (!read_u32(kActorCcMgr, mgr) || !mgr) return;
    uint32_t active = 0, create = 0;
    if (read_u32(mgr + kCcActiveHead, active) && active) capture_cc_list(c, s, active);
    if (read_u32(mgr + kCcCreateHead, create) && create) capture_cc_list(c, s, create);
}

void capture_hitboxes_actors(Cpu* c, Snapshot& s) {
    uint32_t mgr = 0;
    if (!read_u32(kActorMgr, mgr) || !mgr) return;
    const uint32_t cache = mgr + kActorMgrPtrCache;
    uint32_t array = 0, live = 0;
    if (!read_u32(cache + kCacheArray, array) || !array) return;
    if (!read_u32(cache + kCacheLiveEnd, live) || live < array) return;
    if (((live - array) & 3) || live - array > kMaxActors * 4) return;
    for (uint32_t p = array; p < live && s.boxes.size() < kMaxActors + kMaxTiles; p += 4) {
        uint32_t actor = 0;
        if (!read_u32(p, actor) || !actor) continue;
        uint8_t del = 0;
        if (read_u8(actor + kActorDeleteFlag, del) && del) continue;
        const uint32_t cc = actor + kActorCc;
        uint32_t owner = 0;
        if (!read_u32(cc + kCcOwner, owner) || owner != actor) continue;
        capture_one_cc(c, s, cc);
    }
}

bool bg_world_pos(uint32_t bgc, uint32_t actor, float& x, float& y) {
    float ox = 0, oy = 0;
    if (!read_f32(bgc + kBgPosOffX, ox) || !read_f32(bgc + kBgPosOffY, oy)) {
        ox = 0;
        oy = 0;
    }
    uint32_t posp = 0;
    if (read_u32(bgc + kBgFollowPos, posp) && posp) {
        if (read_f32(posp, x) && read_f32(posp + 4, y) && std::isfinite(x) && std::isfinite(y)) {
            x += ox;
            y += oy;
            return true;
        }
    }
    if (actor && read_f32(actor + kActorPos, x) && read_f32(actor + kActorPos + 4, y) &&
        std::isfinite(x) && std::isfinite(y)) {
        x += ox;
        y += oy;
        return true;
    }
    return false;
}

bool push_aabb(Snapshot& s, float x0, float y0, float x1, float y1) {
    if (x0 > x1) {
        const float t = x0;
        x0 = x1;
        x1 = t;
    }
    if (y0 > y1) {
        const float t = y0;
        y0 = y1;
        y1 = t;
    }
    constexpr float kMin = 1.0f;
    if (x1 - x0 < kMin) {
        const float c = (x0 + x1) * 0.5f;
        x0 = c - kMin * 0.5f;
        x1 = c + kMin * 0.5f;
    }
    if (y1 - y0 < kMin) {
        const float c = (y0 + y1) * 0.5f;
        y0 = c - kMin * 0.5f;
        y1 = c + kMin * 0.5f;
    }
    const float w = x1 - x0, h = y1 - y0;
    if (w > 512.0f || h > 512.0f) return false;
    push_box(s, x0, y0, x1, y1, true, false);
    return true;
}

bool expand_aabb(float& x0, float& y0, float& x1, float& y1, float x, float y, bool& any) {
    if (!std::isfinite(x) || !std::isfinite(y)) return false;
    if (!any) {
        x0 = x1 = x;
        y0 = y1 = y;
        any = true;
    } else {
        if (x < x0) x0 = x;
        if (y < y0) y0 = y;
        if (x > x1) x1 = x;
        if (y > y1) y1 = y;
    }
    return true;
}

bool ride_aabb(uint32_t bgc, float ox, float oy, float& x0, float& y0, float& x1, float& y1) {
    uint32_t count_u = 0, lines = 0;
    if (!read_u32(bgc + kBgRideCount, count_u) || !read_u32(bgc + kBgRidePtr, lines)) return false;
    const int32_t count = (int32_t)count_u;
    if (count < 1 || count > 32 || !lines) return false;
    bool any = false;
    for (int32_t i = 0; i < count; i++) {
        const uint32_t seg = lines + (uint32_t)i * kRideStride + kRideSeg;
        float ax = 0, ay = 0, bx = 0, by = 0;
        if (!read_f32(seg, ax) || !read_f32(seg + 4, ay) || !read_f32(seg + 8, bx) || !read_f32(seg + 12, by))
            return false;
        if (!expand_aabb(x0, y0, x1, y1, ax + ox, ay + oy, any) ||
            !expand_aabb(x0, y0, x1, y1, bx + ox, by + oy, any))
            return false;
    }
    return any;
}

bool near_pos(float x0, float y0, float x1, float y1, float px, float py) {
    const float cx = (x0 + x1) * 0.5f, cy = (y0 + y1) * 0.5f;
    return std::fabs(cx - px) < 256.0f && std::fabs(cy - py) < 256.0f;
}

bool capture_bg_ride(Snapshot& s, uint32_t bgc, float px, float py) {
    float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    if (ride_aabb(bgc, 0, 0, x0, y0, x1, y1) && near_pos(x0, y0, x1, y1, px, py))
        return push_aabb(s, x0, y0, x1, y1);
    x0 = y0 = x1 = y1 = 0;
    if (ride_aabb(bgc, px, py, x0, y0, x1, y1) && near_pos(x0, y0, x1, y1, px, py))
        return push_aabb(s, x0, y0, x1, y1);
    return false;
}

bool capture_bg_points(Snapshot& s, uint32_t bgc, uint32_t actor) {
    uint32_t count_u = 0, pts = 0;
    if (!read_u32(bgc + kBgPointCount, count_u) || !read_u32(bgc + kBgPointPtr, pts)) return false;
    const int32_t count = (int32_t)count_u;
    if (count < 2 || count > 32 || !pts) return false;
    float px = 0, py = 0;
    if (!bg_world_pos(bgc, actor, px, py)) return false;
    float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    bool any = false;
    for (int32_t i = 0; i < count; i++) {
        float lx = 0, ly = 0;
        if (!read_f32(pts + (uint32_t)i * 8, lx) || !read_f32(pts + (uint32_t)i * 8 + 4, ly)) return false;
        if (!expand_aabb(x0, y0, x1, y1, px + lx, py + ly, any)) return false;
    }
    if (!any) return false;
    return push_aabb(s, x0, y0, x1, y1);
}

bool capture_bg_affected(Snapshot& s, uint32_t bgc) {
    float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    if (!read_f32(bgc + kBgAffected, x0) || !read_f32(bgc + kBgAffected + 4, y0) ||
        !read_f32(bgc + kBgAffected + 8, x1) || !read_f32(bgc + kBgAffected + 12, y1))
        return false;
    if (!std::isfinite(x0) || !std::isfinite(y0) || !std::isfinite(x1) || !std::isfinite(y1)) return false;
    return push_aabb(s, x0, y0, x1, y1);
}

bool capture_one_bg(Snapshot& s, uint32_t bgc, std::unordered_set<uint32_t>& seen) {
    if (!bgc || !seen.insert(bgc).second) return false;
    uint8_t inactive = 0;
    if (read_u8(bgc + kBgInactive, inactive) && inactive) return false;
    uint32_t owner = 0;
    read_u32(bgc + kBgOwner, owner);
    if (capture_bg_points(s, bgc, owner)) return true;
    float px = 0, py = 0;
    if (bg_world_pos(bgc, owner, px, py) && capture_bg_ride(s, bgc, px, py)) return true;
    return capture_bg_affected(s, bgc);
}

void capture_bg_list(Snapshot& s, uint32_t head, std::unordered_set<uint32_t>& seen) {
    uint32_t node = head;
    size_t n = 0;
    while (node && n++ < kMaxActors && s.boxes.size() < kMaxActors + kMaxTiles) {
        uint32_t bgc = 0, next = 0;
        if (!read_u32(node, bgc) || !read_u32(node + 4, next)) break;
        node = next;
        capture_one_bg(s, bgc, seen);
    }
}

void capture_block_boxes(Snapshot& s, std::unordered_set<uint32_t>& seen) {
    uint32_t mgr = 0;
    if (!read_u32(kActorMgr, mgr) || !mgr) return;
    const uint32_t cache = mgr + kActorMgrPtrCache;
    uint32_t array = 0, live = 0;
    if (!read_u32(cache + kCacheArray, array) || !array) return;
    if (!read_u32(cache + kCacheLiveEnd, live) || live < array) return;
    if (((live - array) & 3) || live - array > kMaxActors * 4) return;
    for (uint32_t p = array; p < live && s.boxes.size() < kMaxActors + kMaxTiles; p += 4) {
        uint32_t actor = 0;
        if (!read_u32(p, actor) || !actor) continue;
        uint8_t del = 0;
        if (read_u8(actor + kActorDeleteFlag, del) && del) continue;
        const uint32_t bgc = actor + kBlockBoxBg;
        uint32_t owner = 0;
        if (!read_u32(bgc + kBgOwner, owner) || owner != actor) continue;
        capture_one_bg(s, bgc, seen);
    }
}

void capture_actor_bg(Snapshot& s) {
    std::unordered_set<uint32_t> seen;
    seen.reserve(256);
    capture_block_boxes(s, seen);
    uint32_t mgr = 0;
    if (!read_u32(kActorBgMgr, mgr) || !mgr) return;
    for (uint32_t head_off : kBgListHeads) {
        uint32_t head = 0;
        if (read_u32(mgr + head_off, head) && head) capture_bg_list(s, head, seen);
    }
}

void content_rect(float display_w, float display_h, float& ox, float& oy, float& ow, float& oh) {
    overlay::game_content_rect(display_w, display_h, ox, oy, ow, oh);
}

}

bool enabled() { return g_enabled.load(std::memory_order_relaxed); }

void set_enabled(bool on) { g_enabled.store(on, std::memory_order_relaxed); }

void capture(Cpu* c) {
    if (!c || !enabled()) {
        std::lock_guard<std::mutex> lk(g_mu);
        g_snap = {};
        return;
    }
    Snapshot s;
    uint32_t scroll = 0, bg = 0;
    if (!read_u32(kBgScrollMgr, scroll) || !scroll) {
        std::lock_guard<std::mutex> lk(g_mu);
        g_snap = {};
        return;
    }
    if (!read_screen_prev(scroll, s.left, s.right, s.top, s.bottom) &&
        !read_screen(scroll, s.left, s.right, s.top, s.bottom)) {
        std::lock_guard<std::mutex> lk(g_mu);
        g_snap = {};
        return;
    }
    s.boxes.reserve(1024);
    if (read_u32(kBg, bg) && bg) capture_tiles(c, s, bg);
    capture_actor_bg(s);
    capture_hitboxes_mgr(c, s);
    capture_hitboxes_actors(c, s);
    s.valid = true;
    std::lock_guard<std::mutex> lk(g_mu);
    g_snap = std::move(s);
}

void draw(ImDrawList* dl, float display_w, float display_h) {
    if (!dl || !enabled()) return;
    Snapshot s;
    {
        std::lock_guard<std::mutex> lk(g_mu);
        s = g_snap;
    }
    if (!s.valid) return;

    const float left = s.left, right = s.right, top = s.top, bottom = s.bottom;
    const float w = right - left, h = top - bottom;
    if (w <= 1 || h <= 1 || display_w <= 1 || display_h <= 1) return;

    float ox = 0, oy = 0, ow = display_w, oh = display_h;
    content_rect(display_w, display_h, ox, oy, ow, oh);
    if (ow <= 1 || oh <= 1) return;

    auto to_screen = [&](float x, float y) {
        return ImVec2(ox + (x - left) / w * ow, oy + (top - y) / h * oh);
    };
    const ImU32 tile_col = IM_COL32(40, 120, 255, 220);
    const ImU32 hit_col = IM_COL32(255, 40, 40, 220);
    const ImU32 hit_fill = IM_COL32(255, 40, 40, 50);
    for (const Box& b : s.boxes) {
        const ImVec2 a = to_screen(b.x0, b.y1), c = to_screen(b.x1, b.y0);
        if (b.tile) {
            dl->AddRect(a, c, tile_col, 0.0f, 1.0f);
            dl->AddLine(a, c, tile_col, 1.0f);
            continue;
        }
        if (b.circle) {
            const ImVec2 center((a.x + c.x) * 0.5f, (a.y + c.y) * 0.5f);
            const float rx = std::fabs(c.x - a.x) * 0.5f;
            const float ry = std::fabs(c.y - a.y) * 0.5f;
            dl->AddEllipse(center, ImVec2(rx, ry), hit_col, 0.0f, 0, 1.0f);
            continue;
        }
        dl->AddRectFilled(a, c, hit_fill);
        dl->AddRect(a, c, hit_col, 0.0f, 1.0f);
        dl->AddLine(a, c, hit_col, 1.0f);
    }
}

}
