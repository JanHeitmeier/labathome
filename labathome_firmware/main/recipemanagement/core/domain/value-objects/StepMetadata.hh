#pragma once

#include <vector>
#include <memory>
#include <optional>
#include <cstdint>
#include <string_view>
#include <string>
#include "IValue.hh"

struct ParamDef {
    std::string_view key;
    std::unique_ptr<IValue> value;
    std::optional<std::unique_ptr<IValue>> minValue;
    std::optional<std::unique_ptr<IValue>> maxValue;
    std::string_view label;
    std::string_view description;

    explicit ParamDef(std::string_view key_,
                      std::unique_ptr<IValue> value_,
                      std::string_view label_,
                      std::string_view description_,
                      std::optional<std::unique_ptr<IValue>> minValue_ = std::nullopt,
                      std::optional<std::unique_ptr<IValue>> maxValue_ = std::nullopt)
        : key(key_), value(std::move(value_)), 
          minValue(std::move(minValue_)),
          maxValue(std::move(maxValue_)), 
          label(label_), description(description_) {}

    ParamDef(const ParamDef& o)
        : key(o.key),
          value(o.value ? o.value->clone() : nullptr),
          minValue(o.minValue && *o.minValue ? std::optional((*o.minValue)->clone()) : std::nullopt),
          maxValue(o.maxValue && *o.maxValue ? std::optional((*o.maxValue)->clone()) : std::nullopt),
          label(o.label),
          description(o.description) {}

    ParamDef& operator=(const ParamDef& o) {
        if (this != &o) {
            key = o.key;
            value = o.value ? o.value->clone() : nullptr;
            minValue = o.minValue && *o.minValue ? std::optional((*o.minValue)->clone()) : std::nullopt;
            maxValue = o.maxValue && *o.maxValue ? std::optional((*o.maxValue)->clone()) : std::nullopt;
            label = o.label;
            description = o.description;
        }
        return *this;
    }

    ParamDef(ParamDef&&) = default;
    ParamDef& operator=(ParamDef&&) = default;
    ParamDef() = delete;
};

struct IoAliasDef {
    std::string_view aliasName;
    bool isInput;
    bool isOutput;
    bool isSensor;
    std::optional<std::unique_ptr<IValue>> exampleValue;

    explicit IoAliasDef(std::string_view aliasName_,
                        bool isInput_,
                        bool isOutput_,
                        bool isSensor_,
                        std::optional<std::unique_ptr<IValue>> exampleValue_ = std::nullopt)
        : aliasName(aliasName_), 
          isInput(isInput_), 
          isOutput(isOutput_), 
          isSensor(isSensor_),
          exampleValue(std::move(exampleValue_)) {}

    IoAliasDef(const IoAliasDef& o)
        : aliasName(o.aliasName),
          isInput(o.isInput),
          isOutput(o.isOutput),
          isSensor(o.isSensor),
          exampleValue(o.exampleValue && *o.exampleValue ? std::optional((*o.exampleValue)->clone()) : std::nullopt) {}

    IoAliasDef& operator=(const IoAliasDef& o) {
        if (this != &o) {
            aliasName = o.aliasName;
            isInput = o.isInput;
            isOutput = o.isOutput;
            isSensor = o.isSensor;
            exampleValue = o.exampleValue && *o.exampleValue ? std::optional((*o.exampleValue)->clone()) : std::nullopt;
        }
        return *this;
    }

    IoAliasDef(IoAliasDef&&) = default;
    IoAliasDef& operator=(IoAliasDef&&) = default;
    IoAliasDef() = delete;
};

struct StepMetadata {
    std::uint32_t typeId{0};
    std::string_view displayName{};
    std::string_view description{};
    std::string_view version{};
    std::vector<ParamDef> params;
    std::vector<IoAliasDef> ioAliases;
};
