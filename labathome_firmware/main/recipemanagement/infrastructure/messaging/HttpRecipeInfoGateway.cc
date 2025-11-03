#include "HttpRecipeInfoGateway.hh"

// Implementierung für HttpRecipeInfoGateway.
HttpRecipeInfoGateway::HttpRecipeInfoGateway(httpd_handle_t server)
    : m_server(server) {
    // Hier HTTP-Routen registrieren, z.B. "/api/recipes" und "/api/steps".
}

/*
void HttpRecipeInfoGateway::sendRecipeList(const AvailableRecipesDto& dto) {
    // ...
}

void HttpRecipeInfoGateway::sendAvailableSteps(const AvailableStepsDto& dto) {
    // ...
}
*/
