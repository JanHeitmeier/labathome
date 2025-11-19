#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <functional>
#include "../../core/domain/value-objects/StepMetadata.hh"
#include "../../core/interfaces/engine/IStep.hh"
#include "../../application/dtos/AvailableStepsDto.hh"

/**
 * @brief Factory-Funktion die einen neuen Step erzeugt
 */
using StepFactory = std::function<std::unique_ptr<IStep>()>;

/**
 * @brief Singleton-Registry für alle verfügbaren Step-Typen
 * 
 * Verwendet Factory-Pattern statt Prototyp-Instanzen:
 * - Steps werden nur on-demand erzeugt
 * - Metadaten werden einmal beim Start gecacht
 * - Kein permanenter Speicherverbrauch für Prototypen
 */
class StepTypeRegistry {
private:
    struct StepTypeInfo {
        StepFactory factory;
        StepMetadata metadata;
    };
    //Hier werden die impl. Steps ihrer TypeId gemappt
    std::unordered_map<uint32_t, StepTypeInfo> m_types;
    mutable std::mutex m_mutex;
    uint32_t m_nextTypeId = 0x0001;  // Auto-incrementing TypeId
    
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
     *     registerStepType<RedLedButtonStep>();
     *     registerStepType<YellowGreenLedButtonStep>();
     * }
     */
    void init();
    
    /**
     * @brief Registriert einen Step-Typ mit Template (bevorzugte Methode)
     * @tparam StepType Der Step-Typ (muss von IStep erben)
     * 
     * Die Registry vergibt automatisch eine aufsteigende TypeId.
     * Die Reihenfolge der Registrierung in init() bestimmt die IDs.
     * Die TypeId wird via setTypeId() in die Step-Instanz injiziert.
     */
    template<typename StepType>
    void registerStepType() {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // Aktuelle TypeId
        uint32_t assignedTypeId = m_nextTypeId++;
        
        // Temporär erzeugen OHNE TypeId im Konstruktor
        auto temp = std::make_unique<StepType>();
        
        // TypeId nachträglich setzen
        temp->setTypeId(assignedTypeId);
        
        // Metadaten auslesen (enthalten jetzt die korrekte TypeId) dieser schritt zum vereinfachten auslesen der Metadataen nach Start Ohne obj. zur laufzeit bei abfrage generieren zu müssen.
        StepMetadata metadata = temp->getMetadata();
        
        // Factory-Funktion speichern (mit Capture der assignedTypeId)
        StepFactory factory = [assignedTypeId]() -> std::unique_ptr<IStep> {
            auto instance = std::make_unique<StepType>();
            instance->setTypeId(assignedTypeId);
            return instance;
        };
        
        m_types[assignedTypeId] = StepTypeInfo{std::move(factory), std::move(metadata)};
        
        // temp wird hier automatisch zerstört
    }
    
    /**
     * @brief Gibt Metadaten aller registrierten Step-Typen zurück
     * @return Vector mit StepMetadata für alle verfügbaren Steps
     */
    std::vector<StepMetadata> availableTypes() const;
    
    /**
     * @brief Gibt alle verfügbaren Step-Typen als DTO für die Web-UI zurück
     * @return AvailableStepsDto bereit für JSON-Serialisierung
     */
    AvailableStepsDto availableTypesAsDto() const;
    
    /**
     * @brief Erstellt eine neue Instanz eines Steps anhand seiner TypeId
     * @param typeId Die eindeutige TypeId des gewünschten Steps
     * @return Neue Step-Instanz oder nullptr wenn TypeId unbekannt
     */
    std::unique_ptr<IStep> createInstance(uint32_t typeId) const;
};
