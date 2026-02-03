// StorageImplementation.cc
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

static const char* TAG_STORAGE = "Storage_Implementation";

static const char* JSON_DIR = "/spiffs/recipes_json";
static const char* BIN_DIR = "/spiffs/recipes_bin";
static const char* EXEC_DIR = "/spiffs/executions";
static const char* TIMESERIES_DIR = "/spiffs/timeseries";

class StorageImplementation : public IRecipeStorage, 
                          public IRecipeExecutionStorage,
                          public ITimeSeriesStorage {
public:
    StorageImplementation() {
        ensureDirsExist();
    }
    
    ~StorageImplementation() override = default;

    // ========== Serialization (Recipe → binary bytes) ==========
    
    bool serialize(const Recipe& r, std::vector<uint8_t>& outBlob) override {
        outBlob.clear();
        
        // Convert Recipe → RecipeDto
        RecipeDto dto = recipeToDto(r);
        
        // Serialize to JSON string (can be used as binary format too)
        std::string json = JsonSerialization::serialize(dto);
        if (json.empty()) {
            return false;
        }
        
        // Convert std::string → std::vector<uint8_t>
        outBlob.assign(json.begin(), json.end());
        return true;
    }

    // ========== Deserialization (binary bytes → Recipe) ==========
    
    bool deserialize(const std::vector<uint8_t>& blob, Recipe& outRecipe) override {
        if (blob.empty()) {
            return false;
        }
        
        // Convert std::vector<uint8_t> → std::string_view
        std::string_view json(reinterpret_cast<const char*>(blob.data()), blob.size());
        
        // Deserialize JSON → RecipeDto
        RecipeDto dto;
        if (!JsonSerialization::deserialize(json, dto)) {
            return false;
        }
        
        // Convert RecipeDto → Recipe
        outRecipe = dtoToRecipe(dto);
        return true;
    }

    // ========== File Operations (binary format) ==========

    bool save(uint32_t id, const std::vector<uint8_t>& data) override {
        bool result = saveBlobToFile(id, data, false);
        if (result) ESP_LOGI(TAG_STORAGE, "Saved Serialized");
        return result;
    }

    std::optional<std::vector<uint8_t>> get(uint32_t id) override {
        auto result = loadBlobFromFile(id, false);
        if (result) ESP_LOGI(TAG_STORAGE, "Loaded Serialized");
        return result;
    }

    bool update(uint32_t id, const std::vector<uint8_t>& data) override {
        // Same as save - overwrites existing
        return save(id, data);
    }

    bool remove(uint32_t id) override {
        bool binDeleted = (unlink(buildPath(BIN_DIR, id, ".bin").c_str()) == 0 || errno == ENOENT);
        bool jsonDeleted = (unlink(buildPath(JSON_DIR, id, ".json").c_str()) == 0 || errno == ENOENT);
        return (binDeleted || jsonDeleted);
    }

    bool exists(uint32_t id) override {
        struct stat st;
        return (stat(buildPath(BIN_DIR, id, ".bin").c_str(), &st) == 0) || 
               (stat(buildPath(JSON_DIR, id, ".json").c_str(), &st) == 0);
    }

    std::vector<uint32_t> listIds() override {
        std::vector<uint32_t> ids;
        scanDirectory(JSON_DIR, ".json", ids);
        scanDirectory(BIN_DIR, ".bin", ids);
        std::sort(ids.begin(), ids.end());
        return ids;
    }

    // ========== JSON Operations ==========

    bool saveJson(uint32_t id, const std::string& json) override {
        std::vector<uint8_t> blob(json.begin(), json.end());
        bool result = saveBlobToFile(id, blob, true);
        if (result) ESP_LOGI(TAG_STORAGE, "Saved JSON");
        return result;
    }

    std::optional<std::string> getJson(uint32_t id) override {
        auto blob = loadBlobFromFile(id, true);
        if (!blob) {
            return std::nullopt;
        }
        ESP_LOGI(TAG_STORAGE, "Loaded JSON");
        return std::string(blob->begin(), blob->end());
    }

private:
    // ========== Helper Methods ==========
    
    void ensureDirsExist() {
        auto ensureDir = [](const char* dir) {
            struct stat st;
            if (stat(dir, &st) != 0) {
                mkdir(dir, 0755);
            }
        };
        
        ensureDir(JSON_DIR);
        ensureDir(BIN_DIR);
        ensureDir(EXEC_DIR);
        ensureDir(TIMESERIES_DIR);
    }
    
    std::string buildPath(const char* dir, uint32_t id, const char* ext) {
        return std::string(dir) + "/" + std::to_string(id) + ext;
    }
    
    std::string buildPath(const char* dir, const std::string& name, const char* ext) {
        return std::string(dir) + "/" + name + ext;
    }
    
    std::optional<std::vector<uint8_t>> readFile(const std::string& path) {
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) return std::nullopt;
        
        fseek(f, 0, SEEK_END);
        long fileSize = ftell(f);
        fseek(f, 0, SEEK_SET);
        
        if (fileSize <= 0) {
            fclose(f);
            return std::nullopt;
        }
        
        std::vector<uint8_t> data(fileSize);
        size_t bytesRead = fread(data.data(), 1, fileSize, f);
        fclose(f);
        
        if (bytesRead != static_cast<size_t>(fileSize)) {
            return std::nullopt;
        }
        
        return data;
    }
    
    bool writeFile(const std::string& path, const std::vector<uint8_t>& data) {
        FILE* f = fopen(path.c_str(), "wb");
        if (!f) return false;
        
        size_t written = fwrite(data.data(), 1, data.size(), f);
        fclose(f);
        
        return (written == data.size());
    }
    
    void scanDirectory(const char* dir, const char* ext, std::vector<uint32_t>& ids) {
        DIR* d = opendir(dir);
        if (!d) return;
        
        size_t extLen = strlen(ext);
        struct dirent* entry;
        while ((entry = readdir(d)) != nullptr) {
            if (entry->d_type != DT_REG) continue;
            
            std::string filename(entry->d_name);
            if (filename.size() <= extLen || filename.substr(filename.size() - extLen) != ext) {
                continue;
            }
            
            char* endptr = nullptr;
            unsigned long id = std::strtoul(filename.substr(0, filename.size() - extLen).c_str(), &endptr, 10);
            if (endptr && *endptr == '\0' && id <= UINT32_MAX) {
                uint32_t id32 = static_cast<uint32_t>(id);
                if (std::find(ids.begin(), ids.end(), id32) == ids.end()) {
                    ids.push_back(id32);
                }
            }
        }
        closedir(d);
    }

    bool saveBlobToFile(uint32_t id, const std::vector<uint8_t>& blob, bool isJson) {
        const char* dir = isJson ? JSON_DIR : BIN_DIR;
        const char* ext = isJson ? ".json" : ".bin";
        return writeFile(buildPath(dir, id, ext), blob);
    }

    std::optional<std::vector<uint8_t>> loadBlobFromFile(uint32_t id, bool isJson) {
        const char* dir = isJson ? JSON_DIR : BIN_DIR;
        const char* ext = isJson ? ".json" : ".bin";
        return readFile(buildPath(dir, id, ext));
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
        std::string filename = buildPath(EXEC_DIR, execution.executionId(), ".exec");
        
        std::string blob;
        blob += execution.executionId() + "|";
        blob += execution.recipeId() + "|";
        blob += execution.recipeName() + "|";
        blob += std::to_string(execution.startTimestamp()) + "|";
        blob += std::to_string(execution.endTimestamp()) + "|";
        blob += std::to_string(static_cast<int>(execution.status())) + "|";
        blob += execution.errorMessage();
        
        FILE* f = fopen(filename.c_str(), "w");
        if (!f) {
            return false;
        }
        
        fwrite(blob.data(), 1, blob.size(), f);
        fclose(f);
        
        ESP_LOGI(TAG_STORAGE, "Saved Execution");
        return true;
    }
    
    std::optional<RecipeExecution> load(const std::string& executionId) override {
        std::string filename = buildPath(EXEC_DIR, executionId, ".exec");
        
        FILE* f = fopen(filename.c_str(), "r");
        if (!f) {
            return std::nullopt;
        }
        
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        
        if (size <= 0) {
            fclose(f);
            return std::nullopt;
        }
        
        std::string blob(size, '\0');
        fread(&blob[0], 1, size, f);
        fclose(f);
        
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
        
        ESP_LOGI(TAG_STORAGE, "Loaded Execution");
        return exec;
    }
    
    std::vector<RecipeExecution> loadAll() override {
        std::vector<RecipeExecution> result;
        
        DIR* dir = opendir(EXEC_DIR);
        if (!dir) {
            return result;
        }
        
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_REG) {
                std::string filename(entry->d_name);
                size_t len = filename.size();
                
                if (len > 5 && filename.substr(len - 5) == ".exec") {
                    std::string execId = filename.substr(0, len - 5);
                    auto exec = load(execId);
                    if (exec.has_value()) {
                        result.push_back(exec.value());
                    }
                }
            }
        }
        closedir(dir);
        
        return result;
    }
    
    bool deleteById(const std::string& executionId) override {
        return (unlink(buildPath(EXEC_DIR, executionId, ".exec").c_str()) == 0 || errno == ENOENT);
    }
    
    bool exists(const std::string& executionId) override {
        struct stat st;
        return stat(buildPath(EXEC_DIR, executionId, ".exec").c_str(), &st) == 0;
    }
    
    // ========== ITimeSeriesStorage Implementation ==========
    
    bool saveTimeSeries(const std::string& executionId, const std::vector<SensorTimeSeries>& series) override {
        if (series.empty()) return false;
        
        std::vector<uint8_t> data = TimeSeriesSerializer::serialize(series);
        if (data.empty()) return false;
        
        bool result = writeFile(buildPath(TIMESERIES_DIR, executionId, ".tsdata"), data);
        if (result) ESP_LOGI(TAG_STORAGE, "Saved TimeSeries");
        return result;
    }
    
    std::vector<SensorTimeSeries> loadTimeSeries(const std::string& executionId) override {
        std::vector<SensorTimeSeries> result;
        
        auto data = readFile(buildPath(TIMESERIES_DIR, executionId, ".tsdata"));
        if (!data || !TimeSeriesSerializer::deserialize(*data, result)) {
            result.clear();
        } else {
            ESP_LOGI(TAG_STORAGE, "Loaded TimeSeries");
        }
        
        return result;
    }
    
    bool deleteTimeSeries(const std::string& executionId) override {
        return (unlink(buildPath(TIMESERIES_DIR, executionId, ".tsdata").c_str()) == 0 || errno == ENOENT);
    }
    
    size_t getStorageSize(const std::string& executionId) override {
        struct stat st;
        return (stat(buildPath(TIMESERIES_DIR, executionId, ".tsdata").c_str(), &st) == 0) ? st.st_size : 0;
    }
};