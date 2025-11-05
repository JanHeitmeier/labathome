// Initialisierung der StepTypeRegistry mit allen verfügbaren Steps
// Diese Datei implementiert die init()-Methode, die im Framework deklariert wurde

#include "../../../recipemanagement/infrastructure/engine/StepTypeRegistry.hh"
#include "../engine/Steps.cc"

/**
 * @brief Registriert alle für diese Maschine verfügbaren Step-Typen
 * 
 * Diese Methode wird vom DeviceManager beim Programmstart aufgerufen.
 * Hier müssen alle konkreten Step-Implementierungen registriert werden,
 * damit sie später im Recipe Editor verfügbar sind und vom RecipeParser
 * instanziiert werden können.
 */
void StepTypeRegistry::init() {
    // Registriere alle verfügbaren Steps
    registerStepType(std::make_unique<TwoButtonFanStep>());
    
    // Weitere Steps können hier hinzugefügt werden:
    // registerStepType(std::make_unique<WaitStep>());
    // registerStepType(std::make_unique<TemperatureControlStep>());
    // registerStepType(std::make_unique<MixingStep>());
    // ...
}
