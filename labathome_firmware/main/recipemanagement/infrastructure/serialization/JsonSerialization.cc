#include "JsonSerialization.hh"
#include "../third_party/rapidjson/document.h"
#include "../third_party/rapidjson/writer.h"
#include "../third_party/rapidjson/stringbuffer.h"
#include <cstring>
#include <esp_log.h>

// ========== Serialisierung mit Buffer (DTO → JSON, Zero-Allocation) ==========

bool JsonSerialization::serializeToBuffer(const LiveViewDto& dto, char* buffer, size_t bufferSize, size_t& outLength) {
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    
    writer.StartObject();
    
    writer.Key("type");
    writer.String("liveview");
    
    writer.Key("recipeId");
    writer.String(dto.recipeId.c_str());
    
    writer.Key("recipeName");
    writer.String(dto.recipeName.c_str());
    
    writer.Key("currentStepIndex");
    writer.Int(dto.currentStepIndex);
    
    writer.Key("totalSteps");
    writer.Int(dto.totalSteps);
    
    writer.Key("currentStepName");
    writer.String(dto.currentStepName.c_str());
    
    writer.Key("stepState");
    writer.String(dto.stepState.c_str());
    
    writer.Key("recipeStatus");
    writer.String(dto.recipeStatus.c_str());
    
    writer.Key("userInstruction");
    writer.String(dto.userInstruction.c_str());
    
    writer.Key("awaitingUserAcknowledgment");
    writer.Bool(dto.awaitingUserAcknowledgment);
    
    writer.Key("progress");
    writer.Double(dto.progress);
    
    writer.Key("timestamp");
    writer.Uint64(dto.timestamp);
    
    writer.Key("errorMessage");
    writer.String(dto.errorMessage.c_str());
    
    writer.Key("sensorValues");
    writer.StartObject();
    for (const auto& [key, value] : dto.sensorValues) {
        writer.Key(key.c_str());
        writer.Double(value);
    }
    writer.EndObject();
    
    writer.EndObject();
    
    if (sb.GetSize() >= bufferSize) {
        outLength = 0;
        return false;
    }
    
    std::memcpy(buffer, sb.GetString(), sb.GetSize());
    buffer[sb.GetSize()] = '\0';
    outLength = sb.GetSize();
    
    return true;
}

bool JsonSerialization::serializeToBuffer(const AvailableStepsDto& dto, char* buffer, size_t bufferSize, size_t& outLength) {
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    
    writer.StartObject();
    writer.Key("type");
    writer.String("available_steps");
    
    writer.Key("steps");
    writer.StartArray();
    
    for (const auto& step : dto.steps) {
        writer.StartObject();
        
        writer.Key("typeId");
        writer.String(step.typeId.c_str());
        
        writer.Key("displayName");
        writer.String(step.displayName.c_str());
        
        writer.Key("description");
        writer.String(step.description.c_str());
        
        writer.Key("category");
        writer.String(step.category.c_str());
        
        writer.Key("parameters");
        writer.StartArray();
        for (const auto& param : step.parameters) {
            writer.StartObject();
            writer.Key("name");
            writer.String(param.name.c_str());
            writer.Key("type");
            writer.String(param.type.c_str());
            writer.Key("description");
            writer.String(param.description.c_str());
            writer.Key("defaultValue");
            writer.String(param.defaultValue.c_str());
            writer.Key("minValue");
            writer.String(param.minValue.c_str());
            writer.Key("maxValue");
            writer.String(param.maxValue.c_str());
            writer.Key("required");
            writer.Bool(param.required);
            writer.Key("unit");
            writer.String(param.unit.c_str());
            writer.EndObject();
        }
        writer.EndArray();
        
        writer.Key("ioAliases");
        writer.StartArray();
        for (const auto& io : step.ioAliases) {
            writer.StartObject();
            writer.Key("aliasName");
            writer.String(io.aliasName.c_str());
            writer.Key("ioType");
            writer.String(io.ioType.c_str());
            writer.Key("valueType");
            writer.String(io.valueType.c_str());
            writer.Key("description");
            writer.String(io.description.c_str());
            writer.Key("defaultPhysicalName");
            writer.String(io.defaultPhysicalName.c_str());
            writer.EndObject();
        }
        writer.EndArray();
        
        writer.EndObject();
    }
    
    writer.EndArray();
    writer.EndObject();
    
    if (sb.GetSize() >= bufferSize) {
        outLength = 0;
        return false;
    }
    
    std::memcpy(buffer, sb.GetString(), sb.GetSize());
    buffer[sb.GetSize()] = '\0';
    outLength = sb.GetSize();
    
    return true;
}

