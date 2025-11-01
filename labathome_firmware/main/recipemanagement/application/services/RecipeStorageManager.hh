#pragma once
#include "../entities/Recipe.hh"
#include "../interfaces/services/IRecipeStorage.hh"
#include <string>
#include <memory>
#include <vector>
#include <optional>
#include <mutex>
#include <cstdint>
#include <ctime>

// Diese Klasse verwaltet die Dateien und bietet CRUD-Operationen für Rezepte.
// Sie verwaltet zwei verschiedene Arten von rezepten. 
//1. Rezepte in Json format (dies ist für export und Import gedacht, sowie für die Anzeige im browser)
//2. Rezepte in serializierter Form (für die interne Speicherung und das schnelle Laden in die RecipeEngine, WICHTIG = Vermeiden von häufigem Parsen/Kompilieren)
// Die tatsächliche Speicherung wird durch die Implementation von IRecipeStorage erledigt. (Anbindung an verschiedene Speicherarten möglich)
class RecipeStorageManager {

public:
    // CRUD for JSON recipes

    // Speichert ein neues JSON-Rezept und weist automatisch eine ID zu
    bool saveJsonRecipe(const std::string& jsonRecipe);
    // Lädt ein JSON-Rezept anhand seiner ID
    std::optional<std::string> getJsonRecipe(uint32_t id);
    // Aktualisiert ein bestehendes JSON-Rezept mit der gegebenen ID
    bool updateJsonRecipe(uint32_t id, const std::string& jsonRecipe);
    // Löscht ein JSON-Rezept anhand seiner ID
    bool deleteJsonRecipe(uint32_t id);
    // Lädt alle gespeicherten JSON-Rezepte
    std::vector<std::string> getAllJsonRecipes();
    // Prüft, ob ein JSON-Rezept mit der gegebenen ID existiert
    bool existsJsonRecipe(uint32_t id);
    // Gibt alle IDs der gespeicherten JSON-Rezepte zurück
    std::vector<uint32_t> getAllJsonRecipeIds();
    // Gibt die Anzahl der gespeicherten JSON-Rezepte zurück
    size_t getJsonRecipeCount();

    // CRUD for serialized recipes

    // Speichert ein neues serialisiertes Rezept und weist automatisch eine ID zu
    bool saveSerializedRecipe(const std::vector<uint8_t>& serializedRecipe);
    // Lädt ein serialisiertes Rezept anhand seiner ID
    std::optional<std::vector<uint8_t>> getSerializedRecipe(uint32_t id);
    // Aktualisiert ein bestehendes serialisiertes Rezept mit der gegebenen ID
    bool updateSerializedRecipe(uint32_t id, const std::vector<uint8_t>& serializedRecipe);
    // Löscht ein serialisiertes Rezept anhand seiner ID
    bool deleteSerializedRecipe(uint32_t id);
    // Lädt alle gespeicherten serialisierten Rezepte
    std::vector<std::vector<uint8_t>> getAllSerializedRecipes();
    // Prüft, ob ein serialisiertes Rezept mit der gegebenen ID existiert
    bool existsSerializedRecipe(uint32_t id);
    // Gibt alle IDs der gespeicherten serialisierten Rezepte zurück
    std::vector<uint32_t> getAllSerializedRecipeIds();
    // Gibt die Anzahl der gespeicherten serialisierten Rezepte zurück
    size_t getSerializedRecipeCount();

};
