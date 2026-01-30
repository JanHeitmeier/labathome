// IRecipeStorage.hpp
#pragma once

#include "../../domain/entities/Recipe.hh"
#include <string>
#include <vector>
#include <optional>

//CRUD operationen sollten mit serialisierten daten und Json arbeiten können.

class IRecipeStorage {
public:
    virtual ~IRecipeStorage() = default;
    virtual bool serialize(const Recipe& r, std::vector<uint8_t>& outBlob) = 0;
    virtual bool deserialize(const std::vector<uint8_t>& blob, Recipe& outRecipe) = 0;
    virtual bool save(uint32_t id, const std::vector<uint8_t>& data) = 0;
    virtual std::optional<std::vector<uint8_t>> get(uint32_t id) = 0;
    virtual bool update(uint32_t id, const std::vector<uint8_t>& data) = 0;
    virtual bool remove(uint32_t id) = 0;
    virtual bool exists(uint32_t id) = 0;
    virtual std::vector<uint32_t> listIds() = 0;
    virtual bool saveJson(uint32_t id, const std::string& json) = 0;
    virtual std::optional<std::string> getJson(uint32_t id) = 0;
};