bool JsonSerialization::serializeToBuffer(const RecipeListDto& dto, char* buffer, size_t bufferSize, size_t& outLength) {
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    
    writer.StartObject();
    writer.Key("type");
    writer.String("available_recipes");
    
    writer.Key("recipes");
    writer.StartArray();
    
    for (const auto& recipe : dto.recipes) {
        writer.StartObject();
        writer.Key("id");
        writer.String(recipe.id.c_str());
        writer.Key("name");
        writer.String(recipe.name.c_str());
        writer.Key("description");
        writer.String(recipe.description.c_str());
        writer.Key("version");
        writer.String(recipe.version.c_str());
        writer.Key("createdAt");
        writer.Uint64(recipe.createdAt);
        writer.Key("lastModified");
        writer.Uint64(recipe.lastModified);
        writer.EndObject();
    }
    
    writer.EndArray();
    writer.EndObject();
    
    if (sb.GetSize() >= bufferSize) {
        outLength = 0;
        return false;
    }
    
    std::memcpy(buffer, sb.GetString(), sb.GetSize());
    buffer[sb.GetSize()] = '\0';
    outLength = sb.GetSize();
    
    return true;
}

bool JsonSerialization::serializeToBuffer(const RecipeDto& dto, char* buffer, size_t bufferSize, size_t& outLength) {
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    
    writer.StartObject();
    
    writer.Key("id");
    writer.String(dto.id.c_str());
    
    writer.Key("name");
    writer.String(dto.name.c_str());
    
    writer.Key("description");
    writer.String(dto.description.c_str());
    
    writer.Key("author");
    writer.String(dto.author.c_str());
    
    writer.Key("version");
    writer.String(dto.version.c_str());
    
    writer.Key("createdAt");
    writer.Uint64(dto.createdAt);
    
    writer.Key("lastModified");
    writer.Uint64(dto.lastModified);
    
    writer.Key("steps");
    writer.StartArray();
    for (const auto& step : dto.steps) {
        writer.StartObject();
        
        writer.Key("stepTypeId");
        writer.String(step.stepTypeId.c_str());
        
        writer.Key("order");
        writer.Int(step.order);
        
        writer.Key("parameters");
        writer.StartObject();
        for (const auto& [key, value] : step.parameters) {
            writer.Key(key.c_str());
            writer.String(value.c_str());
        }
        writer.EndObject();
        
        writer.Key("aliases");
        writer.StartObject();
        for (const auto& [key, value] : step.aliases) {
            writer.Key(key.c_str());
            writer.String(value.c_str());
        }
        writer.EndObject();
        
        writer.EndObject();
    }
    writer.EndArray();
    
    writer.EndObject();
    
    if (sb.GetSize() >= bufferSize) {
        outLength = 0;
        return false;
    }
    
    std::memcpy(buffer, sb.GetString(), sb.GetSize());
    buffer[sb.GetSize()] = '\0';
    outLength = sb.GetSize();
    
    return true;
}

