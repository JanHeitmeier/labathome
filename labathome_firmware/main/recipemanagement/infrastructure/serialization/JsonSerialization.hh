#pragma once

#include <string>
#include <string_view>
#include <cstddef>
#include <map>
#include "../../application/dtos/CommandDto.hh"
#include "../../application/dtos/LiveViewDto.hh"
#include "../../application/dtos/AvailableStepsDto.hh"
#include "../../application/dtos/AvailableRecipesDto.hh"
#include "../../application/dtos/RecipeDto.hh"
#include "../../application/dtos/ExecutionHistoryDto.hh"
#include "../../application/dtos/TimeSeriesBinaryDto.hh"
#include "../../application/dtos/AuthResponseDto.hh"

class JsonSerialization {
public:
    // ========== Serialisierung (DTO → JSON) ==========

    static bool serializeToBuffer(const LiveViewDto& dto, char* buffer, size_t bufferSize, size_t& outLength);
    static bool serializeToBuffer(const AvailableStepsDto& dto, char* buffer, size_t bufferSize, size_t& outLength);
    static bool serializeToBuffer(const RecipeListDto& dto, char* buffer, size_t bufferSize, size_t& outLength);
    static bool serializeToBuffer(const RecipeDto& dto, char* buffer, size_t bufferSize, size_t& outLength);
    static bool serializeToBuffer(const ExecutionHistoryDto& dto, char* buffer, size_t bufferSize, size_t& outLength);
    static bool serializeToBuffer(const AuthResponseDto& dto, char* buffer, size_t bufferSize, size_t& outLength);
    static bool serializeToBuffer(const CommandResponseDto& dto, char* buffer, size_t bufferSize, size_t& outLength);
    
    // Convenience-Varianten mit std::string (für weniger kritische Pfade)
    
    static std::string serialize(const LiveViewDto& dto); 
    static std::string serialize(const AvailableStepsDto& dto);
    static std::string serialize(const RecipeListDto& dto);
    static std::string serialize(const RecipeDto& dto); 
    static std::string serialize(const ExecutionHistoryDto& dto);
    static std::string serialize(const TimeSeriesBinaryDto& dto);
    static std::string serialize(const AuthResponseDto& dto);
    static std::string serialize(const CommandResponseDto& dto);

    static bool deserialize(std::string_view json, CommandDto& outDto);
    static bool deserialize(std::string_view json, RecipeDto& outDto);
    static bool extractGlobalParameters(std::string_view json, std::map<std::string, std::string>& outParams);

private:
    // Private Konstruktor - reine Utility-Klasse, keine Instanziierung
    JsonSerialization() = delete;
    ~JsonSerialization() = delete;
    JsonSerialization(const JsonSerialization&) = delete;
    JsonSerialization& operator=(const JsonSerialization&) = delete;
};
