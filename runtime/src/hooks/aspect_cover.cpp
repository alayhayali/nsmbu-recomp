#include "core/runtime.h"
#include "gx2/gx2_cmd.h"

extern "C" void f_029B85C8_orig(Cpu* c);
extern "C" void hook_029B85C8(Cpu* c) {
    bool cover = c->r[4] == 17 || c->r[4] == 29;
    if (cover) gx2::emit(gx2::OP_COVER_DRAW, {1u});
    f_029B85C8_orig(c);
    if (cover) gx2::emit(gx2::OP_COVER_DRAW, {0u});
}
