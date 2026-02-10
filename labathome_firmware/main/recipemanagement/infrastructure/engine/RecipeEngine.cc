#include "RecipeEngine.hh"
#include "../../application/services/StorageManager.hh"
#include "../../application/services/RecipeHistoryService.hh"

RecipeEngine::RecipeEngine() {
    m_state = RecipeEngineState::Idle;
}

RecipeEngine::~RecipeEngine() {
    cleanup();
}

bool RecipeEngine::loadRecipe(const std::vector<StepInstanceDescriptor>& steps, const std::string& recipeId, const std::string& recipeName) {
    if (m_state == RecipeEngineState::Running || m_state == RecipeEngineState::Paused) {
        return false;
    }
    
    cleanup();
    
    m_recipeId = recipeId;
    m_recipeName = recipeName;
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
            cleanup();
            m_state = RecipeEngineState::Error;
            m_errorMessage = "Failed to create step instance";
            return false;
        }
        
        step->initialize();
        
        auto aliasPointers = step->getIoAliasPointers();
        for (IoAliasDef* aliasPtr : aliasPointers) {
            if (!aliasPtr) continue;
            auto it = desc.aliases.find(aliasPtr->aliasName);
            if (it != desc.aliases.end()) {
                aliasPtr->physicalName = it->second;
            }
        }
        
        for (const auto& [key, stringValue] : desc.params) {
            ParamDef* paramDef = step->findParamDefPtr(key);
            
            if (paramDef && paramDef->value.isValid()) {
                ParameterType expectedType = paramDef->value.getType();
                ParameterValue parsedValue = ParameterValue::parseFromString(stringValue, expectedType);
                
                if (parsedValue.isValid()) {
                    paramDef->value = parsedValue;
                }
            }
        }
        
        m_stepInstances.push_back(std::move(step));
        
        StepMetadata metadata = m_stepInstances.back()->getMetadata();
        m_stepContexts.push_back(std::make_unique<StepContext>(metadata, IoResourceManager::instance()));
    }
    
    m_currentStepIndex = 0;
    m_elapsedMs = 0;
    m_state = RecipeEngineState::Loaded;
    notifyStateChange();
    return true;
}

void RecipeEngine::setGlobalParameters(const std::map<std::string, std::string>& params) {
    m_globalParameters = params;
}

bool RecipeEngine::start() {
    if (m_state != RecipeEngineState::Loaded && m_state != RecipeEngineState::Paused) {
        return false;
    }
    
    if (m_state == RecipeEngineState::Loaded && m_historyService && m_storageManager) {
        m_currentExecutionId = m_historyService->startExecution(m_recipeId, m_recipeName, m_globalParameters);
        std::vector<std::pair<std::string, std::string>> sensorInfo = getSensorInfo();
        m_storageManager->startRecording(m_currentExecutionId, sensorInfo);
    }
    
    if (m_state == RecipeEngineState::Loaded && !m_stepInstances.empty()) {
        IStep* firstStep = m_stepInstances[m_currentStepIndex].get();
        StepContext* ctx = m_stepContexts[m_currentStepIndex].get();
        firstStep->onActivating(*ctx);
    }
    
    m_state = RecipeEngineState::Running;
    notifyStateChange();
    return true;
}

bool RecipeEngine::pause() {
    if (m_state != RecipeEngineState::Running) {
        return false;
    }
    
    if (m_currentStepIndex < m_stepInstances.size() && m_currentStepIndex < m_stepContexts.size()) {
        IStep* currentStep = m_stepInstances[m_currentStepIndex].get();
        StepContext* ctx = m_stepContexts[m_currentStepIndex].get();
        if (currentStep && ctx) {
            currentStep->onPause(*ctx);
        }
    }
    
    m_state = RecipeEngineState::Paused;
    notifyStateChange();
    return true;
}

