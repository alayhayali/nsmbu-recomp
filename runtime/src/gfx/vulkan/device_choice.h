#pragma once
#include <cctype>
#include <mutex>
#include <string>
#include <vector>

namespace gfxvk {

enum GpuClass {
    kGpuOther = 0,
    kGpuIntegrated = 1,
    kGpuDiscrete = 2,
    kGpuVirtual = 3,
    kGpuCpu = 4,
};

struct GpuCandidate {
    std::string name;
    std::string identity;
    std::string unavailable;
    int kind = kGpuOther;
    int rank = 1;
    bool usable() const { return unavailable.empty(); }
};

struct GpuChoice {
    int index = -1;
    std::string reason;
    std::string fallback;
};

inline const char* gpu_kind_name(int kind) {
    switch (kind) {
    case kGpuIntegrated: return "integrated";
    case kGpuDiscrete: return "dedicated";
    case kGpuVirtual: return "virtual";
    case kGpuCpu: return "CPU";
    default: return "other";
    }
}

inline bool gpu_name_has(const std::string& name, const std::string& want) {
    auto lower = [](std::string s) {
        for (char& c : s) c = char(std::tolower(static_cast<unsigned char>(c)));
        return s;
    };
    return lower(name).find(lower(want)) != std::string::npos;
}

inline int first_ranked(const std::vector<GpuCandidate>& devices, int kind) {
    for (int rank = 0; rank < 2; rank++)
        for (size_t i = 0; i < devices.size(); i++)
            if (devices[i].usable() && devices[i].rank == rank && (kind < 0 || devices[i].kind == kind))
                return int(i);
    return -1;
}

inline GpuChoice choose_gpu(const std::vector<GpuCandidate>& devices, const std::string& saved, const std::string& env) {
    const std::string mode = !env.empty() ? env : (saved.empty() ? "dedicated" : saved);
    GpuChoice result;
    auto take = [&](int index, const char* why) {
        result.index = index;
        result.reason = why;
    };
    auto prefer = [&](int kind, const char* why) {
        int index = first_ranked(devices, kind);
        if (index < 0) {
            index = first_ranked(devices, -1);
            if (index >= 0) why = kind == kGpuIntegrated ? "integrated unavailable" : "dedicated unavailable";
        }
        if (index >= 0) take(index, why);
    };
    const bool dedicated = mode == "dedicated" || mode == "discrete";
    const bool integrated = mode == "integrated";
    if (dedicated) prefer(kGpuDiscrete, env.empty() ? "dedicated" : "env");
    else if (integrated) prefer(kGpuIntegrated, env.empty() ? "integrated" : "env");
    else {
        int selected = -1;
        for (size_t i = 0; i < devices.size(); i++)
            if (devices[i].identity == mode || gpu_name_has(devices[i].name, mode)) {
                selected = int(i);
                break;
            }
        if (selected >= 0 && devices[selected].usable()) take(selected, env.empty() ? "setting" : "env");
        else {
            result.fallback = selected < 0 ? "The chosen GPU is not available"
                                           : "The chosen GPU is not available (" + devices[selected].unavailable + ")";
            prefer(kGpuDiscrete, "dedicated");
        }
    }
    return result;
}

inline bool gpu_restart_pending(const std::string& saved, const std::string& active_identity, int active_kind,
                                bool have_discrete, bool have_integrated) {
    const std::string mode = saved.empty() ? "dedicated" : saved;
    if (mode == "dedicated" || mode == "discrete") return have_discrete && active_kind != kGpuDiscrete;
    if (mode == "integrated") return have_integrated && active_kind != kGpuIntegrated;
    return mode != active_identity;
}

inline std::vector<GpuCandidate> device_inventory;
inline std::string active_gpu_name, active_gpu_identity, gpu_fallback;
inline int active_gpu_kind = kGpuOther;
inline std::mutex device_setting_mutex;
inline std::string saved_device;
inline std::string device_setting() {
    std::lock_guard<std::mutex> lock(device_setting_mutex);
    return saved_device;
}
inline void set_device_setting(const std::string& identity) {
    std::lock_guard<std::mutex> lock(device_setting_mutex);
    saved_device = identity;
}

}
