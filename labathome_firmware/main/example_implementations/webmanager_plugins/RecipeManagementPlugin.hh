#pragma once

#include "webmanager_interfaces.hh"
#include "../../recipemanagement/core/interfaces/messaging/IMessageGateway.hh"
#include <string>

// Forward declaration
class DeviceManager;

/**
 * @brief WebManager Plugin für Recipe Management
 * 
 * Verbindet das Recipe Management Framework mit dem WebManager:
 * - Empfängt JSON-Befehle vom Frontend (über ProvideWebsocketMessage)
 * - Sendet DTOs an das Frontend (über IMessageGateway-Interface)
 */
class RecipeManagementPlugin 
    : public webmanager::iWebmanagerPlugin,
      public IMessageGateway {
      
    DeviceManager* m_deviceManager;
    webmanager::iWebmanagerCallback* m_callback;
    
public:
    explicit RecipeManagementPlugin(DeviceManager* deviceManager);
    
    // === iWebmanagerPlugin Interface ===
    void SetCallback(webmanager::iWebmanagerCallback* callback) override;
    void ProvideWebsocketMessage(const std::string& json) override;
    std::string GetPluginName() override;
    
    // === IMessageGateway Interface (Ausgehende Nachrichten) ===
    void send(const LiveViewDto& dto) override;
    void send(const AvailableStepsDto& dto) override;
    void send(const AvailableRecipesDto& dto) override;
    void send(const RecipeDto& dto) override;
    void send(const MetricsDto& dto) override;
};
