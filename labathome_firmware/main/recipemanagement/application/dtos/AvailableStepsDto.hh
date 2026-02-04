#pragma once

#include <vector>
#include <string>

struct ParameterMetadataDto {
    std::string name;
    std::string type;
    std::string description;
    std::string defaultValue;
    std::string minValue;
    std::string maxValue;
    bool required;
    std::string unit;
    bool isGlobal{false};
};

struct IoAliasMetadataDto {
    std::string aliasName;
    std::string ioType;
    std::string valueType;
    std::string description;
    std::string defaultPhysicalName;
};

// BACKEND → FRONTEND
struct StepMetadataDto {
    std::string typeId;
    std::string displayName;
    std::string description;
    std::string category;
    std::vector<ParameterMetadataDto> parameters;
    std::vector<IoAliasMetadataDto> ioAliases;
};

struct AvailableStepsDto {
    std::vector<StepMetadataDto> steps;
};
