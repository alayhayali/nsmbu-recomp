#include "core/runtime.h"
#include "mods/sdk/nsmbu.h"

extern "C" void f_02931C1C_orig(Cpu* c);

extern "C" void hook_02931C1C(Cpu* c) {
    if (mods::lua_sdk::pickup_locked(c->r[3], (int32_t)c->r[4])) {
        c->r[3] = 0;
        return;
    }
    f_02931C1C_orig(c);
}
