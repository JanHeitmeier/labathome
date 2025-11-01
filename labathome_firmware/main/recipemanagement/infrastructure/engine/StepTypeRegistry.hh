#pragma once 

#include <functional>
#include <unordered_map>
#include <vector>
#include <mutex>
#include "../entities/StepMetadata.hh"
#include "../interfaces/IStep.hh"
#include "StepContext.hh"


class StepTypeRegistry {
    //es soll singleton sein

    // Steps werden implemtniert durch das interface IStep
    // jeder schritt soll nach dem kompiliren hier über dieses Register zur verfügung stehen.
    //eine Funktion gibt die Metadaten des Schritttyps anhand einer id zurück.()
    //eine funktion gibt alle existierenden Schrittypen zurück mit ihren Metadaten. (zur schrittauswahl im RecipeEditor)
    //Die Registrierung von Schritttypen erfolgt beim kompilieren. 
    // die verschiedenenSchritte implementieren das interfase IStep. 

public:

    static StepTypeRegistry& instance();
    //get all current avilable step types / für RecipeEditor online benötigt. 
    std::vector<StepMetadata> availableTypes() const;

};