bool JsonSerialization::serializeToBuffer(const MetricsDto& dto, char* buffer, size_t bufferSize, size_t& outLength) {
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    
    writer.StartObject();
    
    // recipeId
    writer.Key("recipeId");
    writer.String(dto.recipeId.c_str());
    
    // series: Array von MetricSeriesDto
    writer.Key("series");
    writer.StartArray();
    for (const auto& series : dto.series) {
        writer.StartObject();
        
        writer.Key("name");
        writer.String(series.name.c_str());
        
        writer.Key("unit");
        writer.String(series.unit.c_str());
        
        // data: Array von MetricDataPointDto
        writer.Key("data");
        writer.StartArray();
        for (const auto& point : series.data) {
            writer.StartObject();
            writer.Key("timestamp");
            writer.Uint64(point.timestamp);
            writer.Key("value");
            writer.Double(point.value);
            writer.EndObject();
        }
        writer.EndArray();
        
        writer.EndObject();
    }
    writer.EndArray();
    
    writer.EndObject();
    
    if (sb.GetSize() >= bufferSize) {
        outLength = 0;
        return false;
    }
    
    std::memcpy(buffer, sb.GetString(), sb.GetSize());
    buffer[sb.GetSize()] = '\0';
    outLength = sb.GetSize();
    
    return true;
}

// ========== Serialisierung mit std::string (Convenience-Wrapper) ==========

std::string JsonSerialization::serialize(const LiveViewDto& dto) {
    char buffer[2048];
    size_t length;
    if (serializeToBuffer(dto, buffer, sizeof(buffer), length)) {
        return std::string(buffer, length);
    }
    return "";
}

std::string JsonSerialization::serialize(const AvailableStepsDto& dto) {
    ESP_LOGI("JsonSerialization", "serialize(AvailableStepsDto) with %d steps", dto.steps.size());
    
    // Heap allocation to avoid stack overflow (8KB is too large for stack)
    constexpr size_t BUFFER_SIZE = 8192;
    char* buffer = new(std::nothrow) char[BUFFER_SIZE];
    if (!buffer) {
        ESP_LOGE("JsonSerialization", "Failed to allocate buffer!");
        return "";
    }
    
    size_t length;
    bool success = serializeToBuffer(dto, buffer, BUFFER_SIZE, length);
    
    if (success) {
        ESP_LOGI("JsonSerialization", "Serialized successfully: %d bytes", length);
        std::string result(buffer, length);
        delete[] buffer;
        return result;
    }
    
    ESP_LOGE("JsonSerialization", "Serialization FAILED!");
    delete[] buffer;
    return "";
}

std::string JsonSerialization::serialize(const RecipeListDto& dto) {
    constexpr size_t BUFFER_SIZE = 4096;
    char* buffer = new(std::nothrow) char[BUFFER_SIZE];
    if (!buffer) {
        ESP_LOGE("JsonSerialization", "Failed to allocate buffer for RecipeListDto!");
        return "";
    }
    
    size_t length;
    bool success = serializeToBuffer(dto, buffer, BUFFER_SIZE, length);
    
    if (success) {
        std::string result(buffer, length);
        delete[] buffer;
        return result;
    }
    
    delete[] buffer;
    return "";
}

std::string JsonSerialization::serialize(const RecipeDto& dto) {
    constexpr size_t BUFFER_SIZE = 8192;
    char* buffer = new(std::nothrow) char[BUFFER_SIZE];
    if (!buffer) {
        ESP_LOGE("JsonSerialization", "Failed to allocate buffer for RecipeDto!");
        return "";
    }
    
    size_t length;
    bool success = serializeToBuffer(dto, buffer, BUFFER_SIZE, length);
    
    if (success) {
        std::string result(buffer, length);
        delete[] buffer;
        return result;
    }
    
    delete[] buffer;
    return "";
}

std::string JsonSerialization::serialize(const MetricsDto& dto) {
    constexpr size_t BUFFER_SIZE = 8192;
    char* buffer = new(std::nothrow) char[BUFFER_SIZE];
    if (!buffer) {
        ESP_LOGE("JsonSerialization", "Failed to allocate buffer for MetricsDto!");
        return "";
    }
    
    size_t length;
    bool success = serializeToBuffer(dto, buffer, BUFFER_SIZE, length);
    
    if (success) {
        std::string result(buffer, length);
        delete[] buffer;
        return result;
    }
    
    delete[] buffer;
    return "";
}

// ========== Deserialisierung (JSON → DTO, Zero-Copy mit string_view) ==========

