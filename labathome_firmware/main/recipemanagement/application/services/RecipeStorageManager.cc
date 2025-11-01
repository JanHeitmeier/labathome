#include "RecipeStorageManager.hh"

// CRUD for JSON recipes

bool RecipeStorageManager::saveJsonRecipe(const std::string& jsonRecipe) {
    // TODO: Implement
    return false;
}

std::optional<std::string> RecipeStorageManager::getJsonRecipe(uint32_t id) {
    // TODO: Implement
    return std::nullopt;
}

bool RecipeStorageManager::updateJsonRecipe(uint32_t id, const std::string& jsonRecipe) {
    // TODO: Implement
    return false;
}

bool RecipeStorageManager::deleteJsonRecipe(uint32_t id) {
    // TODO: Implement
    return false;
}

std::vector<std::string> RecipeStorageManager::getAllJsonRecipes() {
    // TODO: Implement
    return {};
}

bool RecipeStorageManager::existsJsonRecipe(uint32_t id) {
    // TODO: Implement
    return false;
}

std::vector<uint32_t> RecipeStorageManager::getAllJsonRecipeIds() {
    // TODO: Implement
    return {};
}

size_t RecipeStorageManager::getJsonRecipeCount() {
    // TODO: Implement
    return 0;
}

// CRUD for serialized recipes

bool RecipeStorageManager::saveSerializedRecipe(const std::vector<uint8_t>& serializedRecipe) {
    // TODO: Implement
    return false;
}

std::optional<std::vector<uint8_t>> RecipeStorageManager::getSerializedRecipe(uint32_t id) {
    // TODO: Implement
    return std::nullopt;
}

bool RecipeStorageManager::updateSerializedRecipe(uint32_t id, const std::vector<uint8_t>& serializedRecipe) {
    // TODO: Implement
    return false;
}

bool RecipeStorageManager::deleteSerializedRecipe(uint32_t id) {
    // TODO: Implement
    return false;
}

std::vector<std::vector<uint8_t>> RecipeStorageManager::getAllSerializedRecipes() {
    // TODO: Implement
    return {};
}

bool RecipeStorageManager::existsSerializedRecipe(uint32_t id) {
    // TODO: Implement
    return false;
}

std::vector<uint32_t> RecipeStorageManager::getAllSerializedRecipeIds() {
    // TODO: Implement
    return {};
}

size_t RecipeStorageManager::getSerializedRecipeCount() {
    // TODO: Implement
    return 0;
}
