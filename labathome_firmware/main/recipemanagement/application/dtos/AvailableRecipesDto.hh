#pragma once

#include <vector>
#include <string>
#include <cstdint>

struct RecipeInfoDto {
    std::string id;
    std::string name;
    std::string description;
    uint64_t createdAt;
    uint64_t lastModified;
};

/**
 * @brief Liste von Rezepten für das Dashboard (ausgehend: Backend → Frontend)
 */
struct AvailableRecipesDto {
    std::vector<RecipeInfoDto> recipes;
};

using RecipeListDto = AvailableRecipesDto;
