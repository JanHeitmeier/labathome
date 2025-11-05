// Initialisierung des IoResourceManagers mit allen verfügbaren I/O-Geräten
// Diese Datei implementiert die init()-Methode, die im Framework deklariert wurde

#include "../../../recipemanagement/infrastructure/engine/IoResourceManager.hh"
#include "../engine/Io_impl.cc"

/**
 * @brief Registriert alle für diese Maschine verfügbaren I/O-Geräte
 * 
 * Diese Methode wird vom DeviceManager beim Programmstart aufgerufen.
 * Hier müssen alle konkreten IInput- und IOutput-Implementierungen registriert werden,
 * damit sie später in Steps über ihre Alias-Namen verwendet werden können.
 * 
 * @param hal Hardware Abstraction Layer für Zugriff auf physische Pins
 */
void IoResourceManager::init(iHAL* hal) {
    if (!hal) return;
    
    // Registriere alle bekannten Inputs
    registerInput("GreenButton", std::make_shared<GreenButtonInput>(hal));
    registerInput("RedButton", std::make_shared<RedButtonInput>(hal));
    
    // Weitere Inputs können hier hinzugefügt werden:
    // registerInput("TempSensor", std::make_shared<TempSensorInput>(hal));
    // registerInput("PressureSensor", std::make_shared<PressureSensorInput>(hal));
    // ...
    
    // Registriere alle bekannten Outputs
    registerOutput("Fan", std::make_shared<FanOutput>(hal, 0, "Fan"));
    
    // Weitere Outputs können hier hinzugefügt werden:
    // registerOutput("Heater", std::make_shared<HeaterOutput>(hal));
    // registerOutput("Pump", std::make_shared<PumpOutput>(hal, 0));
    // registerOutput("Valve", std::make_shared<ValveOutput>(hal, 1));
    // ...
}
