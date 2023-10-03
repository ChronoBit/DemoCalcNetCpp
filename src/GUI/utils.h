#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

inline std::string join(char separator, const std::vector<std::string>& items) {
    std::string res;
    const auto sz = items.size();
    for (size_t i = 0; i < sz; i++) {
        res += items[i];
        if (i + 1 != sz) res += separator;
    }
    return res;
}

inline std::string join(const std::vector<std::string>& items) {
    std::string res;
    for (const auto& item : items) {
        res += item;
    }
    return res;
}

inline bool contains(const std::string& str, const std::string& val) {
    return str.find_first_of(val) != std::string::npos;
}

inline bool contains(const std::string& str, char val) {
    return str.find_first_of(val) != std::string::npos;
}

inline std::string toString(double value) {
    std::stringstream stream;
    stream << std::fixed << std::setprecision(2) << value;
    return stream.str();
}