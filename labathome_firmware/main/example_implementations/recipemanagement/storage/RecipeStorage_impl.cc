// RecipeStorageImpl.cpp
// Implementation of IRecipeStorage, IRecipeExecutionStorage, ITimeSeriesStorage
// Uses SPIFFS for all storage (hardware-agnostic interface)
// - Recipes: /spiffs/recipes_json/ and /spiffs/recipes_bin/
// - Executions: /spiffs/executions/
// - TimeSeries: /spiffs/timeseries/

#include "../../../recipemanagement/core/interfaces/storage/IRecipeStorage.hh"
#include "../../../recipemanagement/core/interfaces/storage/IRecipeExecutionStorage.hh"
#include "../../../recipemanagement/core/interfaces/storage/ITimeSeriesStorage.hh"
#include "../../../recipemanagement/core/domain/entities/Recipe.hh"
#include "../../../recipemanagement/core/domain/entities/RecipeExecution.hh"
#include "../../../recipemanagement/core/domain/value-objects/SensorTimeSeries.hh"
#include "../../../recipemanagement/application/dtos/RecipeDto.hh"
#include "../../../recipemanagement/infrastructure/serialization/JsonSerialization.hh"
#include "../../../recipemanagement/infrastructure/serialization/TimeSeriesSerializer.hh"
#include <string>
#include <vector>
#include <optional>
#include <cstdio>
#include <cstdint>
#include <dirent.h>
#include <sys/stat.h>
#include <esp_log.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <algorithm>

static const char* TAG_RECIPE_STORAGE = "RecipeStorage";
static const char* JSON_DIR = "/spiffs/recipes_json";
static const char* BIN_DIR = "/spiffs/recipes_bin";
static const char* EXEC_DIR = "/spiffs/executions";
static const char* TIMESERIES_DIR = "/spiffs/timeseries";

