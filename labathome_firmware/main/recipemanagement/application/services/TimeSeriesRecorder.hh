#pragma once
#include "../../core/interfaces/storage/ITimeSeriesStorage.hh"
#include "../../core/domain/value-objects/SensorTimeSeries.hh"
#include "../dtos/TimeSeriesDataDto.hh"
#include <string>
#include <vector>
#include <map>
#include <mutex>

class TimeSeriesRecorder {
public:
    explicit TimeSeriesRecorder(ITimeSeriesStorage* storage);
    ~TimeSeriesRecorder();
    
    void startRecording(const std::string& executionId, const std::vector<std::string>& sensorNames);
    void recordDataPoint(const std::map<std::string, float>& sensorValues, uint64_t relativeTimestamp);
    void stopRecording();
    
    TimeSeriesDataDto getTimeSeries(const std::string& executionId);
    bool isRecording() const;

private:
    ITimeSeriesStorage* m_storage;
    std::string m_currentExecutionId;
    std::vector<SensorTimeSeries> m_buffer;
    mutable std::mutex m_mutex;
    bool m_recording;
    uint64_t m_lastRecordedTimestamp;
    
    static constexpr size_t BUFFER_SIZE = 100;
    static constexpr size_t FLUSH_THRESHOLD = 80;
    static constexpr uint64_t SAMPLING_INTERVAL_MS = 250;  // Record one point every 250ms
    
    void flushBuffer();
    size_t getTotalPoints() const;
};
