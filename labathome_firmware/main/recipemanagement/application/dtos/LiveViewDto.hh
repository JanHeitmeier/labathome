#pragma once

#include <string>
#include <cstdint>
#include <map>

/**
 * @brief DTO für Live-Informationen während der Rezeptausführung (ausgehend: Backend → Frontend)
 * 
 * Wird periodisch vom Backend an die UI gesendet, um den aktuellen Rezept-Status anzuzeigen.
 */
struct LiveViewDto {
    std::string recipeId;
    std::string recipeName;
    int currentStepIndex;
    int totalSteps;
    std::string currentStepName;
    std::string stepState;              // "activating", "active", "deactivating", "idle"
    std::string recipeStatus;           // "running", "paused", "stopped", "completed", "error"
    std::string userInstruction;        // Anweisung an den Benutzer (z.B. "Behälter leeren")
    bool awaitingUserAcknowledgment;    // true wenn Schritt auf Benutzer-Quittierung wartet
    float progress;                     // Fortschritt 0.0 - 1.0
    uint64_t timestamp;
    std::string errorMessage;
    std::map<std::string, float> sensorValues;  // Sensor-Werte (Key: Sensor-Name, Value: aktueller Wert)
};
