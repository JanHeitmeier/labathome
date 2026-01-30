#pragma once

#include <vector>
#include <string>
#include <map>
#include <cstdint>

struct StepConfigDto {
    std::string stepTypeId;
    std::map<std::string, std::string> parameters;
    std::map<std::string, std::string> aliases;
    int order;
};

// FRONTEND ↔ BACKEND
struct RecipeDto {
    std::string id;
    std::string name;
    std::string description;
    std::vector<StepConfigDto> steps;
    std::string author;
    std::string version;
    uint64_t createdAt{0};
    uint64_t lastModified{0};
};
