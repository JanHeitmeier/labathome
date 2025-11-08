#ifndef RECIPE_CONTROLLER_HH
#define RECIPE_CONTROLLER_HH

#pragma once 

#include <vector>
#include <string>
#include "../../application/services/RecipeStorageManager.hh"
#include "../../infrastructure/engine/IoResourceManager.hh"
#include "../../infrastructure/engine/RecipeEngine.hh"  
#include <algorithm>

class DeviceManager; // Forward declaration  

class RecipeController {
public:
    // Konstruktor, initialisiert vom DeviceManager <-----!!!!!!! Einstigspunkt in Rezeptsystem
    RecipeController( DeviceManager& deviceManager, RecipeStorageManager& storageManager );
    ~RecipeController();

    // Funktion für Webplugin: Liste der verfügbaren Rezepte holen
    std::vector<std::string> getRecipeList();

    void stopEngine();
    bool startRecipe(const std::string& recipeName);
    bool loadRecipe(const std::string& recipeName);

private:
    RecipeStorageManager& m_storageManager;
    DeviceManager& m_deviceManager; // Referenz für callbacks an HAL

    bool m_engineRunning;
    std::string m_currentRecipe;
};

#endif // RECIPE_CONTROLLER_HH