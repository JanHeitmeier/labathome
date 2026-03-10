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
    static std::vector<uint8_t> serialize(const Recipe& recipe);
    static bool deserialize(const std::vector<uint8_t>& data, Recipe& outRecipe);

private:
    RecipeSerializer() = delete;
    ~RecipeSerializer() = delete;
};
