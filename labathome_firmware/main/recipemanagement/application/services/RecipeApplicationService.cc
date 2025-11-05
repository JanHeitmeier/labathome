#include "RecipeApplicationService.hh"
#include "../../infrastructure/serialization/JsonSerialization.hh"
#include "../../infrastructure/engine/StepTypeRegistry.hh"
#include <ctime>

RecipeApplicationService::RecipeApplicationService(
    IRecipeStorage* storage,
    RecipeEngine* engine,
    IMessageGateway* gateway
) : m_storage(storage), 
    m_storageManager(new RecipeStorageManager(storage)),
    m_engine(engine), 
    m_gateway(gateway) {
}

RecipeApplicationService::~RecipeApplicationService() {
    delete m_storageManager;
}

void RecipeApplicationService::setMessageGateway(IMessageGateway* gateway) {
    m_gateway = gateway;
}

void RecipeApplicationService::handleCommand(const CommandDto& dto) {
    const std::string& cmd = dto.command;
    
    if (cmd == "start_recipe") {
        handleStartRecipe(dto.recipeId);
    }
    else if (cmd == "stop_recipe") {
        handleStopRecipe();
    }
    else if (cmd == "pause_recipe") {
        handlePauseRecipe();
    }
    else if (cmd == "resume_recipe") {
        handleResumeRecipe();
    }
    else if (cmd == "acknowledge_step") {
        handleAcknowledgeStep();
    }
    else if (cmd == "get_recipe_list") {
        handleGetRecipeList();
    }
    else if (cmd == "get_available_steps") {
        handleGetAvailableSteps();
    }
    else if (cmd == "save_recipe") {
        handleSaveRecipe(dto.payload);
    }
    else if (cmd == "delete_recipe") {
        handleDeleteRecipe(dto.recipeId);
    }
    else if (cmd == "get_recipe") {
        handleGetRecipe(dto.recipeId);
    }
    else if (cmd == "get_metrics") {
        handleGetMetrics();
    }
    else {
        // Unbekannter Befehl - ignorieren oder Fehler senden
    }
}

// ========== Command-Handler ==========

void RecipeApplicationService::handleStartRecipe(const std::string& recipeId) {
    if (!m_engine || !m_storage) return;
    
    // TODO: Rezept aus Storage laden
    // auto recipeOpt = m_storage->loadRecipe(recipeId);
    // if (!recipeOpt) {
    //     // Fehler: Rezept nicht gefunden
    //     return;
    // }
    
    // TODO: Rezept in Engine laden und starten
    // m_engine->loadRecipe(*recipeOpt);
    // m_engine->start();
    
    // Live-View Update senden
    sendLiveViewUpdate();
}

void RecipeApplicationService::handleStopRecipe() {
    if (!m_engine) return;
    
    // TODO: Engine stoppen
    // m_engine->stop();
    
    sendLiveViewUpdate();
}

void RecipeApplicationService::handlePauseRecipe() {
    if (!m_engine) return;
    
    // TODO: Engine pausieren
    // m_engine->pause();
    
    sendLiveViewUpdate();
}

void RecipeApplicationService::handleResumeRecipe() {
    if (!m_engine) return;
    
    // TODO: Engine fortsetzen
    // m_engine->resume();
    
    sendLiveViewUpdate();
}

void RecipeApplicationService::handleAcknowledgeStep() {
    if (!m_engine) return;
    
    m_engine->acknowledgeStep();
    sendLiveViewUpdate();
}

void RecipeApplicationService::handleGetRecipeList() {
    if (!m_gateway) return;
    
    AvailableRecipesDto dto = buildAvailableRecipesDto();
    m_gateway->send(dto);
}

void RecipeApplicationService::handleGetAvailableSteps() {
    if (!m_gateway) return;
    
    // StepTypeRegistry nach DTOs fragen
    AvailableStepsDto dto = StepTypeRegistry::instance().availableTypesAsDto();
    m_gateway->send(dto);
}

void RecipeApplicationService::handleSaveRecipe(const std::string& payloadJson) {
    if (!m_storage || !m_gateway) return;
    
    // Payload deserialisieren
    RecipeDto recipeDto;
    if (!JsonSerialization::deserialize(payloadJson, recipeDto)) {
        // Fehler: Ungültiges JSON
        return;
    }
    
    // TODO: Rezept speichern
    // bool success = m_storage->saveRecipe(recipeDto);
    
    // Erfolg/Fehler an UI senden
    // (TODO: Dediziertes Response-DTO oder Status in CommandDto?)
    
    // Liste aktualisieren
    handleGetRecipeList();
}

void RecipeApplicationService::handleDeleteRecipe(const std::string& recipeId) {
    if (!m_storage || !m_gateway) return;
    
    // TODO: Rezept löschen
    // bool success = m_storage->deleteRecipe(recipeId);
    
    // Liste aktualisieren
    handleGetRecipeList();
}

void RecipeApplicationService::handleGetRecipe(const std::string& recipeId) {
    if (!m_storage || !m_gateway) return;
    
    // TODO: Rezept laden
    // auto recipeOpt = m_storage->loadRecipe(recipeId);
    // if (!recipeOpt) return;
    
    // RecipeDto dto = *recipeOpt;
    // m_gateway->send(dto);
}

