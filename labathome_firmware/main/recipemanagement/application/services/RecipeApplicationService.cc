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
    IRecipeStorage *storage,
    RecipeEngine *engine,
    IMessageGateway *gateway,
    RecipeHistoryService* historyService) : m_storage(storage),
                                m_storageManager(new RecipeStorageManager(storage)),
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
    delete m_storageManager;
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
    ESP_LOGI(TAG, "[HANDLE_CMD] Received command");
    
    if (cmd == "start_recipe")
    {
        if (!dto.payload.empty())
        {
            handleStartRecipeFromJson(dto.payload);
        }
        else
        {
            handleStartRecipe(dto.recipeId);
        }
    }
    else if (cmd == "stop_recipe")
    {
        handleStopRecipe();
    }
    else if (cmd == "pause_recipe")
    {
        handlePauseRecipe();
    }
    else if (cmd == "resume_recipe")
    {
        handleResumeRecipe();
    }
    else if (cmd == "acknowledge_step")
    {
        ESP_LOGI(TAG, "Command received: acknowledge_step");
        handleAcknowledgeStep();
    }
    else if (cmd == "get_recipe_list")
    {
        handleGetRecipeList();
    }
    else if (cmd == "get_available_steps")
    {
        handleGetAvailableSteps();
    }
    else if (cmd == "save_recipe")
    {
        handleSaveRecipe(dto.payload);
    }
    else if (cmd == "delete_recipe")
    {
        handleDeleteRecipe(dto.recipeId);
    }
    else if (cmd == "get_recipe")
    {
        handleGetRecipe(dto.recipeId);
    }
    else if (cmd == "request_live_view")
    {
        handleRequestLiveView();
    }
    else if (cmd == "get_execution_history")
    {
        handleGetExecutionHistory();
    }
    else if (cmd == "get_timeseries")
    {
        handleGetTimeSeries(dto.recipeId);
    }
    else if (cmd == "delete_execution")
    {
        handleDeleteExecution(dto.recipeId);
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
    if (!m_engine->loadRecipe(recipe.steps(), recipe.id()))
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

    if (!m_engine->loadRecipe(recipe.steps(), recipe.id()))
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
    ESP_LOGI(TAG, "handleAcknowledgeStep called");
    if (!m_engine)
    {
        ESP_LOGW(TAG, "No engine available for acknowledgment");
        return;
    }
    m_engine->acknowledgeStep();
    ESP_LOGI(TAG, "Engine acknowledgeStep() executed");
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
        RecipeInfoDto recipe1;
        recipe1.id = "recipe_001";
        recipe1.name = "Fermentation Process";
        recipe1.description = "Standard fermentation with temperature control";
        recipe1.createdAt = 1699200000000;
        recipe1.lastModified = 1699200000000;
        dto.recipes.push_back(recipe1);
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
            switch (pType)
            {
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
    ESP_LOGI(TAG, "[GET_EXEC_HIST] handleGetExecutionHistory called");
    
    if (!m_historyService || !m_gateway) {
        ESP_LOGE(TAG, "[GET_EXEC_HIST] Missing historyService or gateway");
        return;
    }
    
    ESP_LOGI(TAG, "[GET_EXEC_HIST] Calling getExecutionHistory");
    ExecutionHistoryDto dto = m_historyService->getExecutionHistory();
    
    ESP_LOGI(TAG, "[GET_EXEC_HIST] Sending ExecutionHistoryDto to frontend");
    m_gateway->send(dto);
}

void RecipeApplicationService::handleGetTimeSeries(const std::string& executionId) {
    ESP_LOGI(TAG, "[GET_TS] handleGetTimeSeries called for executionId='%s'", executionId.c_str());
    
    if (!m_historyService || !m_gateway) {
        ESP_LOGE(TAG, "[GET_TS] Missing historyService or gateway!");
        return;
    }
    
    TimeSeriesDataDto dto;
    dto.executionId = executionId;
    
    ESP_LOGI(TAG, "[GET_TS] Loading time series from storage...");
    std::vector<SensorTimeSeries> series = m_historyService->getTimeSeriesStorage()->loadTimeSeries(executionId);
    
    if (series.empty()) {
        ESP_LOGW(TAG, "[GET_TS] TimeSeries for execution %s not found or empty", executionId.c_str());
        m_gateway->send(dto);
        return;
    }
    
    ESP_LOGI(TAG, "[GET_TS] Found %zu series, converting to DTO...", series.size());
    
    for (const auto& ts : series) {
        ESP_LOGI(TAG, "  [GET_TS] Converting sensor '%s' with %zu data points", ts.sensorName.c_str(), ts.dataPoints.size());
        
        SensorTimeSeriesDto sensorDto;
        sensorDto.sensorName = ts.sensorName;
        sensorDto.unit = ts.unit;
        for (const auto& pt : ts.dataPoints) {
            TimeSeriesPointDto ptDto;
            ptDto.timestamp = pt.timestamp;
            ptDto.value = pt.value;
            sensorDto.dataPoints.push_back(ptDto);
        }
        dto.series.push_back(sensorDto);
    }
    
    ESP_LOGI(TAG, "[GET_TS] Sending TimeSeriesDataDto with %zu series to frontend", dto.series.size());
    m_gateway->send(dto);
}

void RecipeApplicationService::handleDeleteExecution(const std::string& executionId) {
    if (!m_historyService) return;
    
    m_historyService->deleteExecution(executionId);
    handleGetExecutionHistory();
}

void RecipeApplicationService::handleRequestLiveView() {
    sendLiveViewUpdate();
}
