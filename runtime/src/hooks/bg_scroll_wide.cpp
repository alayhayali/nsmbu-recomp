#include "gfx/aspect.h"
#include "gfx/aspect_view.h"
#include "gfx/bg_scroll_wide.h"
#include "guest_addr.h"
#include "ppc.h"

extern "C" void f_029A6170_orig(Cpu* c);

namespace {

constexpr uint32_t kMgrMin = 0x10000000u;
constexpr uint32_t kScreenMinX = 0x28u;
constexpr uint32_t kScreenMinY = 0x2Cu;
constexpr uint32_t kScreenMaxX = 0x30u;
constexpr uint32_t kScreenMaxY = 0x34u;
constexpr uint32_t kPrevMinX = 0x38u;
constexpr uint32_t kPrevMinY = 0x3Cu;
constexpr uint32_t kPrevMaxX = 0x40u;
constexpr uint32_t kPrevMaxY = 0x44u;
constexpr uint32_t kZoomMinX = 0x48u;
constexpr uint32_t kZoomMinY = 0x4Cu;
constexpr uint32_t kZoomMaxX = 0x50u;
constexpr uint32_t kZoomMaxY = 0x54u;
constexpr uint32_t kDefaultMinX = 0x58u;
constexpr uint32_t kDefaultMinY = 0x5Cu;
constexpr uint32_t kDefaultMaxX = 0x60u;
constexpr uint32_t kDefaultMaxY = 0x64u;
constexpr uint32_t kPrepCall = 0x029A63ECu;
constexpr uint32_t kWidth = 0x68u;
constexpr uint32_t kHeight = 0x6Cu;
constexpr uint32_t kLeft = 0x70u;
constexpr uint32_t kCenterX = 0x74u;
constexpr uint32_t kRight = 0x78u;
constexpr uint32_t kBgMinX = 0x88u;
constexpr uint32_t kBgMinY = 0x8Cu;
constexpr uint32_t kBgMaxX = 0x90u;
constexpr uint32_t kBgMaxY = 0x94u;
constexpr uint32_t kBgPrevMinX = 0x98u;
constexpr uint32_t kBgPrevMinY = 0x9Cu;
constexpr uint32_t kBgPrevMaxX = 0xA0u;
constexpr uint32_t kBgPrevMaxY = 0xA4u;
const uint32_t kCourseInfo = GD(0x101D18EC);
const uint32_t kPlayerMgr = GD(0x101E6994);
constexpr uint32_t kCourseKind = 0x11u;
constexpr uint32_t kTitleCourse = 0x26u;
constexpr uint32_t kPlayerSlot = 0x20u;
constexpr uint32_t kPlayerPosX = 0x6Cu;
constexpr float kKxEpsilon = 1.0001f;

bool title_course() {
    uint32_t info = ld32(kCourseInfo);
    if (info < kMgrMin)
        return false;
    return ld8(info + kCourseKind) == kTitleCourse;
}

float midpoint(float left, float right) { return (left + right) * 0.5f; }

float player_pivot(float left, float right) {
    float lo = left < right ? left : right;
    float hi = left < right ? right : left;
    uint32_t mgr = ld32(kPlayerMgr);
    if (mgr < kMgrMin)
        return midpoint(left, right);
    float sum = 0.0f;
    int n = 0;
    for (uint32_t i = 0; i < 4; i++) {
        uint32_t actor = ld32(mgr + kPlayerSlot + i * 4u);
        if (actor < kMgrMin)
            continue;
        float x = (float)ldf32(actor + kPlayerPosX);
        if (x < lo || x > hi)
            continue;
        sum += x;
        n++;
    }
    if (!n)
        return midpoint(left, right);
    return sum / (float)n;
}

void widen_box(uint32_t mgr, uint32_t min_x, uint32_t max_x, uint32_t min_y, uint32_t max_y, float kx, float pivot) {
    float left = (float)ldf32(mgr + min_x);
    float right = (float)ldf32(mgr + max_x);
    float low = (float)ldf32(mgr + min_y);
    float high = (float)ldf32(mgr + max_y);
    float width = std::fabs(right - left);
    float height = std::fabs(high - low);
    if (!aspect::scroll::should_widen(width, height, kx))
        return;
    aspect::scroll::Edges box = aspect::scroll::widen(left, right, pivot, kx);
    stf32(mgr + min_x, box.left);
    stf32(mgr + max_x, box.right);
}

void widen_scalars(uint32_t mgr, float kx, float pivot) {
    float left = (float)ldf32(mgr + kLeft);
    float right = (float)ldf32(mgr + kRight);
    float height = std::fabs((float)ldf32(mgr + kHeight));
    if (!aspect::scroll::should_widen(std::fabs(right - left), height, kx))
        return;
    aspect::scroll::Edges scalar = aspect::scroll::widen(left, right, pivot, kx);
    stf32(mgr + kLeft, scalar.left);
    stf32(mgr + kRight, scalar.right);
    stf32(mgr + kWidth, scalar.width);
    stf32(mgr + kCenterX, midpoint(scalar.left, scalar.right));
}

}

extern "C" void hook_029A6170(Cpu* c) {
    uint32_t mgr = c->r[3];
    f_029A6170_orig(c);
    if (mgr < kMgrMin)
        return;
    if (aspect::mode() == aspect::kOriginal)
        return;
    float aspect = aspect::game();
    float kx, ky;
    aspect::view::factors(aspect, kx, ky);
    if (kx <= kKxEpsilon)
        return;
    bool prep = c->lr == kPrepCall;
    float saved[8];
    if (prep) {
        for (int i = 0; i < 8; i++) saved[i] = (float)ldf32(mgr + kWidth + (uint32_t)i * 4u);
    }
    if (!title_course()) {
        float left = (float)ldf32(mgr + kScreenMinX);
        float right = (float)ldf32(mgr + kScreenMaxX);
        float pivot = player_pivot(left, right);
        widen_box(mgr, kScreenMinX, kScreenMaxX, kScreenMinY, kScreenMaxY, kx, pivot);
        widen_box(mgr, kPrevMinX, kPrevMaxX, kPrevMinY, kPrevMaxY, kx, pivot);
        widen_box(mgr, kZoomMinX, kZoomMaxX, kZoomMinY, kZoomMaxY, kx, pivot);
        widen_box(mgr, kDefaultMinX, kDefaultMaxX, kDefaultMinY, kDefaultMaxY, kx, pivot);
        widen_scalars(mgr, kx, pivot);
        widen_box(mgr, kBgMinX, kBgMaxX, kBgMinY, kBgMaxY, kx, pivot);
        widen_box(mgr, kBgPrevMinX, kBgPrevMaxX, kBgPrevMinY, kBgPrevMaxY, kx, pivot);
    } else {
        float bg = midpoint((float)ldf32(mgr + kBgMinX), (float)ldf32(mgr + kBgMaxX));
        float bg_prev = midpoint((float)ldf32(mgr + kBgPrevMinX), (float)ldf32(mgr + kBgPrevMaxX));
        widen_box(mgr, kBgMinX, kBgMaxX, kBgMinY, kBgMaxY, kx, bg);
        widen_box(mgr, kBgPrevMinX, kBgPrevMaxX, kBgPrevMinY, kBgPrevMaxY, kx, bg_prev);
    }
    if (prep) {
        for (int i = 0; i < 8; i++) stf32(mgr + kWidth + (uint32_t)i * 4u, saved[i]);
    }
}
