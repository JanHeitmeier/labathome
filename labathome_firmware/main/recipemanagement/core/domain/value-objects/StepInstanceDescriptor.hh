#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>

/**
 * @brief Beschreibt eine Step-Instanz innerhalb eines Rezepts
 * 
 * Enthält alle notwendigen Informationen um einen Step zu instantiieren:
 * - Welcher Step-Typ (typeId)
 * - Mit welchen Parametern (params)
 * - Mit welchen I/O-Aliases (aliases)
 * - Wiederholungen (repeatCount)
 */
struct StepInstanceDescriptor {
    std::string systemId;                                    // Eindeutige ID dieser Step-Instanz
    uint32_t typeId{0};                                       // Step-Typ ID (z.B. 0x0001 für RedLedButtonStep)
    std::unordered_map<std::string, std::string> params;     // Parameter für den Step
    std::unordered_map<std::string, std::string> aliases;    // I/O-Aliase (logischer Name → physischer Name)
    int repeatCount{1};                                       // Anzahl Wiederholungen
};
