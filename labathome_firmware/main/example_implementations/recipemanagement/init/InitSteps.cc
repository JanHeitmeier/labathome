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
    
    // Weitere Steps können hier hinzugefügt werden
}
