#pragma once

#include <vector>
#include <string>
#include <map>

struct StepConfigDto {
    std::string stepTypeId;
    std::map<std::string, std::string> parameters;
    int order;
};

/**
 * @brief Komplettes Rezept (bidirektional: Frontend ↔ Backend)
 * 
 * Verwendet für: save_recipe, get_recipe
 */
struct RecipeDto {
    std::string id;
    std::string name;
    std::string description;
    std::vector<StepConfigDto> steps;
    std::string author;
    std::string version;
};
