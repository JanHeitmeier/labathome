#pragma once

#include <vector>
#include <string>
#include <cstdint>

struct RecipeInfoDto {
    std::string id;
    std::string name;
    std::string description;
    std::string version;
    uint64_t createdAt;
    uint64_t lastModified;
};

// BACKEND → FRONTEND
struct AvailableRecipesDto {
    std::vector<RecipeInfoDto> recipes;
};

using RecipeListDto = AvailableRecipesDto;
