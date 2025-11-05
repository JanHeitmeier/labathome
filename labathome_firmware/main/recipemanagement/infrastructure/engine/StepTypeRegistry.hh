#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <mutex>
#include "../../core/domain/value-objects/StepMetadata.hh"
#include "../../core/interfaces/engine/IStep.hh"
#include "../../application/dtos/AvailableStepsDto.hh"

/**
 * @brief Singleton-Registry für alle verfügbaren Step-Typen
 * 
 * Verwaltet Prototyp-Instanzen aller registrierten Steps und ermöglicht:
 * - Abfrage aller verfügbaren Step-Metadaten (für Recipe Editor)
 * - Instanziierung neuer Steps anhand ihrer TypeId (für RecipeParser)
 * 
 * Die init()-Methode MUSS vom Entwickler in example_implementations implementiert werden!
 */
class StepTypeRegistry {
private:
    std::unordered_map<uint32_t, std::unique_ptr<IStep>> m_prototypes;
    mutable std::mutex m_mutex;
    
    StepTypeRegistry() = default;
    
public:
    static StepTypeRegistry& instance();
    
    /**
     * @brief Initialisiert die Registry mit allen verfügbaren Steps
     * 
     * WICHTIG: Diese Methode wird vom Framework deklariert, aber NICHT implementiert!
     * Der Entwickler muss sie in example_implementations/recipemanagement/init/InitSteps.cc
     * implementieren und dort alle Steps registrieren.
     * 
     * Beispiel-Implementierung:
     * void StepTypeRegistry::init() {
     *     registerStepType(std::make_unique<TwoButtonFanStep>());
     *     registerStepType(std::make_unique<WaitStep>());
     * }
     */
    void init();
    
    /**
     * @brief Registriert einen neuen Step-Typ
     * @param prototype Prototyp-Instanz des Steps (wird geklont für neue Instanzen)
     */
    void registerStepType(std::unique_ptr<IStep> prototype);
    
    /**
     * @brief Gibt Metadaten aller registrierten Step-Typen zurück
     * @return Vector mit StepMetadata für alle verfügbaren Steps
     */
    std::vector<StepMetadata> availableTypes() const;
    
    /**
     * @brief Gibt alle verfügbaren Step-Typen als DTO für die Web-UI zurück
     * 
     * Konvertiert intern alle StepMetadata in StepMetadataDto mit vollständigen
     * Informationen über Parameter (inkl. min/max/unit) und IoAliases.
     * 
     * @return AvailableStepsDto bereit für JSON-Serialisierung
     */
    AvailableStepsDto availableTypesAsDto() const;
    
    /**
     * @brief Erstellt eine neue Instanz eines Steps anhand seiner TypeId
     * @param typeId Die eindeutige TypeId des gewünschten Steps
     * @return Neue Step-Instanz (leer, ohne Parameter) oder nullptr wenn TypeId unbekannt
     */
    std::unique_ptr<IStep> createInstance(uint32_t typeId) const;
};
