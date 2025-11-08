#include "RecipeApplicationService.hh"
#include "../../infrastructure/serialization/JsonSerialization.hh"
#include "../../infrastructure/engine/StepTypeRegistry.hh"
#include "../../infrastructure/parsers/RecipeParser.hh"
#include "../../core/domain/entities/Recipe.hh"
#include <esp_log.h>
#include <ctime>

//ToDos von Ki Claude Sonnect 4.5 generiert

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
    
    ESP_LOGE("RecipeAppService", "========== CHECKPOINT 10: handleCommand called ==========");
    
    if (cmd == "start_recipe") {
        ESP_LOGE("RecipeAppService", "CHECKPOINT 11: Command is start_recipe");
        // Wenn payload vorhanden, ist es ein komplettes JSON-Rezept zum Speichern und Starten
        if (!dto.payload.empty()) {
            ESP_LOGE("RecipeAppService", "CHECKPOINT 12: Payload available, calling handleStartRecipeFromJson");
            handleStartRecipeFromJson(dto.payload);
        } else {
            ESP_LOGE("RecipeAppService", "CHECKPOINT 13: No payload, using recipeId");
            // Ansonsten: recipeId aus Storage laden
            handleStartRecipe(dto.recipeId);
        }
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

// Command-Handlers

void RecipeApplicationService::handleStartRecipeFromJson(const std::string& jsonRecipe) {
    ESP_LOGE("RecipeAppService", "========== CHECKPOINT 14: handleStartRecipeFromJson START ==========");
    
    if (!m_engine || !m_storageManager) {
        ESP_LOGE("RecipeAppService", "ERROR CHECKPOINT 15: Engine or StorageManager is null");
        return;
    }
    
    ESP_LOGE("RecipeAppService", "CHECKPOINT 16: Engine and StorageManager OK");
    
    // 1. Parse JSON to extract recipe ID
    ESP_LOGE("RecipeAppService", "CHECKPOINT 17: Creating RecipeParser");
    RecipeParser parser;
    Recipe recipe;
    
    ESP_LOGE("RecipeAppService", "CHECKPOINT 18: Calling parseJsonToRecipe");
    if (!parser.parseJsonToRecipe(jsonRecipe, recipe)) {
        ESP_LOGE("RecipeAppService", "ERROR CHECKPOINT 19: Failed to parse recipe JSON");
        return;
    }
    ESP_LOGE("RecipeAppService", "CHECKPOINT 20: Successfully parsed recipe");
    
    // 2. Save recipe JSON using storage manager
    ESP_LOGE("RecipeAppService", "CHECKPOINT 21: Saving recipe via storage");
    // Convert recipe string ID to uint32_t hash for storage
    uint32_t recipeIdHash = 0;
    for (char c : recipe.id()) {
        recipeIdHash = recipeIdHash * 31 + static_cast<uint32_t>(c);
    }
    if (!m_storageManager->updateJsonRecipe(recipeIdHash, jsonRecipe)) {
        ESP_LOGE("RecipeAppService", "ERROR CHECKPOINT 23: Failed to save recipe");
    } else {
        ESP_LOGE("RecipeAppService", "CHECKPOINT 22: Recipe saved successfully");
    }
    
    // 3. Parse to StepInstanceDescriptors
    ESP_LOGE("RecipeAppService", "CHECKPOINT 24: Parsing JSON to StepInstanceDescriptors");
    std::vector<StepInstanceDescriptor> steps;
    if (!parser.parseJsonToStepDescriptors(jsonRecipe, steps)) {
        ESP_LOGE("RecipeAppService", "ERROR CHECKPOINT 25: Failed to parse steps from JSON");
        return;
    }
    ESP_LOGE("RecipeAppService", "CHECKPOINT 26: Successfully parsed steps");
    
    // 4. Load into engine and start
    ESP_LOGE("RecipeAppService", "CHECKPOINT 27: Loading recipe into engine");
    if (!m_engine->loadRecipe(steps, recipe.id())) {
        ESP_LOGE("RecipeAppService", "ERROR CHECKPOINT 28: Failed to load recipe into engine");
        return;
    }
    ESP_LOGE("RecipeAppService", "CHECKPOINT 29: Recipe loaded successfully");
    
    ESP_LOGE("RecipeAppService", "CHECKPOINT 30: Starting recipe execution");
    if (!m_engine->start()) {
        ESP_LOGE("RecipeAppService", "ERROR CHECKPOINT 31: Failed to start recipe execution");
        return;
    }
    
    ESP_LOGE("RecipeAppService", "CHECKPOINT 32: Recipe started successfully");
    ESP_LOGE("RecipeAppService", "========== CHECKPOINT 33: handleStartRecipeFromJson END ==========");
    sendLiveViewUpdate();
}

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
