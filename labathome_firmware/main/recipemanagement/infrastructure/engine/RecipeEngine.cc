#include "RecipeEngine.hh"
#include <esp_log.h>

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
    
    for (const auto& desc : steps) {
        IStep* step = StepTypeRegistry::instance().createInstance(desc.typeId);
        if (!step) {
            ESP_LOGE(TAG, "Failed to create step with typeId %u", desc.typeId);
            cleanup();
            m_state = RecipeEngineState::Error;
            m_errorMessage = "Failed to create step instance";
            return false;
        }
        
        step->initialize();
        m_stepInstances.push_back(step);
    }
    
    m_currentStepIndex = 0;
    m_elapsedMs = 0;
    m_state = RecipeEngineState::Loaded;
    ESP_LOGI(TAG, "Recipe loaded: id=%s, steps=%u", recipeId.c_str(), (unsigned)steps.size());
    return true;
}

bool RecipeEngine::start() {
    if (m_state != RecipeEngineState::Loaded && m_state != RecipeEngineState::Paused) {
        ESP_LOGW(TAG, "Cannot start: engine not in Loaded/Paused state");
        return false;
    }
    
    if (m_state == RecipeEngineState::Loaded && !m_stepInstances.empty()) {
        IStep* firstStep = m_stepInstances[m_currentStepIndex];
        StepMetadata metadata = firstStep->getMetadata();
        StepContext ctx(metadata, IoResourceManager::instance());
        firstStep->onActivating(ctx);
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
        IStep* currentStep = m_stepInstances[m_currentStepIndex];
        StepMetadata metadata = currentStep->getMetadata();
        StepContext ctx(metadata, IoResourceManager::instance());
        currentStep->onDeactivating(ctx);
        currentStep->onDeactivated(ctx);
    }
    
    for (auto* step : m_stepInstances) {
        delete step;
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
    
    IStep* currentStep = m_stepInstances[m_currentStepIndex];
    
    // StepContext erstellen mit Metadata vom Step
    StepMetadata metadata = currentStep->getMetadata();
    StepContext ctx(metadata, IoResourceManager::instance());
    
    // Wenn User quittiert hat, Context informieren damit isAcknowledged() true zurückgibt
    if (m_acknowledgedByUser) {
        ctx.setAcknowledgedState(true);
    }
    
    currentStep->onActive(ctx);
    
    // Prüfen ob Step Quittierung angefordert hat
    if (ctx.isAwaitingAcknowledgment() && !m_waitingForAcknowledgment) {
        m_waitingForAcknowledgment = true;
        m_currentUserInstruction = ctx.getUserInstruction();
        ESP_LOGI(TAG, "Step requests acknowledgment: %s", m_currentUserInstruction.c_str());
        return; // Engine pausiert automatisch
    }
    
    // Wenn auf Quittierung gewartet wird, nicht weitermachen
    if (m_waitingForAcknowledgment && !m_acknowledgedByUser) {
        return;
    }
    
    if (currentStep->isTransitionConditionMet(ctx)) {
        currentStep->onDeactivating(ctx);
        currentStep->onDeactivated(ctx);
        // Acknowledged-Flag zurücksetzen für nächsten Step
        m_acknowledgedByUser = false;
        advanceToNextStep();
    }
}

void RecipeEngine::advanceToNextStep() {
    m_currentStepIndex++;
    
    if (m_currentStepIndex >= m_stepInstances.size()) {
        ESP_LOGI(TAG, "Recipe completed");
        stop();
    } else {
        IStep* nextStep = m_stepInstances[m_currentStepIndex];
        StepMetadata metadata = nextStep->getMetadata();
        StepContext ctx(metadata, IoResourceManager::instance());
        nextStep->onActivating(ctx);
        ESP_LOGI(TAG, "Advanced to step %u/%u", (unsigned)(m_currentStepIndex + 1), (unsigned)m_stepInstances.size());
    }
}

void RecipeEngine::cleanup() {
    for (auto* step : m_stepInstances) {
        delete step;
    }
    m_stepInstances.clear();
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
