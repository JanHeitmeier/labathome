#pragma once

// Interface für das Senden von statischen Informationen über Rezepte und Step-Typen.
// Wird von der Infrastructure-Schicht implementiert (z.B. als HttpGateway).
class IRecipeInfoGateway {
public:
    virtual ~IRecipeInfoGateway() = default;
    // virtual void sendRecipeList(const AvailableRecipesDto& dto) = 0;
    // virtual void sendAvailableSteps(const AvailableStepsDto& dto) = 0;
};
