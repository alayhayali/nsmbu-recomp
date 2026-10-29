#pragma once
#include <algorithm>
#include <cmath>
namespace mobile_ui {
constexpr float kText = 17.0f;
constexpr float kClassicPad = 16.0f;
constexpr float kPhonePad = 21.0f;
constexpr float kPhoneAspect = 1.9f;
constexpr float kPhoneDiagonal = 7.5f;
constexpr float kPhoneAutomaticMm = 7.62f;
constexpr float kAndroidSettingsMm = 11.0f;
constexpr float kAndroidSettingsScaleBoost = 1.15f;
enum Preset { kAutomatic, kSmall, kMedium, kLarge };

inline float dpi_of(float dpi, float density) {
    if (std::isfinite(dpi) && dpi >= 72 && dpi <= 1000) return dpi;
    return 160 * std::clamp(std::isfinite(density) ? density : 1.0f, 1.0f, 6.0f);
}

inline bool phone_screen(float width, float height, float dpi) {
    const float lo = std::min(width, height), hi = std::max(width, height);
    if (!(lo > 0) || !(dpi > 0)) return false;
    return hi / lo >= kPhoneAspect && std::hypot(width, height) / dpi <= kPhoneDiagonal;
}

struct Layout {
    float scale = 1;
    float pad_y = kClassicPad;
    bool phone = false;
    float control() const { return kText + 2 * pad_y; }
    float control_mm(float dpi) const { return control() * scale * 25.4f / dpi; }
    float text_mm(float dpi) const { return kText * scale * 25.4f / dpi; }
};

inline Layout layout(float dpi, float density, int preset, float width = 0, float height = 0) {
    dpi = dpi_of(dpi, density);
    Layout l;
    l.phone = phone_screen(width, height, dpi);
    l.pad_y = l.phone ? kPhonePad : kClassicPad;
    const float mm = preset == kSmall ? 7.0f : preset == kLarge ? 9.0f : preset == kMedium ? 8.0f
                     : l.phone        ? kPhoneAutomaticMm
                                      : 8.0f;
    l.scale = std::clamp(dpi * mm / (25.4f * l.control()), 1.0f, 12.0f);
    return l;
}

inline float scale(float dpi, float density, int preset) { return layout(dpi, density, preset).scale; }

inline Layout android_settings(float dpi, float density, float width, float height) {
    dpi = dpi_of(dpi, density);
    Layout l;
    l.phone = phone_screen(width, height, dpi);
    l.pad_y = kPhonePad;
    l.scale = std::clamp(dpi * kAndroidSettingsMm / (25.4f * l.control()), 1.0f, 12.0f);
    l.scale = std::clamp(l.scale * kAndroidSettingsScaleBoost, 1.0f, 12.0f);
    return l;
}

inline float desktop_scale(float density, float window_height) {
    return std::max({1.0f, density, window_height / 820.0f});
}

template <class Style> void apply_touch_style(Style& s, const Layout& l) {
    s.FramePadding.x = 10;
    s.FramePadding.y = l.pad_y;
    s.ItemInnerSpacing.x = 10;
    s.ItemInnerSpacing.y = 8;
    s.ScrollbarSize = l.control();
    s.GrabMinSize = l.control();
}
}
