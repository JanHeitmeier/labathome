#include "RecipeParser.hh"
#include "../third_party/rapidjson/document.h"
#include "../third_party/rapidjson/error/en.h"
#include <esp_log.h>
#include <inttypes.h>

static const char* TAG = "RecipeParser";

RecipeParser::RecipeParser() {
}

RecipeParser::~RecipeParser() {
}

bool RecipeParser::validateRecipeJson(const std::string& jsonText) {
    rapidjson::Document doc;
    doc.Parse(jsonText.c_str());
    
    if (doc.HasParseError()) {
        ESP_LOGE(TAG, "JSON Parse Error at offset %u: %s", 
                 (unsigned)doc.GetErrorOffset(), 
                 rapidjson::GetParseError_En(doc.GetParseError()));
        return false;
    }
    
    // Check required fields
    if (!doc.IsObject()) {
        ESP_LOGE(TAG, "JSON root is not an object");
        return false;
    }
    
    if (!doc.HasMember("id") || !doc["id"].IsString()) {
        ESP_LOGE(TAG, "Missing or invalid 'id' field");
        return false;
    }
    
    if (!doc.HasMember("name") || !doc["name"].IsString()) {
        ESP_LOGE(TAG, "Missing or invalid 'name' field");
        return false;
    }
    
    if (!doc.HasMember("steps") || !doc["steps"].IsArray()) {
        ESP_LOGE(TAG, "Missing or invalid 'steps' array");
        return false;
    }
    
    return true;
}

bool RecipeParser::parseJsonToRecipe(const std::string& jsonText, Recipe& outRecipe) {
    ESP_LOGE(TAG, "CHECKPOINT P1: parseJsonToRecipe START");
    rapidjson::Document doc;
    doc.Parse(jsonText.c_str());
    
    if (doc.HasParseError()) {
        ESP_LOGE(TAG, "ERROR CHECKPOINT P2: JSON Parse Error");
        return false;
    }
    
    if (!doc.IsObject()) {
        ESP_LOGE(TAG, "ERROR CHECKPOINT P3: JSON root is not an object");
        return false;
    }
    
    ESP_LOGE(TAG, "CHECKPOINT P4: JSON parsed OK");
    
    // Parse basic fields
    if (doc.HasMember("id") && doc["id"].IsString()) {
        outRecipe.setId(doc["id"].GetString());
    }
    
    if (doc.HasMember("name") && doc["name"].IsString()) {
        outRecipe.setName(doc["name"].GetString());
    }
    
    if (doc.HasMember("description") && doc["description"].IsString()) {
        outRecipe.setDescription(doc["description"].GetString());
    }
    
    if (doc.HasMember("version") && doc["version"].IsString()) {
        outRecipe.setVersion(doc["version"].GetString());
    }
    
    ESP_LOGE(TAG, "CHECKPOINT P5: Basic fields parsed, calling parseJsonToStepDescriptors");
    
    // Parse steps
    std::vector<StepInstanceDescriptor> steps;
    if (!parseJsonToStepDescriptors(jsonText, steps)) {
        ESP_LOGE(TAG, "ERROR CHECKPOINT P6: parseJsonToStepDescriptors failed");
        return false;
    }
    
    ESP_LOGE(TAG, "CHECKPOINT P7: Steps parsed, setting steps on recipe");
    
    outRecipe.setSteps(steps);
    
    ESP_LOGE(TAG, "CHECKPOINT P8: parseJsonToRecipe END - SUCCESS");
    
    return true;
}

