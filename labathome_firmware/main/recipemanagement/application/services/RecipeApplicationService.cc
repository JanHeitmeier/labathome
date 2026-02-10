#include "RecipeApplicationService.hh"
#include "../../infrastructure/serialization/JsonSerialization.hh"
#include "../../infrastructure/engine/StepTypeRegistry.hh"
#include "../../infrastructure/parsers/RecipeParser.hh"
#include "../../core/domain/entities/Recipe.hh"
#include <esp_log.h>
#include <ctime>
#include <cinttypes>

static const char* TAG = "RecipeAppService";

RecipeApplicationService::RecipeApplicationService(
    StorageManager *storageManager,
    RecipeEngine *engine,
    IMessageGateway *gateway,
    RecipeHistoryService* historyService,
    AuthenticationManager* authManager) : m_storageManager(storageManager),
                                m_engine(engine),
                                m_gateway(gateway),
                                m_historyService(historyService),
                                m_authManager(authManager)
{
    if (m_engine)
    {
        m_engine->setStateChangeCallback([this]()
                                         { this->sendLiveViewUpdate(); });
    }
}
RecipeApplicationService::~RecipeApplicationService()
{
}
static uint32_t calculateRecipeIdHash(const std::string &recipeId)
{
    uint32_t hash = 0;
    for (char c : recipeId)
    {
        hash = hash * 31 + static_cast<uint32_t>(c);
    }
    return hash;
}
void RecipeApplicationService::setMessageGateway(IMessageGateway *gateway)
{
    m_gateway = gateway;
}

