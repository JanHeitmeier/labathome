#include "LittleFSRecipeStorage.hh"

// Implementierung für LittleFSRecipeStorage.
LittleFSRecipeStorage::LittleFSRecipeStorage() {
    // Initialisiere LittleFS, falls noch nicht geschehen.
}

bool LittleFSRecipeStorage::serialize(const Recipe& r, std::vector<uint8_t>& outBlob) { return false; }
bool LittleFSRecipeStorage::deserialize(const std::vector<uint8_t>& blob, Recipe& outRecipe) { return false; }
bool LittleFSRecipeStorage::save(uint32_t id, const std::vector<uint8_t>& data) { return false; }
std::optional<std::vector<uint8_t>> LittleFSRecipeStorage::get(uint32_t id) { return std::nullopt; }
bool LittleFSRecipeStorage::update(uint32_t id, const std::vector<uint8_t>& data) { return false; }
bool LittleFSRecipeStorage::remove(uint32_t id) { return false; }
bool LittleFSRecipeStorage::exists(uint32_t id) { return false; }
std::vector<uint32_t> LittleFSRecipeStorage::listIds() { return {}; }
