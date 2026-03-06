#pragma once

#include "../dtos/LiveViewDto.hh"
#include "../dtos/AvailableStepsDto.hh"
#include "../dtos/AvailableRecipesDto.hh"
#include "../dtos/RecipeDto.hh"
#include "../dtos/ExecutionHistoryDto.hh"
#include "../dtos/TimeSeriesBinaryDto.hh"
#include "../dtos/AuthResponseDto.hh"

class IMessageGateway {
public:
    virtual ~IMessageGateway() = default;
    
    virtual void send(const LiveViewDto& dto) = 0;
    virtual void send(const AvailableStepsDto& dto) = 0;
    virtual void send(const AvailableRecipesDto& dto) = 0;
    virtual void send(const RecipeDto& dto) = 0;
    virtual void send(const ExecutionHistoryDto& dto) = 0;
    virtual void send(const TimeSeriesBinaryDto& dto) = 0;
    virtual void send(const AuthResponseDto& dto) = 0;
    virtual void send(const CommandResponseDto& dto) = 0;
};
