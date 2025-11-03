#pragma once

#include <string>

// DTO für einen Befehl, der von der Web-UI kommt.
struct CommandDto {
    std::string command; // z.B. "start_recipe", "stop_recipe"
    std::string recipeId;
    // Weitere Parameter...
};