void RecipeApplicationService::handleCommand(const CommandDto &dto)
{
    const std::string &cmd = dto.command;
    
    // Commands requiring RecipeStarter role
    if (cmd == "start_recipe")
    {
        ESP_LOGI(TAG, "Called start_recipe: payload.len=%d, recipeId=%s", 
                 dto.payload.length(), dto.recipeId.c_str());
        validateAndExecute(dto, UserRole::RecipeStarter, [&]() {
            if (!dto.payload.empty()) {
                ESP_LOGI(TAG, "-> Using handleStartRecipeFromJson (with payload)");
                handleStartRecipeFromJson(dto.payload);
            } else {
                ESP_LOGI(TAG, "-> Using handleStartRecipe (by ID)");
                handleStartRecipe(dto.recipeId);
            }
        });
    }
    else if (cmd == "stop_recipe")
    {
        ESP_LOGI(TAG, "Called handleStopRecipe()");
        validateAndExecute(dto, UserRole::RecipeStarter, [&]() {
            handleStopRecipe();
        });
    }
    else if (cmd == "pause_recipe")
    {
        ESP_LOGI(TAG, "Called handlePauseRecipe()");
        validateAndExecute(dto, UserRole::RecipeStarter, [&]() {
            handlePauseRecipe();
        });
    }
    else if (cmd == "resume_recipe")
    {
        ESP_LOGI(TAG, "Called handleResumeRecipe()");
        validateAndExecute(dto, UserRole::RecipeStarter, [&]() {
            handleResumeRecipe();
        });
    }
    else if (cmd == "acknowledge_step")
    {
        ESP_LOGI(TAG, "Called handleAcknowledgeStep()");
        validateAndExecute(dto, UserRole::RecipeStarter, [&]() {
            handleAcknowledgeStep();
        });
    }
    // Commands requiring RecipeEditor role
    else if (cmd == "save_recipe")
    {
        ESP_LOGI(TAG, "Called handleSaveRecipe()");
        validateAndExecute(dto, UserRole::RecipeEditor, [&]() {
            handleSaveRecipe(dto.payload);
        });
    }
    else if (cmd == "delete_recipe")
    {
        ESP_LOGI(TAG, "Called handleDeleteRecipe()");
        validateAndExecute(dto, UserRole::RecipeEditor, [&]() {
            handleDeleteRecipe(dto.recipeId);
        });
    }
    else if (cmd == "delete_execution")
    {
        ESP_LOGI(TAG, "Called handleDeleteExecution(%s)", dto.executionId.c_str());
        validateAndExecute(dto, UserRole::RecipeEditor, [&]() {
            handleDeleteExecution(dto.executionId);
        });
    }
    // Observer commands (no authentication required)
    else if (cmd == "get_recipe_list")
    {
        ESP_LOGI(TAG, "Called handleGetRecipeList()");
        handleGetRecipeList();
    }
    else if (cmd == "get_available_steps")
    {
        ESP_LOGI(TAG, "Called handleGetAvailableSteps()");
        handleGetAvailableSteps();
    }
    else if (cmd == "get_recipe")
    {
        ESP_LOGI(TAG, "Called handleGetRecipe()");
        handleGetRecipe(dto.recipeId);
    }
    else if (cmd == "request_live_view")
    {
        ESP_LOGI(TAG, "Called handleRequestLiveView()");
        handleRequestLiveView();
    }
    else if (cmd == "get_execution_history")
    {
        ESP_LOGI(TAG, "Called handleGetExecutionHistory()");
        handleGetExecutionHistory();
    }
    else if (cmd == "get_timeseries")
    {
        ESP_LOGI(TAG, "Called handleGetTimeSeries(%s)", dto.executionId.c_str());
        handleGetTimeSeries(dto.executionId);
    }
    // Authentication commands
    else if (cmd == "authenticate")
    {
        ESP_LOGI(TAG, "Called handleAuthenticate()");
        handleAuthenticate(dto);
    }
    else if (cmd == "change_password")
    {
        ESP_LOGI(TAG, "Called handleChangePassword()");
        handleChangePassword(dto.payload);
    }
    else if (cmd == "reset_passwords")
    {
        ESP_LOGI(TAG, "Called handleResetPasswords()");
        validateAndExecute(dto, UserRole::Admin, [&]() {
            handleResetPasswords(dto.password);
        });
    }
}
void RecipeApplicationService::handleStartRecipeFromJson(const std::string &jsonRecipe)
{
    ESP_LOGI(TAG, "[START_FROM_JSON] Starting... (JSON length=%d)", jsonRecipe.length());
    ESP_LOGI(TAG, "[START_FROM_JSON] JSON snippet: %.200s", jsonRecipe.c_str());
    if (!m_engine || !m_storageManager)
    {
        ESP_LOGE(TAG, "[START_FROM_JSON] Failed: engine=%p storage=%p", m_engine, m_storageManager);
        return;
    }
    RecipeParser parser;
    Recipe recipe;
    if (!parser.parseJsonToRecipe(jsonRecipe, recipe))
    {
        ESP_LOGE(TAG, "[START_FROM_JSON] Failed to parse JSON to Recipe");
        return;
    }
    ESP_LOGI(TAG, "[START_FROM_JSON] Recipe parsed: id=%s, name=%s", recipe.id().c_str(), recipe.name().c_str());
    
    std::map<std::string, std::string> globalParams;
    if (!JsonSerialization::extractGlobalParameters(jsonRecipe, globalParams)) {
        ESP_LOGW(TAG, "[START_FROM_JSON] Failed to extract globalParameters");
    } else {
        ESP_LOGI(TAG, "[START_FROM_JSON] Extracted %d global parameters", globalParams.size());
    }
    
    uint32_t recipeIdHash = calculateRecipeIdHash(recipe.id());
    m_storageManager->saveRecipeWithCache(recipeIdHash, jsonRecipe, recipe);
    ESP_LOGI(TAG, "[START_FROM_JSON] Recipe saved to storage");
    
    if (!m_engine->loadRecipe(recipe.steps(), recipe.id(), recipe.name()))
    {
        ESP_LOGE(TAG, "[START_FROM_JSON] Engine failed to load recipe");
        return;
    }
    ESP_LOGI(TAG, "[START_FROM_JSON] Recipe loaded into engine");
    
    m_engine->setGlobalParameters(globalParams);
    ESP_LOGI(TAG, "[START_FROM_JSON] Global parameters set");
    
    if (!m_engine->start())
    {
        ESP_LOGE(TAG, "[START_FROM_JSON] Engine failed to start");
        return;
    }
    ESP_LOGI(TAG, "[START_FROM_JSON] Recipe started successfully!");
    sendLiveViewUpdate();
}
void RecipeApplicationService::handleStartRecipe(const std::string &recipeId)
{
    ESP_LOGI(TAG, "[START_RECIPE] Starting recipe: %s", recipeId.c_str());
    if (!m_storageManager || !m_engine)
    {
        ESP_LOGE(TAG, "[START_RECIPE] Failed: engine=%p storage=%p", m_engine, m_storageManager);
        return;
    }

    uint32_t recipeIdHash = calculateRecipeIdHash(recipeId);
    
    auto jsonRecipeOpt = m_storageManager->getJsonRecipe(recipeIdHash);
    if (!jsonRecipeOpt) {
        ESP_LOGE(TAG, "[START_RECIPE] Recipe not found in storage: %s", recipeId.c_str());
        return;
    }
    ESP_LOGI(TAG, "[START_RECIPE] Recipe loaded from storage");
    
    std::string jsonRecipe = *jsonRecipeOpt;
    Recipe recipe;
    RecipeParser parser;
    if (!parser.parseJsonToRecipe(jsonRecipe, recipe)) {
        ESP_LOGE(TAG, "[START_RECIPE] Failed to parse JSON to Recipe");
        return;
    }
    ESP_LOGI(TAG, "[START_RECIPE] Recipe parsed: name=%s", recipe.name().c_str());
    
    std::map<std::string, std::string> globalParams;
    if (!JsonSerialization::extractGlobalParameters(jsonRecipe, globalParams)) {
        ESP_LOGW(TAG, "[START_RECIPE] Failed to extract globalParameters");
    } else {
        ESP_LOGI(TAG, "[START_RECIPE] Extracted %d global parameters", globalParams.size());
    }

    if (!m_engine->loadRecipe(recipe.steps(), recipe.id(), recipe.name()))
    {
        ESP_LOGE(TAG, "[START_RECIPE] Engine failed to load recipe");
        return;
    }
    ESP_LOGI(TAG, "[START_RECIPE] Recipe loaded into engine");
    
    m_engine->setGlobalParameters(globalParams);
    ESP_LOGI(TAG, "[START_RECIPE] Global parameters set");
    
    if (!m_engine->start())
    {
        ESP_LOGE(TAG, "[START_RECIPE] Engine failed to start");
        return;
    }
    ESP_LOGI(TAG, "[START_RECIPE] Recipe started successfully!");
    sendLiveViewUpdate();
}
void RecipeApplicationService::handleStopRecipe()
{
    if (!m_engine)
        return;
    m_engine->stop();
    sendLiveViewUpdate();
}
void RecipeApplicationService::handlePauseRecipe()
{
    if (!m_engine)
        return;
    m_engine->pause();
    sendLiveViewUpdate();
}
void RecipeApplicationService::handleResumeRecipe()
{
    if (!m_engine)
        return;
    m_engine->resume();
    sendLiveViewUpdate();
}
void RecipeApplicationService::handleAcknowledgeStep()
{
    if (!m_engine)
    {
        return;
    }
    m_engine->acknowledgeStep();
}
void RecipeApplicationService::handleGetRecipeList()
{
    if (!m_gateway)
    {

        return;
    }
    AvailableRecipesDto dto = buildAvailableRecipesDto();
    m_gateway->send(dto);
}
void RecipeApplicationService::handleGetAvailableSteps()
{
    if (!m_gateway)
    {
        return;
    }
    AvailableStepsDto dto = buildAvailableStepsDto();
    m_gateway->send(dto);
}
void RecipeApplicationService::handleSaveRecipe(const std::string &payloadJson)
{
    if (!m_storageManager || !m_gateway)
        return;
    RecipeDto recipeDto;
    if (!JsonSerialization::deserialize(payloadJson, recipeDto))
    {
        return;
    }
    uint32_t recipeIdHash = calculateRecipeIdHash(recipeDto.id);
    // Timestamp Fallback
    uint64_t currentTime = static_cast<uint64_t>(std::time(nullptr)) * 1000;
    if (recipeDto.createdAt == 0)
    {
        recipeDto.createdAt = currentTime;
    }
    if (recipeDto.lastModified == 0)
    {
        recipeDto.lastModified = currentTime;
    }
    std::string updatedJson = JsonSerialization::serialize(recipeDto);
    Recipe recipe;
    RecipeParser parser;
    if (parser.parseJsonToRecipe(updatedJson, recipe))
    {
        m_storageManager->saveRecipeWithCache(recipeIdHash, updatedJson, recipe);
    }
    else
    {
        m_storageManager->saveJsonRecipeWithStringId(updatedJson);
    }
    handleGetRecipeList();
}
void RecipeApplicationService::handleDeleteRecipe(const std::string &recipeId)
{
    if (!m_storageManager || !m_gateway)
        return;
    uint32_t recipeIdHash = calculateRecipeIdHash(recipeId);
    m_storageManager->deleteJsonRecipe(recipeIdHash);
    handleGetRecipeList();
}
void RecipeApplicationService::handleGetRecipe(const std::string &recipeId)
{
    if (!m_storageManager || !m_gateway)
    {
        return;
    }
    uint32_t recipeIdHash = calculateRecipeIdHash(recipeId);
    auto recipeJsonOpt = m_storageManager->getJsonRecipe(recipeIdHash);
    if (!recipeJsonOpt.has_value())
    {
        return;
    }
    RecipeDto dto;
    if (!JsonSerialization::deserialize(recipeJsonOpt.value(), dto))
    {
        return;
    }
    m_gateway->send(dto);
}

