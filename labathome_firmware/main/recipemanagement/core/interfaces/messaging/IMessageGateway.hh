#pragma once

#include "../../../application/dtos/LiveViewDto.hh"
#include "../../../application/dtos/AvailableStepsDto.hh"
#include "../../../application/dtos/AvailableRecipesDto.hh"
#include "../../../application/dtos/RecipeDto.hh"
#include "../../../application/dtos/MetricsDto.hh"

/**
 * @brief Interface für ausgehende Nachrichten vom Backend zum Frontend
 * 
 * Wird vom RecipeApplicationService verwendet, um DTOs an die UI zu senden.
 * Die konkrete Implementierung erfolgt im RecipeManagementPlugin.
 */
class IMessageGateway {
public:
    virtual ~IMessageGateway() = default;
    
    // Ausgehende Nachrichten (Backend → Frontend)
    virtual void send(const LiveViewDto& dto) = 0;
    virtual void send(const AvailableStepsDto& dto) = 0;
    virtual void send(const AvailableRecipesDto& dto) = 0;
    virtual void send(const RecipeDto& dto) = 0;
    virtual void send(const MetricsDto& dto) = 0;
};
