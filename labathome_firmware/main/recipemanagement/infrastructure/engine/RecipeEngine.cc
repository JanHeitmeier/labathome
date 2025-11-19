#include "RecipeEngine.hh"
#include <esp_log.h>
#include <inttypes.h>

static const char* TAG = "RecipeEngine";

RecipeEngine::RecipeEngine() {
    m_state = RecipeEngineState::Idle;
}

RecipeEngine::~RecipeEngine() {
    cleanup();
}

bool RecipeEngine::loadRecipe(const std::vector<StepInstanceDescriptor>& steps, const std::string& recipeId) {
    if (m_state == RecipeEngineState::Running || m_state == RecipeEngineState::Paused) {
        ESP_LOGW(TAG, "Cannot load recipe while engine is running");
        return false;
    }
    
    cleanup();
    
    m_recipeId = recipeId;
    m_stepDescriptors = steps;
    m_stepInstances.clear();
    m_stepInstances.reserve(steps.size());
    m_stepContexts.clear();
    m_stepContexts.reserve(steps.size());
    
    auto& registry = StepTypeRegistry::instance();
    
    for (size_t i = 0; i < steps.size(); ++i) {
        const auto& desc = steps[i];
        std::unique_ptr<IStep> step = registry.createInstance(desc.typeId);
        if (!step) {
            ESP_LOGE(TAG, "Failed to create step with typeId");
            cleanup();
            m_state = RecipeEngineState::Error;
            m_errorMessage = "Failed to create step instance";
            return false;
        }
        
        // Initialize the step, das wird benötgit damit alle Steps in passender ausgangslage kommen
        step->initialize();
        
        // Apply aliases from descriptor to step
        auto aliasPointers = step->getIoAliasPointers();
        for (IoAliasDef* aliasPtr : aliasPointers) {
            if (!aliasPtr) continue;
            // Find physical name for this alias in descriptor
            auto it = desc.aliases.find(aliasPtr->aliasName);
            if (it != desc.aliases.end()) {
                aliasPtr->physicalName = it->second;
                std::string msg = "Applied alias: " + aliasPtr->aliasName + " -> " + aliasPtr->physicalName;
                ESP_LOGI(TAG, "%s", msg.c_str());
            }
        }
        
        // Apply parameters from descriptor to step
        for (const auto& [key, value] : desc.params) {
            step->setParamValue(key, value);
        }
        
        m_stepInstances.push_back(std::move(step));
        
        // Create persistent context for this step
        StepMetadata metadata = m_stepInstances.back()->getMetadata();
        m_stepContexts.push_back(std::make_unique<StepContext>(metadata, IoResourceManager::instance()));
    }
    
    m_currentStepIndex = 0;
    m_elapsedMs = 0;
    m_state = RecipeEngineState::Loaded;
    ESP_LOGI(TAG, "Recipe loaded: id=%s, steps=%zu", recipeId.c_str(), steps.size());
    return true;
}

bool RecipeEngine::start() {
    if (m_state != RecipeEngineState::Loaded && m_state != RecipeEngineState::Paused) {
        ESP_LOGW(TAG, "Cannot start: engine not in Loaded/Paused state");
        return false;
    }
    
    if (m_state == RecipeEngineState::Loaded && !m_stepInstances.empty()) {
        IStep* firstStep = m_stepInstances[m_currentStepIndex].get();
        StepContext* ctx = m_stepContexts[m_currentStepIndex].get();
        StepMetadata metadata = firstStep->getMetadata();
        ESP_LOGI(TAG, ">> Starting Step 1/%zu: typeId=%lu name=%s", m_stepInstances.size(), (unsigned long)metadata.typeId, metadata.displayName.c_str());
        firstStep->onActivating(*ctx);
    }
    
    m_state = RecipeEngineState::Running;
    ESP_LOGI(TAG, "Recipe started");
    return true;
}

bool RecipeEngine::pause() {
    if (m_state != RecipeEngineState::Running) {
        ESP_LOGW(TAG, "Cannot pause: engine not running");
        return false;
    }
    
    m_state = RecipeEngineState::Paused;
    ESP_LOGI(TAG, "Recipe paused");
    return true;
}

bool RecipeEngine::resume() {
    if (m_state != RecipeEngineState::Paused) {
        ESP_LOGW(TAG, "Cannot resume: engine not paused");
        return false;
    }
    
    m_state = RecipeEngineState::Running;
    ESP_LOGI(TAG, "Recipe resumed");
    return true;
}

