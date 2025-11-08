// RecipeStorageImpl.cpp
// Implementation of IRecipeStorage using LittleFS (SPIFFS mount) on ESP32-S3
// Stores recipes as JSON files in /spiffs/recipes_json/
// Uses RapidJSON (via JsonSerialization) and RecipeDto for serialization

#include "../../../recipemanagement/core/interfaces/storage/IRecipeStorage.hh"
#include "../../../recipemanagement/core/domain/entities/Recipe.hh"
#include "../../../recipemanagement/application/dtos/RecipeDto.hh"
#include "../../../recipemanagement/infrastructure/serialization/JsonSerialization.hh"
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

class RecipeStorageImpl : public IRecipeStorage {
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
        if (stat(JSON_DIR, &st) != 0) {
            if (mkdir(JSON_DIR, 0755) != 0 && errno != EEXIST) {
                ESP_LOGE(TAG_RECIPE_STORAGE, "Failed to create directory '%s': %s", JSON_DIR, strerror(errno));
            } else {
                ESP_LOGI(TAG_RECIPE_STORAGE, "Created directory '%s'", JSON_DIR);
            }
        }
        if (stat(BIN_DIR, &st) != 0) {
            if (mkdir(BIN_DIR, 0755) != 0 && errno != EEXIST) {
                ESP_LOGE(TAG_RECIPE_STORAGE, "Failed to create directory '%s': %s", BIN_DIR, strerror(errno));
            } else {
                ESP_LOGI(TAG_RECIPE_STORAGE, "Created directory '%s'", BIN_DIR);
            }
        }
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
            
            desc.repeatCount = 1;
            steps.push_back(desc);
        }
        r.setSteps(steps);
        
        return r;
    }
};