bool RecipeEngine::resume() {
    if (m_state != RecipeEngineState::Paused) {
        return false;
    }
    
    if (m_currentStepIndex < m_stepInstances.size() && m_currentStepIndex < m_stepContexts.size()) {
        IStep* currentStep = m_stepInstances[m_currentStepIndex].get();
        StepContext* ctx = m_stepContexts[m_currentStepIndex].get();
        if (currentStep && ctx) {
            currentStep->onResume(*ctx);
        }
    }
    
    m_state = RecipeEngineState::Running;
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
    
    if (m_storageManager) {
        m_storageManager->stopRecording();
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
    
    if (m_storageManager && m_storageManager->isRecording()) {
        auto sensorData = getSensorValues();
        if (!sensorData.empty()) {
            m_storageManager->recordDataPoint(sensorData, m_elapsedMs);
        }
    }
    
    executeCurrentStep(deltaMs);
}

void RecipeEngine::buildStepContext(StepContext& ctx, size_t stepIndex) {
    (void)ctx;
    (void)stepIndex;
}

void RecipeEngine::executeCurrentStep(uint32_t deltaMs) {
    (void)deltaMs;
    
    IStep* currentStep = m_stepInstances[m_currentStepIndex].get();
    StepContext* ctx = m_stepContexts[m_currentStepIndex].get();
    StepMetadata metadata = currentStep->getMetadata();
    
    bool justAcknowledged = false;
    
    if (m_acknowledgedByUser) {
        ctx->setAcknowledgedState(true);
        justAcknowledged = true;
        m_waitingForAcknowledgment = false;
        m_acknowledgedByUser = false;
    }
    
    currentStep->onActive(*ctx);
    
    if (ctx->isAwaitingAcknowledgment() && !m_waitingForAcknowledgment) {
        m_waitingForAcknowledgment = true;
        m_currentUserInstruction = ctx->getUserInstruction();
        notifyStateChange();
        return;
    }
    
    if (m_waitingForAcknowledgment && !justAcknowledged) {
        return;
    }
    
    if (currentStep->isTransitionConditionMet(*ctx)) {
        currentStep->onDeactivating(*ctx);
        currentStep->onDeactivated(*ctx);
        advanceToNextStep();
    }
}

void RecipeEngine::advanceToNextStep() {
    m_currentStepIndex++;
    notifyStateChange();
    
    if (m_currentStepIndex >= m_stepInstances.size()) {
        if (m_historyService && !m_currentExecutionId.empty()) {
            m_historyService->endExecution(m_currentExecutionId, ExecutionStatus::Completed);
            m_currentExecutionId.clear();
        }
        
        stop();
    } else {
        IStep* nextStep = m_stepInstances[m_currentStepIndex].get();
        StepContext* ctx = m_stepContexts[m_currentStepIndex].get();
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
    
    if (m_currentStepIndex >= m_stepInstances.size()) {
        return 1.0f;
    }
    
    return (static_cast<float>(m_currentStepIndex) + 0.5f) / static_cast<float>(m_stepInstances.size());
}

void RecipeEngine::acknowledgeStep() {
    if (!m_waitingForAcknowledgment) {
        return;
    }
    
    m_waitingForAcknowledgment = false;
    m_acknowledgedByUser = true;
    m_currentUserInstruction.clear();
    notifyStateChange();
}

std::map<std::string, float> RecipeEngine::getSensorValues() const {
    std::map<std::string, float> sensorValues;
    
    if (m_state != RecipeEngineState::Running && m_state != RecipeEngineState::Paused) {
        return sensorValues;
    }
    
    if (m_currentStepIndex >= m_stepInstances.size() || m_currentStepIndex >= m_stepContexts.size()) {
        return sensorValues;
    }
    
    IStep* currentStep = m_stepInstances[m_currentStepIndex].get();
    StepContext* ctx = m_stepContexts[m_currentStepIndex].get();
    
    if (!currentStep || !ctx) {
        return sensorValues;
    }
    
    auto aliasPointers = currentStep->getIoAliasPointers();
    
    for (const IoAliasDef* aliasPtr : aliasPointers) {
        if (!aliasPtr || !aliasPtr->isInput) continue;
        
        auto input = ctx->getInput(aliasPtr->aliasName);
        if (!input) continue;
        
        ParameterValue val = input->read();
        if (val.isValid()) {
            sensorValues[aliasPtr->aliasName] = val.toFloat();
        }
    }
    
    return sensorValues;
}

std::vector<std::string> RecipeEngine::getSensorNames() const {
    std::vector<std::string> names;
    
    for (size_t i = 0; i < m_stepInstances.size(); i++) {
        IStep* step = m_stepInstances[i].get();
        if (!step) continue;
        
        StepMetadata metadata = step->getMetadata();
        for (const auto& ioAlias : metadata.ioAliases) {
            if (ioAlias.isInput && ioAlias.isSensor) {
                if (std::find(names.begin(), names.end(), ioAlias.aliasName) == names.end()) {
                    names.push_back(ioAlias.aliasName);
                }
            }
        }
    }
    
    return names;
}

std::vector<std::pair<std::string, std::string>> RecipeEngine::getSensorInfo() const {
    std::vector<std::pair<std::string, std::string>> sensorInfo;
    
    for (size_t i = 0; i < m_stepInstances.size(); i++) {
        IStep* step = m_stepInstances[i].get();
        if (!step) continue;
        
        StepMetadata metadata = step->getMetadata();
        for (const auto& ioAlias : metadata.ioAliases) {
            if (ioAlias.isInput && ioAlias.isSensor) {
                // Check if sensor already in list
                bool found = false;
                for (const auto& info : sensorInfo) {
                    if (info.first == ioAlias.aliasName) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    sensorInfo.emplace_back(ioAlias.aliasName, ioAlias.unit);
                }
            }
        }
    }
    
    return sensorInfo;
}
