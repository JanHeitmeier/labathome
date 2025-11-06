#include "RecipeManagementPlugin.hh"
#include "../../recipemanagement/infrastructure/serialization/JsonSerialization.hh"
#include "../../recipemanagement/application/DeviceManager.hh"

RecipeManagementPlugin::RecipeManagementPlugin(DeviceManager* deviceManager)
    : m_deviceManager(deviceManager), m_callback(nullptr) {
}

void RecipeManagementPlugin::SetCallback(webmanager::iWebmanagerCallback* callback) {
    m_callback = callback;
}

void RecipeManagementPlugin::ProvideWebsocketMessage(const std::string& json) {
    // Eingehende Nachricht vom Frontend an den DeviceManager weiterleiten
    if (m_deviceManager) {
        m_deviceManager->HandleRecipeCommand(json);
    }
}

std::string RecipeManagementPlugin::GetPluginName() {
    return "RecipeManagement";
}

// IMessageGateway Implementation (Ausgehend)

void RecipeManagementPlugin::send(const LiveViewDto& dto) {
    if (!m_callback) return;
    
    std::string json = JsonSerialization::serialize(dto);
    m_callback->WrapAndSendAsync(json);
}

void RecipeManagementPlugin::send(const AvailableStepsDto& dto) {
    if (!m_callback) return;
    
    std::string json = JsonSerialization::serialize(dto);
    m_callback->WrapAndSendAsync(json);
}

void RecipeManagementPlugin::send(const AvailableRecipesDto& dto) {
    if (!m_callback) return;
    
    std::string json = JsonSerialization::serialize(dto);
    m_callback->WrapAndSendAsync(json);
}

void RecipeManagementPlugin::send(const RecipeDto& dto) {
    if (!m_callback) return;
    
    std::string json = JsonSerialization::serialize(dto);
    m_callback->WrapAndSendAsync(json);
}

void RecipeManagementPlugin::send(const MetricsDto& dto) {
    if (!m_callback) return;
    
    std::string json = JsonSerialization::serialize(dto);
    m_callback->WrapAndSendAsync(json);
}
