
#include "RecipeController.hh"


RecipeController::RecipeController(DeviceManager& deviceManager, RecipeStorageManager& storageManager)
    : m_deviceManager(deviceManager), m_storageManager(storageManager), m_engineRunning(false) {
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
    // Hole Liste der verfügbaren Rezepte aus StorageManager
    return m_storageManager.getRecipeList();  // Angenommen, Methode existiert – passe an
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
    // Lade Rezept aus StorageManager
    auto recipe = m_storageManager.loadRecipe(recipeName);  // Angenommen, Methode existiert – passe an
    if (recipe) {
        m_currentRecipe = recipeName;
        // Beispiel: m_engine->load(recipe);
        return true;
    }
    return false;
}