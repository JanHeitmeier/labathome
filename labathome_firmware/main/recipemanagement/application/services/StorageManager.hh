#pragma once
#include "../../core/interfaces/storage/IRecipeStorage.hh"
#include "../../core/interfaces/storage/IRecipeExecutionStorage.hh"
#include "../../core/interfaces/storage/ITimeSeriesStorage.hh"
#include "../../core/domain/value-objects/SensorTimeSeries.hh"
#include "../../core/domain/entities/RecipeExecution.hh"
#include "../dtos/RecipeDto.hh"
#include "../dtos/TimeSeriesDataDto.hh"
#include "../dtos/TimeSeriesBinaryDto.hh"
#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <map>

// CENTRAL STORAGE MANAGER: Coordinates all storage operations (Recipes, Executions, TimeSeries)
class StorageManager {
private:
    IRecipeStorage* m_recipeStorage;
    IRecipeExecutionStorage* m_executionStorage;
    ITimeSeriesStorage* m_timeSeriesStorage;
    
    // TimeSeries Recording Buffer
    std::vector<SensorTimeSeries> m_buffer;
    std::string m_currentExecutionId;
    bool m_recording{false};
    uint64_t m_lastRecordedTimestamp{0};
    
    static constexpr size_t BUFFER_SIZE = 100;
    static constexpr size_t FLUSH_THRESHOLD = 80;
    static constexpr uint64_t SAMPLING_INTERVAL_MS = 250;
    
    static uint32_t calculateIdHash(const std::string& stringId);
    void flushBuffer();
    size_t getTotalPoints() const;

public:
    StorageManager(IRecipeStorage* recipeStorage, 
                   IRecipeExecutionStorage* executionStorage,
                   ITimeSeriesStorage* timeSeriesStorage) 
        : m_recipeStorage(recipeStorage),
          m_executionStorage(executionStorage),
          m_timeSeriesStorage(timeSeriesStorage) {}
    
    ~StorageManager();
    
    // HIGH-LEVEL OPERATIONS

    bool loadRecipeForExecution(uint32_t recipeIdHash, Recipe& outRecipe);
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
    size_t getSerializedRecipeCount();
    
    // EXECUTION OPERATIONS
    
    bool saveExecution(const RecipeExecution& execution);
    std::optional<RecipeExecution> loadExecution(const std::string& executionId);
    std::vector<RecipeExecution> loadAllExecutions();
    bool deleteExecution(const std::string& executionId);
    bool existsExecution(const std::string& executionId);
    bool deleteExecutionCompletely(const std::string& executionId);

    //TIMESERIES
    
    void startRecording(const std::string& executionId, const std::vector<std::pair<std::string, std::string>>& sensorInfo);
    void recordDataPoint(const std::map<std::string, float>& sensorValues, uint64_t relativeTimestamp);
    void stopRecording();
    bool isRecording() const;
    TimeSeriesDataDto getTimeSeries(const std::string& executionId);
    TimeSeriesBinaryDto getTimeSeriesBinary(const std::string& executionId);
    bool deleteTimeSeries(const std::string& executionId);
    size_t getTimeSeriesStorageSize(const std::string& executionId);
    
    // AUTHENTICATION PASSWORD STORAGE (Plain Text)
    
    void saveAuthPassword(const std::string& key, const std::string& password);
    std::optional<std::string> getAuthPassword(const std::string& key);
};
