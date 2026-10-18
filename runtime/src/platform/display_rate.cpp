

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
}

void poll(SDL_Window* tv) {
#ifdef __ANDROID__
    (void)tv;

    static int requested = -1, maxHz = 0;
    if (!maxHz) maxHz = (int)std::lround(call_float("maxRefreshRate"));
    const int want = interp::mode() == 1 && interp::fps() > 60 && maxHz > 60 ? std::min(interp::fps(), maxHz) : 0;
    if (want != requested) {
        requested = want;
        request((float)want);
        if (want) LOG("[display] %d fps frame interpolation: asking for a %d Hz display mode (the display offers up to %d Hz)", interp::fps(), want, maxHz);
    }
    const float hz = call_float("displayRefreshRate");
    interp::set_display_hz((int)std::lround(hz));
    gx2::set_display_vsync_ns(period_ns(hz, 0, 0));
#else
    const SDL_DisplayID id = tv ? SDL_GetDisplayForWindow(tv) : SDL_GetPrimaryDisplay();
    const SDL_DisplayMode* m = id ? SDL_GetCurrentDisplayMode(id) : nullptr;
    interp::set_display_hz(m ? (int)std::lround(m->refresh_rate) : 0);
    const int64_t ns = m ? period_ns(m->refresh_rate, m->refresh_rate_numerator, m->refresh_rate_denominator) : 0;
    static int64_t seen = -1;
    if (ns != seen) {
        seen = ns;
        LOG("[display] mode %.3f Hz (%d/%d), period %lld ns", m ? m->refresh_rate : 0.f,
            m ? m->refresh_rate_numerator : 0, m ? m->refresh_rate_denominator : 0, (long long)ns);
    }
    gx2::set_display_vsync_ns(ns);
#endif
}
}
