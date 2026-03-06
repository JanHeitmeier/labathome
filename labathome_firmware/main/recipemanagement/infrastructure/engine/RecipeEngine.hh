#pragma once 

#include "StepTypeRegistry.hh"
#include "../../core/domain/value-objects/StepContext.hh"
#include "../../core/domain/value-objects/StepInstanceDescriptor.hh"
#include "../../core/services/IoResourceManager.hh"
#include "../../core/interfaces/engine/IStep.hh"
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <cstdint>
#include <functional>
#include <map>

enum class RecipeEngineState { Idle, Loaded, Running, Paused, Error };

using StateChangeCallback = std::function<void()>;

class StorageManager;
class RecipeHistoryService;

class RecipeEngine {
public:
    RecipeEngine();
    ~RecipeEngine();

    bool loadRecipe(const std::vector<StepInstanceDescriptor>& steps, const std::string& recipeId, const std::string& recipeName = "");
    void setGlobalParameters(const std::map<std::string, std::string>& params);
    bool start();
    bool pause();
    bool resume();
    bool stop();
    void acknowledgeStep();
    
    void tick(uint32_t deltaMs);
    
    RecipeEngineState getState() const { return m_state; }
    std::string getRecipeId() const { return m_recipeId; }
    std::string getRecipeName() const { return m_recipeName; }
    size_t getCurrentStepIndex() const { return m_currentStepIndex; }
    size_t getTotalSteps() const { return m_stepInstances.size(); }
    std::string getCurrentStepName() const;
    float getProgress() const;
    std::string getErrorMessage() const { return m_errorMessage; }
    
    bool isAwaitingAcknowledgment() const { return m_waitingForAcknowledgment; }
    std::string getUserInstruction() const { return m_currentUserInstruction; }
    
    std::map<std::string, float> getSensorValues() const;
    
    void setStateChangeCallback(StateChangeCallback callback) { m_stateChangeCallback = callback; }
    
    void setStorageManager(StorageManager* storageManager) { m_storageManager = storageManager; }
    void setHistoryService(RecipeHistoryService* service) { m_historyService = service; }

private:
    void buildStepContext(StepContext& ctx, size_t stepIndex);
    void executeCurrentStep();
    void advanceToNextStep();
    void cleanup();
    std::vector<std::string> getSensorNames() const;
    std::vector<std::pair<std::string, std::string>> getSensorInfo() const;
    
    std::atomic<RecipeEngineState> m_state{RecipeEngineState::Idle};
    std::string m_recipeId;
    std::string m_recipeName;
    std::vector<StepInstanceDescriptor> m_stepDescriptors;
    std::vector<std::unique_ptr<IStep>> m_stepInstances;
    std::vector<std::unique_ptr<StepContext>> m_stepContexts;
    size_t m_currentStepIndex{0};
    uint32_t m_elapsedMs{0};
    std::string m_errorMessage;
    
    bool m_waitingForAcknowledgment{false};
    bool m_acknowledgedByUser{false};
    std::string m_currentUserInstruction;
    
    StateChangeCallback m_stateChangeCallback;
    void notifyStateChange() {
        if (m_stateChangeCallback) {
            m_stateChangeCallback();
        }
    }
    
    StorageManager* m_storageManager{nullptr};
    RecipeHistoryService* m_historyService{nullptr};
    std::string m_currentExecutionId;
    std::map<std::string, std::string> m_globalParameters;
};
