#pragma once

#include <string>

// FRONTEND → BACKEND
struct CommandDto {
    std::string command;
    std::string recipeId;
    std::string payload;
    std::string requestId;
};
/*
* Unterstützte Befehle:
 * - "start_recipe": Startet ein Rezept
 * - "stop_recipe": Stoppt das laufende Rezept
 * - "pause_recipe": Pausiert das laufende Rezept
 * - "resume_recipe": Setzt pausiertes Rezept fort
 * - "acknowledge_step": Benutzer quittiert Anweisung/wartet auf Fortsetzung
 * - "get_recipe_list": Fordert Liste aller Rezepte an
 * - "get_available_steps": Fordert Liste aller Step-Typen an
 * - "save_recipe": Speichert ein Rezept (payload enthält RecipeDto)
 * - "delete_recipe": Löscht ein Rezept
 * - "get_recipe": Lädt ein spezifisches Rezept
 */