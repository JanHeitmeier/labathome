#pragma once

#include <string>

/**
 * @brief DTO für Befehle von der Web-UI (eingehend: Frontend → Backend)
 * 
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
struct CommandDto {
    std::string command;
    std::string recipeId;       // Rezept-ID (für start, delete, get)
    std::string payload;        // Zusätzliche Daten als JSON (für save_recipe)
    std::string requestId;      // Für Request/Response-Matching
};
