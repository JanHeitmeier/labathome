#pragma once

#include <string>

// DTO für Live-Informationen während der Rezeptausführung.
struct LiveViewDto {
    std::string recipeId;
    int stepIndex;
    std::string stepState;
    std::string instruction;
};
