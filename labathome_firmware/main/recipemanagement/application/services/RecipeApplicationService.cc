#include "RecipeApplicationService.hh"
#include "AuthenticationManager.hh"
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
    RecipeHistoryService* historyService) : m_storageManager(storageManager),
                                m_engine(engine),
                                m_gateway(gateway),
                                m_historyService(historyService),
                                m_authManager(nullptr)
{
    m_authManager = new AuthenticationManager(m_storageManager);
    
    if (m_engine)
    {
        m_engine->setStateChangeCallback([this]()
                                         { this->sendLiveViewUpdate(); });
    }
}
RecipeApplicationService::~RecipeApplicationService()
{
    delete m_authManager;
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
    
    // Authentication commands (no token required)
    if (cmd == "login")
    {
        ESP_LOGI(TAG, "Called handleLogin()");
        handleLogin(dto);
        return;
    }
    else if (cmd == "logout")
    {
        ESP_LOGI(TAG, "Called handleLogout()");
        handleLogout(dto);
        return;
    }
    else if (cmd == "change_pin")
    {
        ESP_LOGI(TAG, "Called handleChangePin()");
        handleChangePin(dto);
        return;
    }
    
    // Public read-only commands (no authentication required)
    if (cmd == "get_recipe_list")
    {
        ESP_LOGI(TAG, "Called handleGetRecipeList() [PUBLIC]");
        handleGetRecipeList();
    }
    else if (cmd == "get_available_steps")
    {
        ESP_LOGI(TAG, "Called handleGetAvailableSteps() [PUBLIC]");
        handleGetAvailableSteps();
    }
    else if (cmd == "get_recipe")
    {
        ESP_LOGI(TAG, "Called handleGetRecipe() [PUBLIC]");
        handleGetRecipe(dto.recipeId);
    }
    else if (cmd == "request_live_view")
    {
        ESP_LOGI(TAG, "Called handleRequestLiveView() [PUBLIC]");
        handleRequestLiveView();
    }
    else if (cmd == "get_execution_history")
    {
        ESP_LOGI(TAG, "Called handleGetExecutionHistory() [PUBLIC]");
        handleGetExecutionHistory();
    }
    else if (cmd == "get_timeseries")
    {
        ESP_LOGI(TAG, "Called handleGetTimeSeries(%s) [PUBLIC]", dto.executionId.c_str());
        handleGetTimeSeries(dto.executionId);
    }
    // RecipeStarter commands
    else if (cmd == "start_recipe")
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
    // RecipeEditor commands
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
    
    // NEW: Load as binary data for efficient transmission
    TimeSeriesBinaryDto dto = m_storageManager->getTimeSeriesBinary(executionId);
    m_gateway->send(dto);
    
    ESP_LOGI(TAG, "[GET_TS] Sent binary TimeSeries: executionId=%s, size=%zu bytes",
             executionId.c_str(), dto.binaryData.size());
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
        action();
        return true;
    }
    
    // Check if token is valid (user is authenticated)
    UserRole userRole = m_authManager->validateToken(dto.sessionToken);
    if (dto.sessionToken.empty() || (userRole == UserRole::Observer && dto.sessionToken.length() < 16)) {
        // No token or invalid token -> 401 Unauthorized
        sendAuthError(dto, "Not authenticated. Please login first.", 401);
        ESP_LOGW(TAG, "Auth failed for command '%s': No valid session token", dto.command.c_str());
        return false;
    }
    
    // Check if user has required role (sufficient permissions)
    if (!m_authManager->hasPermission(dto.sessionToken, requiredRole)) {
        // Valid token but insufficient role -> 403 Forbidden
        const char* roleNames[] = {"Observer", "RecipeStarter", "RecipeEditor", "Admin"};
        sendAuthError(dto, "Not allowed with current role. Required: " + std::string(roleNames[static_cast<int>(requiredRole)]), 403);
        ESP_LOGW(TAG, "Auth failed for command '%s': User has role %d, required %d", 
                 dto.command.c_str(), static_cast<int>(userRole), static_cast<int>(requiredRole));
        return false;
    }
    
    action();
    return true;
}

void RecipeApplicationService::sendAuthError(const CommandDto& dto, const std::string& message, int errorCode) {
    if (!m_gateway) return;
    
    CommandResponseDto response;
    response.success = false;
    response.errorCode = errorCode;
    response.errorMessage = message;
    response.requestId = dto.requestId;
    m_gateway->send(response);
    
    ESP_LOGI(TAG, "Sent auth error: code=%d, message='%s'", errorCode, message.c_str());
}

