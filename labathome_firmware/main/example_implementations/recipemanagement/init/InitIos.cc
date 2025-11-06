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
    
    // Weitere Inputs können hier hinzugefügt werden

    
    // Registriere alle bekannten Outputs
    registerOutput("Fan", std::make_shared<FanOutput>(hal, 0, "Fan"));
    
    // Weitere Outputs können hier hinzugefügt werden

}
