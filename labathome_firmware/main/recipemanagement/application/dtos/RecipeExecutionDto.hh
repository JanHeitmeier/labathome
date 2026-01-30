#pragma once
#include <string>
#include <cstdint>

struct RecipeExecutionDto {
    std::string executionId;
    std::string recipeId;
    std::string recipeName;
    uint64_t startTime;
    uint64_t endTime;
    uint64_t duration;
    std::string status;
    std::string errorMessage;
};
