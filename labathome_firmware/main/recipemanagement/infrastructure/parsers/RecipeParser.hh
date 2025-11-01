#pragma once

#include "./../entities/Recipe.hh"

class RecipeParser {
    //stellt eine funktion bereit die von dem RecipeController genutzt werden kann,
    // um eine über den RecipeStorageMAnager geholte json in ein Recipe Objekt umzuwandeln.
public:
    RecipeParser();
    ~RecipeParser();
    bool parseJsonToRecipe(const std::string& jsonText, Recipe& outRecipe);
    bool validateRecipeJson(const std::string& jsonText);
};
