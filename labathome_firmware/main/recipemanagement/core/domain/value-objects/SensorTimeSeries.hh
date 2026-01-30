#pragma once
#include "TimeSeriesPoint.hh"
#include <string>
#include <vector>

struct SensorTimeSeries {
    std::string sensorName;
    std::string unit;
    std::vector<TimeSeriesPoint> dataPoints;
    
    SensorTimeSeries() = default;
    SensorTimeSeries(const std::string& name, const std::string& u) 
        : sensorName(name), unit(u) {}
};
