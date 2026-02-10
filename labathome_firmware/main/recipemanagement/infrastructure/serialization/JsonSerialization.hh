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
#include "../../application/dtos/TimeSeriesDataDto.hh"
#include "../../application/dtos/AuthResponseDto.hh"

class JsonSerialization {
public:
    // ========== Serialisierung (DTO → JSON) ==========
    
    /**
     * @brief Serialisiert ein LiveViewDto in einen bereitgestellten Buffer
     * @param dto Das zu serialisierende LiveViewDto
     * @param buffer Zeiger auf den Zielpuffer (caller-allocated)
     * @param bufferSize Größe des Puffers in Bytes
     * @param outLength Ausgabe: Tatsächliche Länge des JSON-Strings (ohne null-terminator)
     * @return true bei Erfolg, false wenn Buffer zu klein oder anderer Fehler
     */
    static bool serializeToBuffer(const LiveViewDto& dto, char* buffer, size_t bufferSize, size_t& outLength);
    
    /**
     * @brief Serialisiert ein AvailableStepsDto in einen bereitgestellten Buffer
     * @param dto Das zu serialisierende AvailableStepsDto
     * @param buffer Zeiger auf den Zielpuffer (caller-allocated)
     * @param bufferSize Größe des Puffers in Bytes
     * @param outLength Ausgabe: Tatsächliche Länge des JSON-Strings (ohne null-terminator)
     * @return true bei Erfolg, false wenn Buffer zu klein oder anderer Fehler
     */
    static bool serializeToBuffer(const AvailableStepsDto& dto, char* buffer, size_t bufferSize, size_t& outLength);
    
    /**
     * @brief Serialisiert ein RecipeListDto in einen bereitgestellten Buffer
     * @param dto Das zu serialisierende RecipeListDto (Alias für AvailableRecipesDto)
     * @param buffer Zeiger auf den Zielpuffer (caller-allocated)
     * @param bufferSize Größe des Puffers in Bytes
     * @param outLength Ausgabe: Tatsächliche Länge des JSON-Strings (ohne null-terminator)
     * @return true bei Erfolg, false wenn Buffer zu klein oder anderer Fehler
     */
    static bool serializeToBuffer(const RecipeListDto& dto, char* buffer, size_t bufferSize, size_t& outLength);
    
    static bool serializeToBuffer(const RecipeDto& dto, char* buffer, size_t bufferSize, size_t& outLength);
    
    // Convenience-Varianten mit std::string (für weniger kritische Pfade)
    
    /**
     * @brief Serialisiert ein LiveViewDto in einen std::string (mit Heap-Allokation)
     * @param dto Das zu serialisierende LiveViewDto
     * @return JSON-String oder leerer String bei Fehler
     * @note Weniger effizient als serializeToBuffer(), aber bequemer für unkritische Pfade
     */
    static std::string serialize(const LiveViewDto& dto);
    
    /**
     * @brief Serialisiert ein AvailableStepsDto in einen std::string (mit Heap-Allokation)
     * @param dto Das zu serialisierende AvailableStepsDto
     * @return JSON-String oder leerer String bei Fehler
     */
    static std::string serialize(const AvailableStepsDto& dto);
    
    /**
     * @brief Serialisiert ein RecipeListDto in einen std::string (mit Heap-Allokation)
     * @param dto Das zu serialisierende RecipeListDto
     * @return JSON-String oder leerer String bei Fehler
     */
    static std::string serialize(const RecipeListDto& dto);
    
    /**
     * @brief Serialisiert ein RecipeDto in einen std::string (mit Heap-Allokation)
     * @param dto Das zu serialisierende RecipeDto
     * @return JSON-String oder leerer String bei Fehler
     */
    static std::string serialize(const RecipeDto& dto);
    
    static bool serializeToBuffer(const ExecutionHistoryDto& dto, char* buffer, size_t bufferSize, size_t& outLength);
    static std::string serialize(const ExecutionHistoryDto& dto);
    
    static bool serializeToBuffer(const TimeSeriesDataDto& dto, char* buffer, size_t bufferSize, size_t& outLength);
    static std::string serialize(const TimeSeriesDataDto& dto);
    
    static bool serializeToBuffer(const AuthResponseDto& dto, char* buffer, size_t bufferSize, size_t& outLength);
    static std::string serialize(const AuthResponseDto& dto);
    
    static bool serializeToBuffer(const CommandResponseDto& dto, char* buffer, size_t bufferSize, size_t& outLength);
    static std::string serialize(const CommandResponseDto& dto);
    
    // ========== Deserialisierung (JSON → DTO) ==========
    
    /**
     * @brief Deserialisiert einen JSON-String in ein CommandDto
     * @param json Der zu deserialisierende JSON-String (std::string_view für Zero-Copy)
     * @param outDto Referenz auf das Ziel-CommandDto
     * @return true bei Erfolg, false bei Fehler
     * @note Verwendet string_view um Kopien zu vermeiden - Aufrufer muss Lebensdauer des Strings sicherstellen
     */
    static bool deserialize(std::string_view json, CommandDto& outDto);
    
    /**
     * @brief Deserialisiert einen JSON-String in ein RecipeDto
     * @param json Der zu deserialisierende JSON-String (std::string_view für Zero-Copy)
     * @param outDto Referenz auf das Ziel-RecipeDto
     * @return true bei Erfolg, false bei Fehler
     * @note Verwendet string_view um Kopien zu vermeiden - Aufrufer muss Lebensdauer des Strings sicherstellen
     */
    static bool deserialize(std::string_view json, RecipeDto& outDto);
    
    /**
     * @brief Extrahiert globalParameters aus einem JSON-String
     * @param json Der JSON-String mit einem "globalParameters"-Objekt
     * @param outParams Map, in die die Parameter geschrieben werden (key-value)
     * @return true bei Erfolg, false bei Parse-Fehler oder fehlendem globalParameters-Feld
     */
    static bool extractGlobalParameters(std::string_view json, std::map<std::string, std::string>& outParams);

private:
    // Private Konstruktor - reine Utility-Klasse, keine Instanziierung
    JsonSerialization() = delete;
    ~JsonSerialization() = delete;
    JsonSerialization(const JsonSerialization&) = delete;
    JsonSerialization& operator=(const JsonSerialization&) = delete;
};