void RecipeApplicationService::handleLogin(const CommandDto& dto) {
    ESP_LOGI(TAG, "handleLogin: pin='%s', role='%s', authManager=%p, gateway=%p", 
             dto.pin.c_str(), dto.loginRole.c_str(), m_authManager, m_gateway);
    
    if (!m_authManager || !m_gateway) {
        ESP_LOGE(TAG, "handleLogin: Missing authManager or gateway!");
        return;
    }
    
    // Parse role from string
    UserRole requestedRole = UserRole::Observer;
    if (dto.loginRole == "Admin") {
        requestedRole = UserRole::Admin;
    } else if (dto.loginRole == "RecipeEditor") {
        requestedRole = UserRole::RecipeEditor;
    } else if (dto.loginRole == "RecipeStarter") {
        requestedRole = UserRole::RecipeStarter;
    } else if (dto.loginRole == "Observer") {
        requestedRole = UserRole::Observer;
    } else {
        ESP_LOGW(TAG, "handleLogin: Unknown role '%s', defaulting to Observer", dto.loginRole.c_str());
        requestedRole = UserRole::Observer;
    }
    
    ESP_LOGI(TAG, "handleLogin: Calling m_authManager->login() for role=%d", static_cast<int>(requestedRole));
    std::string token = m_authManager->login(dto.pin, requestedRole);
    ESP_LOGI(TAG, "handleLogin: Token received, length=%d, empty=%d", 
             token.length(), token.empty());
    
    AuthResponseDto response;
    if (token.empty()) {
        response.success = false;
        response.role = "Observer";
        response.sessionToken = "";
        response.errorMessage = "Invalid PIN for selected role";
        ESP_LOGW(TAG, "handleLogin: Invalid PIN for role '%s'", dto.loginRole.c_str());
    } else {
        response.success = true;
        UserRole role = m_authManager->validateToken(token);
        
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
        
        response.sessionToken = token;
        response.errorMessage = "";
        ESP_LOGI(TAG, "handleLogin: Login successful, role=%s", response.role.c_str());
    }
    
    ESP_LOGI(TAG, "handleLogin: Sending response via gateway");
    m_gateway->send(response);
    ESP_LOGI(TAG, "Login attempt for role '%s': %s", dto.loginRole.c_str(), response.success ? "success" : "failed");
}

void RecipeApplicationService::handleLogout(const CommandDto& dto) {
    if (!m_authManager) {
        return;
    }
    
    m_authManager->logout(dto.sessionToken);
    ESP_LOGI(TAG, "Logout successful");
}

void RecipeApplicationService::handleChangePin(const CommandDto& dto) {
    if (!m_authManager || !m_gateway) {
        return;
    }
    
    // Payload: "role,oldPin,newPin" (simple CSV format)
    size_t comma1 = dto.payload.find(',');
    size_t comma2 = dto.payload.find(',', comma1 + 1);
    
    if (comma1 == std::string::npos || comma2 == std::string::npos) {
        CommandResponseDto response;
        response.success = false;
        response.errorCode = 400;
        response.errorMessage = "Invalid payload format (expected: role,oldPin,newPin)";
        response.requestId = dto.requestId;
        m_gateway->send(response);
        return;
    }
    
    std::string roleStr = dto.payload.substr(0, comma1);
    std::string oldPin = dto.payload.substr(comma1 + 1, comma2 - comma1 - 1);
    std::string newPin = dto.payload.substr(comma2 + 1);
    
    // Trim whitespace from all parts (common issue with string parsing)
    auto trim = [](std::string& s) {
        // Remove leading whitespace
        while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\n' || s.front() == '\r')) {
            s.erase(s.begin());
        }
        // Remove trailing whitespace
        while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\n' || s.back() == '\r')) {
            s.pop_back();
        }
    };
    
    trim(roleStr);
    trim(oldPin);
    trim(newPin);
    
    ESP_LOGI(TAG, "changePin: role='%s', oldPin.len=%d, newPin.len=%d", 
             roleStr.c_str(), oldPin.length(), newPin.length());
    
    UserRole role = UserRole::Observer;
    if (roleStr == "Admin") role = UserRole::Admin;
    else if (roleStr == "RecipeEditor") role = UserRole::RecipeEditor;
    else if (roleStr == "RecipeStarter") role = UserRole::RecipeStarter;
    else if (roleStr == "Observer") role = UserRole::Observer;
    
    bool success = m_authManager->changePin(dto.sessionToken, role, oldPin, newPin);
    
    CommandResponseDto response;
    response.success = success;
    response.errorCode = success ? 0 : 403;
    response.errorMessage = success ? "PIN changed successfully" : "Failed to change PIN (wrong oldPin or unauthorized)";
    response.requestId = dto.requestId;
    m_gateway->send(response);
    
    ESP_LOGI(TAG, "PIN change for role %s: %s", roleStr.c_str(), success ? "success" : "failed");
}

