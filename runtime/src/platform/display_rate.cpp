

#include "display_rate.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>

#include "../gx2/gx2.h"
#include "hooks/interp.h"
#include "core/runtime.h"

#ifdef __ANDROID__
#include <jni.h>
#endif

namespace display_rate {
namespace {
#ifdef __ANDROID__

jclass activity_class(JNIEnv* env) {
    jobject activity = (jobject)SDL_GetAndroidActivity();
    if (!activity) return nullptr;
    jclass cls = env->GetObjectClass(activity);
    env->DeleteLocalRef(activity);
    return cls;
}
float call_float(const char* name) {
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    if (!env) return 0;
    jclass cls = activity_class(env);
    if (!cls) return 0;
    float v = 0;
    if (jmethodID m = env->GetStaticMethodID(cls, name, "()F")) v = env->CallStaticFloatMethod(cls, m);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        v = 0;
    }
    env->DeleteLocalRef(cls);
    return v;
}
void request(float hz) {
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    if (!env) return;
    jclass cls = activity_class(env);
    if (!cls) return;
    if (jmethodID m = env->GetStaticMethodID(cls, "requestRefreshRate", "(F)V")) env->CallStaticVoidMethod(cls, m, (jfloat)hz);
    if (env->ExceptionCheck()) env->ExceptionClear();
    env->DeleteLocalRef(cls);
}
#endif

int64_t period_ns(float hz, int numerator, int denominator) {
    if (numerator > 0 && denominator > 0)
        return (int64_t)(((uint64_t)denominator * 1000000000ull + (uint64_t)numerator / 2) / (uint64_t)numerator);
    if (hz >= 1.f) return (int64_t)std::llround(1000000000.0 / hz);
    return 0;
}

float mode_hz(const SDL_DisplayMode* m) {
    if (!m) return 0.f;
    if (m->refresh_rate > 0.f) return m->refresh_rate;
    if (m->refresh_rate_numerator > 0 && m->refresh_rate_denominator > 0)
        return float(m->refresh_rate_numerator) / float(m->refresh_rate_denominator);
    return 0.f;
}

const SDL_DisplayMode* mode_with_refresh(SDL_DisplayID id, SDL_DisplayMode& stored) {
    const SDL_DisplayMode* current = id ? SDL_GetCurrentDisplayMode(id) : nullptr;
    if (mode_hz(current) > 0.f) return current;
    const SDL_DisplayMode* desktop = id ? SDL_GetDesktopDisplayMode(id) : nullptr;
    if (mode_hz(desktop) > 0.f) return desktop;
    if (!id) return current ? current : desktop;
    int count = 0;
    SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(id, &count);
    const SDL_DisplayMode* match = nullptr;
    if (modes) {
        const int w = current && current->w > 0 ? current->w : desktop ? desktop->w : 0;
        const int h = current && current->h > 0 ? current->h : desktop ? desktop->h : 0;
        for (int i = 0; i < count; i++) {
            if (!modes[i] || mode_hz(modes[i]) <= 0.f) continue;
            if (w > 0 && h > 0 && (modes[i]->w != w || modes[i]->h != h)) continue;
            match = modes[i];
            break;
        }
        if (!match) {
            for (int i = 0; i < count; i++) {
                if (modes[i] && mode_hz(modes[i]) > 0.f) { match = modes[i]; break; }
            }
        }
        if (match) stored = *match;
        SDL_free(modes);
    }
    return match ? &stored : current ? current : desktop;
}
}

void poll(SDL_Window* tv) {
#ifdef __ANDROID__
    (void)tv;

    static int requested = -1, maxHz = 0;
    if (!maxHz) maxHz = (int)std::lround(call_float("maxRefreshRate"));
    const int want = interp::refresh_request_hz(maxHz);
    if (want != requested) {
        const int prev = requested;
        requested = want;
        request((float)want);
        if (interp::course_loading() && want) LOG("[display] course load: asking for a %d Hz display mode (the display offers up to %d Hz)", want, maxHz);
        else if (interp::unlocked() && want) LOG("[display] unlocked fps: asking for a %d Hz display mode (the display offers up to %d Hz)", want, maxHz);
        else if (want) LOG("[display] %d fps frame interpolation: asking for a %d Hz display mode (the display offers up to %d Hz)", interp::fps(), want, maxHz);
        else if (prev > 0) LOG("[display] default refresh rate");
    }
    const float hz = call_float("displayRefreshRate");
    interp::set_display_hz((int)std::lround(hz));
    gx2::set_display_vsync_ns(period_ns(hz, 0, 0));
#else
    const SDL_DisplayID id = tv ? SDL_GetDisplayForWindow(tv) : SDL_GetPrimaryDisplay();
    SDL_DisplayMode stored{};
    const SDL_DisplayMode* m = mode_with_refresh(id, stored);
    static int last_hz = 0;
    static int last_num = 0;
    static int last_den = 0;
    static int64_t last_ns = 0;
    const float hz_f = mode_hz(m);
    const int hz = (int)std::lround(hz_f);
    const int num = m ? m->refresh_rate_numerator : 0;
    const int den = m ? m->refresh_rate_denominator : 0;
    const int64_t ns = m ? period_ns(hz_f, num, den) : 0;
    if (ns <= 0) {
        if (last_ns > 0) return;
        if (last_hz != -1) {
            last_hz = -1;
            LOG("[display] mode %.3f Hz (%d/%d), period 0 ns", hz_f, num, den);
        }
        return;
    }
    if (ns != last_ns || hz != last_hz || num != last_num || den != last_den) {
        last_ns = ns;
        last_hz = hz;
        last_num = num;
        last_den = den;
        interp::set_display_hz(hz);
        LOG("[display] mode %.3f Hz (%d/%d), period %lld ns", hz_f, num, den, (long long)ns);
        gx2::set_display_vsync_ns(ns);
    }
#endif
}
}
