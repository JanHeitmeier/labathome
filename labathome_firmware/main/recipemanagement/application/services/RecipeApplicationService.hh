#pragma once

#include "../dtos/CommandDto.hh"
#include "../dtos/LiveViewDto.hh"
#include "../dtos/AvailableStepsDto.hh"
#include "../dtos/AvailableRecipesDto.hh"
#include "../dtos/RecipeDto.hh"
#include "../dtos/ExecutionHistoryDto.hh"
#include "../dtos/TimeSeriesDataDto.hh"
#include "../dtos/AuthResponseDto.hh"
#include "../../core/interfaces/storage/IRecipeStorage.hh"
#include "../interfaces/IMessageGateway.hh"
#include "../../infrastructure/engine/RecipeEngine.hh"
#include "StorageManager.hh"
#include "RecipeHistoryService.hh"
#include "AuthenticationManager.hh"
#include <memory>
#include <string>
#include <functional>

class RecipeApplicationService {
public:
    RecipeApplicationService(
        StorageManager* storageManager,
        RecipeEngine* engine,
        IMessageGateway* gateway,
        RecipeHistoryService* historyService = nullptr
    );
    
    ~RecipeApplicationService();
    
    /**
     * Unterstützte Commands:
     * - "start_recipe": Startet Rezept (recipeId erforderlich) [RecipeStarter]
     * - "stop_recipe": Stoppt laufendes Rezept [RecipeStarter]
     * - "pause_recipe": Pausiert laufendes Rezept [RecipeStarter]
     * - "resume_recipe": Setzt pausiertes Rezept fort [RecipeStarter]
     * - "acknowledge_step": Benutzer quittiert Warteposition [RecipeStarter]
     * - "get_recipe_list": Fordert Liste aller Rezepte an [Observer]
     * - "get_available_steps": Fordert Liste aller Step-Typen an [Observer]
     * - "save_recipe": Speichert Rezept (payload enthält RecipeDto als JSON) [RecipeEditor]
     * - "delete_recipe": Löscht Rezept (recipeId erforderlich) [RecipeEditor]
     * - "get_recipe": Lädt spezifisches Rezept (recipeId erforderlich) [Observer]
     * - "get_execution_history": Fordert Ausführungshistorie an [Observer]
     * - "get_timeseries": Lädt Zeitreihendaten (executionId in recipeId) [Observer]
     * - "delete_execution": Löscht Ausführung (executionId in recipeId) [RecipeEditor]
     * - "authenticate": Authentifiziert Benutzer, gibt Rolle zurück
     * - "change_password": Ändert Passwort (payload: role, oldPw, newPw) [entsprechende Rolle]
     * - "reset_passwords": Setzt alle Passwörter zurück [Admin]
     */
    void handleCommand(const CommandDto& dto);
    
    void setMessageGateway(IMessageGateway* gateway);
    
    void sendLiveViewUpdate();
    
private:
    void handleStartRecipe(const std::string& recipeId);
    void handleStartRecipeFromJson(const std::string& jsonRecipe);
    void handleStopRecipe();
    void handlePauseRecipe();
    void handleResumeRecipe();
    void handleAcknowledgeStep();
    void handleGetRecipeList();
    void handleGetAvailableSteps();
    void handleSaveRecipe(const std::string& payloadJson);
    void handleDeleteRecipe(const std::string& recipeId);
    void handleGetRecipe(const std::string& recipeId);
    void handleGetExecutionHistory();
    void handleGetTimeSeries(const std::string& executionId);
    void handleDeleteExecution(const std::string& executionId);
    void handleRequestLiveView();
    void handleLogin(const CommandDto& dto);  // Session-Token login
    void handleLogout(const CommandDto& dto);  // Invalidate token
    void handleChangePin(const CommandDto& dto);  // Admin: change PIN
    
    LiveViewDto buildLiveViewDto() const;
    AvailableRecipesDto buildAvailableRecipesDto() const;
    AvailableStepsDto buildAvailableStepsDto() const;
    
    bool validateAndExecute(const CommandDto& dto, UserRole requiredRole, std::function<void()> action);
    void sendAuthError(const CommandDto& dto, const std::string& message, int errorCode = 401);

private:
    StorageManager* m_storageManager;
    RecipeEngine* m_engine;
    IMessageGateway* m_gateway;
    RecipeHistoryService* m_historyService;
    AuthenticationManager* m_authManager;
    
    RecipeApplicationService(const RecipeApplicationService&) = delete;
    RecipeApplicationService& operator=(const RecipeApplicationService&) = delete;
    RecipeApplicationService(RecipeApplicationService&&) = delete;
    RecipeApplicationService& operator=(RecipeApplicationService&&) = delete;
};
