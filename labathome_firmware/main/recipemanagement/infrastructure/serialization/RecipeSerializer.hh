#pragma once

#include "../../core/domain/entities/Recipe.hh"
#include <vector>
#include <cstdint>

/**
 * @brief Serialisiert und deserialisiert komplette Recipe-Objekte für schnellen Binary-Cache
 * 
 * Speichert ALLE Recipe-Informationen (ID, Name, Description, Author, Version, Steps)
 * als kompaktes Binary-Format für schnelles Laden ohne JSON-Parsing
 */
class RecipeSerializer {
public:
    /**
     * @brief Serialisiert komplettes Recipe-Objekt zu Binary
     * @param recipe Das zu serialisierende Recipe
     * @return Binary-Daten (kompaktes Format)
     */
    static std::vector<uint8_t> serialize(const Recipe& recipe);
    
    /**
     * @brief Deserialisiert Binary-Daten zu Recipe-Objekt
     * @param data Binary-Daten
     * @param outRecipe Ausgabe-Recipe-Objekt
     * @return true bei Erfolg, false bei Fehler
     */
    static bool deserialize(const std::vector<uint8_t>& data, Recipe& outRecipe);

private:
    RecipeSerializer() = delete;
    ~RecipeSerializer() = delete;
};
