#pragma once

#include <vector>
#include <string>
#include <cstdint>

struct MetricDataPointDto {
    uint64_t timestamp;     // Unix-Timestamp (ms)
    float value;
};

struct MetricSeriesDto {
    std::string name;
    std::string unit;
    std::vector<MetricDataPointDto> data;
};

/**
 * @brief Sensor-/Messwerte für Graphen (ausgehend: Backend → Frontend)
 */
struct MetricsDto {
    std::string recipeId;
    std::vector<MetricSeriesDto> series;
};
