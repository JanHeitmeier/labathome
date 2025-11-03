#pragma once

#include <vector>
#include <string>

// Use Case zum Abrufen der Liste aller verfügbaren Rezepte.
// Holt die Daten vom Storage und konvertiert sie in ein DTO.
class GetRecipeListUseCase {
public:
    GetRecipeListUseCase(/* Abhängigkeiten wie IRecipeStorage hier injecten */);
    // AvailableRecipesDto execute();
};
