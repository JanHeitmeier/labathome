#include "RecipeHistoryService.hh"
#include "StorageManager.hh"
#include <sstream>
#include <iomanip>

RecipeHistoryService::RecipeHistoryService(StorageManager* storageManager, TimestampProvider timestampProvider)
    : m_storageManager(storageManager), m_timestampProvider(timestampProvider) {
}

RecipeHistoryService::~RecipeHistoryService() {
}

std::string RecipeHistoryService::generateExecutionId() {
    uint64_t timestamp = m_timestampProvider();
    std::ostringstream oss;
    oss << "exec_" << timestamp;
    return oss.str();
}

std::string RecipeHistoryService::startExecution(const std::string& recipeId, const std::string& recipeName, const std::map<std::string, std::string>& globalParams) {
    std::string executionId = generateExecutionId();
    uint64_t timestamp = m_timestampProvider();
    
    RecipeExecution exec(executionId, recipeId, recipeName);
    exec.setStartTimestamp(timestamp);
    exec.setStatus(ExecutionStatus::Running);
    exec.setGlobalParameters(globalParams);
    
    m_storageManager->saveExecution(exec);
    
    return executionId;
}

void RecipeHistoryService::endExecution(const std::string& executionId, ExecutionStatus status, const std::string& errorMsg) {
    auto execOpt = m_storageManager->loadExecution(executionId);
    if (!execOpt) {
        return;
    }
    
    RecipeExecution exec = *execOpt;
    exec.setEndTimestamp(m_timestampProvider());
    exec.setStatus(status);
    if (!errorMsg.empty()) {
        exec.setErrorMessage(errorMsg);
    }
    
    m_storageManager->saveExecution(exec);
}

ExecutionHistoryDto RecipeHistoryService::getExecutionHistory() {
    ExecutionHistoryDto dto;
    auto executions = m_storageManager->loadAllExecutions();
    
    dto.executions.reserve(executions.size());
    for (const auto& exec : executions) {
        dto.executions.push_back(toDto(exec));
    }
    
    return dto;
}

RecipeExecutionDto RecipeHistoryService::getExecution(const std::string& executionId) {
    auto execOpt = m_storageManager->loadExecution(executionId);
    if (!execOpt) {
        return RecipeExecutionDto{};
    }
    return toDto(*execOpt);
}

bool RecipeHistoryService::deleteExecution(const std::string& executionId) {
    return m_storageManager->deleteExecutionCompletely(executionId);
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
    dto.globalParameters = exec.globalParameters();
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
