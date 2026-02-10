#include "StorageManager.hh"
#include "../../infrastructure/serialization/JsonSerialization.hh"
#include "../../infrastructure/serialization/RecipeSerializer.hh"
#include "../../infrastructure/serialization/TimeSeriesSerializer.hh"
#include "../../infrastructure/parsers/RecipeParser.hh"
#include <algorithm>
#include <numeric>
#include <esp_log.h>
#include <cinttypes>

StorageManager::~StorageManager() {
    if (m_recording) {
        stopRecording();
    }
}

uint32_t StorageManager::calculateIdHash(const std::string& stringId) {
    uint32_t hash = 0;
    for (char c : stringId) {
        hash = hash * 31 + static_cast<uint32_t>(c);
    }
    return hash;
}

// ========== RECIPE OPERATIONS ==========

bool StorageManager::loadRecipeForExecution(uint32_t recipeIdHash, Recipe& outRecipe) {
    if (!m_recipeStorage) return false;
    
    // 1. Versuche serialisierte Version zu laden (SCHNELL - kein Parsing nötig)
    auto serializedOpt = getSerializedRecipe(recipeIdHash);
    if (serializedOpt.has_value()) {
        if (RecipeSerializer::deserialize(serializedOpt.value(), outRecipe)) {
            ESP_LOGI("StorageManager", "Loaded recipe '%s' (0x%08" PRIx32 ") from binary cache", 
                     outRecipe.id().c_str(), recipeIdHash);
            return true;
        }
        ESP_LOGW("StorageManager", "Binary cache corrupted for 0x%08" PRIx32 ", falling back to JSON", recipeIdHash);
    }
    
    // 2. Fallback: Lade und parse JSON (LANGSAM)
    auto jsonOpt = getJsonRecipe(recipeIdHash);
    if (!jsonOpt.has_value()) {
        ESP_LOGE("StorageManager", "Recipe 0x%08" PRIx32 " not found", recipeIdHash);
        return false;
    }
    
    RecipeParser parser;
    if (!parser.parseJsonToRecipe(jsonOpt.value(), outRecipe)) {
        ESP_LOGE("StorageManager", "Failed to parse JSON for recipe 0x%08" PRIx32, recipeIdHash);
        return false;
    }
    
    // 3. Erstelle Binary-Cache für nächstes Mal
    auto serialized = RecipeSerializer::serialize(outRecipe);
    if (updateSerializedRecipe(recipeIdHash, serialized)) {
        ESP_LOGI("StorageManager", "Created binary cache for recipe '%s' (0x%08" PRIx32 ")", 
                 outRecipe.id().c_str(), recipeIdHash);
    }
    
    return true;
}

bool StorageManager::saveRecipeWithCache(uint32_t recipeIdHash, const std::string& jsonRecipe, const Recipe& recipe) {
    if (!m_recipeStorage) return false;
    
    // 1. Speichere JSON (für Editor, Export, etc.)
    if (!updateJsonRecipe(recipeIdHash, jsonRecipe)) {
        ESP_LOGE("StorageManager", "Failed to save JSON for recipe '%s' (0x%08" PRIx32 ")", 
                 recipe.id().c_str(), recipeIdHash);
        return false;
    }
    
    // 2. Erstelle und speichere Binary-Cache (komplettes Recipe-Objekt)
    auto serialized = RecipeSerializer::serialize(recipe);
    if (!updateSerializedRecipe(recipeIdHash, serialized)) {
        ESP_LOGW("StorageManager", "Failed to save binary cache for recipe '%s' (0x%08" PRIx32 ")", 
                 recipe.id().c_str(), recipeIdHash);
        // JSON ist gespeichert, also kein kompletter Fehler
    }
    
    ESP_LOGI("StorageManager", "Saved recipe '%s' (0x%08" PRIx32 ") with JSON + binary cache", 
             recipe.id().c_str(), recipeIdHash);
    return true;
}

// JSON RECIPES

