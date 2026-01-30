#pragma once
#include "../../core/interfaces/storage/IRecipeExecutionStorage.hh"

class RecipeExecutionStorage_NVS : public IRecipeExecutionStorage {
public:
    RecipeExecutionStorage_NVS();
    ~RecipeExecutionStorage_NVS() override;
    
    bool save(const RecipeExecution& execution) override;
    std::optional<RecipeExecution> load(const std::string& executionId) override;
    std::vector<RecipeExecution> loadAll() override;
    bool deleteById(const std::string& executionId) override;
    bool exists(const std::string& executionId) override;

private:
    static constexpr const char* NAMESPACE = "exec_hist";
    static constexpr size_t MAX_EXECUTIONS = 50;
};