class RecipeStorageImpl : public IRecipeStorage, 
                          public IRecipeExecutionStorage,
                          public ITimeSeriesStorage {
public:
    RecipeStorageImpl() {
        ensureDirsExist();
    }
    
    ~RecipeStorageImpl() override = default;

    // ========== Serialization (Recipe → binary bytes) ==========
    
    bool serialize(const Recipe& r, std::vector<uint8_t>& outBlob) override {
        outBlob.clear();
        
        // Convert Recipe → RecipeDto
        RecipeDto dto = recipeToDto(r);
        
        // Serialize to JSON string (can be used as binary format too)
        std::string json = JsonSerialization::serialize(dto);
        if (json.empty()) {
            ESP_LOGE(TAG_RECIPE_STORAGE, "Failed to serialize recipe '%s' to JSON", r.id().c_str());
            return false;
        }
        
        // Convert std::string → std::vector<uint8_t>
        outBlob.assign(json.begin(), json.end());
        return true;
    }

    // ========== Deserialization (binary bytes → Recipe) ==========
    
    bool deserialize(const std::vector<uint8_t>& blob, Recipe& outRecipe) override {
        if (blob.empty()) {
            ESP_LOGW(TAG_RECIPE_STORAGE, "Cannot deserialize empty blob");
            return false;
        }
        
        // Convert std::vector<uint8_t> → std::string_view
        std::string_view json(reinterpret_cast<const char*>(blob.data()), blob.size());
        
        // Deserialize JSON → RecipeDto
        RecipeDto dto;
        if (!JsonSerialization::deserialize(json, dto)) {
            ESP_LOGE(TAG_RECIPE_STORAGE, "Failed to deserialize JSON to RecipeDto");
            return false;
        }
        
        // Convert RecipeDto → Recipe
        outRecipe = dtoToRecipe(dto);
        return true;
    }

    // ========== File Operations (binary format) ==========

    bool save(uint32_t id, const std::vector<uint8_t>& data) override {
        return saveBlobToFile(id, data, false); // false = binary format
    }

    std::optional<std::vector<uint8_t>> get(uint32_t id) override {
        return loadBlobFromFile(id, false); // false = binary format
    }

    bool update(uint32_t id, const std::vector<uint8_t>& data) override {
        // Same as save - overwrites existing
        return save(id, data);
    }

    bool remove(uint32_t id) override {
        std::string binPath = std::string(BIN_DIR) + "/" + std::to_string(id) + ".bin";
        std::string jsonPath = std::string(JSON_DIR) + "/" + std::to_string(id) + ".json";
        
        bool binDeleted = (unlink(binPath.c_str()) == 0 || errno == ENOENT);
        bool jsonDeleted = (unlink(jsonPath.c_str()) == 0 || errno == ENOENT);
        
        if (binDeleted || jsonDeleted) {
            ESP_LOGI(TAG_RECIPE_STORAGE, "Deleted recipe %lu", (unsigned long)id);
            return true;
        }
        
        ESP_LOGE(TAG_RECIPE_STORAGE, "Failed to delete recipe %lu: %s", (unsigned long)id, strerror(errno));
        return false;
    }

    bool exists(uint32_t id) override {
        std::string binPath = std::string(BIN_DIR) + "/" + std::to_string(id) + ".bin";
        std::string jsonPath = std::string(JSON_DIR) + "/" + std::to_string(id) + ".json";
        
        struct stat st;
        return (stat(binPath.c_str(), &st) == 0) || (stat(jsonPath.c_str(), &st) == 0);
    }

    std::vector<uint32_t> listIds() override {
        std::vector<uint32_t> ids;
        
        // Scan JSON directory
        DIR* dir = opendir(JSON_DIR);
        if (dir) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != nullptr) {
                if (entry->d_type != DT_REG) {
                    continue;
                }
                
                std::string filename(entry->d_name);
                if (filename.size() > 5 && filename.substr(filename.size() - 5) == ".json") {
                    // Parse ID without exceptions (they're disabled)
                    char* endptr = nullptr;
                    unsigned long id = std::strtoul(filename.substr(0, filename.size() - 5).c_str(), &endptr, 10);
                    if (endptr && *endptr == '\0' && id <= UINT32_MAX) {
                        uint32_t id32 = static_cast<uint32_t>(id);
                        if (std::find(ids.begin(), ids.end(), id32) == ids.end()) {
                            ids.push_back(id32);
                        }
                    } else {
                        ESP_LOGW(TAG_RECIPE_STORAGE, "Invalid recipe filename: %s", filename.c_str());
                    }
                }
            }
            closedir(dir);
        }
        
        // Scan binary directory
        dir = opendir(BIN_DIR);
        if (dir) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != nullptr) {
                if (entry->d_type != DT_REG) {
                    continue;
                }
                
                std::string filename(entry->d_name);
                if (filename.size() > 4 && filename.substr(filename.size() - 4) == ".bin") {
                    // Parse ID without exceptions (they're disabled)
                    char* endptr = nullptr;
                    unsigned long id = std::strtoul(filename.substr(0, filename.size() - 4).c_str(), &endptr, 10);
                    if (endptr && *endptr == '\0' && id <= UINT32_MAX) {
                        uint32_t id32 = static_cast<uint32_t>(id);
                        if (std::find(ids.begin(), ids.end(), id32) == ids.end()) {
                            ids.push_back(id32);
                        }
                    } else {
                        ESP_LOGW(TAG_RECIPE_STORAGE, "Invalid recipe filename: %s", filename.c_str());
                    }
                }
            }
            closedir(dir);
        }
        
        std::sort(ids.begin(), ids.end());
        ESP_LOGI(TAG_RECIPE_STORAGE, "Found %zu recipes", ids.size());
        return ids;
    }

    // ========== JSON Operations ==========

    bool saveJson(uint32_t id, const std::string& json) override {
        std::vector<uint8_t> blob(json.begin(), json.end());
        return saveBlobToFile(id, blob, true); // true = JSON format
    }

    std::optional<std::string> getJson(uint32_t id) override {
        auto blob = loadBlobFromFile(id, true); // true = JSON format
        if (!blob) {
            return std::nullopt;
        }
        return std::string(blob->begin(), blob->end());
    }

