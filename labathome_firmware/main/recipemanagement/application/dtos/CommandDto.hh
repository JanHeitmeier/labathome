#pragma once

#include <string>

// FRONTEND → BACKEND
struct CommandDto {
    std::string command;
    std::string recipeId;
    std::string executionId;  // For get_timeseries, delete_execution commands
    std::string payload;
    std::string requestId;
    std::string sessionToken;  // Session token from login
    std::string pin;           // Only for "login" command
    std::string loginRole;     // Role for login (Admin, RecipeEditor, RecipeStarter, Observer)
};
/*
* Unterstützte Befehle:
 * - "login": Login mit PIN (pin field), returns sessionToken
 * - "logout": Logout (invalidates sessionToken)
 * - "change_pin": Ändert PIN einer Rolle (Admin only, payload: role,oldPin,newPin)
 * - "start_recipe": Startet ein Rezept (needs RecipeStarter+)
 * - "stop_recipe": Stoppt das laufende Rezept
 * - "pause_recipe": Pausiert das laufende Rezept
 * - "resume_recipe": Setzt pausiertes Rezept fort
 * - "acknowledge_step": Benutzer quittiert Anweisung
 * - "get_recipe_list": Fordert Liste aller Rezepte an (Observer+)
 * - "get_available_steps": Fordert Liste aller Step-Typen an (Observer+)
 * - "get_liveview": Fordert LiveView an (Observer+)
 * - "save_recipe": Speichert ein Rezept (RecipeEditor+)
 * - "delete_recipe": Löscht ein Rezept (RecipeEditor+)
 * - "get_recipe": Lädt ein spezifisches Rezept (Observer+)
 */