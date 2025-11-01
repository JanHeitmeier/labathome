// IRecipeStorage.hpp
#pragma once

#include "Recipe.hh"
#include <string>
#include <vector>
#include <optional>

//CRUD operationen sollten mit serialisierten daten und Json arbeiten können.

class IRecipeStorage {
public:
    virtual ~IRecipeStorage() = default;
    // serialize recipe to a compact binary blob
    virtual bool serialize(const Recipe& r, std::vector<uint8_t>& outBlob) = 0;
    // deserialize
    virtual bool deserialize(const std::vector<uint8_t>& blob, Recipe& outRecipe) = 0;
    // Speichert Daten mit einer ID
    virtual bool save(uint32_t id, const std::vector<uint8_t>& data) = 0;
    // Lädt Daten anhand der ID
    virtual std::optional<std::vector<uint8_t>> get(uint32_t id) = 0;
    // Aktualisiert Daten mit der ID (überschreibt vorhandene)
    virtual bool update(uint32_t id, const std::vector<uint8_t>& data) = 0;
    // Entfernt Daten mit der ID
    virtual bool remove(uint32_t id) = 0;
    // Prüft, ob Daten mit der ID existieren
    virtual bool exists(uint32_t id) = 0;
    // Liefert alle bekannten IDs
    virtual std::vector<uint32_t> listIds() = 0;
};
