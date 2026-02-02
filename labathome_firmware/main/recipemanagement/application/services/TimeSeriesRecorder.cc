#include "TimeSeriesRecorder.hh"
#include <esp_log.h>
#include <algorithm>
#include <numeric>

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
    
    ESP_LOGI(TAG, "[START] startRecording called for executionId='%s' with %zu sensors", executionId.c_str(), sensorNames.size());
    
    if (m_recording) {
        ESP_LOGW(TAG, "Already recording, stopping previous");
        flushBuffer();
    }
    
    m_currentExecutionId = executionId;
    m_buffer.clear();
    m_buffer.reserve(sensorNames.size());
    m_lastRecordedTimestamp = 0;
    
    for (const auto& name : sensorNames) {
        ESP_LOGI(TAG, "  - Adding sensor to buffer: '%s'", name.c_str());
        m_buffer.emplace_back(name, "");
        m_buffer.back().dataPoints.reserve(BUFFER_SIZE);
    }
    
    m_recording = true;
    ESP_LOGI(TAG, "[START] Recording started successfully with %zu sensor buffers", m_buffer.size());
}

void TimeSeriesRecorder::recordDataPoint(const std::map<std::string, float>& sensorValues, uint64_t relativeTimestamp) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_recording) {
        ESP_LOGW(TAG, "[RECORD] recordDataPoint called but not recording!");
        return;
    }
    
    // Only record if sampling interval has passed
    if (m_lastRecordedTimestamp > 0 && (relativeTimestamp - m_lastRecordedTimestamp) < SAMPLING_INTERVAL_MS) {
        ESP_LOGD(TAG, "[RECORD] Skipping - only %llu ms since last point (need %llu ms)", 
                 relativeTimestamp - m_lastRecordedTimestamp, SAMPLING_INTERVAL_MS);
        return;
    }
    
    m_lastRecordedTimestamp = relativeTimestamp;
    ESP_LOGD(TAG, "[RECORD] Recording %zu sensor values at timestamp %llu ms", sensorValues.size(), relativeTimestamp);
    
    size_t pointsAdded = 0;
    for (auto& series : m_buffer) {
        auto it = sensorValues.find(series.sensorName);
        if (it != sensorValues.end()) {
            series.dataPoints.emplace_back(relativeTimestamp, it->second);
            pointsAdded++;
            ESP_LOGD(TAG, "  - '%s' = %.2f (total points: %zu)", series.sensorName.c_str(), it->second, series.dataPoints.size());
        } else {
            ESP_LOGD(TAG, "  - '%s' not found in sensorValues", series.sensorName.c_str());
        }
    }
    
    if (pointsAdded == 0) {
        ESP_LOGW(TAG, "[RECORD] No data points added! Buffer sensors: %zu, Incoming values: %zu", m_buffer.size(), sensorValues.size());
    }
    
    size_t totalPoints = getTotalPoints();
    if (totalPoints >= FLUSH_THRESHOLD * m_buffer.size()) {
        ESP_LOGI(TAG, "[RECORD] Flush threshold reached (%zu points), flushing buffer", totalPoints);
        flushBuffer();
    }
}

void TimeSeriesRecorder::stopRecording() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_recording) {
        ESP_LOGW(TAG, "[STOP] stopRecording called but not recording");
        return;
    }
    
    size_t totalPoints = getTotalPoints();
    ESP_LOGI(TAG, "[STOP] Stopping recording for '%s' with %zu total data points", m_currentExecutionId.c_str(), totalPoints);
    
    flushBuffer();
    m_recording = false;
    
    ESP_LOGI(TAG, "[STOP] Recording stopped successfully");
}

void TimeSeriesRecorder::flushBuffer() {
    if (m_buffer.empty() || m_currentExecutionId.empty()) {
        ESP_LOGW(TAG, "[FLUSH] Cannot flush: buffer empty=%d, executionId empty=%d", m_buffer.empty(), m_currentExecutionId.empty());
        return;
    }
    
    size_t pointsToFlush = getTotalPoints();
    ESP_LOGI(TAG, "[FLUSH] Flushing %zu data points for executionId='%s'", pointsToFlush, m_currentExecutionId.c_str());
    
    auto existing = m_storage->loadTimeSeries(m_currentExecutionId);
    ESP_LOGI(TAG, "[FLUSH] Loaded %zu existing series from storage", existing.size());
    
    for (auto& newSeries : m_buffer) {
        ESP_LOGI(TAG, "[FLUSH] Processing sensor '%s' with %zu new points", newSeries.sensorName.c_str(), newSeries.dataPoints.size());
        
        auto it = std::find_if(existing.begin(), existing.end(),
            [&](const SensorTimeSeries& s) { return s.sensorName == newSeries.sensorName; });
        
        if (it != existing.end()) {
            size_t oldSize = it->dataPoints.size();
            it->dataPoints.insert(it->dataPoints.end(), 
                newSeries.dataPoints.begin(), newSeries.dataPoints.end());
            ESP_LOGI(TAG, "  - Merged into existing series (old: %zu, new total: %zu)", oldSize, it->dataPoints.size());
        } else {
            existing.push_back(newSeries);
            ESP_LOGI(TAG, "  - Added as new series");
        }
        
        newSeries.dataPoints.clear();
        newSeries.dataPoints.reserve(BUFFER_SIZE);
    }
    
    ESP_LOGI(TAG, "[FLUSH] Saving %zu series to storage", existing.size());
    if (!m_storage->saveTimeSeries(m_currentExecutionId, existing)) {
        ESP_LOGE(TAG, "[FLUSH] FAILED to save to storage!");
    } else {
        ESP_LOGI(TAG, "[FLUSH] Successfully saved to storage");
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
    
    ESP_LOGI(TAG, "[GET] Loading time series for executionId='%s'", executionId.c_str());
    
    auto series = m_storage->loadTimeSeries(executionId);
    ESP_LOGI(TAG, "[GET] Storage returned %zu series", series.size());
    
    dto.series.reserve(series.size());
    
    for (const auto& ts : series) {
        ESP_LOGI(TAG, "[GET] Processing sensor '%s' with %zu data points", ts.sensorName.c_str(), ts.dataPoints.size());
        
        SensorTimeSeriesDto seriesDto;
        seriesDto.sensorName = ts.sensorName;
        seriesDto.unit = ts.unit;
        seriesDto.dataPoints.reserve(ts.dataPoints.size());
        
        for (const auto& pt : ts.dataPoints) {
            seriesDto.dataPoints.push_back({pt.timestamp, pt.value});
        }
        
        dto.series.push_back(std::move(seriesDto));
    }
    
    ESP_LOGI(TAG, "[GET] Retrieved %zu series with total %zu data points for '%s'", 
             dto.series.size(), 
             std::accumulate(dto.series.begin(), dto.series.end(), 0, 
                           [](size_t sum, const SensorTimeSeriesDto& s) { return sum + s.dataPoints.size(); }),
             executionId.c_str());
    return dto;
}
