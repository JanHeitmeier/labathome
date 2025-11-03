#pragma once

#include "../core/interfaces/messaging/IRecipeInfoGateway.hh"
#include "webmanager_interfaces.hh" // Für HTTP-Server-Funktionen

// Implementiert das IRecipeInfoGateway-Interface mittels HTTP-Endpunkten.
class HttpRecipeInfoGateway : public IRecipeInfoGateway {
public:
    HttpRecipeInfoGateway(httpd_handle_t server);
    // void sendRecipeList(const AvailableRecipesDto& dto) override;
    // void sendAvailableSteps(const AvailableStepsDto& dto) override;
private:
    httpd_handle_t m_server;
};
