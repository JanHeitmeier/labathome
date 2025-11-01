#ifndef RECIPE_CONTROLLER_HH
#define RECIPE_CONTROLLER_HH

#pragma once 

#include <vector>
#include <string>
#include "../storageservice/RecipeStorageManager.hh"
#include "../../devicemanager.hh"
#include "./RecipeController.hh"
#include "../engine/IoResourceManager.hh"
#include "RecipeController.hh"
#include "../engine/IoResourceManager.hh"
#include "../engine/RecipeEngine.hh"  
#include <algorithm>  

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