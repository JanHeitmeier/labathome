#include "TimeSeriesStorage_SPIFFS.hh"
#include "../serialization/TimeSeriesSerializer.hh"
#include <cstdio>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <esp_log.h>

static const char* TAG = "TSStorage_SPIFFS";

TimeSeriesStorage_SPIFFS::TimeSeriesStorage_SPIFFS() {
    ensureDirExists();
}

TimeSeriesStorage_SPIFFS::~TimeSeriesStorage_SPIFFS() {
}

void TimeSeriesStorage_SPIFFS::ensureDirExists() {
    struct stat st;
    if (stat(DIR, &st) != 0) {
        if (mkdir(DIR, 0755) != 0 && errno != EEXIST) {
            ESP_LOGE(TAG, "Failed to create dir '%s': %s", DIR, strerror(errno));
        }
    }
}

std::string TimeSeriesStorage_SPIFFS::getFilePath(const std::string& executionId) {
    return std::string(DIR) + "/" + executionId + ".tsdata";
}

bool TimeSeriesStorage_SPIFFS::saveTimeSeries(const std::string& executionId, const std::vector<SensorTimeSeries>& series) {
    if (series.empty()) {
        ESP_LOGW(TAG, "No series to save for %s", executionId.c_str());
        return false;
    }
    
    std::vector<uint8_t> data = TimeSeriesSerializer::serialize(series);
    if (data.empty()) {
        ESP_LOGE(TAG, "Serialization failed for %s", executionId.c_str());
        return false;
    }
    
    std::string path = getFilePath(executionId);
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open '%s': %s", path.c_str(), strerror(errno));
        return false;
    }
    
    size_t written = fwrite(data.data(), 1, data.size(), f);
    fclose(f);
    
    if (written != data.size()) {
        ESP_LOGE(TAG, "Write failed for '%s': %zu/%zu bytes", path.c_str(), written, data.size());
        return false;
    }
    
    ESP_LOGI(TAG, "Saved %zu series (%zu bytes) to '%s'", series.size(), data.size(), path.c_str());
    return true;
}

std::vector<SensorTimeSeries> TimeSeriesStorage_SPIFFS::loadTimeSeries(const std::string& executionId) {
    std::vector<SensorTimeSeries> result;
    
    std::string path = getFilePath(executionId);
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) {
        if (errno != ENOENT) {
            ESP_LOGE(TAG, "Failed to open '%s': %s", path.c_str(), strerror(errno));
        }
        return result;
    }
    
    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (fileSize <= 0) {
        fclose(f);
        ESP_LOGW(TAG, "Invalid file size %ld for '%s'", fileSize, path.c_str());
        return result;
    }
    
    std::vector<uint8_t> data(fileSize);
    size_t bytesRead = fread(data.data(), 1, fileSize, f);
    fclose(f);
    
    if (bytesRead != static_cast<size_t>(fileSize)) {
        ESP_LOGE(TAG, "Read failed for '%s': %zu/%ld bytes", path.c_str(), bytesRead, fileSize);
        return result;
    }
    
    if (!TimeSeriesSerializer::deserialize(data, result)) {
        ESP_LOGE(TAG, "Deserialization failed for '%s'", path.c_str());
        result.clear();
    }
    
    return result;
}

bool TimeSeriesStorage_SPIFFS::deleteTimeSeries(const std::string& executionId) {
    std::string path = getFilePath(executionId);
    if (unlink(path.c_str()) == 0) {
        ESP_LOGI(TAG, "Deleted '%s'", path.c_str());
        return true;
    }
    
    if (errno == ENOENT) {
        return true;
    }
    
    ESP_LOGE(TAG, "Failed to delete '%s': %s", path.c_str(), strerror(errno));
    return false;
}

size_t TimeSeriesStorage_SPIFFS::getStorageSize(const std::string& executionId) {
    std::string path = getFilePath(executionId);
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return st.st_size;
    }
    return 0;
}
