#include "TimeSeriesRecorder.hh"
#include <esp_log.h>
#include <algorithm>

static const char* TAG = "TimeSeriesRecorder";

TimeSeriesRecorder::TimeSeriesRecorder(ITimeSeriesStorage* storage)
    : m_storage(storage), m_recording(false) {
}

TimeSeriesRecorder::~TimeSeriesRecorder() {
    if (m_recording) {
        stopRecording();
    }
}

void TimeSeriesRecorder::startRecording(const std::string& executionId, const std::vector<std::string>& sensorNames) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_recording) {
        ESP_LOGW(TAG, "Already recording, stopping previous");
        flushBuffer();
    }
    
    m_currentExecutionId = executionId;
    m_buffer.clear();
    m_buffer.reserve(sensorNames.size());
    
    for (const auto& name : sensorNames) {
        m_buffer.emplace_back(name, "");
        m_buffer.back().dataPoints.reserve(BUFFER_SIZE);
    }
    
    m_recording = true;
    ESP_LOGI(TAG, "Started recording for %s with %zu sensors", executionId.c_str(), sensorNames.size());
}

void TimeSeriesRecorder::recordDataPoint(const std::map<std::string, float>& sensorValues, uint64_t relativeTimestamp) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_recording) {
        return;
    }
    
    for (auto& series : m_buffer) {
        auto it = sensorValues.find(series.sensorName);
        if (it != sensorValues.end()) {
            series.dataPoints.emplace_back(relativeTimestamp, it->second);
        }
    }
    
    if (getTotalPoints() >= FLUSH_THRESHOLD * m_buffer.size()) {
        flushBuffer();
    }
}

void TimeSeriesRecorder::stopRecording() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_recording) {
        return;
    }
    
    flushBuffer();
    m_recording = false;
    
    ESP_LOGI(TAG, "Stopped recording for %s", m_currentExecutionId.c_str());
}

void TimeSeriesRecorder::flushBuffer() {
    if (m_buffer.empty() || m_currentExecutionId.empty()) {
        return;
    }
    
    auto existing = m_storage->loadTimeSeries(m_currentExecutionId);
    
    for (auto& newSeries : m_buffer) {
        auto it = std::find_if(existing.begin(), existing.end(),
            [&](const SensorTimeSeries& s) { return s.sensorName == newSeries.sensorName; });
        
        if (it != existing.end()) {
            it->dataPoints.insert(it->dataPoints.end(), 
                newSeries.dataPoints.begin(), newSeries.dataPoints.end());
        } else {
            existing.push_back(newSeries);
        }
        
        newSeries.dataPoints.clear();
        newSeries.dataPoints.reserve(BUFFER_SIZE);
    }
    
    if (!m_storage->saveTimeSeries(m_currentExecutionId, existing)) {
        ESP_LOGE(TAG, "Failed to flush buffer for %s", m_currentExecutionId.c_str());
    } else {
        ESP_LOGD(TAG, "Flushed buffer for %s", m_currentExecutionId.c_str());
    }
}

size_t TimeSeriesRecorder::getTotalPoints() const {
    size_t total = 0;
    for (const auto& series : m_buffer) {
        total += series.dataPoints.size();
    }
    return total;
}

bool TimeSeriesRecorder::isRecording() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_recording;
}

TimeSeriesDataDto TimeSeriesRecorder::getTimeSeries(const std::string& executionId) {
    TimeSeriesDataDto dto;
    dto.executionId = executionId;
    
    auto series = m_storage->loadTimeSeries(executionId);
    dto.series.reserve(series.size());
    
    for (const auto& ts : series) {
        SensorTimeSeriesDto seriesDto;
        seriesDto.sensorName = ts.sensorName;
        seriesDto.unit = ts.unit;
        seriesDto.dataPoints.reserve(ts.dataPoints.size());
        
        for (const auto& pt : ts.dataPoints) {
            seriesDto.dataPoints.push_back({pt.timestamp, pt.value});
        }
        
        dto.series.push_back(std::move(seriesDto));
    }
    
    ESP_LOGI(TAG, "Retrieved %zu series for %s", dto.series.size(), executionId.c_str());
    return dto;
}
