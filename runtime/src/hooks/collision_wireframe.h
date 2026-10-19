#pragma once

struct Cpu;
struct ImDrawList;

namespace collision_wireframe {

bool enabled();
void set_enabled(bool on);

void capture(Cpu* c);
void draw(ImDrawList* dl, float display_w, float display_h);

}
