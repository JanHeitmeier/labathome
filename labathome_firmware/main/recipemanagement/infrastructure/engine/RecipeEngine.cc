#include "RecipeEngine.hh"
#include "../../application/services/TimeSeriesRecorder.hh"
#include "../../application/services/RecipeHistoryService.hh"
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
        for (const auto& [key, stringValue] : desc.params) {
            // Hole Referenz zum ParamDef
            ParamDef* paramDef = step->findParamDefPtr(key);
            
            if (paramDef && paramDef->value.isValid()) {
                // Parse String basierend auf Type aus Default-Wert
                ParameterType expectedType = paramDef->value.getType();
                ParameterValue parsedValue = ParameterValue::parseFromString(stringValue, expectedType);
                
                if (parsedValue.isValid()) {
                    // Direkte Zuweisung zur Referenz
                    paramDef->value = parsedValue;
                } else {
                    ESP_LOGW(TAG, "Failed to parse parameter '%s' = '%s'", key.c_str(), stringValue.c_str());
                }
            } else {
                ESP_LOGW(TAG, "Unknown parameter key: %s", key.c_str());
            }
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
    notifyStateChange();
    return true;
}

bool RecipeEngine::start() {
    if (m_state != RecipeEngineState::Loaded && m_state != RecipeEngineState::Paused) {
        ESP_LOGW(TAG, "Cannot start: engine not in Loaded/Paused state");
        return false;
    }
    
    if (m_state == RecipeEngineState::Loaded && m_historyService && m_recorder) {
        m_currentExecutionId = m_historyService->startExecution(m_recipeId, m_recipeName);
        m_recorder->startRecording(m_currentExecutionId, getSensorNames());
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
    notifyStateChange();
    return true;
}

bool RecipeEngine::pause() {
    if (m_state != RecipeEngineState::Running) {
        ESP_LOGW(TAG, "Cannot pause: engine not running");
        return false;
    }
    
    // Notify current step about pause
    if (m_currentStepIndex < m_stepInstances.size() && m_currentStepIndex < m_stepContexts.size()) {
        IStep* currentStep = m_stepInstances[m_currentStepIndex].get();
        StepContext* ctx = m_stepContexts[m_currentStepIndex].get();
        if (currentStep && ctx) {
            currentStep->onPause(*ctx);
            ESP_LOGI(TAG, "Step paused");
        }
    }
    
    m_state = RecipeEngineState::Paused;
    ESP_LOGI(TAG, "Recipe paused");
    notifyStateChange();
    return true;
}

bool RecipeEngine::resume() {
    if (m_state != RecipeEngineState::Paused) {
        ESP_LOGW(TAG, "Cannot resume: engine not paused");
        return false;
    }
    
    // Notify current step about resume
    if (m_currentStepIndex < m_stepInstances.size() && m_currentStepIndex < m_stepContexts.size()) {
        IStep* currentStep = m_stepInstances[m_currentStepIndex].get();
        StepContext* ctx = m_stepContexts[m_currentStepIndex].get();
        if (currentStep && ctx) {
            currentStep->onResume(*ctx);
            ESP_LOGI(TAG, "Step resumed");
        }
    }
    
    m_state = RecipeEngineState::Running;
    ESP_LOGI(TAG, "Recipe resumed");
    notifyStateChange();
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
    
    if (m_recorder) {
        m_recorder->stopRecording();
    }
    if (m_historyService && !m_currentExecutionId.empty()) {
        ExecutionStatus status = (m_state == RecipeEngineState::Error) 
            ? ExecutionStatus::Failed : ExecutionStatus::Aborted;
        m_historyService->endExecution(m_currentExecutionId, status, m_errorMessage);
        m_currentExecutionId.clear();
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
    notifyStateChange();
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
    
    m_elapsedMs += deltaMs;
    
    if (m_recorder && m_recorder->isRecording()) {
        auto sensorData = getSensorValues();
        if (!sensorData.empty()) {
            m_recorder->recordDataPoint(sensorData, m_elapsedMs);
        }
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
    
    // Flag für frische Quittierung
    bool justAcknowledged = false;
    
    // Wenn User quittiert hat, Context informieren damit isAcknowledged() true zurückgibt
    if (m_acknowledgedByUser) {
        ctx->setAcknowledgedState(true);
        justAcknowledged = true;
        // Nach erfolgreicher Quittierung: Engine-Flags zurücksetzen
        m_waitingForAcknowledgment = false;
        m_acknowledgedByUser = false;
        ESP_LOGI(TAG, "Acknowledgment processed, flags reset");
    }
    
    currentStep->onActive(*ctx);
    
    // Prüfen ob Step (erneut) Quittierung angefordert hat
    if (ctx->isAwaitingAcknowledgment() && !m_waitingForAcknowledgment) {
        m_waitingForAcknowledgment = true;
        m_currentUserInstruction = ctx->getUserInstruction();
        notifyStateChange();
        ESP_LOGI(TAG, "Step requests acknowledgment: %s", m_currentUserInstruction.c_str());
        return; // Engine pausiert automatisch
    }
    
    // Wenn auf Quittierung gewartet wird, nicht weitermachen
    // AUSNAHME: Wenn gerade frisch quittiert wurde, dann weitermachen zur Transition-Prüfung
    if (m_waitingForAcknowledgment && !justAcknowledged) {
        return;
    }
    
    if (currentStep->isTransitionConditionMet(*ctx)) {
        ESP_LOGI(TAG, "<< Step %zu/%zu completed: typeId=%lu", m_currentStepIndex + 1, m_stepInstances.size(), (unsigned long)metadata.typeId);
        currentStep->onDeactivating(*ctx);
        currentStep->onDeactivated(*ctx);
        advanceToNextStep();
    }
}

void RecipeEngine::advanceToNextStep() {
    m_currentStepIndex++;
    notifyStateChange();
    
    if (m_currentStepIndex >= m_stepInstances.size()) {
        ESP_LOGI(TAG, "Recipe completed - all steps finished");
        
        if (m_recorder) {
            m_recorder->stopRecording();
        }
        if (m_historyService && !m_currentExecutionId.empty()) {
            m_historyService->endExecution(m_currentExecutionId, ExecutionStatus::Completed);
            m_currentExecutionId.clear();
        }
        
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
    
    // If all steps completed, return 100%
    if (m_currentStepIndex >= m_stepInstances.size()) {
        return 1.0f;
    }
    
    return (static_cast<float>(m_currentStepIndex) + 0.5f) / static_cast<float>(m_stepInstances.size());
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
    notifyStateChange();
    
    // Im nächsten tick() wird ctx.setAcknowledgedState(true) gesetzt
    // und der Step kann dann isAcknowledged() prüfen
}

std::map<std::string, float> RecipeEngine::getSensorValues() const {
    std::map<std::string, float> sensorValues;
    
    // Only collect if we have a running step
    if (m_state != RecipeEngineState::Running && m_state != RecipeEngineState::Paused) {
        ESP_LOGD(TAG, "getSensorValues: Not running or paused, state=%d", static_cast<int>(m_state.load()));
        return sensorValues;
    }
    
    if (m_currentStepIndex >= m_stepInstances.size() || m_currentStepIndex >= m_stepContexts.size()) {
        ESP_LOGD(TAG, "getSensorValues: Invalid step index");
        return sensorValues;
    }
    
    IStep* currentStep = m_stepInstances[m_currentStepIndex].get();
    StepContext* ctx = m_stepContexts[m_currentStepIndex].get();
    
    if (!currentStep || !ctx) {
        ESP_LOGD(TAG, "getSensorValues: Null step or context");
        return sensorValues;
    }
    
    // Get all input aliases from the step
    auto aliasPointers = currentStep->getIoAliasPointers();
    ESP_LOGI(TAG, "getSensorValues: Checking %zu aliases", aliasPointers.size());
    
    for (const IoAliasDef* aliasPtr : aliasPointers) {
        if (!aliasPtr) continue;
        
        ESP_LOGI(TAG, "  Alias: %s, isInput=%d, physicalName=%s", 
                 aliasPtr->aliasName.c_str(), aliasPtr->isInput, aliasPtr->physicalName.c_str());
        
        if (!aliasPtr->isInput) continue;
        
        // Try to read the input value
        auto input = ctx->getInput(aliasPtr->aliasName);
        if (!input) {
            ESP_LOGW(TAG, "  Input not found for alias: %s", aliasPtr->aliasName.c_str());
            continue;
        }
        
        ParameterValue val = input->read();
        ESP_LOGI(TAG, "  Read value type: %s", val.getTypeName());
        
        // Convert value to float based on type
        if (val.isType(ParameterType::BOOLEAN)) {
            float floatVal = val.getBoolean() ? 1.0f : 0.0f;
            sensorValues[aliasPtr->aliasName] = floatVal;
            ESP_LOGI(TAG, "  Added BOOLEAN sensor '%s' = %.1f", aliasPtr->aliasName.c_str(), floatVal);
        } else if (val.isType(ParameterType::GENERIC_INT)) {
            sensorValues[aliasPtr->aliasName] = static_cast<float>(val.getGenericInt());
            ESP_LOGI(TAG, "  Added INT sensor '%s'", aliasPtr->aliasName.c_str());
        } else if (val.isType(ParameterType::TIME_MILLISECONDS)) {
            sensorValues[aliasPtr->aliasName] = static_cast<float>(val.getTimeMilliseconds());
            ESP_LOGI(TAG, "  Added TIME_MS sensor '%s'", aliasPtr->aliasName.c_str());
        } else if (val.isType(ParameterType::PERCENTAGE)) {
            sensorValues[aliasPtr->aliasName] = val.getPercentage();
            ESP_LOGI(TAG, "  Added PERCENTAGE sensor '%s'", aliasPtr->aliasName.c_str());
        } else if (val.isType(ParameterType::TEMPERATURE)) {
            sensorValues[aliasPtr->aliasName] = val.getTemperature(TemperatureUnit::CELSIUS);
            ESP_LOGI(TAG, "  Added TEMPERATURE sensor '%s'", aliasPtr->aliasName.c_str());
        } else {
            ESP_LOGW(TAG, "  Unsupported sensor type for '%s'", aliasPtr->aliasName.c_str());
        }
    }
    
    ESP_LOGI(TAG, "getSensorValues: Returning %zu sensor values", sensorValues.size());
    return sensorValues;
}

std::vector<std::string> RecipeEngine::getSensorNames() const {
    std::vector<std::string> names;
    
    if (m_currentStepIndex >= m_stepContexts.size()) {
        return names;
    }
    
    const StepContext* ctx = m_stepContexts[m_currentStepIndex].get();
    if (!ctx) {
        return names;
    }
    
    const auto& aliasPointers = ctx->getMetadata().ioAliases;
    for (const auto& aliasPtr : aliasPointers) {
        if (aliasPtr.isInput && aliasPtr.isSensor) {
            names.push_back(aliasPtr.aliasName);
        }
    }
    
    return names;
}
