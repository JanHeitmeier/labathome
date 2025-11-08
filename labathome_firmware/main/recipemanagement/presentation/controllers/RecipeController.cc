
#include "RecipeController.hh"
#include "../../infrastructure/parsers/RecipeParser.hh"
#include "../../core/domain/entities/Recipe.hh"

RecipeController::RecipeController(DeviceManager& deviceManager, RecipeStorageManager& storageManager)
    : m_storageManager(storageManager), m_deviceManager(deviceManager), m_engineRunning(false) {
    // WICHTIG: IoResourceManager und StepTypeRegistry werden NICHT hier initialisiert!
    // Diese Initialisierungen müssen im DeviceManager::init() oder main.cc erfolgen,
    // BEVOR dieser Controller erstellt wird:
    //   - IoResourceManager::instance().init(hal)
    //   - StepTypeRegistry::instance().init()
    
    // Weitere Initialisierung, z. B. RecipeEngine starten oder Callbacks setzen
    // Beispiel: m_engine = std::make_unique<RecipeEngine>(...);
}

RecipeController::~RecipeController() {
    stopEngine();
    // Cleanup, z. B. Callbacks entfernen
}

std::vector<std::string> RecipeController::getRecipeList() {
    // Get all JSON recipes and extract their names
    std::vector<std::string> recipeNames;
    auto jsonRecipes = m_storageManager.getAllJsonRecipes();
    RecipeParser parser;
    
    for (const auto& jsonRecipe : jsonRecipes) {
        Recipe recipe;
        if (parser.parseJsonToRecipe(jsonRecipe, recipe)) {
            recipeNames.push_back(recipe.name());
        }
    }
    
    return recipeNames;
}

void RecipeController::stopEngine() {
    if (m_engineRunning) {
        // Stoppe RecipeEngine (angenommen, es gibt eine Instanz)
        // Beispiel: m_engine->stop();
        m_engineRunning = false;
        m_currentRecipe.clear();
    }
}

bool RecipeController::startRecipe(const std::string& recipeName) {
    if (m_engineRunning) {
        return false;  // Bereits läuft
    }
    // Lade und starte Rezept
    if (loadRecipe(recipeName)) {
        // Beispiel: m_engine->start(recipeName);
        m_engineRunning = true;
        return true;
    }
    return false;
}

bool RecipeController::loadRecipe(const std::string& recipeName) {
    // Find recipe by name in JSON recipes
    auto jsonRecipes = m_storageManager.getAllJsonRecipes();
    RecipeParser parser;
    
    for (const auto& jsonRecipe : jsonRecipes) {
        Recipe recipe;
        if (parser.parseJsonToRecipe(jsonRecipe, recipe)) {
            if (recipe.name() == recipeName) {
                m_currentRecipe = recipeName;
                // TODO: Load recipe into engine when engine is implemented
                return true;
            }
        }
    }
    
    return false;
}