private:
    // ========== Helper Methods ==========
    
    void ensureDirsExist() {
        struct stat st;
        auto ensureDir = [](const char* dir) {
            struct stat st;
            if (stat(dir, &st) != 0) {
                if (mkdir(dir, 0755) != 0 && errno != EEXIST) {
                    ESP_LOGE(TAG_RECIPE_STORAGE, "Failed to create directory '%s': %s", dir, strerror(errno));
                } else {
                    ESP_LOGI(TAG_RECIPE_STORAGE, "Created directory '%s'", dir);
                }
            }
        };
        
        ensureDir(JSON_DIR);
        ensureDir(BIN_DIR);
        ensureDir(EXEC_DIR);
        ensureDir(TIMESERIES_DIR);
    }

    bool saveBlobToFile(uint32_t id, const std::vector<uint8_t>& blob, bool isJson) {
        const char* dir = isJson ? JSON_DIR : BIN_DIR;
        const char* ext = isJson ? ".json" : ".bin";
        std::string path = std::string(dir) + "/" + std::to_string(id) + ext;
        
        FILE* f = fopen(path.c_str(), "wb");
        if (!f) {
            ESP_LOGE(TAG_RECIPE_STORAGE, "Failed to open '%s' for writing: %s", path.c_str(), strerror(errno));
            return false;
        }
        
        size_t written = fwrite(blob.data(), 1, blob.size(), f);
        fclose(f);
        
        if (written != blob.size()) {
            ESP_LOGE(TAG_RECIPE_STORAGE, "Failed to write complete blob to '%s' (wrote %zu/%zu bytes)", 
                     path.c_str(), written, blob.size());
            return false;
        }
        
        ESP_LOGI(TAG_RECIPE_STORAGE, "Saved recipe %lu (%zu bytes) to '%s'", (unsigned long)id, blob.size(), path.c_str());
        return true;
    }

    std::optional<std::vector<uint8_t>> loadBlobFromFile(uint32_t id, bool isJson) {
        const char* dir = isJson ? JSON_DIR : BIN_DIR;
        const char* ext = isJson ? ".json" : ".bin";
        std::string path = std::string(dir) + "/" + std::to_string(id) + ext;
        
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) {
            if (errno != ENOENT) {
                ESP_LOGE(TAG_RECIPE_STORAGE, "Failed to open '%s' for reading: %s", path.c_str(), strerror(errno));
            }
            return std::nullopt;
        }
        
        // Get file size
        fseek(f, 0, SEEK_END);
        long fileSize = ftell(f);
        fseek(f, 0, SEEK_SET);
        
        if (fileSize <= 0) {
            fclose(f);
            ESP_LOGW(TAG_RECIPE_STORAGE, "File '%s' is empty or invalid size %ld", path.c_str(), fileSize);
            return std::nullopt;
        }
        
        // Read file into buffer
        std::vector<uint8_t> blob(fileSize);
        size_t bytesRead = fread(blob.data(), 1, fileSize, f);
        fclose(f);
        
        if (bytesRead != static_cast<size_t>(fileSize)) {
            ESP_LOGE(TAG_RECIPE_STORAGE, "Failed to read complete file '%s' (read %zu/%ld bytes)", 
                     path.c_str(), bytesRead, fileSize);
            return std::nullopt;
        }
        
        ESP_LOGD(TAG_RECIPE_STORAGE, "Loaded recipe %lu (%zu bytes) from '%s'", (unsigned long)id, blob.size(), path.c_str());
        return blob;
    }

    // ========== Conversion Methods ==========
    
    RecipeDto recipeToDto(const Recipe& r) {
        RecipeDto dto;
        dto.id = r.id();
        dto.name = r.name();
        dto.description = r.description();
        dto.version = r.version();
        
        // Convert steps
        dto.steps.clear();
        int order = 0;
        for (const auto& stepDesc : r.steps()) {
            StepConfigDto stepDto;
            // Convert typeId (uint32_t) to string in hex format
            char buf[16];
            snprintf(buf, sizeof(buf), "0x%04lX", (unsigned long)stepDesc.typeId);
            stepDto.stepTypeId = buf;
            
            // Copy parameters - manually convert unordered_map to map
            for (const auto& [key, val] : stepDesc.params) {
                stepDto.parameters[key] = val;
            }
            
            // Add aliases to parameters with prefix
            for (const auto& [key, val] : stepDesc.aliases) {
                stepDto.parameters["alias_" + key] = val;
            }
            
            stepDto.order = order++;
            dto.steps.push_back(stepDto);
        }
        
        return dto;
    }
    
    Recipe dtoToRecipe(const RecipeDto& dto) {
        Recipe r;
        r.setId(dto.id);
        r.setName(dto.name);
        r.setDescription(dto.description);
        r.setVersion(dto.version);
        
        // Convert steps
        std::vector<StepInstanceDescriptor> steps;
        for (const auto& stepDto : dto.steps) {
            StepInstanceDescriptor desc;
            
            // Convert stepTypeId (string) to typeId (uint32_t) without exceptions
            char* endptr = nullptr;
            unsigned long typeId = std::strtoul(stepDto.stepTypeId.c_str(), &endptr, 0); // Auto-detect base (0x for hex)
            if (endptr && *endptr == '\0' && typeId <= UINT32_MAX) {
                desc.typeId = static_cast<uint32_t>(typeId);
            } else {
                ESP_LOGW(TAG_RECIPE_STORAGE, "Invalid step type ID: %s", stepDto.stepTypeId.c_str());
                desc.typeId = 0;
            }
            
            // Copy parameters, separating aliases
            for (const auto& [key, val] : stepDto.parameters) {
                if (key.rfind("alias_", 0) == 0) {
                    // It's an alias
                    desc.aliases[key.substr(6)] = val; // Remove "alias_" prefix
                } else {
                    desc.params[key] = val;
                }
            }
            
            steps.push_back(desc);
        }
        r.setSteps(steps);
        
        return r;
    }
    
    // ========== IRecipeExecutionStorage Implementation ==========
    
    bool save(const RecipeExecution& execution) override {
        ESP_LOGI(TAG_RECIPE_STORAGE, "[SAVE_EXEC] Saving execution");
        
        std::string filename = std::string(EXEC_DIR) + "/" + execution.executionId() + ".exec";
        
        std::string blob;
        blob += execution.executionId() + "|";
        blob += execution.recipeId() + "|";
        blob += execution.recipeName() + "|";
        blob += std::to_string(execution.startTimestamp()) + "|";
        blob += std::to_string(execution.endTimestamp()) + "|";
        blob += std::to_string(static_cast<int>(execution.status())) + "|";
        blob += execution.errorMessage();
        
        ESP_LOGI(TAG_RECIPE_STORAGE, "[SAVE_EXEC] Serialized blob");
        
        FILE* f = fopen(filename.c_str(), "w");
        if (!f) {
            ESP_LOGE(TAG_RECIPE_STORAGE, "[SAVE_EXEC] Failed to save execution");
            return false;
        }
        
        fwrite(blob.data(), 1, blob.size(), f);
        fclose(f);
        
        ESP_LOGI(TAG_RECIPE_STORAGE, "[SAVE_EXEC] Successfully saved execution");
        return true;
    }
    
    std::optional<RecipeExecution> load(const std::string& executionId) override {
        ESP_LOGI(TAG_RECIPE_STORAGE, "[LOAD_EXEC] Loading execution");
        
        std::string filename = std::string(EXEC_DIR) + "/" + executionId + ".exec";
        ESP_LOGI(TAG_RECIPE_STORAGE, "[LOAD_EXEC] Opening file");
        
        FILE* f = fopen(filename.c_str(), "r");
        if (!f) {
            ESP_LOGW(TAG_RECIPE_STORAGE, "[LOAD_EXEC] File not found");
            return std::nullopt;
        }
        
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        
        ESP_LOGI(TAG_RECIPE_STORAGE, "[LOAD_EXEC] Reading file");
        
        if (size <= 0) {
            fclose(f);
            ESP_LOGW(TAG_RECIPE_STORAGE, "[LOAD_EXEC] Invalid file size");
            return std::nullopt;
        }
        
        std::string blob(size, '\0');
        fread(&blob[0], 1, size, f);
        fclose(f);
        
        ESP_LOGI(TAG_RECIPE_STORAGE, "[LOAD_EXEC] Deserializing");
        
        size_t pos = 0;
        auto getToken = [&]() -> std::string {
            size_t next = blob.find('|', pos);
            if (next == std::string::npos) next = blob.size();
            std::string token = blob.substr(pos, next - pos);
            pos = next + 1;
            return token;
        };
        
        RecipeExecution exec;
        exec.setExecutionId(getToken());
        exec.setRecipeId(getToken());
        exec.setRecipeName(getToken());
        exec.setStartTimestamp(std::stoull(getToken()));
        exec.setEndTimestamp(std::stoull(getToken()));
        exec.setStatus(static_cast<ExecutionStatus>(std::stoi(getToken())));
        exec.setErrorMessage(getToken());
        
        ESP_LOGI(TAG_RECIPE_STORAGE, "[LOAD_EXEC] Successfully loaded");
        return exec;
    }
    
    std::vector<RecipeExecution> loadAll() override {
        ESP_LOGI(TAG_RECIPE_STORAGE, "[LOAD_ALL_EXEC] ===== STARTING loadAll() =====");
        
        std::vector<RecipeExecution> result;
        
        ESP_LOGI(TAG_RECIPE_STORAGE, "[LOAD_ALL_EXEC] Attempting to open directory: %s", EXEC_DIR);
        DIR* dir = opendir(EXEC_DIR);
        if (!dir) {
            ESP_LOGE(TAG_RECIPE_STORAGE, "[LOAD_ALL_EXEC] FAILED to open executions directory!");
            return result;
        }
        ESP_LOGI(TAG_RECIPE_STORAGE, "[LOAD_ALL_EXEC] Directory opened successfully");
        
        int fileCount = 0;
        int execCount = 0;
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            ESP_LOGI(TAG_RECIPE_STORAGE, "[LOAD_ALL_EXEC] Directory entry type=%d", entry->d_type);
            
            if (entry->d_type == DT_REG) {
                fileCount++;
                std::string filename(entry->d_name);
                ESP_LOGI(TAG_RECIPE_STORAGE, "[LOAD_ALL_EXEC] Found regular file #%d", fileCount);
                
                size_t len = filename.size();
                ESP_LOGI(TAG_RECIPE_STORAGE, "[LOAD_ALL_EXEC] Filename length: %zu", len);
                
                if (len > 5 && filename.substr(len - 5) == ".exec") {
                    execCount++;
                    std::string execId = filename.substr(0, len - 5);
                    ESP_LOGI(TAG_RECIPE_STORAGE, "[LOAD_ALL_EXEC] Found .exec file #%d", execCount);
                    
                    auto exec = load(execId);
                    if (exec.has_value()) {
                        result.push_back(exec.value());
                        ESP_LOGI(TAG_RECIPE_STORAGE, "[LOAD_ALL_EXEC] Successfully loaded and added execution #%d", execCount);
                    } else {
                        ESP_LOGE(TAG_RECIPE_STORAGE, "[LOAD_ALL_EXEC] FAILED to load execution #%d", execCount);
                    }
                } else {
                    ESP_LOGI(TAG_RECIPE_STORAGE, "[LOAD_ALL_EXEC] File does not end with .exec, skipping");
                }
            }
        }
        closedir(dir);
        
        ESP_LOGI(TAG_RECIPE_STORAGE, "[LOAD_ALL_EXEC] ===== COMPLETED: Found %d files, %d .exec files, returning %d executions =====", fileCount, execCount, (int)result.size());
        return result;
    }
    
    bool deleteById(const std::string& executionId) override {
        std::string filename = std::string(EXEC_DIR) + "/" + executionId + ".exec";
        if (unlink(filename.c_str()) == 0) {
            ESP_LOGI(TAG_RECIPE_STORAGE, "Deleted execution %s", executionId.c_str());
            return true;
        }
        return errno == ENOENT;
    }
    
    bool exists(const std::string& executionId) override {
        std::string filename = std::string(EXEC_DIR) + "/" + executionId + ".exec";
        struct stat st;
        return stat(filename.c_str(), &st) == 0;
    }
    
    // ========== ITimeSeriesStorage Implementation ==========
    
    bool saveTimeSeries(const std::string& executionId, const std::vector<SensorTimeSeries>& series) override {
        if (series.empty()) {
            ESP_LOGW(TAG_RECIPE_STORAGE, "No series to save for %s", executionId.c_str());
            return false;
        }
        
        std::vector<uint8_t> data = TimeSeriesSerializer::serialize(series);
        if (data.empty()) {
            ESP_LOGE(TAG_RECIPE_STORAGE, "Serialization failed for %s", executionId.c_str());
            return false;
        }
        
        std::string path = std::string(TIMESERIES_DIR) + "/" + executionId + ".tsdata";
        FILE* f = fopen(path.c_str(), "wb");
        if (!f) {
            ESP_LOGE(TAG_RECIPE_STORAGE, "Failed to open '%s': %s", path.c_str(), strerror(errno));
            return false;
        }
        
        size_t written = fwrite(data.data(), 1, data.size(), f);
        fclose(f);
        
        if (written != data.size()) {
            ESP_LOGE(TAG_RECIPE_STORAGE, "Write failed for '%s': %zu/%zu bytes", path.c_str(), written, data.size());
            return false;
        }
        
        ESP_LOGI(TAG_RECIPE_STORAGE, "Saved %zu series (%zu bytes) to '%s'", series.size(), data.size(), path.c_str());
        return true;
    }
    
    std::vector<SensorTimeSeries> loadTimeSeries(const std::string& executionId) override {
        std::vector<SensorTimeSeries> result;
        
        std::string path = std::string(TIMESERIES_DIR) + "/" + executionId + ".tsdata";
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) {
            if (errno != ENOENT) {
                ESP_LOGE(TAG_RECIPE_STORAGE, "Failed to open '%s': %s", path.c_str(), strerror(errno));
            }
            return result;
        }
        
        fseek(f, 0, SEEK_END);
        long fileSize = ftell(f);
        fseek(f, 0, SEEK_SET);
        
        if (fileSize <= 0) {
            fclose(f);
            ESP_LOGW(TAG_RECIPE_STORAGE, "Invalid file size %ld for '%s'", fileSize, path.c_str());
            return result;
        }
        
        std::vector<uint8_t> data(fileSize);
        size_t bytesRead = fread(data.data(), 1, fileSize, f);
        fclose(f);
        
        if (bytesRead != static_cast<size_t>(fileSize)) {
            ESP_LOGE(TAG_RECIPE_STORAGE, "Read failed for '%s': %zu/%ld bytes", path.c_str(), bytesRead, fileSize);
            return result;
        }
        
        if (!TimeSeriesSerializer::deserialize(data, result)) {
            ESP_LOGE(TAG_RECIPE_STORAGE, "Deserialization failed for '%s'", path.c_str());
            result.clear();
        }
        
        return result;
    }
    
    bool deleteTimeSeries(const std::string& executionId) override {
        std::string path = std::string(TIMESERIES_DIR) + "/" + executionId + ".tsdata";
        if (unlink(path.c_str()) == 0) {
            ESP_LOGI(TAG_RECIPE_STORAGE, "Deleted timeseries '%s'", path.c_str());
            return true;
        }
        return errno == ENOENT;
    }
    
    size_t getStorageSize(const std::string& executionId) override {
        std::string path = std::string(TIMESERIES_DIR) + "/" + executionId + ".tsdata";
        struct stat st;
        if (stat(path.c_str(), &st) == 0) {
            return st.st_size;
        }
        return 0;
    }
};