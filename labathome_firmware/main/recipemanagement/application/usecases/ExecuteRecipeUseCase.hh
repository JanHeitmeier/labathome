#pragma once

#include <string>

// Use Case zum Starten eines Rezepts.
// Orchestriert das Laden des Rezepts und den Start der RecipeEngine.
class ExecuteRecipeUseCase {
public:
    ExecuteRecipeUseCase(/* Abhängigkeiten wie IRecipeStorage, RecipeEngine etc. hier injecten */);
    void execute(const std::string& recipeName);
};
