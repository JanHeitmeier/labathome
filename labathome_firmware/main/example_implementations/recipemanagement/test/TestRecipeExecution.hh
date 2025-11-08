#pragma once

#include "../../../iHAL.hh"
#include "../../../recipemanagement/infrastructure/engine/RecipeEngine.hh"
#include <memory>

/**
 * @brief Test-Klasse für das Ausführen eines hardcodierten Test-Rezepts
 * 
 * Diese Klasse dient als Meilenstein-Implementierung, um das Recipe Management System
 * ohne Web-Anbindung zu testen. Sie lädt ein hardcodiertes JSON-Rezept mit 3 Steps
 * und führt es aus.
 * 
 * Verwendung:
 * 1. TestRecipeExecution test;
 * 2. test.init(hal);  // Im DeviceManager::init()
 * 3. test.loadAndStart();  // Startet das Test-Rezept
 * 4. test.tick(deltaMs);  // In der Hauptschleife aufrufen
 */
class TestRecipeExecution {
public:
    TestRecipeExecution();
    ~TestRecipeExecution();
    
    /**
     * @brief Initialisiert das Recipe Management System
     * @param hal Hardware Abstraction Layer
     * @return true bei Erfolg, false bei Fehler
     */
    bool init(iHAL* hal);
    
    /**
     * @brief Lädt und startet das Test-Rezept
     * @return true bei Erfolg, false bei Fehler
     */
    bool loadAndStart();
    
    /**
     * @brief Tick-Funktion für die Rezept-Ausführung
     * @param deltaMs Zeit seit letztem Tick in Millisekunden
     */
    void tick(uint32_t deltaMs);
    
    /**
     * @brief Stoppt die Rezept-Ausführung
     */
    void stop();
    
    /**
     * @brief Prüft ob ein Rezept läuft
     * @return true wenn ein Rezept ausgeführt wird
     */
    bool isRunning() const;
    
    /**
     * @brief Gibt den aktuellen Engine-Status zurück
     */
    RecipeEngineState getState() const;
    
private:
    std::unique_ptr<RecipeEngine> m_engine;
    bool m_isRunning;
};
