#include "RecipeParser.hh"
#include "../third_party/rapidjson/document.h"
#include "../third_party/rapidjson/error/en.h"
#include <esp_log.h>

static const char* TAG = "RecipeParser";

RecipeParser::RecipeParser() {
}

RecipeParser::~RecipeParser() {
}

bool RecipeParser::parseJsonToRecipe(const std::string& jsonText, Recipe& outRecipe) {
    ESP_LOGI(TAG, "[PARSE] Parsing recipe JSON (length=%d)", jsonText.length());
    
    rapidjson::Document doc;
    doc.Parse(jsonText.c_str());
    
    if (doc.HasParseError()) {
        ESP_LOGE(TAG, "[PARSE] JSON parse error at offset %u: %s", 
                 doc.GetErrorOffset(), rapidjson::GetParseError_En(doc.GetParseError()));
        ESP_LOGE(TAG, "[PARSE] JSON snippet: %.100s", jsonText.c_str());
        return false;
    }
    
    if (!doc.IsObject()) {
        ESP_LOGE(TAG, "[PARSE] JSON root is not an object");
        return false;
    }
    
    ESP_LOGI(TAG, "[PARSE] JSON parsed successfully");
    
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
    
    std::map<std::string, std::string> globalParams;
    if (doc.HasMember("globalParameters") && doc["globalParameters"].IsObject()) {
        const rapidjson::Value& gp = doc["globalParameters"];
        for (auto it = gp.MemberBegin(); it != gp.MemberEnd(); ++it) {
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
            
            globalParams[key] = value;
        }
    }
    
    std::vector<StepInstanceDescriptor> steps;
    if (!parseJsonToStepDescriptors(jsonText, steps)) {
        ESP_LOGE(TAG, "[PARSE] Failed to parse step descriptors");
        return false;
    }
    ESP_LOGI(TAG, "[PARSE] Parsed %d steps", steps.size());
    
    for (auto& step : steps) {
        for (const auto& [key, value] : globalParams) {
            if (step.params.find(key) != step.params.end()) {
                step.params[key] = value;
            }
        }
    }
    
    outRecipe.setSteps(steps);
    
    ESP_LOGI(TAG, "[PARSE] Recipe parsing complete: id=%s, name=%s, steps=%d", 
             outRecipe.id().c_str(), outRecipe.name().c_str(), steps.size());
    return true;
}

bool RecipeParser::parseJsonToStepDescriptors(const std::string& jsonText, std::vector<StepInstanceDescriptor>& outSteps) {
    rapidjson::Document doc;
    doc.Parse(jsonText.c_str());
    
    if (doc.HasParseError()) {
        return false;
    }
    
    if (!doc.HasMember("steps") || !doc["steps"].IsArray()) {
        return false;
    }
    
    const rapidjson::Value& stepsArray = doc["steps"];
    outSteps.clear();
    outSteps.reserve(stepsArray.Size());
    
    for (rapidjson::SizeType i = 0; i < stepsArray.Size(); i++) {
        const rapidjson::Value& stepObj = stepsArray[i];
        
        if (!stepObj.IsObject()) {
            continue;
        }
        
        StepInstanceDescriptor desc;
        
        if (stepObj.HasMember("stepTypeId") && stepObj["stepTypeId"].IsString()) {
            const char* typeIdStr = stepObj["stepTypeId"].GetString();
            desc.typeId = static_cast<uint32_t>(strtoul(typeIdStr, nullptr, 0));
        } else if (stepObj.HasMember("stepTypeId") && stepObj["stepTypeId"].IsNumber()) {
            desc.typeId = stepObj["stepTypeId"].GetUint();
        } else {
            continue;
        }
        
        if (stepObj.HasMember("systemId") && stepObj["systemId"].IsString()) {
            desc.systemId = stepObj["systemId"].GetString();
        } else {
            char buf[32];
            snprintf(buf, sizeof(buf), "step_%u", i);
            desc.systemId = buf;
        }
        
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
        
        outSteps.push_back(std::move(desc));
    }
    
    return true;
}
