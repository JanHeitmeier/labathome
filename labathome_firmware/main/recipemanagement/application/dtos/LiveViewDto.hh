#pragma once

#include <string>
#include <cstdint>
#include <map>

// BACKEND → FRONTEND
struct LiveViewDto {
    std::string recipeId;
    std::string recipeName;
    int currentStepIndex;
    int totalSteps;
    std::string currentStepName;
    std::string stepState;
    std::string recipeStatus;
    std::string userInstruction;
    bool awaitingUserAcknowledgment;
    float progress;
    uint64_t timestamp;
    std::string errorMessage;
    std::map<std::string, float> sensorValues;
};
