#pragma once

#include <vector>
#include <optional>
#include <cstdint>
#include <string_view>
#include <string>
#include "ParameterValue.hh"

struct ParamDef {
    std::string key;
    ParameterValue value;
    std::optional<ParameterValue> minValue;
    std::optional<ParameterValue> maxValue;
    std::string label;
    std::string description;
    std::string unit; // z.B. "°C", "RPM", "ms"
    bool isGlobal{false}; // Rezeptweiter Parameter (z.B. Batch-Größe)

    // Konstruktor für einfache Initialisierung (akzeptiert string_view für Flexibilität)
    explicit ParamDef(std::string_view key,
                      ParameterValue value,
                      std::string_view label,
                      std::string_view description,
                      std::string unit = "",
                      std::optional<ParameterValue> minValue = std::nullopt,
                      std::optional<ParameterValue> maxValue = std::nullopt,
                      bool isGlobal = false)
        : key(key), 
          value(std::move(value)), 
          minValue(std::move(minValue)),
          maxValue(std::move(maxValue)), 
          label(label), 
          description(description),
          unit(std::move(unit)),
          isGlobal(isGlobal) {}

    // Standard-Kopier- und Move-Operationen sind ausreichend
    ParamDef(const ParamDef&) = default;
    ParamDef& operator=(const ParamDef&) = default;
    ParamDef(ParamDef&&) = default;
    ParamDef& operator=(ParamDef&&) = default;
    ParamDef() = delete;
};

struct IoAliasDef {
    std::string aliasName;
    bool isInput;
    bool isOutput;
    bool isSensor;
    std::string valueType; // z.B. "bool", "float"
    std::optional<ParameterValue> exampleValue;
    std::string physicalName; // Physical resource name (set at runtime from recipe)
    std::string unit; // Unit for sensor values (e.g. "°C", "%", "RPM")

    explicit IoAliasDef(std::string_view aliasName,
                        bool isInput,
                        bool isOutput,
                        bool isSensor,
                        std::string valueType = "",
                        std::optional<ParameterValue> exampleValue = std::nullopt,
                        std::string physicalName = "",
                        std::string unit = "")
        : aliasName(aliasName), 
          isInput(isInput), 
          isOutput(isOutput), 
          isSensor(isSensor),
          valueType(std::move(valueType)),
          exampleValue(std::move(exampleValue)),
          physicalName(std::move(physicalName)),
          unit(std::move(unit)) {}

    
    IoAliasDef(const IoAliasDef&) = default;
    IoAliasDef& operator=(const IoAliasDef&) = default;
    IoAliasDef(IoAliasDef&&) = default;
    IoAliasDef& operator=(IoAliasDef&&) = default;
    IoAliasDef() = delete;
};

struct StepMetadata {
    std::uint32_t typeId{0};
    std::string displayName{};
    std::string description{};
    std::string version{};
    std::vector<ParamDef> params;
    std::vector<IoAliasDef> ioAliases;
};
