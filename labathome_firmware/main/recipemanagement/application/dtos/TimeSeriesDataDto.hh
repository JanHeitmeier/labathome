#pragma once
#include <string>
#include <vector>

struct TimeSeriesPointDto {
    uint64_t timestamp;
    float value;
};

struct SensorTimeSeriesDto {
    std::string sensorName;
    std::string unit;
    std::vector<TimeSeriesPointDto> dataPoints;
};

struct TimeSeriesDataDto {
    std::string executionId;
    std::vector<SensorTimeSeriesDto> series;
};
