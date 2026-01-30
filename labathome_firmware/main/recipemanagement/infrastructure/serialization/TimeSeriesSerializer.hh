#pragma once
#include "../../core/domain/value-objects/SensorTimeSeries.hh"
#include <vector>
#include <cstdint>

class TimeSeriesSerializer {
public:
    static std::vector<uint8_t> serialize(const std::vector<SensorTimeSeries>& series);
    static bool deserialize(const std::vector<uint8_t>& data, std::vector<SensorTimeSeries>& outSeries);

private:
    TimeSeriesSerializer() = delete;
};