void RecipeApplicationService::handleGetMetrics() {
    if (!m_gateway) return;
    
    MetricsDto dto = buildMetricsDto();
    m_gateway->send(dto);
}

// ========== Live-View Update ==========

void RecipeApplicationService::sendLiveViewUpdate() {
    if (!m_gateway) return;
    
    LiveViewDto dto = buildLiveViewDto();
    m_gateway->send(dto);
}

// ========== Builder-Methoden ==========

LiveViewDto RecipeApplicationService::buildLiveViewDto() const {
    LiveViewDto dto;
    
    if (!m_engine) {
        // Engine nicht verfügbar - Idle-Status
        dto.recipeId = "";
        dto.recipeName = "";
        dto.currentStepIndex = 0;
        dto.totalSteps = 0;
        dto.currentStepName = "";
        dto.stepState = "idle";
        dto.recipeStatus = "stopped";
        dto.userInstruction = "";
        dto.awaitingUserAcknowledgment = false;
        dto.progress = 0.0f;
        dto.timestamp = static_cast<uint64_t>(std::time(nullptr)) * 1000;
        dto.errorMessage = "";
        return dto;
    }
    
    // Von RecipeEngine abfragen
    dto.recipeId = m_engine->getRecipeId();
    dto.recipeName = m_engine->getRecipeName();
    dto.currentStepIndex = m_engine->getCurrentStepIndex();
    dto.totalSteps = m_engine->getTotalSteps();
    dto.currentStepName = m_engine->getCurrentStepName();
    
    // Status-Mapping
    RecipeEngineState state = m_engine->getState();
    switch (state) {
        case RecipeEngineState::Idle:
            dto.recipeStatus = "stopped";
            dto.stepState = "idle";
            break;
        case RecipeEngineState::Loaded:
            dto.recipeStatus = "loaded";
            dto.stepState = "inactive";
            break;
        case RecipeEngineState::Running:
            dto.recipeStatus = "running";
            dto.stepState = m_engine->isAwaitingAcknowledgment() ? "waiting_for_user" : "active";
            break;
        case RecipeEngineState::Paused:
            dto.recipeStatus = "paused";
            dto.stepState = "paused";
            break;
        case RecipeEngineState::Error:
            dto.recipeStatus = "error";
            dto.stepState = "error";
            break;
    }
    
    dto.userInstruction = m_engine->getUserInstruction();
    dto.awaitingUserAcknowledgment = m_engine->isAwaitingAcknowledgment();
    dto.progress = m_engine->getProgress();
    dto.timestamp = static_cast<uint64_t>(std::time(nullptr)) * 1000;
    dto.errorMessage = m_engine->getErrorMessage();
    
    // TODO: Sensor-Werte sammeln
    // dto.sensorValues["temperature"] = getSensorValue("temperature");
    // dto.sensorValues["humidity"] = getSensorValue("humidity");
    
    return dto;
}

AvailableRecipesDto RecipeApplicationService::buildAvailableRecipesDto() const {
    AvailableRecipesDto dto;
    
    if (!m_storage) return dto;
    
    // TODO: Rezepte aus Storage laden
    // auto recipeIds = m_storage->getAllRecipeIds();
    // for (const auto& id : recipeIds) {
    //     auto recipeOpt = m_storage->loadRecipeMetadata(id);
    //     if (recipeOpt) {
    //         RecipeInfoDto info;
    //         info.id = recipeOpt->id;
    //         info.name = recipeOpt->name;
    //         info.description = recipeOpt->description;
    //         info.createdAt = recipeOpt->createdAt;
    //         info.lastModified = recipeOpt->lastModified;
    //         dto.recipes.push_back(info);
    //     }
    // }
    
    // Placeholder-Daten
    RecipeInfoDto recipe1;
    recipe1.id = "recipe_001";
    recipe1.name = "Fermentation Process";
    recipe1.description = "Standard fermentation with temperature control";
    recipe1.createdAt = 1699200000000;
    recipe1.lastModified = 1699200000000;
    dto.recipes.push_back(recipe1);
    
    return dto;
}

MetricsDto RecipeApplicationService::buildMetricsDto() const {
    MetricsDto dto;
    
    if (!m_engine) return dto;
    
    // TODO: Von Engine oder Metrics-Service abfragen
    // dto.recipeId = m_engine->getCurrentRecipeId();
    // auto metrics = m_metricsService->getMetrics(dto.recipeId);
    // dto.series = metrics;
    
    // Placeholder
    dto.recipeId = "recipe_123";
    
    // Beispiel-Serie
    MetricSeriesDto tempSeries;
    tempSeries.name = "Temperature";
    tempSeries.unit = "°C";
    
    // Beispiel-Datenpunkte
    for (int i = 0; i < 10; ++i) {
        MetricDataPointDto point;
        point.timestamp = static_cast<uint64_t>(std::time(nullptr) - (10 - i) * 60) * 1000;
        point.value = 20.0f + i * 0.5f;
        tempSeries.data.push_back(point);
    }
    
    dto.series.push_back(tempSeries);
    
    return dto;
}
