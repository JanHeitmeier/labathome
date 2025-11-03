#pragma once

#include "../core/interfaces/storage/IRecipeStorage.hh"
#include "../core/domain/entities/Recipe.hh"
#include <vector>
#include <string>

// Implementiert das IRecipeStorage-Interface mit LittleFS für den ESP32.
class LittleFSRecipeStorage : public IRecipeStorage {
public:
    LittleFSRecipeStorage();
    
    bool serialize(const Recipe& r, std::vector<uint8_t>& outBlob) override;
    bool deserialize(const std::vector<uint8_t>& blob, Recipe& outRecipe) override;
    bool save(uint32_t id, const std::vector<uint8_t>& data) override;
    std::optional<std::vector<uint8_t>> get(uint32_t id) override;
    bool update(uint32_t id, const std::vector<uint8_t>& data) override;
    bool remove(uint32_t id) override;
    bool exists(uint32_t id) override;
    std::vector<uint32_t> listIds() override;
};
