#include "../../../recipemanagement/infrastructure/engine/StepTypeRegistry.hh"
#include "../engine/Steps.cc"

/**
 * @brief Registriert alle für diese Maschine verfügbaren Step-Typen
 * 
 * Diese Methode wird vom DeviceManager beim Programmstart aufgerufen.
 * Hier müssen alle konkreten Step-Implementierungen registriert werden.
 * 
 * WICHTIG: Es werden KEINE Instanzen permanent gespeichert!
 * - Beim Registrieren wird temporär eine Instanz erzeugt um Metadaten auszulesen
 * - Diese wird sofort wieder zerstört
 * - Gespeichert wird nur eine Factory-Funktion zum späteren Erzeugen
 */
void StepTypeRegistry::init() {
    // Test Steps
    registerStepType<FanRampStep>();
    registerStepType<LedSequenceStep>();
    registerStepType<MovementLedTriggerStep>();
    registerStepType<AcknowledgeLedFanStep>();
    
    // Steps werden nur on-demand erzeugt wenn:
    // 1. Metadaten für Web-UI angefordert werden
    // 2. Ein Recipe geladen wird und Steps instanziiert werden müssen
}
