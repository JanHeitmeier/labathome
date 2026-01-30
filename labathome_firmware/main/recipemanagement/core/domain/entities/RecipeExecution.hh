#pragma once
#include <string>
#include <cstdint>

enum class ExecutionStatus {
    Running,
    Completed,
    Failed,
    Aborted
};

class RecipeExecution {
public:
    RecipeExecution() = default;
    RecipeExecution(const std::string& execId, const std::string& recId, const std::string& recName);
    
    void setExecutionId(const std::string& id);
    std::string executionId() const;
    
    void setRecipeId(const std::string& id);
    std::string recipeId() const;
    
    void setRecipeName(const std::string& name);
    std::string recipeName() const;
    
    void setStartTimestamp(uint64_t ts);
    uint64_t startTimestamp() const;
    
    void setEndTimestamp(uint64_t ts);
    uint64_t endTimestamp() const;
    
    void setStatus(ExecutionStatus s);
    ExecutionStatus status() const;
    
    void setErrorMessage(const std::string& msg);
    std::string errorMessage() const;

private:
    std::string executionId_;
    std::string recipeId_;
    std::string recipeName_;
    uint64_t startTimestamp_{0};
    uint64_t endTimestamp_{0};
    ExecutionStatus status_{ExecutionStatus::Running};
    std::string errorMessage_;
};
