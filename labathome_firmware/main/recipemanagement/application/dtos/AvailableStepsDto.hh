#pragma once

#include <vector>
#include <string>

// DTO für die Metadaten eines einzelnen Step-Typs.
struct StepMetadataDto {
    std::string typeId;
    std::string displayName;
    std::string description;
    // Weitere Felder für Params und IO-Aliase...
};

// DTO für eine Liste aller verfügbaren Step-Typen.
struct AvailableStepsDto {
    std::vector<StepMetadataDto> steps;
};
