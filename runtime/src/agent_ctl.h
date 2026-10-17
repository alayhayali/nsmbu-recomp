#pragma once

#include "input.h"

namespace agent_ctl {

void init();
void shutdown();
void service();
void apply_pad(input::PadState& pad);
bool active();

}
