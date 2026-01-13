#include "RecipeApplicationService.hh"
#include "../../infrastructure/serialization/JsonSerialization.hh"
#include "../../infrastructure/engine/StepTypeRegistry.hh"
#include "../../infrastructure/parsers/RecipeParser.hh"
#include "../../core/domain/entities/Recipe.hh"
#include <esp_log.h>
#include <ctime>
#include <cinttypes>

RecipeApplicationService::RecipeApplicationService(
    IRecipeStorage* storage,
    RecipeEngine* engine,
    IMessageGateway* gateway
) : m_storage(storage), 
    m_storageManager(new RecipeStorageManager(storage)),
    m_engine(engine), 
    m_gateway(gateway) {
    
    // Register state change callback to send live view updates
    if (m_engine) {
        m_engine->setStateChangeCallback([this]() {
            this->sendLiveViewUpdate();
        });
    }
}

RecipeApplicationService::~RecipeApplicationService() {
    delete m_storageManager;
}

void RecipeApplicationService::setMessageGateway(IMessageGateway* gateway) {
    m_gateway = gateway;
}

void RecipeApplicationService::handleCommand(const CommandDto& dto) {
    const std::string& cmd = dto.command;
    
    ESP_LOGI("RecipeAppService", "handleCommand: %s", cmd.c_str());
    
    if (cmd == "start_recipe") {
        // Wenn payload vorhanden, ist es ein komplettes JSON-Rezept zum Speichern und Starten
        if (!dto.payload.empty()) {
            handleStartRecipeFromJson(dto.payload);
        } else {
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
    if (!m_engine || !m_storageManager) {
        ESP_LOGE("RecipeAppService", "Engine or StorageManager is null");
        return;
    }
    // 1. Parse JSON to extract recipe ID
    RecipeParser parser;
    Recipe recipe;
    
    if (!parser.parseJsonToRecipe(jsonRecipe, recipe)) {
        ESP_LOGE("RecipeAppService", "Failed to parse recipe JSON");
        return;
    }
    
    // 2. Save recipe JSON using storage manager
    // Convert recipe string ID to uint32_t hash for storage
    uint32_t recipeIdHash = 0;
    for (char c : recipe.id()) {
        recipeIdHash = recipeIdHash * 31 + static_cast<uint32_t>(c);
    }
    if (!m_storageManager->updateJsonRecipe(recipeIdHash, jsonRecipe)) {
        ESP_LOGE("RecipeAppService", "Failed to save recipe");
    }
    
    // 3. Parse to StepInstanceDescriptors
    std::vector<StepInstanceDescriptor> steps;
    if (!parser.parseJsonToStepDescriptors(jsonRecipe, steps)) {
        ESP_LOGE("RecipeAppService", "Failed to parse steps from JSON");
        return;
    }
    
    // 4. Load into engine and start
    if (!m_engine->loadRecipe(steps, recipe.id())) {
        ESP_LOGE("RecipeAppService", "Failed to load recipe into engine");
        return;
    }
    
    if (!m_engine->start()) {
        ESP_LOGE("RecipeAppService", "Failed to start recipe execution");
        return;
    }
    
    sendLiveViewUpdate();
}

void RecipeApplicationService::handleStartRecipe(const std::string& recipeId) {
    ESP_LOGI("RecipeAppService", "handleStartRecipe called for ID: %s", recipeId.c_str());
    
    if (!m_storageManager || !m_engine) {
        ESP_LOGE("RecipeAppService", "StorageManager or Engine is NULL!");
        return;
    }
    
    // Convert string ID to uint32_t hash for storage lookup
    uint32_t recipeIdHash = 0;
    for (char c : recipeId) {
        recipeIdHash = recipeIdHash * 31 + static_cast<uint32_t>(c);
    }
    
    ESP_LOGI("RecipeAppService", "Looking for recipe with hash: 0x%08" PRIx32, recipeIdHash);
    
    // Load recipe JSON from storage
    auto recipeJsonOpt = m_storageManager->getJsonRecipe(recipeIdHash);
    if (!recipeJsonOpt.has_value()) {
        ESP_LOGE("RecipeAppService", "Recipe not found in storage: %s (hash: 0x%08" PRIx32 ")", recipeId.c_str(), recipeIdHash);
        return;
    }
    
    ESP_LOGI("RecipeAppService", "Recipe JSON loaded, parsing to StepDescriptors...");
    
    // Parse JSON to StepInstanceDescriptor list
    std::vector<StepInstanceDescriptor> stepDescriptors;
    RecipeParser parser;
    if (!parser.parseJsonToStepDescriptors(recipeJsonOpt.value(), stepDescriptors)) {
        ESP_LOGE("RecipeAppService", "Failed to parse recipe JSON to StepDescriptors");
        return;
    }
    
    ESP_LOGI("RecipeAppService", "Parsed %d steps, loading into RecipeEngine...", stepDescriptors.size());
    
    // Load recipe into engine
    if (!m_engine->loadRecipe(stepDescriptors, recipeId)) {
        ESP_LOGE("RecipeAppService", "Failed to load recipe into engine");
        return;
    }
    
    ESP_LOGI("RecipeAppService", "Recipe loaded, starting execution...");
    
    // Start recipe execution
    if (!m_engine->start()) {
        ESP_LOGE("RecipeAppService", "Failed to start recipe execution");
        return;
    }
    
    ESP_LOGI("RecipeAppService", "✅ Recipe started successfully: %s", recipeId.c_str());
    
    // Send live-view update to show running state
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
    
    //sendLiveViewUpdate();
}

void RecipeApplicationService::handleAcknowledgeStep() {
    if (!m_engine) return;
    
    m_engine->acknowledgeStep();
    //sendLiveViewUpdate();
}

void RecipeApplicationService::handleGetRecipeList() {
    ESP_LOGI("RecipeAppService", "handleGetRecipeList called");
    
    if (!m_gateway) {
        ESP_LOGE("RecipeAppService", "Gateway is NULL!");
        return;
    }
    
    AvailableRecipesDto dto = buildAvailableRecipesDto();
    ESP_LOGI("RecipeAppService", "Built DTO with %d recipes, sending...", dto.recipes.size());
    m_gateway->send(dto);
    ESP_LOGI("RecipeAppService", "AvailableRecipesDto sent");
}

void RecipeApplicationService::handleGetAvailableSteps() {
    ESP_LOGI("RecipeAppService", "handleGetAvailableSteps called");
    
    if (!m_gateway) {
        ESP_LOGE("RecipeAppService", "Gateway is NULL!");
        return;
    }
    
    ESP_LOGI("RecipeAppService", "Building AvailableStepsDto...");
    AvailableStepsDto dto = buildAvailableStepsDto();
    
    ESP_LOGI("RecipeAppService", "Built DTO with %d steps, sending...", dto.steps.size());
    m_gateway->send(dto);
    
    ESP_LOGI("RecipeAppService", "AvailableStepsDto sent");
}

void RecipeApplicationService::handleSaveRecipe(const std::string& payloadJson) {
    if (!m_storageManager || !m_gateway) return;
    
    // Payload deserialisieren
    RecipeDto recipeDto;
    if (!JsonSerialization::deserialize(payloadJson, recipeDto)) {
        ESP_LOGE("RecipeAppService", "Failed to deserialize RecipeDto");
        return;
    }
    
    // Rezept als JSON mit String-ID Hash speichern (für Frontend-Kompatibilität)
    bool success = m_storageManager->saveJsonRecipeWithStringId(payloadJson);
    
    if (success) {
        ESP_LOGI("RecipeAppService", "Recipe saved successfully: %s", recipeDto.id.c_str());
    } else {
        ESP_LOGE("RecipeAppService", "Failed to save recipe: %s", recipeDto.id.c_str());
    }
    
    // Liste aktualisieren (damit das Dashboard das neue Rezept sieht)
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
    ESP_LOGI("RecipeAppService", "handleGetRecipe called for ID: %s", recipeId.c_str());
    
    if (!m_storageManager || !m_gateway) {
        ESP_LOGE("RecipeAppService", "StorageManager or Gateway is NULL!");
        return;
    }
    
    // Convert string ID to uint32_t hash for storage lookup
    uint32_t recipeIdHash = 0;
    for (char c : recipeId) {
        recipeIdHash = recipeIdHash * 31 + static_cast<uint32_t>(c);
    }
    
    auto recipeJsonOpt = m_storageManager->getJsonRecipe(recipeIdHash);
    if (!recipeJsonOpt.has_value()) {
        ESP_LOGW("RecipeAppService", "Recipe not found: %s (hash: %" PRIx32 ")", recipeId.c_str(), recipeIdHash);
        return;
    }
    
    RecipeDto dto;
    if (!JsonSerialization::deserialize(recipeJsonOpt.value(), dto)) {
        ESP_LOGE("RecipeAppService", "Failed to deserialize recipe: %s", recipeId.c_str());
        return;
    }
    
    ESP_LOGI("RecipeAppService", "Sending recipe: %s", dto.name.c_str());
    m_gateway->send(dto);
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
    ESP_LOGI("RecipeAppService", "Building AvailableRecipesDto...");
    AvailableRecipesDto dto;
    
    if (!m_storageManager) {
        ESP_LOGW("RecipeAppService", "StorageManager is NULL, using placeholder data");
        
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
    
    // Get all recipe IDs from storage
    auto recipeIds = m_storageManager->getAllJsonRecipeIds();
    ESP_LOGI("RecipeAppService", "Found %d recipes in storage", recipeIds.size());
    
    for (const auto& id : recipeIds) {
        auto recipeJsonOpt = m_storageManager->getJsonRecipe(id);
        if (recipeJsonOpt.has_value()) {
            // Parse minimal metadata from JSON
            RecipeDto recipeDto;
            if (JsonSerialization::deserialize(recipeJsonOpt.value(), recipeDto)) {
                RecipeInfoDto info;
                info.id = recipeDto.id;
                info.name = recipeDto.name;
                info.description = recipeDto.description;
                info.createdAt = 0; // Could be extracted from ID timestamp
                info.lastModified = 0;
                dto.recipes.push_back(info);
                ESP_LOGI("RecipeAppService", "Added recipe: %s", info.name.c_str());
            }
        }
    }
    
    ESP_LOGI("RecipeAppService", "Built DTO with %d recipes", dto.recipes.size());
    return dto;
}

AvailableStepsDto RecipeApplicationService::buildAvailableStepsDto() const {
    AvailableStepsDto dto;
    
    // Hole alle Step-Metadaten von der Registry
    auto stepMetadataList = StepTypeRegistry::instance().availableTypes();
    
    // Konvertiere StepMetadata zu StepMetadataDto
    for (const auto& meta : stepMetadataList) {
        StepMetadataDto stepDto;
        
        // Konvertiere typeId zu Hex-String (fix: verwende PRIx32 für uint32_t)
        char typeIdBuf[16];
        snprintf(typeIdBuf, sizeof(typeIdBuf), "0x%04" PRIx32, meta.typeId);
        stepDto.typeId = typeIdBuf;
        
        stepDto.displayName = meta.displayName;
        stepDto.description = meta.description;
        stepDto.category = "General"; // TODO: Category aus Metadata holen wenn vorhanden
        
        // Parameter konvertieren
        for (const auto& param : meta.params) {
            ParameterMetadataDto paramDto;
            paramDto.name = param.key;
            paramDto.description = param.description;
            paramDto.unit = param.unit;
            paramDto.required = true;
            
            // Type aus ParameterValue ableiten
            ParameterType pType = param.value.getType();
            switch (pType) {
                case ParameterType::TEMPERATURE:
                case ParameterType::PRESSURE:
                case ParameterType::HUMIDITY:
                case ParameterType::PERCENTAGE:
                case ParameterType::VOLUME:
                case ParameterType::MASS:
                case ParameterType::LENGTH:
                case ParameterType::VOLTAGE:
                case ParameterType::CURRENT:
                case ParameterType::POWER:
                case ParameterType::CONCENTRATION:
                case ParameterType::PH_VALUE:
                case ParameterType::VELOCITY:
                case ParameterType::ANGLE:
                    paramDto.type = "float";
                    break;
                    
                case ParameterType::BOOLEAN:
                    paramDto.type = "bool";
                    break;
                    
                default:
                    paramDto.type = "int";
                    break;
            }
            
            // Verwende toNumericString() für alle Werte - keine Typ-spezifische Logik mehr!
            paramDto.defaultValue = param.value.toNumericString();
            if (param.minValue.has_value()) {
                paramDto.minValue = param.minValue->toNumericString();
                ESP_LOGI("RecipeAppService", "Param '%s': minValue = %s", param.key.c_str(), paramDto.minValue.c_str());
            } else {
                ESP_LOGI("RecipeAppService", "Param '%s': minValue = NOT SET", param.key.c_str());
            }
            if (param.maxValue.has_value()) {
                paramDto.maxValue = param.maxValue->toNumericString();
                ESP_LOGI("RecipeAppService", "Param '%s': maxValue = %s", param.key.c_str(), paramDto.maxValue.c_str());
            } else {
                ESP_LOGI("RecipeAppService", "Param '%s': maxValue = NOT SET", param.key.c_str());
            }
            
            stepDto.parameters.push_back(paramDto);
        }
        
        // I/O Aliases konvertieren
        for (const auto& ioAlias : meta.ioAliases) {
            IoAliasMetadataDto ioDto;
            ioDto.aliasName = ioAlias.aliasName;
            ioDto.description = "";
            ioDto.defaultPhysicalName = ioAlias.physicalName;  // Default physical resource
            
            // Type bestimmen
            if (ioAlias.isInput && ioAlias.isOutput) {
                ioDto.ioType = "input/output";
            } else if (ioAlias.isInput) {
                ioDto.ioType = "input";
            } else if (ioAlias.isOutput) {
                ioDto.ioType = "output";
            } else if (ioAlias.isSensor) {
                ioDto.ioType = "sensor";
            } else {
                ioDto.ioType = "unknown";
            }
            
            ioDto.valueType = ioAlias.valueType;
            
            stepDto.ioAliases.push_back(ioDto);
        }
        
        dto.steps.push_back(stepDto);
    }
    
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
