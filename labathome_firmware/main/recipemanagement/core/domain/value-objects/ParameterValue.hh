#pragma once

#include <variant>
#include <string>
#include <cstdint>
#include <type_traits>

//Noch Analyse ob es sinnvol ist die Visit Funktion witer zu überladen. 

// Definiert alle möglichen Typen, die ein Parameter annehmen kann.
using ParameterValue = std::variant<
    std::monostate, // Leerer Zustand - wird als default verwendet, da ansonsten mit 0 initialisiert wird
    bool, 
    float, 
    double, 
    std::string, 
    int32_t,
    uint32_t,
    int64_t,
    uint64_t
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

//https://www.cppstories.com/2018/06/variant/
// Variant funktions weisen :
//  try with get_if:
//    if (const auto intPtr (     std::get_if<int>(&intFloatString)        ); intPtr)
//      std::cout << "int!" << *intPtr << "\n";


//std::get auf variant returns a reference

//keine unnötigen Heap allocations und String wird sicher destruckted