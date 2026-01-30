#pragma once
#include "../../core/interfaces/storage/ITimeSeriesStorage.hh"

class TimeSeriesStorage_SPIFFS : public ITimeSeriesStorage {
public:
    TimeSeriesStorage_SPIFFS();
    ~TimeSeriesStorage_SPIFFS() override;
    
    bool saveTimeSeries(const std::string& executionId, const std::vector<SensorTimeSeries>& series) override;
    std::vector<SensorTimeSeries> loadTimeSeries(const std::string& executionId) override;
    bool deleteTimeSeries(const std::string& executionId) override;
    size_t getStorageSize(const std::string& executionId) override;

private:
    static constexpr const char* DIR = "/spiffs/timeseries";
    void ensureDirExists();
    std::string getFilePath(const std::string& executionId);
};
