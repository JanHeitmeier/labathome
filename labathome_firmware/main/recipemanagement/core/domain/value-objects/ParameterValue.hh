#pragma once

#include <variant>
#include <string>
#include <cstdint>
#include <type_traits>

// Definiert alle möglichen Typen, die ein Parameter annehmen kann.
using ParameterValue = std::variant<
    std::monostate, // Leerer Zustand
    bool,
    int32_t,
    uint32_t,
    int64_t,
    uint64_t,
    float,
    double,
    std::string
>;

// Hilfsfunktion, um einen ParameterValue zur Anzeige in einen String umzuwandeln. für DTOs
inline std::string to_string(const ParameterValue& pv) {
    if (std::holds_alternative<std::monostate>(pv)) {
        return "";
    }
    return std::visit([](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return ""; // Sollte nicht passieren, aber sicher ist sicher
        } else if constexpr (std::is_same_v<T, std::string>) {
            return arg;
        } else if constexpr (std::is_same_v<T, bool>) {
            return arg ? "true" : "false";
        } else {
            return std::to_string(arg);
        }
    }, pv);
}
