#include "RecipeManagementPlugin.hh"
#include "../../devicemanager.hh"
#include "infrastructure/serialization/JsonSerialization.hh"
#include "application/dtos/LiveViewDto.hh"
#include "application/dtos/AvailableStepsDto.hh"
#include "application/dtos/AvailableRecipesDto.hh"
#include "application/dtos/RecipeDto.hh"
#include "application/dtos/MetricsDto.hh"
#include "flatbuffers_cpp/ns11recipemanagement_generated.h"
#include <esp_log.h>

static const char* TAG = "RecipePlugin";

RecipeManagementPlugin::RecipeManagementPlugin(DeviceManager* deviceManager)
    : m_deviceManager(deviceManager), m_callback(nullptr) {
}

// === iWebmanagerPlugin Interface Implementation ===

void RecipeManagementPlugin::OnBegin(webmanager::iWebmanagerCallback* callback) {
    m_callback = callback;
    ESP_LOGI(TAG, "RecipeManagement Plugin initialized with namespace %lu", RECIPE_NAMESPACE);
}

void RecipeManagementPlugin::OnWifiConnect(webmanager::iWebmanagerCallback* callback) {
    (void)callback;
    ESP_LOGD(TAG, "WiFi connected");
}

void RecipeManagementPlugin::OnWifiDisconnect(webmanager::iWebmanagerCallback* callback) {
    (void)callback;
    ESP_LOGD(TAG, "WiFi disconnected");
}

void RecipeManagementPlugin::OnTimeUpdate(webmanager::iWebmanagerCallback* callback) {
    (void)callback;
    ESP_LOGD(TAG, "Time updated");
}

webmanager::eMessageReceiverResult RecipeManagementPlugin::ProvideWebsocketMessage(
    webmanager::iWebmanagerCallback* callback,
    httpd_req_t* req,
    httpd_ws_frame_t* ws_pkt,
    uint32_t ns,
    uint8_t* buf
) {
    (void)req;
    (void)callback;
    
    // Check if this message is for us
    if (ns != RECIPE_NAMESPACE) {
        return webmanager::eMessageReceiverResult::NOT_FOR_ME;
    }
    
    if (!m_deviceManager) {
        ESP_LOGW(TAG, "No DeviceManager available");
        return webmanager::eMessageReceiverResult::FOR_ME_BUT_FAILED;
    }
    
    // Parse FlatBuffers payload
    auto wrapper = ::flatbuffers::GetRoot<recipemanagement::RequestWrapper>(buf);
    if (!wrapper || wrapper->request_type() != recipemanagement::Requests_RequestJson) {
        ESP_LOGW(TAG, "Invalid FlatBuffers message or wrong request type");
        return webmanager::eMessageReceiverResult::FOR_ME_BUT_FAILED;
    }
    
    auto request = wrapper->request_as_RequestJson();
    if (!request || !request->payload() || !request->payload()->json()) {
        ESP_LOGW(TAG, "Missing JSON payload in request");
        return webmanager::eMessageReceiverResult::FOR_ME_BUT_FAILED;
    }
    
    const char* json_cstr = request->payload()->json()->c_str();
    std::string json(json_cstr ? json_cstr : "{}");
    
    ESP_LOGI(TAG, "Received JSON command: %s", json.c_str());
    
    // Forward to DeviceManager
    m_deviceManager->HandleRecipeCommand(json);
    
    return webmanager::eMessageReceiverResult::OK;
}

// === Helper Method ===

void RecipeManagementPlugin::sendJsonWrapped(const std::string& json) {
    if (!m_callback) {
        ESP_LOGW(TAG, "Cannot send JSON - no callback available");
        return;
    }
    
    // Build FlatBuffers message with JSON payload
    flatbuffers::FlatBufferBuilder builder(json.size() + 128);
    
    auto jsonOffset = builder.CreateString(json);
    auto payloadOffset = recipemanagement::CreateJsonPayload(builder, jsonOffset);
    auto responseOffset = recipemanagement::CreateResponseJson(builder, payloadOffset);
    auto wrapperOffset = recipemanagement::CreateResponseWrapper(
        builder, 
        recipemanagement::Responses_ResponseJson, 
        responseOffset.Union()
    );
    
    builder.Finish(wrapperOffset);
    
    // Send via WebManager
    esp_err_t result = m_callback->WrapAndSendAsync(RECIPE_NAMESPACE, builder);
    
    if (result != ESP_OK) {
        ESP_LOGW(TAG, "Failed to send JSON: %d", result);
    } else {
        ESP_LOGD(TAG, "Sent JSON (%zu bytes)", json.size());
    }
}

// === IMessageGateway Implementation (Ausgehend) ===

void RecipeManagementPlugin::send(const LiveViewDto& dto) {
    std::string json = JsonSerialization::serialize(dto);
    sendJsonWrapped(json);
}

void RecipeManagementPlugin::send(const AvailableStepsDto& dto) {
    std::string json = JsonSerialization::serialize(dto);
    sendJsonWrapped(json);
}

void RecipeManagementPlugin::send(const AvailableRecipesDto& dto) {
    std::string json = JsonSerialization::serialize(dto);
    sendJsonWrapped(json);
}

void RecipeManagementPlugin::send(const RecipeDto& dto) {
    std::string json = JsonSerialization::serialize(dto);
    sendJsonWrapped(json);
}

void RecipeManagementPlugin::send(const MetricsDto& dto) {
    std::string json = JsonSerialization::serialize(dto);
    sendJsonWrapped(json);
}
