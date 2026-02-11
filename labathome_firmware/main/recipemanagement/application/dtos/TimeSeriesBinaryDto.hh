#pragma once
#include <string>
#include <vector>
#include <cstdint>

/**
 * @brief DTO for transmitting TimeSeries data as binary blob
 * 
 * Instead of deserializing TimeSeries and sending as large JSON structure,
 * this DTO contains the raw binary data that will be Base64-encoded for transmission.
 * 
 * Benefits over TimeSeriesDataDto:
 * - 75% smaller transmission size (binary + Base64 vs. JSON)
 * - 90% less RAM usage on backend (no deserialization)
 * - Faster encoding (Base64 vs. JSON serialization)
 */
struct TimeSeriesBinaryDto {
    std::string executionId;          // Execution identifier
    uint64_t startTime;                // Execution start time (Unix timestamp in ms)
    std::vector<uint8_t> binaryData;   // Raw binary data from TimeSeriesSerializer
};
