#pragma once

#include <string>

// BACKEND → FRONTEND: Login response with session token
struct AuthResponseDto {
    bool success;
    std::string role;  // "Admin", "RecipeEditor", "RecipeStarter", "Observer"
    std::string sessionToken;  // Token for subsequent requests
    std::string errorMessage;
};

// BACKEND → FRONTEND: Command execution response
struct CommandResponseDto {
    bool success;
    int errorCode;  // 0 = OK, 401 = Unauthorized, 403 = Forbidden
    std::string errorMessage;
    std::string requestId;
};
