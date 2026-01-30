#include "RecipeSerializer.hh"
#include "../parsers/RecipeParser.hh"
#include "../third_party/rapidjson/document.h"
#include "../third_party/rapidjson/writer.h"
#include "../third_party/rapidjson/stringbuffer.h"
#include <esp_log.h>

static const char* TAG = "RecipeSerializer";

std::vector<uint8_t> RecipeSerializer::serialize(const Recipe& recipe) {
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    
    writer.StartObject();
    
    // Recipe Metadaten
    writer.Key("id");
    writer.String(recipe.id().c_str());
    
    writer.Key("name");
    writer.String(recipe.name().c_str());
    
    writer.Key("description");
    writer.String(recipe.description().c_str());
    
    writer.Key("version");
    writer.String(recipe.version().c_str());
    
    // Steps als Array
    writer.Key("steps");
    writer.StartArray();
    
    for (const auto& step : recipe.steps()) {
        writer.StartObject();
        
        writer.Key("systemId");
        writer.String(step.systemId.c_str());
        
        writer.Key("stepTypeId");
        writer.Uint(step.typeId);
        
        writer.Key("parameters");
        writer.StartObject();
        for (const auto& [key, val] : step.params) {
            writer.Key(key.c_str());
            writer.String(val.c_str());
        }
        writer.EndObject();
        
        writer.Key("aliases");
        writer.StartObject();
        for (const auto& [key, val] : step.aliases) {
            writer.Key(key.c_str());
            writer.String(val.c_str());
        }
        writer.EndObject();
        
        writer.EndObject();
    }
    
    writer.EndArray();
    writer.EndObject();
    
    std::string jsonText = sb.GetString();
    ESP_LOGI(TAG, "Serialized recipe '%s' with %zu steps to %zu bytes", 
             recipe.id().c_str(), recipe.steps().size(), jsonText.size());
    return std::vector<uint8_t>(jsonText.begin(), jsonText.end());
}

bool RecipeSerializer::deserialize(const std::vector<uint8_t>& data, Recipe& outRecipe) {
    if (data.empty()) {
        ESP_LOGE(TAG, "Empty data for deserialization");
        return false;
    }
    
    std::string jsonText(data.begin(), data.end());
    RecipeParser parser;
    bool success = parser.parseJsonToRecipe(jsonText, outRecipe);
    
    if (success) {
        ESP_LOGI(TAG, "Deserialized recipe '%s' with %zu steps from %zu bytes", 
                 outRecipe.id().c_str(), outRecipe.steps().size(), data.size());
    } else {
        ESP_LOGE(TAG, "Failed to deserialize recipe");
    }
    
    return success;
}