bool StorageManager::saveJsonRecipe(const std::string& jsonRecipe) {
    if (!m_recipeStorage) return false;
    auto ids = m_recipeStorage->listIds();
    uint32_t newId = ids.empty() ? 1 : (*std::max_element(ids.begin(), ids.end()) + 1);
    return m_recipeStorage->saveJson(newId, jsonRecipe);
}

bool StorageManager::saveJsonRecipeWithStringId(const std::string& jsonRecipe) {
    if (!m_recipeStorage) return false;
    
    // Extrahiere String-ID aus JSON
    RecipeDto dto;
    if (!JsonSerialization::deserialize(jsonRecipe, dto)) {
        ESP_LOGE("StorageManager", "Failed to deserialize recipe JSON to extract ID");
        return false;
    }
    
    if (dto.id.empty()) {
        ESP_LOGE("StorageManager", "Recipe has empty ID, cannot save");
        return false;
    }
    
    // Berechne Hash der String-ID
    uint32_t idHash = calculateIdHash(dto.id);
    ESP_LOGI("StorageManager", "Saving recipe with String-ID '%s' as Hash 0x%08" PRIx32, dto.id.c_str(), idHash);
    
    // Speichere mit Hash als Storage-ID
    return m_recipeStorage->saveJson(idHash, jsonRecipe);
}

std::optional<std::string> StorageManager::getJsonRecipe(uint32_t id) {
    if (!m_recipeStorage) return std::nullopt;
    return m_recipeStorage->getJson(id);
}

bool StorageManager::updateJsonRecipe(uint32_t id, const std::string& jsonRecipe) {
    if (!m_recipeStorage) return false;
    return m_recipeStorage->saveJson(id, jsonRecipe);
}

bool StorageManager::deleteJsonRecipe(uint32_t id) {
    if (!m_recipeStorage) return false;
    return m_recipeStorage->remove(id);
}

std::vector<std::string> StorageManager::getAllJsonRecipes() {
    std::vector<std::string> recipes;
    if (!m_recipeStorage) return recipes;
    auto ids = m_recipeStorage->listIds();
    for (auto id : ids) {
        auto json = m_recipeStorage->getJson(id);
        if (json) recipes.push_back(*json);
    }
    return recipes;
}

bool StorageManager::existsJsonRecipe(uint32_t id) {
    if (!m_recipeStorage) return false;
    return m_recipeStorage->exists(id);
}

std::vector<uint32_t> StorageManager::getAllJsonRecipeIds() {
    if (!m_recipeStorage) return {};
    return m_recipeStorage->listIds();
}

size_t StorageManager::getJsonRecipeCount() {
    if (!m_recipeStorage) return 0;
    return m_recipeStorage->listIds().size();
}

// CRUD for serialized recipes

bool StorageManager::saveSerializedRecipe(const std::vector<uint8_t>& serializedRecipe) {
    if (!m_recipeStorage) return false;
    auto ids = m_recipeStorage->listIds();
    uint32_t newId = ids.empty() ? 1 : (*std::max_element(ids.begin(), ids.end()) + 1);
    return m_recipeStorage->save(newId, serializedRecipe);
}

std::optional<std::vector<uint8_t>> StorageManager::getSerializedRecipe(uint32_t id) {
    if (!m_recipeStorage) return std::nullopt;
    return m_recipeStorage->get(id);
}

bool StorageManager::updateSerializedRecipe(uint32_t id, const std::vector<uint8_t>& serializedRecipe) {
    if (!m_recipeStorage) return false;
    return m_recipeStorage->update(id, serializedRecipe);
}

bool StorageManager::deleteSerializedRecipe(uint32_t id) {
    if (!m_recipeStorage) return false;
    return m_recipeStorage->remove(id);
}

std::vector<std::vector<uint8_t>> StorageManager::getAllSerializedRecipes() {
    std::vector<std::vector<uint8_t>> recipes;
    if (!m_recipeStorage) return recipes;
    auto ids = m_recipeStorage->listIds();
    for (auto id : ids) {
        auto data = m_recipeStorage->get(id);
        if (data) recipes.push_back(*data);
    }
    return recipes;
}

