#pragma once
#include "../../core/interfaces/storage/IRecipeExecutionStorage.hh"
#include "../../core/interfaces/storage/ITimeSeriesStorage.hh"
#include "../dtos/ExecutionHistoryDto.hh"
#include "../dtos/RecipeExecutionDto.hh"
#include <string>
#include <memory>
#include <functional>

class StorageManager;

class RecipeHistoryService {
public:
    using TimestampProvider = std::function<uint64_t()>;
    
    RecipeHistoryService(StorageManager* storageManager, TimestampProvider timestampProvider);
    ~RecipeHistoryService();
    
    std::string startExecution(const std::string& recipeId, const std::string& recipeName, const std::map<std::string, std::string>& globalParams = {});
    void endExecution(const std::string& executionId, ExecutionStatus status, const std::string& errorMsg = "");
    
    ExecutionHistoryDto getExecutionHistory();
    RecipeExecutionDto getExecution(const std::string& executionId);
    bool deleteExecution(const std::string& executionId);

private:
    StorageManager* m_storageManager;
    TimestampProvider m_timestampProvider;
    
    std::string generateExecutionId();
    RecipeExecutionDto toDto(const RecipeExecution& exec);
    std::string statusToString(ExecutionStatus status);
};
