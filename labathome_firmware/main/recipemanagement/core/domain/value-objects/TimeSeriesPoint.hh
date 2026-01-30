#pragma once
#include <cstdint>

struct TimeSeriesPoint {
    uint64_t timestamp;  // Relative milliseconds since execution start
    float value;
    
    TimeSeriesPoint() : timestamp(0), value(0.0f) {}
    TimeSeriesPoint(uint64_t ts, float val) : timestamp(ts), value(val) {}
};
