#pragma once 

#include "StepTypeRegistry.hh"
#include "StepContext.hh"
#include "IoResourceManager.hh"
#include "IStep.hh"
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <cstdint>

enum class RecipeEngineState { Idle, Loaded, Running, Paused, Error };

struct StepInstanceDescriptor {
    std::string systemId;
    uint32_t typeId{0};
    std::unordered_map<std::string, std::string> params;
    std::unordered_map<std::string, std::string> aliases;
    int repeatCount{1};
};

class RecipeEngine {
public:
    RecipeEngine();
    ~RecipeEngine();

    bool loadRecipe(const std::vector<StepInstanceDescriptor>& steps, const std::string& recipeId);
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
    
    // User acknowledgment status
    bool isAwaitingAcknowledgment() const { return m_waitingForAcknowledgment; }
    std::string getUserInstruction() const { return m_currentUserInstruction; }

private:
    void buildStepContext(StepContext& ctx, size_t stepIndex);
    void executeCurrentStep(uint32_t deltaMs);
    void advanceToNextStep();
    void cleanup();
    
    std::atomic<RecipeEngineState> m_state{RecipeEngineState::Idle};
    std::string m_recipeId;
    std::string m_recipeName;
    std::vector<StepInstanceDescriptor> m_stepDescriptors;
    std::vector<IStep*> m_stepInstances;
    size_t m_currentStepIndex{0};
    uint32_t m_elapsedMs{0};
    std::string m_errorMessage;
    
    // User acknowledgment tracking
    bool m_waitingForAcknowledgment{false};
    bool m_acknowledgedByUser{false};
    std::string m_currentUserInstruction;
};
