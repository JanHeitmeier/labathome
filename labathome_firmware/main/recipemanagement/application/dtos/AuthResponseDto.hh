#pragma once

#include <string>

// BACKEND → FRONTEND: Response to "authenticate" command
struct AuthResponseDto {
    bool success;
    std::string role;  // "Admin", "RecipeEditor", "RecipeStarter", "Observer"
    std::string errorMessage;
};

// BACKEND → FRONTEND: Response with authentication error
struct CommandResponseDto {
    bool success;
    int errorCode;  // 0 = OK, 401 = Unauthorized, 403 = Forbidden
    std::string errorMessage;
    std::string requestId;
};
