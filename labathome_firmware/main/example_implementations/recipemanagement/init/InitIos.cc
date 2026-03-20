#include "../../../recipemanagement/core/services/IoResourceManager.hh"
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
    
    // Registriere alle Button-Inputs
    registerInput("GreenButton", std::make_shared<GreenButtonInput>(hal));
    registerInput("RedButton", std::make_shared<RedButtonInput>(hal));
    
    // Registriere Sensoren
    registerInput("Movement", std::make_shared<MovementInput>(hal));
    registerInput("FanDutySensor", std::make_shared<FanDutySensorInput>(hal, 0, "FanDutySensor"));
    registerInput("Temperature", std::make_shared<TemperatureInput>(hal, "Temperature"));
    registerInput("HeaterTemperature", std::make_shared<HeaterTemperatureInput>(hal, "HeaterTemperature"));
   
    // Registriere Outputs
    registerOutput("Fan", std::make_shared<FanOutput>(hal, 0, "Fan"));
    registerOutput("Heater", std::make_shared<HeaterOutput>(hal, "Heater"));
    registerOutput("LED0", std::make_shared<LedOutput>(hal, 0, "LED0"));
    registerOutput("LED1", std::make_shared<LedOutput>(hal, 1, "LED1"));
    registerOutput("LED2", std::make_shared<LedOutput>(hal, 2, "LED2"));
    registerOutput("LED3", std::make_shared<LedOutput>(hal, 3, "LED3"));
    registerOutput("Servo", std::make_shared<ServoZeroOutput>(hal, "Servo"));
}
