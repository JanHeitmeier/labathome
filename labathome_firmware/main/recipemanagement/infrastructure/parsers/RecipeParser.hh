#pragma once

#include "../../core/domain/entities/Recipe.hh"
#include "../../infrastructure/engine/RecipeEngine.hh"

class RecipeParser {

public:
    RecipeParser();
    ~RecipeParser();
    
    bool parseJsonToRecipe(const std::string& jsonText, Recipe& outRecipe);
    bool parseJsonToStepDescriptors(const std::string& jsonText, std::vector<StepInstanceDescriptor>& outSteps);
};
