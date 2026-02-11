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
    
    /**
     * @brief Load TimeSeries as raw binary data (without deserialization)
     * @param executionId Execution identifier
     * @return Binary data from .tsdata file, or empty optional if not found
     * 
     * This method provides direct access to the binary TimeSeries data for efficient
     * transmission without the overhead of deserialization + JSON serialization.
     */
    virtual std::optional<std::vector<uint8_t>> loadTimeSeriesBinary(const std::string& executionId) = 0;
};
