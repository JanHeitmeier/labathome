#pragma once

#include <vector>
#include <string>
#include <cstdint>

struct MetricDataPointDto {
    uint64_t timestamp;
    float value;
};

struct MetricSeriesDto {
    std::string name;
    std::string unit;
    std::vector<MetricDataPointDto> data;
};

// BACKEND → FRONTEND
struct MetricsDto {
    std::string recipeId;
    std::vector<MetricSeriesDto> series;
};
