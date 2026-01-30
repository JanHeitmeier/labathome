#pragma once
#include "RecipeExecutionDto.hh"
#include <vector>

struct ExecutionHistoryDto {
    std::vector<RecipeExecutionDto> executions;
};
