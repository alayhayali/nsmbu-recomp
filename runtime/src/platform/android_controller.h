#pragma once
#include <cstdint>
#include <string>

namespace android_controller {
inline bool ayaneo(uint16_t vendor) { return vendor == 0x2020; }

inline std::string with_stick_axes(std::string mapping) {
    if (mapping.find("leftx:") != std::string::npos) return mapping;
    if (!mapping.empty() && mapping.back() != ',') mapping += ',';
    return mapping + "leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a4,righttrigger:a5,";
}

inline std::string mapping(uint16_t vendor, uint16_t product, const std::string& original) {
    if (!ayaneo(vendor)) return original;
    if (product != 0x0111) return with_stick_axes(original);
    const auto first = original.find(',');
    const auto second = first == std::string::npos ? first : original.find(',', first + 1);
    if (second == std::string::npos) return with_stick_axes(original);
    std::string result = original.substr(0, second + 1);
    for (size_t start = second + 1; start < original.size();) {
        const auto end = original.find(',', start);
        const auto token = original.substr(start, end == std::string::npos ? end : end - start);
        const auto key = token.substr(0, token.find(':'));
        if (key != "a" && key != "b" && key != "x" && key != "y" && key != "face" && !token.empty())
            result += token + ',';
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return with_stick_axes(result + "a:b1,b:b0,x:b3,y:b2,face:bayx,");
}
}
