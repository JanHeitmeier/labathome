#pragma once

#include "../dtos/CommandDto.hh"
#include "../dtos/LiveViewDto.hh"
#include "../dtos/AvailableStepsDto.hh"
#include "../dtos/AvailableRecipesDto.hh"
#include "../dtos/RecipeDto.hh"
#include "../dtos/MetricsDto.hh"
#include "../../core/interfaces/storage/IRecipeStorage.hh"
#include "../interfaces/IMessageGateway.hh"
#include "../../infrastructure/engine/RecipeEngine.hh"
#include "RecipeStorageManager.hh"
#include <memory>
#include <string>

class RecipeApplicationService {
public:
    /**
     * @brief Konstruktor
     * @param storage Zeiger auf IRecipeStorage-Implementierung
     * @param engine Zeiger auf RecipeEngine-Instanz
     * @param gateway Zeiger auf IMessageGateway für ausgehende Nachrichten
     */
    RecipeApplicationService(
        IRecipeStorage* storage,
        RecipeEngine* engine,
        IMessageGateway* gateway
    );
    
    ~RecipeApplicationService();
    
    // Command-Verarbeitung
    
    /**
     * @brief Verarbeitet eingehende Befehle von der UI
     * @param dto CommandDto mit Befehl und optionalen Parametern
     * 
     * Unterstützte Commands:
     * - "start_recipe": Startet Rezept (recipeId erforderlich)
     * - "stop_recipe": Stoppt laufendes Rezept
     * - "pause_recipe": Pausiert laufendes Rezept
     * - "resume_recipe": Setzt pausiertes Rezept fort
     * - "acknowledge_step": Benutzer quittiert Warteposition
     * - "get_recipe_list": Fordert Liste aller Rezepte an
     * - "get_available_steps": Fordert Liste aller Step-Typen an
     * - "save_recipe": Speichert Rezept (payload enthält RecipeDto als JSON)
     * - "delete_recipe": Löscht Rezept (recipeId erforderlich)
     * - "get_recipe": Lädt spezifisches Rezept (recipeId erforderlich)
     * - "get_metrics": Fordert Metriken für aktives Rezept an
     */
    void handleCommand(const CommandDto& dto);
    
    /**
     * @brief Setzt den IMessageGateway für ausgehende Nachrichten
     * @param gateway Zeiger auf IMessageGateway-Implementierung
     * @note Wird vom DeviceManager aufgerufen nach Instanziierung
     */
    void setMessageGateway(IMessageGateway* gateway);
    
    /**
     * @brief Sendet periodische Status-Updates an die UI
     * @note Wird von RecipeEngine oder periodischem Timer aufgerufen
     */
    void sendLiveViewUpdate();
    
private:
    // Befehlsverarbeitung (interne Handler)
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
    void handleGetMetrics();
    
    // Hilfsmethoden
    LiveViewDto buildLiveViewDto() const;
    AvailableRecipesDto buildAvailableRecipesDto() const;
    AvailableStepsDto buildAvailableStepsDto() const;
    MetricsDto buildMetricsDto() const;
    
private:
    IRecipeStorage* m_storage;      // Storage-Implementierung (nicht owned)
    RecipeStorageManager* m_storageManager;  // Storage-Manager (owned)
    RecipeEngine* m_engine;         // Recipe-Engine (nicht owned)
    IMessageGateway* m_gateway;     // Message-Gateway (nicht owned)
    
    // Kopier-/Move-Konstruktoren deaktivieren
    RecipeApplicationService(const RecipeApplicationService&) = delete;
    RecipeApplicationService& operator=(const RecipeApplicationService&) = delete;
    RecipeApplicationService(RecipeApplicationService&&) = delete;
    RecipeApplicationService& operator=(RecipeApplicationService&&) = delete;
};
