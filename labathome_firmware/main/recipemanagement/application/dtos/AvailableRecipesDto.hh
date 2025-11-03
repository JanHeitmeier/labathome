#pragma once

#include <vector>
#include <string>

// DTO für eine Liste von Rezept-Informationen.
struct RecipeInfoDto {
    std::string id;
    std::string name;
    std::string description;
};

struct AvailableRecipesDto {
    std::vector<RecipeInfoDto> recipes;
};
