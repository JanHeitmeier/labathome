#pragma once
#include "../../core/interfaces/storage/IRecipeStorage.hh"
#include "../dtos/RecipeDto.hh"
#include <string>
#include <vector>
#include <optional>
#include <cstdint>

// STORAGE MANAGER: JSON + SERIALIZED RECIPES
class RecipeStorageManager {
private:
    IRecipeStorage* m_storage;
    
    static uint32_t calculateIdHash(const std::string& stringId);

public:
    RecipeStorageManager(IRecipeStorage* storage) : m_storage(storage) {}
    
    // HIGH-LEVEL SMART OPERATIONS
    // Lädt komplettes Recipe für Ausführung (nutzt Binary-Cache wenn vorhanden, sonst JSON)
    bool loadRecipeForExecution(uint32_t recipeIdHash, Recipe& outRecipe);
    
    // Speichert Recipe mit automatischer Serialisierung (JSON + Binary-Cache)
    bool saveRecipeWithCache(uint32_t recipeIdHash, const std::string& jsonRecipe, const Recipe& recipe);
    
    // JSON RECIPES
    bool saveJsonRecipe(const std::string& jsonRecipe);
    bool saveJsonRecipeWithStringId(const std::string& jsonRecipe);
    std::optional<std::string> getJsonRecipe(uint32_t id);
    bool updateJsonRecipe(uint32_t id, const std::string& jsonRecipe);
    bool deleteJsonRecipe(uint32_t id);
    std::vector<std::string> getAllJsonRecipes();
    bool existsJsonRecipe(uint32_t id);
    std::vector<uint32_t> getAllJsonRecipeIds();
    size_t getJsonRecipeCount();

    // SERIALIZED RECIPES
    bool saveSerializedRecipe(const std::vector<uint8_t>& serializedRecipe);
    std::optional<std::vector<uint8_t>> getSerializedRecipe(uint32_t id);
    bool updateSerializedRecipe(uint32_t id, const std::vector<uint8_t>& serializedRecipe);
    bool deleteSerializedRecipe(uint32_t id);
    std::vector<std::vector<uint8_t>> getAllSerializedRecipes();
    bool existsSerializedRecipe(uint32_t id);
    std::vector<uint32_t> getAllSerializedRecipeIds();
    // Gibt die Anzahl der gespeicherten serialisierten Rezepte zurück
    size_t getSerializedRecipeCount();

};
