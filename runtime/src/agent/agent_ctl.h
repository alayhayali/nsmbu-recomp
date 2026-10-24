#pragma once

#include "input/input.h"

namespace agent_ctl {

void init();
void shutdown();
void service();
void guest_service();
void guest_observe();
void apply_pad(input::PadState& pad);
bool active();

}
