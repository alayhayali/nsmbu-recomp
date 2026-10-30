#include "gamepad_nav.h"
#include "imgui.h"

namespace overlay {

void GamepadNavigation::feed(ImGuiIO& io, const float* values, const float* previous, bool enabled, bool mouse_used,
                             const int* face) {
    using namespace input_map;
    static constexpr int kByLabel[4] = {kPadA, kPadB, kPadX, kPadY};
    if (!face) face = kByLabel;
    constexpr int navigation[] = {kPadA, kPadB, kPadX, kPadY, kPadDUp, kPadDDown, kPadDLeft, kPadDRight,
                                  kPadLSUp, kPadLSDown, kPadLSLeft, kPadLSRight, kPadLT, kPadRT, kPadLB,
                                  kPadRB, kPadMenu, kPadOptions};
    bool nav_press = false;
    if (enabled)
        for (int p : navigation) nav_press |= values[p] > 0.5f && previous[p] <= 0.5f;
    if (mouse_used) mouse_owned_ = true;
    else if (nav_press) mouse_owned_ = false;
    const bool active = enabled && controller_active();
    if (active) io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    else io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
    auto key = [&](ImGuiKey k, int p) {
        const float v = active && values[p] >= 0.35f ? values[p] : 0.0f;
        io.AddKeyAnalogEvent(k, v > 0.5f, v);
    };
    key(ImGuiKey_GamepadFaceDown, face[0]);
    key(ImGuiKey_GamepadFaceRight, face[1]);
    key(ImGuiKey_GamepadFaceLeft, face[2]);
    key(ImGuiKey_GamepadFaceUp, face[3]);
    key(ImGuiKey_GamepadDpadUp, kPadDUp);
    key(ImGuiKey_GamepadDpadDown, kPadDDown);
    key(ImGuiKey_GamepadDpadLeft, kPadDLeft);
    key(ImGuiKey_GamepadDpadRight, kPadDRight);
    key(ImGuiKey_GamepadLStickUp, kPadLSUp);
    key(ImGuiKey_GamepadLStickDown, kPadLSDown);
    key(ImGuiKey_GamepadLStickLeft, kPadLSLeft);
    key(ImGuiKey_GamepadLStickRight, kPadLSRight);
    key(ImGuiKey_GamepadL2, kPadLT);
    key(ImGuiKey_GamepadR2, kPadRT);
    key(ImGuiKey_GamepadStart, kPadMenu);
    key(ImGuiKey_GamepadBack, kPadOptions);
}

}
