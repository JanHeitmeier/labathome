#pragma once
#include "../../domain/entities/RecipeExecution.hh"
#include <vector>
#include <optional>
#include <cstdint>

class IRecipeExecutionStorage {
public:
    virtual ~IRecipeExecutionStorage() = default;
    
    virtual bool save(const RecipeExecution& execution) = 0;
    virtual std::optional<RecipeExecution> load(const std::string& executionId) = 0;
    virtual std::vector<RecipeExecution> loadAll() = 0;
    virtual bool deleteById(const std::string& executionId) = 0;
    virtual bool exists(const std::string& executionId) = 0;
};