void RecipeApplicationService::sendLiveViewUpdate()
{
    if (!m_gateway)
        return;
    LiveViewDto dto = buildLiveViewDto();
    m_gateway->send(dto);
}
LiveViewDto RecipeApplicationService::buildLiveViewDto() const
{
    LiveViewDto dto;
    if (!m_engine)
    {
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
    dto.recipeId = m_engine->getRecipeId();
    dto.recipeName = m_engine->getRecipeName();
    dto.currentStepIndex = m_engine->getCurrentStepIndex();
    dto.totalSteps = m_engine->getTotalSteps();
    dto.currentStepName = m_engine->getCurrentStepName();
    RecipeEngineState state = m_engine->getState();
    switch (state)
    {
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
    
    // Collect sensor values from current step
    dto.sensorValues = m_engine->getSensorValues();
    
    return dto;
}
AvailableRecipesDto RecipeApplicationService::buildAvailableRecipesDto() const
{
    AvailableRecipesDto dto;
    if (!m_storageManager)
    {
        return dto;
    }
    auto recipeIds = m_storageManager->getAllJsonRecipeIds();
    for (const auto &id : recipeIds)
    {
        auto recipeJsonOpt = m_storageManager->getJsonRecipe(id);
        if (recipeJsonOpt.has_value())
        {
            RecipeDto recipeDto;
            if (JsonSerialization::deserialize(recipeJsonOpt.value(), recipeDto))
            {
                RecipeInfoDto info;
                info.id = recipeDto.id;
                info.name = recipeDto.name;
                info.description = recipeDto.description;
                info.version = recipeDto.version;
                info.createdAt = recipeDto.createdAt;
                info.lastModified = recipeDto.lastModified;
                dto.recipes.push_back(info);
            }
        }
    }
    return dto;
}
AvailableStepsDto RecipeApplicationService::buildAvailableStepsDto() const
{
    AvailableStepsDto dto;
    auto stepMetadataList = StepTypeRegistry::instance().availableTypes();
    for (const auto &meta : stepMetadataList)
    {
        StepMetadataDto stepDto;
        char typeIdBuf[16];
        snprintf(typeIdBuf, sizeof(typeIdBuf), "0x%04" PRIx32, meta.typeId);
        stepDto.typeId = typeIdBuf;
        stepDto.displayName = meta.displayName;
        stepDto.description = meta.description;
        stepDto.category = "General";
        // TODO: Category aus Metadata holen wenn vorhanden
        for (const auto &param : meta.params)
        {
            ParameterMetadataDto paramDto;
            paramDto.name = param.key;
            paramDto.description = param.description;
            paramDto.unit = param.unit;
            paramDto.required = true;
            paramDto.isGlobal = param.isGlobal;
            ParameterType pType = param.value.getType();
            if (pType == ParameterType::BOOLEAN) {
                paramDto.type = "bool";
            } else if (pType == ParameterType::COLOR) {
                paramDto.type = "color";
            } else if (pType == ParameterType::NONE || pType == ParameterType::GENERIC_INT || 
                       pType == ParameterType::RPM || pType == ParameterType::TIME_SECONDS || 
                       pType == ParameterType::TIME_MILLISECONDS || pType == ParameterType::FLOW_RATE) {
                paramDto.type = "int";
            } else {
                paramDto.type = "float";
            }
            paramDto.defaultValue = param.value.toNumericString();
            if (param.minValue.has_value())
            {
                paramDto.minValue = param.minValue->toNumericString();
            }
            if (param.maxValue.has_value())
            {
                paramDto.maxValue = param.maxValue->toNumericString();
            }
            stepDto.parameters.push_back(paramDto);
        }
        for (const auto &ioAlias : meta.ioAliases)
        {
            IoAliasMetadataDto ioDto;
            ioDto.aliasName = ioAlias.aliasName;
            ioDto.isInput = ioAlias.isInput;
            ioDto.isOutput = ioAlias.isOutput;
            ioDto.isSensor = ioAlias.isSensor;
            ioDto.description = "";
            ioDto.defaultPhysicalName = ioAlias.physicalName;
            ioDto.valueType = ioAlias.valueType;
            ioDto.unit = ioAlias.unit;
            stepDto.ioAliases.push_back(ioDto);
        }
        dto.steps.push_back(stepDto);
    }
    return dto;
}

void RecipeApplicationService::handleGetExecutionHistory() {
    if (!m_historyService || !m_gateway) {
        return;
    }
    
    ExecutionHistoryDto dto = m_historyService->getExecutionHistory();
    m_gateway->send(dto);
}

void RecipeApplicationService::handleGetTimeSeries(const std::string& executionId) {
    if (!m_storageManager || !m_gateway) {
        return;
    }
    
    TimeSeriesDataDto dto = m_storageManager->getTimeSeries(executionId);
    m_gateway->send(dto);
}

void RecipeApplicationService::handleDeleteExecution(const std::string& executionId) {
    if (!m_storageManager) return;
    
    m_storageManager->deleteExecution(executionId);
    handleGetExecutionHistory();
}

void RecipeApplicationService::handleRequestLiveView() {
    sendLiveViewUpdate();
}

// ========== AUTHENTICATION ==========

bool RecipeApplicationService::validateAndExecute(const CommandDto& dto, UserRole requiredRole, std::function<void()> action) {
    if (!m_authManager) {
        // No auth manager = no authentication required
        action();
        return true;
    }
    
    if (!m_authManager->validatePassword(dto.password, requiredRole)) {
        sendAuthError(dto, "Unauthorized: Invalid password or insufficient permissions");
        ESP_LOGW(TAG, "Auth failed for command '%s' (required role: %d)", dto.command.c_str(), static_cast<int>(requiredRole));
        return false;
    }
    
    action();
    return true;
}

void RecipeApplicationService::sendAuthError(const CommandDto& dto, const std::string& message) {
    if (!m_gateway) return;
    
    CommandResponseDto response;
    response.success = false;
    response.errorCode = 401;
    response.errorMessage = message;
    response.requestId = dto.requestId;
    m_gateway->send(response);
}

void RecipeApplicationService::handleAuthenticate(const CommandDto& dto) {
    if (!m_authManager || !m_gateway) {
        return;
    }
    
    UserRole role = m_authManager->getRoleForPassword(dto.password);
    
    AuthResponseDto response;
    response.success = true;
    
    switch(role) {
        case UserRole::Admin:
            response.role = "Admin";
            break;
        case UserRole::RecipeEditor:
            response.role = "RecipeEditor";
            break;
        case UserRole::RecipeStarter:
            response.role = "RecipeStarter";
            break;
        case UserRole::Observer:
        default:
            response.role = "Observer";
            break;
    }
    
    response.errorMessage = "";
    m_gateway->send(response);
    
    ESP_LOGI(TAG, "Authentication successful: %s", response.role.c_str());
}

void RecipeApplicationService::handleChangePassword(const std::string& payloadJson) {
    if (!m_authManager || !m_gateway) {
        return;
    }
    
    // Expected JSON: {"role": "Admin", "oldPassword": "...", "newPassword": "..."}
    // Simple parsing (in production use proper JSON parser)
    size_t rolePos = payloadJson.find("\"role\"");
    size_t oldPwPos = payloadJson.find("\"oldPassword\"");
    size_t newPwPos = payloadJson.find("\"newPassword\"");
    
    if (rolePos == std::string::npos || oldPwPos == std::string::npos || newPwPos == std::string::npos) {
        CommandResponseDto response;
        response.success = false;
        response.errorCode = 400;
        response.errorMessage = "Invalid payload format";
        response.requestId = "";
        m_gateway->send(response);
        return;
    }
    
    // Extract values (simple approach - in production use JSON library)
    auto extractValue = [](const std::string& json, const std::string& key) -> std::string {
        size_t keyPos = json.find("\"" + key + "\"");
        if (keyPos == std::string::npos) return "";
        size_t colonPos = json.find(":", keyPos);
        if (colonPos == std::string::npos) return "";
        size_t startQuote = json.find("\"", colonPos);
        if (startQuote == std::string::npos) return "";
        size_t endQuote = json.find("\"", startQuote + 1);
        if (endQuote == std::string::npos) return "";
        return json.substr(startQuote + 1, endQuote - startQuote - 1);
    };
    
    std::string roleStr = extractValue(payloadJson, "role");
    std::string oldPassword = extractValue(payloadJson, "oldPassword");
    std::string newPassword = extractValue(payloadJson, "newPassword");
    
    UserRole role = UserRole::Observer;
    if (roleStr == "Admin") role = UserRole::Admin;
    else if (roleStr == "RecipeEditor") role = UserRole::RecipeEditor;
    else if (roleStr == "RecipeStarter") role = UserRole::RecipeStarter;
    else if (roleStr == "Observer") role = UserRole::Observer;
    
    bool success = m_authManager->changePassword(role, oldPassword, newPassword);
    
    CommandResponseDto response;
    response.success = success;
    response.errorCode = success ? 0 : 401;
    response.errorMessage = success ? "" : "Failed to change password (old password incorrect?)";
    response.requestId = "";
    m_gateway->send(response);
}

void RecipeApplicationService::handleResetPasswords(const std::string& adminPassword) {
    if (!m_authManager || !m_gateway) {
        return;
    }
    
    bool success = m_authManager->resetToDefaults(adminPassword);
    
    CommandResponseDto response;
    response.success = success;
    response.errorCode = success ? 0 : 401;
    response.errorMessage = success ? "All passwords reset to defaults" : "Invalid admin password";
    response.requestId = "";
    m_gateway->send(response);
}

