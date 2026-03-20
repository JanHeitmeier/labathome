#pragma once
#include "../../domain/value-objects/SensorTimeSeries.hh"
#include <vector>
#include <string>
#include <cstddef>
#include <optional>
#include <cstdint>

class ITimeSeriesStorage {
public:
    virtual ~ITimeSeriesStorage() = default;
    
    virtual bool saveTimeSeries(const std::string& executionId, const std::vector<SensorTimeSeries>& series) = 0;
    virtual std::vector<SensorTimeSeries> loadTimeSeries(const std::string& executionId) = 0;
    virtual bool deleteTimeSeries(const std::string& executionId) = 0;
    virtual size_t getStorageSize(const std::string& executionId) = 0;
    virtual std::optional<std::vector<uint8_t>> loadTimeSeriesBinary(const std::string& executionId) = 0;
};
