#pragma once

#include "../../core/domain/entities/Recipe.hh"
#include "../../infrastructure/engine/RecipeEngine.hh"

class RecipeParser {
    // Stellt eine Funktion bereit, die von dem RecipeController genutzt werden kann,
    // um eine über den RecipeStorageManager geholte JSON in ein Recipe-Objekt umzuwandeln.
public:
    RecipeParser();
    ~RecipeParser();
    
    // Parse JSON string to Recipe object
    bool parseJsonToRecipe(const std::string& jsonText, Recipe& outRecipe);
    
    // Parse JSON string directly to StepInstanceDescriptor list (for RecipeEngine)
    bool parseJsonToStepDescriptors(const std::string& jsonText, std::vector<StepInstanceDescriptor>& outSteps);
    
    // Validate JSON structure
    bool validateRecipeJson(const std::string& jsonText);
};
