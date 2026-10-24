#pragma once

#include <cstdint>
#include <string>

struct lua_State;
struct Cpu;

namespace mods::lua_mod {
struct Instance;
}

namespace mods::lua_sdk {

void install(lua_State* L, lua_mod::Instance* inst);
void forget(lua_mod::Instance* inst);
void set_cpu(Cpu* c);
Cpu* cpu();
void request_load_story(int world, int course);
std::string load_story_status();
bool story_leave_pending();
uint64_t story_load_generation();
void native_intro_skip_tick(uint64_t step);

}
