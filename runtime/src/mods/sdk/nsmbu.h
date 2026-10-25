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

struct SceneReport {
    std::string scene;
    bool map_known = false;
    bool map = false;
    bool course_known = false;
    bool course = false;
    std::string pack;
    std::string entering;
    bool file_known = false;
    int file = 0;
    bool cp_known = false;
    int32_t cp = 0;
    bool world_known = false;
    int32_t world = 0;
    bool node_known = false;
    int32_t node = 0;
};

void scene_report(SceneReport& out);
bool load_story_course(int32_t world, int32_t course, std::string& err);
bool open_course_select(std::string& err);

}
