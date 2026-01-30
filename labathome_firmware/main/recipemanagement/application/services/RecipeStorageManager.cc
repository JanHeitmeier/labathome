#include "RecipeStorageManager.hh"
#include "../../infrastructure/serialization/JsonSerialization.hh"
#include "../../infrastructure/serialization/RecipeSerializer.hh"
#include "../../infrastructure/parsers/RecipeParser.hh"
#include <algorithm>
#include <esp_log.h>
#include <cinttypes>

uint32_t RecipeStorageManager::calculateIdHash(const std::string& stringId) {
    uint32_t hash = 0;
    for (char c : stringId) {
        hash = hash * 31 + static_cast<uint32_t>(c);
    }
    return hash;
}

// HIGH-LEVEL SMART OPERATIONS

bool RecipeStorageManager::loadRecipeForExecution(uint32_t recipeIdHash, Recipe& outRecipe) {
    if (!m_storage) return false;
    
    // 1. Versuche serialisierte Version zu laden (SCHNELL - kein Parsing nötig)
    auto serializedOpt = getSerializedRecipe(recipeIdHash);
    if (serializedOpt.has_value()) {
        if (RecipeSerializer::deserialize(serializedOpt.value(), outRecipe)) {
            ESP_LOGI("RecipeStorageManager", "Loaded recipe '%s' (0x%08" PRIx32 ") from binary cache", 
                     outRecipe.id().c_str(), recipeIdHash);
            return true;
        }
        ESP_LOGW("RecipeStorageManager", "Binary cache corrupted for 0x%08" PRIx32 ", falling back to JSON", recipeIdHash);
    }
    
    // 2. Fallback: Lade und parse JSON (LANGSAM)
    auto jsonOpt = getJsonRecipe(recipeIdHash);
    if (!jsonOpt.has_value()) {
        ESP_LOGE("RecipeStorageManager", "Recipe 0x%08" PRIx32 " not found", recipeIdHash);
        return false;
    }
    
    RecipeParser parser;
    if (!parser.parseJsonToRecipe(jsonOpt.value(), outRecipe)) {
        ESP_LOGE("RecipeStorageManager", "Failed to parse JSON for recipe 0x%08" PRIx32, recipeIdHash);
        return false;
    }
    
    // 3. Erstelle Binary-Cache für nächstes Mal
    auto serialized = RecipeSerializer::serialize(outRecipe);
    if (updateSerializedRecipe(recipeIdHash, serialized)) {
        ESP_LOGI("RecipeStorageManager", "Created binary cache for recipe '%s' (0x%08" PRIx32 ")", 
                 outRecipe.id().c_str(), recipeIdHash);
    }
    
    return true;
}

bool RecipeStorageManager::saveRecipeWithCache(uint32_t recipeIdHash, const std::string& jsonRecipe, const Recipe& recipe) {
    if (!m_storage) return false;
    
    // 1. Speichere JSON (für Editor, Export, etc.)
    if (!updateJsonRecipe(recipeIdHash, jsonRecipe)) {
        ESP_LOGE("RecipeStorageManager", "Failed to save JSON for recipe '%s' (0x%08" PRIx32 ")", 
                 recipe.id().c_str(), recipeIdHash);
        return false;
    }
    
    // 2. Erstelle und speichere Binary-Cache (komplettes Recipe-Objekt)
    auto serialized = RecipeSerializer::serialize(recipe);
    if (!updateSerializedRecipe(recipeIdHash, serialized)) {
        ESP_LOGW("RecipeStorageManager", "Failed to save binary cache for recipe '%s' (0x%08" PRIx32 ")", 
                 recipe.id().c_str(), recipeIdHash);
        // JSON ist gespeichert, also kein kompletter Fehler
    }
    
    ESP_LOGI("RecipeStorageManager", "Saved recipe '%s' (0x%08" PRIx32 ") with JSON + binary cache", 
             recipe.id().c_str(), recipeIdHash);
    return true;
}

// JSON RECIPES

bool RecipeStorageManager::saveJsonRecipe(const std::string& jsonRecipe) {
    if (!m_storage) return false;
    auto ids = m_storage->listIds();
    uint32_t newId = ids.empty() ? 1 : (*std::max_element(ids.begin(), ids.end()) + 1);
    return m_storage->saveJson(newId, jsonRecipe);
}