bool RecipeEngine::stop() {
    if (m_state == RecipeEngineState::Idle) {
        return true;
    }
    
    if (m_state == RecipeEngineState::Running && m_currentStepIndex < m_stepInstances.size()) {
        IStep* currentStep = m_stepInstances[m_currentStepIndex].get();
        StepMetadata metadata = currentStep->getMetadata();
        StepContext ctx(metadata, IoResourceManager::instance());
        currentStep->onDeactivating(ctx);
        currentStep->onDeactivated(ctx);
    }
    
    m_stepInstances.clear();
    m_stepDescriptors.clear();
    m_currentStepIndex = 0;
    m_elapsedMs = 0;
    m_waitingForAcknowledgment = false;
    m_acknowledgedByUser = false;
    m_currentUserInstruction.clear();
    m_state = RecipeEngineState::Idle;
    ESP_LOGI(TAG, "Recipe stopped");
    return true;
}

void RecipeEngine::tick(uint32_t deltaMs) {
    if (m_state != RecipeEngineState::Running) {
        return;
    }
    
    if (m_stepInstances.empty() || m_currentStepIndex >= m_stepInstances.size()) {
        stop();
        return;
    }
    
    executeCurrentStep(deltaMs);
}

void RecipeEngine::buildStepContext(StepContext& ctx, size_t stepIndex) {
    // NOTE: StepContext now gets metadata from IStep::getMetadata() in constructor
    // This method is kept for backward compatibility but may not be needed anymore
    (void)ctx;
    (void)stepIndex;
}

void RecipeEngine::executeCurrentStep(uint32_t deltaMs) {
    (void)deltaMs;
    
    IStep* currentStep = m_stepInstances[m_currentStepIndex].get();
    StepContext* ctx = m_stepContexts[m_currentStepIndex].get();
    StepMetadata metadata = currentStep->getMetadata();
    
    // Wenn User quittiert hat, Context informieren damit isAcknowledged() true zurückgibt
    if (m_acknowledgedByUser) {
        ctx->setAcknowledgedState(true);
    }
    
    currentStep->onActive(*ctx);
    
    // Prüfen ob Step Quittierung angefordert hat
    if (ctx->isAwaitingAcknowledgment() && !m_waitingForAcknowledgment) {
        m_waitingForAcknowledgment = true;
        m_currentUserInstruction = ctx->getUserInstruction();
        ESP_LOGI(TAG, "Step requests acknowledgment: %s", m_currentUserInstruction.c_str());
        return; // Engine pausiert automatisch
    }
    
    // Wenn auf Quittierung gewartet wird, nicht weitermachen
    if (m_waitingForAcknowledgment && !m_acknowledgedByUser) {
        return;
    }
    
    if (currentStep->isTransitionConditionMet(*ctx)) {
        ESP_LOGI(TAG, "<< Step %zu/%zu completed: typeId=%lu", m_currentStepIndex + 1, m_stepInstances.size(), (unsigned long)metadata.typeId);
        currentStep->onDeactivating(*ctx);
        currentStep->onDeactivated(*ctx);
        // Acknowledged-Flag zurücksetzen für nächsten Step
        m_acknowledgedByUser = false;
        advanceToNextStep();
    }
}

void RecipeEngine::advanceToNextStep() {
    m_currentStepIndex++;
    
    if (m_currentStepIndex >= m_stepInstances.size()) {
        ESP_LOGI(TAG, "Recipe completed - all steps finished");
        stop();
    } else {
        IStep* nextStep = m_stepInstances[m_currentStepIndex].get();
        StepContext* ctx = m_stepContexts[m_currentStepIndex].get();
        StepMetadata metadata = nextStep->getMetadata();
        ESP_LOGI(TAG, ">> Starting Step %zu/%zu: typeId=%lu name=%s", m_currentStepIndex + 1, m_stepInstances.size(), (unsigned long)metadata.typeId, metadata.displayName.c_str());
        nextStep->onActivating(*ctx);
    }
}

void RecipeEngine::cleanup() {
    m_stepInstances.clear();
    m_stepContexts.clear();
    m_stepDescriptors.clear();
}

std::string RecipeEngine::getCurrentStepName() const {
    if (m_currentStepIndex < m_stepDescriptors.size()) {
        return m_stepDescriptors[m_currentStepIndex].systemId;
    }
    return "";
}

float RecipeEngine::getProgress() const {
    if (m_stepInstances.empty()) return 0.0f;
    return static_cast<float>(m_currentStepIndex) / static_cast<float>(m_stepInstances.size());
}

void RecipeEngine::acknowledgeStep() {
    if (!m_waitingForAcknowledgment) {
        ESP_LOGW(TAG, "acknowledgeStep called but not waiting for acknowledgment");
        return;
    }
    
    ESP_LOGI(TAG, "Step acknowledged by user");
    m_waitingForAcknowledgment = false;
    m_acknowledgedByUser = true;
    m_currentUserInstruction.clear();
    
    // Im nächsten tick() wird ctx.setAcknowledgedState(true) gesetzt
    // und der Step kann dann isAcknowledged() prüfen
}