bool StorageManager::existsSerializedRecipe(uint32_t id) {
    if (!m_recipeStorage) return false;
    return m_recipeStorage->exists(id);
}

std::vector<uint32_t> StorageManager::getAllSerializedRecipeIds() {
    if (!m_recipeStorage) return {};
    return m_recipeStorage->listIds();
}

size_t StorageManager::getSerializedRecipeCount() {
    if (!m_recipeStorage) return 0;
    return m_recipeStorage->listIds().size();
}

// ========== EXECUTION OPERATIONS ==========

bool StorageManager::saveExecution(const RecipeExecution& execution) {
    if (!m_executionStorage) return false;
    return m_executionStorage->save(execution);
}

std::optional<RecipeExecution> StorageManager::loadExecution(const std::string& executionId) {
    if (!m_executionStorage) return std::nullopt;
    return m_executionStorage->load(executionId);
}

std::vector<RecipeExecution> StorageManager::loadAllExecutions() {
    if (!m_executionStorage) return {};
    return m_executionStorage->loadAll();
}

bool StorageManager::deleteExecution(const std::string& executionId) {
    if (!m_executionStorage) return false;
    return m_executionStorage->deleteById(executionId);
}

bool StorageManager::existsExecution(const std::string& executionId) {
    if (!m_executionStorage) return false;
    return m_executionStorage->exists(executionId);
}

// ========== TIMESERIES RECORDING ==========

void StorageManager::startRecording(const std::string& executionId, const std::vector<std::pair<std::string, std::string>>& sensorInfo) {
    if (m_recording) {
        flushBuffer();
    }
    
    m_currentExecutionId = executionId;
    m_buffer.clear();
    m_buffer.reserve(sensorInfo.size());
    m_lastRecordedTimestamp = 0;
    
    for (const auto& [name, unit] : sensorInfo) {
        m_buffer.emplace_back(name, unit);
        m_buffer.back().dataPoints.reserve(BUFFER_SIZE);
    }
    
    m_recording = true;
    ESP_LOGI("StorageManager", "Started recording for execution '%s' with %zu sensors", 
             executionId.c_str(), sensorInfo.size());
}

void StorageManager::recordDataPoint(const std::map<std::string, float>& sensorValues, uint64_t relativeTimestamp) {
    if (!m_recording) {
        return;
    }
    
    if (m_lastRecordedTimestamp > 0 && (relativeTimestamp - m_lastRecordedTimestamp) < SAMPLING_INTERVAL_MS) {
        return;
    }
    
    m_lastRecordedTimestamp = relativeTimestamp;
    
    for (auto& series : m_buffer) {
        auto it = sensorValues.find(series.sensorName);
        if (it != sensorValues.end()) {
            series.dataPoints.emplace_back(relativeTimestamp, it->second);
        }
    }
    
    size_t totalPoints = getTotalPoints();
    if (totalPoints >= FLUSH_THRESHOLD * m_buffer.size()) {
        flushBuffer();
    }
}

void StorageManager::stopRecording() {
    if (!m_recording) {
        return;
    }
    
    flushBuffer();
    m_recording = false;
    ESP_LOGI("StorageManager", "Stopped recording for execution '%s'", m_currentExecutionId.c_str());
}

void StorageManager::flushBuffer() {
    if (m_buffer.empty() || m_currentExecutionId.empty() || !m_timeSeriesStorage) {
        return;
    }
    
    auto existing = m_timeSeriesStorage->loadTimeSeries(m_currentExecutionId);
    
    for (auto& newSeries : m_buffer) { 
        auto it = std::find_if(existing.begin(), existing.end(),
            [&](const SensorTimeSeries& s) { return s.sensorName == newSeries.sensorName; });
        
        if (it != existing.end()) {
            it->dataPoints.insert(it->dataPoints.end(), 
                newSeries.dataPoints.begin(), newSeries.dataPoints.end());
        } else {
            existing.push_back(newSeries);
        }
        
        newSeries.dataPoints.clear();
        newSeries.dataPoints.reserve(BUFFER_SIZE);
    }
    
    m_timeSeriesStorage->saveTimeSeries(m_currentExecutionId, existing);
}

