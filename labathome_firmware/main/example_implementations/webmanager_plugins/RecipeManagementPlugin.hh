#pragma once

#include "webmanager_interfaces.hh"
#include "recipemanagement/application/interfaces/IMessageGateway.hh"
#include <string>
#include <flatbuffers/flatbuffers.h>

// Forward declarations
class DeviceManager;
namespace recipemanagement { struct JsonPayload; struct ResponseWrapper; }

/**
 * @brief WebManager Plugin für Recipe Management
 * 
 * Verbindet das Recipe Management Framework mit dem WebManager:
 * - Empfängt JSON-Befehle vom Frontend (über ProvideWebsocketMessage)
 * - Sendet DTOs an das Frontend (über IMessageGateway-Interface)
 * - Nutzt FlatBuffers Namespace 11 mit JSON-Wrapper für Kompatibilität
 */
class RecipeManagementPlugin 
    : public webmanager::iWebmanagerPlugin,
      public IMessageGateway {
      
    DeviceManager* m_deviceManager;
    webmanager::iWebmanagerCallback* m_callback;
    
    static constexpr uint32_t RECIPE_NAMESPACE = 11; // ns11recipemanagement
    static constexpr const char* TAG = "RecipePlugin";
    
    /**
     * Sendet JSON-String als FlatBuffers-Payload an das Frontend
     */
    void sendJsonWrapped(const std::string& json);
    
public:
    explicit RecipeManagementPlugin(DeviceManager* deviceManager);
    
    // === iWebmanagerPlugin Interface (KORREKTES Interface) ===
    void OnBegin(webmanager::iWebmanagerCallback* callback) override;
    void OnWifiConnect(webmanager::iWebmanagerCallback* callback) override;
    void OnWifiDisconnect(webmanager::iWebmanagerCallback* callback) override;
    void OnTimeUpdate(webmanager::iWebmanagerCallback* callback) override;
    webmanager::eMessageReceiverResult ProvideWebsocketMessage(
        webmanager::iWebmanagerCallback* callback,
        httpd_req_t* req,
        httpd_ws_frame_t* ws_pkt,
        uint32_t ns,
        uint8_t* buf
    ) override;
    
    // === IMessageGateway Interface (Ausgehende Nachrichten) ===
    void send(const LiveViewDto& dto) override;
    void send(const AvailableStepsDto& dto) override;
    void send(const AvailableRecipesDto& dto) override;
    void send(const RecipeDto& dto) override;
    void send(const ExecutionHistoryDto& dto) override;
    void send(const TimeSeriesDataDto& dto) override;
};
