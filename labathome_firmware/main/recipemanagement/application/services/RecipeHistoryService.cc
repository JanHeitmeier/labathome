#include "RecipeHistoryService.hh"
#include <esp_log.h>
#include <sstream>
#include <iomanip>

static const char* TAG = "RecipeHistoryService";

RecipeHistoryService::RecipeHistoryService(IRecipeExecutionStorage* execStorage, ITimeSeriesStorage* tsStorage, TimestampProvider timestampProvider)
    : m_execStorage(execStorage), m_tsStorage(tsStorage), m_timestampProvider(timestampProvider) {
}

RecipeHistoryService::~RecipeHistoryService() {
}

std::string RecipeHistoryService::generateExecutionId() {
    uint64_t timestamp = m_timestampProvider();
    std::ostringstream oss;
    oss << "exec_" << timestamp;
    return oss.str();
}

std::string RecipeHistoryService::startExecution(const std::string& recipeId, const std::string& recipeName) {
    std::string executionId = generateExecutionId();
    uint64_t timestamp = m_timestampProvider();
    
    RecipeExecution exec(executionId, recipeId, recipeName);
    exec.setStartTimestamp(timestamp);
    exec.setStatus(ExecutionStatus::Running);
    
    if (!m_execStorage->save(exec)) {
        ESP_LOGE(TAG, "Failed to save execution %s", executionId.c_str());
    }
    
    ESP_LOGI(TAG, "Started execution %s for recipe '%s'", executionId.c_str(), recipeName.c_str());
    return executionId;
}

void RecipeHistoryService::endExecution(const std::string& executionId, ExecutionStatus status, const std::string& errorMsg) {
    auto execOpt = m_execStorage->load(executionId);
    if (!execOpt) {
        ESP_LOGW(TAG, "Execution %s not found", executionId.c_str());
        return;
    }
    
    RecipeExecution exec = *execOpt;
    exec.setEndTimestamp(m_timestampProvider());
    exec.setStatus(status);
    if (!errorMsg.empty()) {
        exec.setErrorMessage(errorMsg);
    }
    
    m_execStorage->save(exec);
    ESP_LOGI(TAG, "Ended execution %s with status %d", executionId.c_str(), static_cast<int>(status));
}

ExecutionHistoryDto RecipeHistoryService::getExecutionHistory() {
    ESP_LOGI(TAG, "[GET_HISTORY] getExecutionHistory called");
    
    ExecutionHistoryDto dto;
    auto executions = m_execStorage->loadAll();
    
    ESP_LOGI(TAG, "[GET_HISTORY] Storage returned executions");
    
    dto.executions.reserve(executions.size());
    for (const auto& exec : executions) {
        auto execDto = toDto(exec);
        ESP_LOGI(TAG, "[GET_HISTORY] Adding execution");
        dto.executions.push_back(execDto);
    }
    
    ESP_LOGI(TAG, "[GET_HISTORY] Returning DTO");
    return dto;
}

RecipeExecutionDto RecipeHistoryService::getExecution(const std::string& executionId) {
    auto execOpt = m_execStorage->load(executionId);
    if (!execOpt) {
        ESP_LOGW(TAG, "Execution %s not found", executionId.c_str());
        return RecipeExecutionDto{};
    }
    return toDto(*execOpt);
}

bool RecipeHistoryService::deleteExecution(const std::string& executionId) {
    bool tsDeleted = m_tsStorage->deleteTimeSeries(executionId);
    bool execDeleted = m_execStorage->deleteById(executionId);
    
    if (execDeleted) {
        ESP_LOGI(TAG, "Deleted execution %s", executionId.c_str());
    }
    
    return execDeleted && tsDeleted;
}

RecipeExecutionDto RecipeHistoryService::toDto(const RecipeExecution& exec) {
    RecipeExecutionDto dto;
    dto.executionId = exec.executionId();
    dto.recipeId = exec.recipeId();
    dto.recipeName = exec.recipeName();
    dto.startTime = exec.startTimestamp();
    dto.endTime = exec.endTimestamp();
    dto.duration = (exec.endTimestamp() > exec.startTimestamp()) 
        ? (exec.endTimestamp() - exec.startTimestamp()) : 0;
    dto.status = statusToString(exec.status());
    dto.errorMessage = exec.errorMessage();
    return dto;
}

std::string RecipeHistoryService::statusToString(ExecutionStatus status) {
    switch (status) {
        case ExecutionStatus::Running: return "running";
        case ExecutionStatus::Completed: return "completed";
        case ExecutionStatus::Failed: return "failed";
        case ExecutionStatus::Aborted: return "aborted";
        default: return "unknown";
    }
}