size_t StorageManager::getTotalPoints() const {
    size_t total = 0;
    for (const auto& series : m_buffer) {
        total += series.dataPoints.size();
    }
    return total;
}

bool StorageManager::isRecording() const {
    return m_recording;
}

// ========== TIMESERIES LOADING ==========

TimeSeriesDataDto StorageManager::getTimeSeries(const std::string& executionId) {
    TimeSeriesDataDto dto;
    dto.executionId = executionId;
    
    if (!m_timeSeriesStorage) {
        return dto;
    }
    
    // Load execution to get startTime for absolute timestamp conversion
    uint64_t executionStartTime = 0;
    if (m_executionStorage) {
        auto execOpt = m_executionStorage->load(executionId);
        if (execOpt.has_value()) {
            executionStartTime = execOpt->startTimestamp();
        }
    }
    
    auto series = m_timeSeriesStorage->loadTimeSeries(executionId);
    
    dto.series.reserve(series.size());
    
    for (const auto& ts : series) {
        SensorTimeSeriesDto seriesDto;
        seriesDto.sensorName = ts.sensorName;
        seriesDto.unit = ts.unit;
        seriesDto.dataPoints.reserve(ts.dataPoints.size());
        
        for (const auto& pt : ts.dataPoints) {
            // Convert relative timestamp to absolute Unix timestamp
            uint64_t absoluteTimestamp = executionStartTime + pt.timestamp;
            seriesDto.dataPoints.push_back({absoluteTimestamp, pt.value});
        }
        
        dto.series.push_back(std::move(seriesDto));
    }
    
    return dto;
}

bool StorageManager::deleteTimeSeries(const std::string& executionId) {
    if (!m_timeSeriesStorage) return false;
    return m_timeSeriesStorage->deleteTimeSeries(executionId);
}

size_t StorageManager::getTimeSeriesStorageSize(const std::string& executionId) {
    if (!m_timeSeriesStorage) return 0;
    return m_timeSeriesStorage->getStorageSize(executionId);
}

// ========== COMBINED OPERATIONS ==========

bool StorageManager::deleteExecutionCompletely(const std::string& executionId) {
    bool tsDeleted = deleteTimeSeries(executionId);
    bool execDeleted = deleteExecution(executionId);
    
    ESP_LOGI("StorageManager", "Deleted execution '%s' completely (exec: %s, ts: %s)", 
             executionId.c_str(), execDeleted ? "ok" : "failed", tsDeleted ? "ok" : "failed");
    
    return execDeleted && tsDeleted;
}

// ========== AUTHENTICATION PASSWORD STORAGE ==========

void StorageManager::saveAuthPassword(const std::string& key, const std::string& password) {
    if (!m_recipeStorage) {
        ESP_LOGW("StorageManager", "Cannot save password, no recipe storage");
        return;
    }
    
    // Store as simple JSON string using recipe storage with special prefix
    std::string storageKey = "auth_" + key;
    uint32_t keyHash = calculateIdHash(storageKey);
    std::string jsonValue = "\"" + password + "\"";  // Simple JSON string
    
    m_recipeStorage->saveJson(keyHash, jsonValue);
    ESP_LOGI("StorageManager", "Saved password for key: %s", key.c_str());
}

std::optional<std::string> StorageManager::getAuthPassword(const std::string& key) {
    if (!m_recipeStorage) {
        ESP_LOGW("StorageManager", "Cannot load password, no recipe storage");
        return std::nullopt;
    }
    
    std::string storageKey = "auth_" + key;
    uint32_t keyHash = calculateIdHash(storageKey);
    
    auto jsonValue = m_recipeStorage->getJson(keyHash);
    if (!jsonValue.has_value()) {
        return std::nullopt;
    }
    
    // Remove quotes from JSON string
    std::string password = jsonValue.value();
    if (password.length() >= 2 && password.front() == '"' && password.back() == '"') {
        password = password.substr(1, password.length() - 2);
    }
    
    return password;
}