bool RecipeParser::parseJsonToStepDescriptors(const std::string& jsonText, std::vector<StepInstanceDescriptor>& outSteps) {
    ESP_LOGE(TAG, "CHECKPOINT P10: parseJsonToStepDescriptors START");
    rapidjson::Document doc;
    doc.Parse(jsonText.c_str());
    
    if (doc.HasParseError()) {
        ESP_LOGE(TAG, "ERROR CHECKPOINT P11: JSON Parse Error");
        return false;
    }
    
    if (!doc.HasMember("steps") || !doc["steps"].IsArray()) {
        ESP_LOGE(TAG, "ERROR CHECKPOINT P12: Missing or invalid steps array");
        return false;
    }
    
    ESP_LOGE(TAG, "CHECKPOINT P13: JSON OK, getting steps array");
    
    const rapidjson::Value& stepsArray = doc["steps"];
    outSteps.clear();
    outSteps.reserve(stepsArray.Size());
    
    ESP_LOGE(TAG, "CHECKPOINT P14: Starting step loop");
    
    for (rapidjson::SizeType i = 0; i < stepsArray.Size(); i++) {
        ESP_LOGE(TAG, "CHECKPOINT P15: Processing step index");
        const rapidjson::Value& stepObj = stepsArray[i];
        
        if (!stepObj.IsObject()) {
            ESP_LOGW(TAG, "Step is not an object, skipping");
            continue;
        }
        
        StepInstanceDescriptor desc;
        
        ESP_LOGE(TAG, "CHECKPOINT P16: Parsing typeId");
        // Parse stepTypeId (kann als String kommen, aber wir brauchen uint32_t)
        if (stepObj.HasMember("stepTypeId") && stepObj["stepTypeId"].IsString()) {
            const char* typeIdStr = stepObj["stepTypeId"].GetString();
            // Konvertiere Hex-String zu uint32_t (z.B. "0x0001" -> 1)
            desc.typeId = static_cast<uint32_t>(strtoul(typeIdStr, nullptr, 0));
        } else if (stepObj.HasMember("stepTypeId") && stepObj["stepTypeId"].IsNumber()) {
            desc.typeId = stepObj["stepTypeId"].GetUint();
        } else {
            ESP_LOGW(TAG, "Step missing typeId, skipping");
            continue;
        }
        
        ESP_LOGE(TAG, "CHECKPOINT P17: Parsing systemId");
        // Generate systemId (könnte aus JSON kommen oder automatisch generiert werden)
        if (stepObj.HasMember("systemId") && stepObj["systemId"].IsString()) {
            desc.systemId = stepObj["systemId"].GetString();
        } else {
            // Auto-generate systemId
            char buf[32];
            snprintf(buf, sizeof(buf), "step_%u", i);
            desc.systemId = buf;
        }
        
        ESP_LOGE(TAG, "CHECKPOINT P18: Parsing parameters");
        // Parse parameters (optional)
        if (stepObj.HasMember("parameters") && stepObj["parameters"].IsObject()) {
            const rapidjson::Value& paramsObj = stepObj["parameters"];
            for (auto it = paramsObj.MemberBegin(); it != paramsObj.MemberEnd(); ++it) {
                std::string key = it->name.GetString();
                std::string value;
                
                if (it->value.IsString()) {
                    value = it->value.GetString();
                } else if (it->value.IsNumber()) {
                    char numBuf[32];
                    if (it->value.IsDouble()) {
                        snprintf(numBuf, sizeof(numBuf), "%f", it->value.GetDouble());
                    } else {
                        snprintf(numBuf, sizeof(numBuf), "%d", it->value.GetInt());
                    }
                    value = numBuf;
                } else if (it->value.IsBool()) {
                    value = it->value.GetBool() ? "true" : "false";
                }
                
                desc.params[key] = value;
            }
        }
        
        ESP_LOGE(TAG, "CHECKPOINT P19: Parsing aliases");
        // Parse aliases (optional - maps alias names to physical I/O resources)
        if (stepObj.HasMember("aliases") && stepObj["aliases"].IsObject()) {
            const rapidjson::Value& aliasesObj = stepObj["aliases"];
            for (auto it = aliasesObj.MemberBegin(); it != aliasesObj.MemberEnd(); ++it) {
                std::string aliasName = it->name.GetString();
                std::string physicalName;
                
                if (it->value.IsString()) {
                    physicalName = it->value.GetString();
                    desc.aliases[aliasName] = physicalName;
                }
            }
        }
        
        ESP_LOGE(TAG, "CHECKPOINT P20: Parsing repeatCount");
        // Parse repeat count (optional, default = 1)
        if (stepObj.HasMember("repeatCount") && stepObj["repeatCount"].IsInt()) {
            desc.repeatCount = stepObj["repeatCount"].GetInt();
        } else {
            desc.repeatCount = 1;
        }
        
        ESP_LOGE(TAG, "CHECKPOINT P21: Adding step to vector");
        outSteps.push_back(std::move(desc));
        
        ESP_LOGI(TAG, "Parsed step");
    }
    
    ESP_LOGE(TAG, "CHECKPOINT P22: parseJsonToStepDescriptors END - SUCCESS");
    return true;
}
