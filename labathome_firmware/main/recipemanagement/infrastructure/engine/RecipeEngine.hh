#pragma once 

#include "StepTypeRegistry.hh"
#include "StepContext.hh"
#include "../storageservice/RecipeStorageManager.hh"
#include "IStep.hh"
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <cstdint>
#include <optional>

enum class RecipeEngineState { Stopped, Loaded, Running, Paused };

struct StepInstanceDescriptor {
    std::string systemId;
    // Numerische 6-stellige Typ-ID
    std::uint32_t typeId{0};
    std::unordered_map<std::string, std::string> params;
    std::unordered_map<std::string, std::string> aliases;
    int repeatCount{1};
};
//Name von Ki gesetzt
class RecipeEngine {
public:
    //Konstruktor/Destruktor
    RecipeEngine();
    ~RecipeEngine();

    // LoadRecipe muss noch auf den neuen Recipe Daten typen umgestellt werden
    bool loadRecipe();
    bool loadSerializedRecipe();
    // Rezept Steuerung
    bool start();
    bool pause();
    bool resume();
    bool stop();
    // Wird beim Ausführen eines Rezeptes aufgerufen aus dem devicemanager 
    void tick(uint32_t deltaMs);
};
