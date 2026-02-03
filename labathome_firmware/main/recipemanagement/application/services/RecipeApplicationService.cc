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
    RecipeHistoryService* historyService) : m_storageManager(storageManager),
                                m_engine(engine),
                                m_gateway(gateway),
                                m_historyService(historyService)
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
    
    if (cmd == "start_recipe")
    {
        if (!dto.payload.empty())
        {
            ESP_LOGI(TAG, "Called handleStartRecipeFromJson()");
            handleStartRecipeFromJson(dto.payload);
        }
        else
        {
            ESP_LOGI(TAG, "Called handleStartRecipe()");
            handleStartRecipe(dto.recipeId);
        }
    }
    else if (cmd == "stop_recipe")
    {
        ESP_LOGI(TAG, "Called handleStopRecipe()");
        handleStopRecipe();
    }
    else if (cmd == "pause_recipe")
    {
        ESP_LOGI(TAG, "Called handlePauseRecipe()");
        handlePauseRecipe();
    }
    else if (cmd == "resume_recipe")
    {
        ESP_LOGI(TAG, "Called handleResumeRecipe()");
        handleResumeRecipe();
    }
    else if (cmd == "acknowledge_step")
    {
        ESP_LOGI(TAG, "Called handleAcknowledgeStep()");
        handleAcknowledgeStep();
    }
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
    else if (cmd == "save_recipe")
    {
        ESP_LOGI(TAG, "Called handleSaveRecipe()");
        handleSaveRecipe(dto.payload);
    }
    else if (cmd == "delete_recipe")
    {
        ESP_LOGI(TAG, "Called handleDeleteRecipe()");
        handleDeleteRecipe(dto.recipeId);
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
    else if (cmd == "delete_execution")
    {
        ESP_LOGI(TAG, "Called handleDeleteExecution(%s)", dto.executionId.c_str());
        handleDeleteExecution(dto.executionId);
    }
}
void RecipeApplicationService::handleStartRecipeFromJson(const std::string &jsonRecipe)
{
    if (!m_engine || !m_storageManager)
    {
        return;
    }
    RecipeParser parser;
    Recipe recipe;
    if (!parser.parseJsonToRecipe(jsonRecipe, recipe))
    {
        return;
    }
    uint32_t recipeIdHash = calculateRecipeIdHash(recipe.id());
    m_storageManager->saveRecipeWithCache(recipeIdHash, jsonRecipe, recipe);
    if (!m_engine->loadRecipe(recipe.steps(), recipe.id(), recipe.name()))
    {
        return;
    }
    if (!m_engine->start())
    {
        return;
    }
    sendLiveViewUpdate();
}
void RecipeApplicationService::handleStartRecipe(const std::string &recipeId)
{
    if (!m_storageManager || !m_engine)
    {
        return;
    }

    uint32_t recipeIdHash = calculateRecipeIdHash(recipeId);
    Recipe recipe;

    if (!m_storageManager->loadRecipeForExecution(recipeIdHash, recipe))
    {
        return;
    }

    if (!m_engine->loadRecipe(recipe.steps(), recipe.id(), recipe.name()))
    {
        return;
    }
    if (!m_engine->start())
    {
        return;
    }
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
            ParameterType pType = param.value.getType();
            if (pType == ParameterType::BOOLEAN) {
                paramDto.type = "bool";
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
            ioDto.description = "";
            ioDto.defaultPhysicalName = ioAlias.physicalName;
            if (ioAlias.isInput && ioAlias.isOutput)
            {
                ioDto.ioType = "input/output";
            }
            else if (ioAlias.isInput)
            {
                ioDto.ioType = "input";
            }
            else if (ioAlias.isOutput)
            {
                ioDto.ioType = "output";
            }
            else if (ioAlias.isSensor)
            {
                ioDto.ioType = "sensor";
            }
            else
            {
                ioDto.ioType = "unknown";
            }
            ioDto.valueType = ioAlias.valueType;
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
