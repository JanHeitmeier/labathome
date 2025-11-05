#pragma once

#include <vector>
#include <string>

struct ParameterMetadataDto {
    std::string name;
    std::string type;           // "int", "float", "string", "bool"
    std::string description;
    std::string defaultValue;
    std::string minValue;       // Min-Wert als String (optional, leer wenn nicht gesetzt)
    std::string maxValue;       // Max-Wert als String (optional, leer wenn nicht gesetzt)
    bool required;
    std::string unit;           // Einheit (optional)
};

struct IoAliasMetadataDto {
    std::string aliasName;
    std::string ioType;         // "input", "output", oder "sensor"
    std::string valueType;      // "bool", "int", "float"
    std::string description;
};

/**
 * @brief Metadaten eines Step-Typs für den Rezept-Editor (ausgehend: Backend → Frontend)
 */
struct StepMetadataDto {
    std::string typeId;
    std::string displayName;
    std::string description;
    std::string category;
    std::vector<ParameterMetadataDto> parameters;
    std::vector<IoAliasMetadataDto> ioAliases;
};

/**
 * @brief Liste aller verfügbaren Step-Typen (ausgehend: Backend → Frontend)
 */
struct AvailableStepsDto {
    std::vector<StepMetadataDto> steps;
};
