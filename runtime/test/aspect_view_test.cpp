#include "gfx/aspect_view.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>

static void check(bool ok) {
    if (!ok) {
        std::fprintf(stderr, "aspect view check failed\n");
        std::exit(1);
    }
}

static bool same(float a, float b) { return std::fabs(a - b) <= 1e-4f * std::fmax(1.0f, std::fabs(a)); }

int main() {
    using namespace aspect::view;
    const float wide = 64.0f / 27.0f, tall = 16.0f / 10.0f;
    Ortho screen = expand_ortho(360, -360, -640, 640, kBase);
    check(screen.top == 360 && screen.bottom == -360 && screen.left == -640 && screen.right == 640);

    Ortho wider = expand_ortho(360, -360, -640, 640, wide);
    check(same(wider.top, 360) && same(wider.bottom, -360));
    check(same(wider.left, -640.0f * (wide / kBase)) && same(wider.right, 640.0f * (wide / kBase)));
    Ortho again = expand_ortho(wider.top, wider.bottom, wider.left, wider.right, wide);
    check(again.top == wider.top && again.bottom == wider.bottom && again.left == wider.left && again.right == wider.right);

    Ortho higher = expand_ortho(360, -360, -640, 640, tall);
    check(same(higher.left, -640) && same(higher.right, 640));
    check(same(higher.top, 360.0f * (kBase / tall)) && same(higher.bottom, -360.0f * (kBase / tall)));

    Ortho coin = expand_ortho(512, 0, 0, 2048, wide);
    check(coin.top == 512 && coin.bottom == 0 && coin.left == 0 && coin.right == 2048);
    Ortho small = expand_ortho(18, -18, -32, 32, wide);
    check(small.top == 18 && small.left == -32);

    Persp level = expand_perspective(0.7f, kBase, kBase);
    check(level.fovy == 0.7f && level.aspect == kBase);
    Persp ultra = expand_perspective(0.7f, kBase, wide);
    check(ultra.fovy == 0.7f && ultra.aspect == wide);
    Persp second = expand_perspective(ultra.fovy, ultra.aspect, wide);
    check(second.fovy == ultra.fovy && second.aspect == ultra.aspect);
    float ky = kBase / tall;
    float want = 2.0f * std::atan(std::tan(0.7f * 0.5f) * ky);
    Persp ten = expand_perspective(0.7f, kBase, tall);
    check(same(ten.fovy, want) && ten.aspect == tall);
    Persp other = expand_perspective(0.7f, 2.0f, wide);
    check(other.fovy == 0.7f && other.aspect == 2.0f);

    std::puts("aspect view: ortho and perspective passed");
}
