#include "RecipeExecution.hh"

RecipeExecution::RecipeExecution(const std::string &execId, const std::string &recId, const std::string &recName)
    : executionId_(execId), recipeId_(recId), recipeName_(recName)
{
}

void RecipeExecution::setExecutionId(const std::string &id) { executionId_ = id; }
std::string RecipeExecution::executionId() const { return executionId_; }

void RecipeExecution::setRecipeId(const std::string &id) { recipeId_ = id; }
std::string RecipeExecution::recipeId() const { return recipeId_; }

void RecipeExecution::setRecipeName(const std::string &name) { recipeName_ = name; }
std::string RecipeExecution::recipeName() const { return recipeName_; }

void RecipeExecution::setStartTimestamp(uint64_t ts) { startTimestamp_ = ts; }
uint64_t RecipeExecution::startTimestamp() const { return startTimestamp_; }

void RecipeExecution::setEndTimestamp(uint64_t ts) { endTimestamp_ = ts; }
uint64_t RecipeExecution::endTimestamp() const { return endTimestamp_; }

void RecipeExecution::setStatus(ExecutionStatus s) { status_ = s; }
ExecutionStatus RecipeExecution::status() const { return status_; }

void RecipeExecution::setErrorMessage(const std::string &msg) { errorMessage_ = msg; }
std::string RecipeExecution::errorMessage() const { return errorMessage_; }