bool RecipeStorageManager::saveJsonRecipeWithStringId(const std::string& jsonRecipe) {
    if (!m_storage) return false;
    
    // Extrahiere String-ID aus JSON
    RecipeDto dto;
    if (!JsonSerialization::deserialize(jsonRecipe, dto)) {
        ESP_LOGE("RecipeStorageManager", "Failed to deserialize recipe JSON to extract ID");
        return false;
    }
    
    if (dto.id.empty()) {
        ESP_LOGE("RecipeStorageManager", "Recipe has empty ID, cannot save");
        return false;
    }
    
    // Berechne Hash der String-ID
    uint32_t idHash = calculateIdHash(dto.id);
    ESP_LOGI("RecipeStorageManager", "Saving recipe with String-ID '%s' as Hash 0x%08" PRIx32, dto.id.c_str(), idHash);
    
    // Speichere mit Hash als Storage-ID
    return m_storage->saveJson(idHash, jsonRecipe);
}

std::optional<std::string> RecipeStorageManager::getJsonRecipe(uint32_t id) {
    if (!m_storage) return std::nullopt;
    return m_storage->getJson(id);
}

bool RecipeStorageManager::updateJsonRecipe(uint32_t id, const std::string& jsonRecipe) {
    if (!m_storage) return false;
    return m_storage->saveJson(id, jsonRecipe);
}

bool RecipeStorageManager::deleteJsonRecipe(uint32_t id) {
    if (!m_storage) return false;
    return m_storage->remove(id);
}

std::vector<std::string> RecipeStorageManager::getAllJsonRecipes() {
    std::vector<std::string> recipes;
    if (!m_storage) return recipes;
    auto ids = m_storage->listIds();
    for (auto id : ids) {
        auto json = m_storage->getJson(id);
        if (json) recipes.push_back(*json);
    }
    return recipes;
}

bool RecipeStorageManager::existsJsonRecipe(uint32_t id) {
    if (!m_storage) return false;
    return m_storage->exists(id);
}

std::vector<uint32_t> RecipeStorageManager::getAllJsonRecipeIds() {
    if (!m_storage) return {};
    return m_storage->listIds();
}

size_t RecipeStorageManager::getJsonRecipeCount() {
    if (!m_storage) return 0;
    return m_storage->listIds().size();
}

// CRUD for serialized recipes

bool RecipeStorageManager::saveSerializedRecipe(const std::vector<uint8_t>& serializedRecipe) {
    if (!m_storage) return false;
    auto ids = m_storage->listIds();
    uint32_t newId = ids.empty() ? 1 : (*std::max_element(ids.begin(), ids.end()) + 1);
    return m_storage->save(newId, serializedRecipe);
}

std::optional<std::vector<uint8_t>> RecipeStorageManager::getSerializedRecipe(uint32_t id) {
    if (!m_storage) return std::nullopt;
    return m_storage->get(id);
}

bool RecipeStorageManager::updateSerializedRecipe(uint32_t id, const std::vector<uint8_t>& serializedRecipe) {
    if (!m_storage) return false;
    return m_storage->update(id, serializedRecipe);
}

bool RecipeStorageManager::deleteSerializedRecipe(uint32_t id) {
    if (!m_storage) return false;
    return m_storage->remove(id);
}

std::vector<std::vector<uint8_t>> RecipeStorageManager::getAllSerializedRecipes() {
    std::vector<std::vector<uint8_t>> recipes;
    if (!m_storage) return recipes;
    auto ids = m_storage->listIds();
    for (auto id : ids) {
        auto data = m_storage->get(id);
        if (data) recipes.push_back(*data);
    }
    return recipes;
}

bool RecipeStorageManager::existsSerializedRecipe(uint32_t id) {
    if (!m_storage) return false;
    return m_storage->exists(id);
}

std::vector<uint32_t> RecipeStorageManager::getAllSerializedRecipeIds() {
    if (!m_storage) return {};
    return m_storage->listIds();
}

size_t RecipeStorageManager::getSerializedRecipeCount() {
    if (!m_storage) return 0;
    return m_storage->listIds().size();
}