bool JsonSerialization::deserialize(std::string_view json, CommandDto& outDto) {
    rapidjson::Document doc;
    doc.Parse(json.data(), json.length());
    
    if (doc.HasParseError()) {
        return false;
    }
    
    if (!doc.IsObject() || !doc.HasMember("command") || !doc["command"].IsString()) {
        return false;
    }
    
    outDto.command = doc["command"].GetString();
    
    // recipeId ist optional
    if (doc.HasMember("recipeId") && doc["recipeId"].IsString()) {
        outDto.recipeId = doc["recipeId"].GetString();
    } else {
        outDto.recipeId.clear();
    }
    
    // requestId ist optional (für Request/Response-Matching)
    if (doc.HasMember("requestId") && doc["requestId"].IsString()) {
        outDto.requestId = doc["requestId"].GetString();
    } else {
        outDto.requestId.clear();
    }
    
    // Payload ist optional
    if (doc.HasMember("payload")) {
        // Payload als JSON-String serialisieren (für generische Verarbeitung)
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        doc["payload"].Accept(writer);
        outDto.payload = sb.GetString();
    } else {
        outDto.payload.clear();
    }
    
    return true;
}

bool JsonSerialization::deserialize(std::string_view json, RecipeDto& outDto) {
    rapidjson::Document doc;
    doc.Parse(json.data(), json.length());
    
    if (doc.HasParseError()) {
        return false;
    }
    
    if (!doc.IsObject()) {
        return false;
    }
    
    // Pflichtfelder
    if (!doc.HasMember("id") || !doc["id"].IsString()) return false;
    outDto.id = doc["id"].GetString();
    
    if (!doc.HasMember("name") || !doc["name"].IsString()) return false;
    outDto.name = doc["name"].GetString();
    
    if (!doc.HasMember("description") || !doc["description"].IsString()) return false;
    outDto.description = doc["description"].GetString();
    
    // Optional: author and version
    if (doc.HasMember("author") && doc["author"].IsString()) {
        outDto.author = doc["author"].GetString();
    } else {
        outDto.author = "";
    }
    
    if (doc.HasMember("version") && doc["version"].IsString()) {
        outDto.version = doc["version"].GetString();
    } else {
        outDto.version = "1.0";
    }
    
    // Optional: timestamps
    if (doc.HasMember("createdAt") && doc["createdAt"].IsUint64()) {
        outDto.createdAt = doc["createdAt"].GetUint64();
    } else {
        outDto.createdAt = 0;
    }
    
    if (doc.HasMember("lastModified") && doc["lastModified"].IsUint64()) {
        outDto.lastModified = doc["lastModified"].GetUint64();
    } else {
        outDto.lastModified = 0;
    }
    
    // Steps-Array
    if (!doc.HasMember("steps") || !doc["steps"].IsArray()) return false;
    
    const auto& stepsArray = doc["steps"].GetArray();
    outDto.steps.clear();
    outDto.steps.reserve(stepsArray.Size());
    
    for (const auto& stepJson : stepsArray) {
        if (!stepJson.IsObject()) continue;
        
        StepConfigDto step;
        
        if (stepJson.HasMember("stepTypeId") && stepJson["stepTypeId"].IsString()) {
            step.stepTypeId = stepJson["stepTypeId"].GetString();
        }
        if (stepJson.HasMember("order") && stepJson["order"].IsInt()) {
            step.order = stepJson["order"].GetInt();
        }
        
        // Parameters-Map
        if (stepJson.HasMember("parameters") && stepJson["parameters"].IsObject()) {
            const auto& paramsObj = stepJson["parameters"].GetObject();
            for (auto it = paramsObj.MemberBegin(); it != paramsObj.MemberEnd(); ++it) {
                if (it->value.IsString()) {
                    step.parameters[it->name.GetString()] = it->value.GetString();
                }
            }
        }
        
        // Aliases-Map
        if (stepJson.HasMember("aliases") && stepJson["aliases"].IsObject()) {
            const auto& aliasesObj = stepJson["aliases"].GetObject();
            for (auto it = aliasesObj.MemberBegin(); it != aliasesObj.MemberEnd(); ++it) {
                if (it->value.IsString()) {
                    step.aliases[it->name.GetString()] = it->value.GetString();
                }
            }
        }
        
        outDto.steps.push_back(std::move(step));
    }
    
    return true;
}
