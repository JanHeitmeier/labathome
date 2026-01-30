#include "TimeSeriesSerializer.hh"
#include <cstring>
#include <esp_log.h>

static const char* TAG = "TimeSeriesSerializer";
static const uint32_t MAGIC = 0x54535244;  // "TSRD" File Fingerprint
static const uint8_t VERSION = 1;

std::vector<uint8_t> TimeSeriesSerializer::serialize(const std::vector<SensorTimeSeries>& series) {
    std::vector<uint8_t> result;
    
    result.reserve(1024);
    
    auto writeU32 = [&](uint32_t val) {
        uint8_t buf[4];
        std::memcpy(buf, &val, 4);
        result.insert(result.end(), buf, buf + 4);
    };
    
    auto writeU64 = [&](uint64_t val) {
        uint8_t buf[8];
        std::memcpy(buf, &val, 8);
        result.insert(result.end(), buf, buf + 8);
    };
    
    auto writeFloat = [&](float val) {
        uint8_t buf[4];
        std::memcpy(buf, &val, 4);
        result.insert(result.end(), buf, buf + 4);
    };
    
    auto writeString = [&](const std::string& str) {
        writeU32(str.size());
        result.insert(result.end(), str.begin(), str.end());
    };
    
    writeU32(MAGIC);
    result.push_back(VERSION);
    writeU32(series.size());
    
    for (const auto& ts : series) {
        writeString(ts.sensorName);
        writeString(ts.unit);
        writeU32(ts.dataPoints.size());
        
        for (const auto& pt : ts.dataPoints) {
            writeU64(pt.timestamp);
            writeFloat(pt.value);
        }
    }
    
    ESP_LOGI(TAG, "Serialized %zu series to %zu bytes", series.size(), result.size());
    return result;
}

bool TimeSeriesSerializer::deserialize(const std::vector<uint8_t>& data, std::vector<SensorTimeSeries>& outSeries) {
    outSeries.clear();
    
    if (data.size() < 9) {
        ESP_LOGE(TAG, "Data too short: %zu bytes", data.size());
        return false;
    }
    
    size_t pos = 0;
    
    auto readU32 = [&]() -> uint32_t {
        if (pos + 4 > data.size()) return 0;
        uint32_t val;
        std::memcpy(&val, &data[pos], 4);
        pos += 4;
        return val;
    };
    
    auto readU64 = [&]() -> uint64_t {
        if (pos + 8 > data.size()) return 0;
        uint64_t val;
        std::memcpy(&val, &data[pos], 8);
        pos += 8;
        return val;
    };
    
    auto readFloat = [&]() -> float {
        if (pos + 4 > data.size()) return 0.0f;
        float val;
        std::memcpy(&val, &data[pos], 4);
        pos += 4;
        return val;
    };
    
    auto readString = [&]() -> std::string {
        uint32_t len = readU32();
        if (pos + len > data.size()) return "";
        std::string str(reinterpret_cast<const char*>(&data[pos]), len);
        pos += len;
        return str;
    };
    
    uint32_t magic = readU32();
    if (magic != MAGIC) {
        ESP_LOGE(TAG, "Invalid magic: 0x%08lX", (unsigned long)magic);
        return false;
    }
    
    uint8_t version = data[pos++];
    if (version != VERSION) {
        ESP_LOGW(TAG, "Version mismatch: %u", version);
    }
    
    uint32_t seriesCount = readU32();
    outSeries.reserve(seriesCount);
    
    for (uint32_t i = 0; i < seriesCount; i++) {
        SensorTimeSeries ts;
        ts.sensorName = readString();
        ts.unit = readString();
        
        uint32_t pointCount = readU32();
        ts.dataPoints.reserve(pointCount);
        
        for (uint32_t j = 0; j < pointCount; j++) {
            TimeSeriesPoint pt;
            pt.timestamp = readU64();
            pt.value = readFloat();
            ts.dataPoints.push_back(pt);
        }
        
        outSeries.push_back(std::move(ts));
    }
    
    ESP_LOGI(TAG, "Deserialized %zu series", outSeries.size());
    return true;
}
