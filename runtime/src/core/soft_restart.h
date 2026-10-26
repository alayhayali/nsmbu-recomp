
#pragma once
#include "core/runtime.h"

namespace soft_restart {

void request(const char* why);
bool pending();
const char* reason();

void prepare();
void run_if_ready();

bool on_guest_main_exit();
void wait_until_idle();
bool process_exit_blocked();

bool take_ui_refresh();

void set_module(const LoadedModule& m);
LoadedModule& module();
uint32_t argv_ea();
void set_argv_ea(uint32_t ea);

}
