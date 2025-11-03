// RecipeStorageImpl.cpp
// Implementation of IRecipeStorage using LittleFS (SPIFFS mount) on ESP32-S3
// JSON storage for readability, binary storage supported as well.
// Uses ArduinoJson for (de)serialization. Replace step serialization with your actual Step structure.

//Erstmal von Ki generiert anhand vom IRecipeStorage.hh (copilot.microsft.com 23.10.25 "think deeper" Promt:ich möchte ein interface für Storage operationen implementieren, das ganze läuft auf dem ESP32-S3 mit littlefs. in dem littlefs ordner gibt es 2 ordner "recipes_bin" und "recipes_json")

#include "IRecipeStorage.hpp"
#include "Recipe.hh"
#include <string>
#include <vector>
#include <optional>
#include <cstdio>
#include <dirent.h>
#include <sys/stat.h>
#include <esp_log.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <ArduinoJson.h>

static const char* TAG = "RecipeStorageImpl";
static const char* JSON_DIR = "/spiffs/recipes_json";
static const char* BIN_DIR  = "/spiffs/recipes_bin";

class RecipeStorageImpl : public IRecipeStorage {
public:
    RecipeStorageImpl() {
        ensureDirsExist();
    }
    ~RecipeStorageImpl() override = default;

    // serialize to compact JSON bytes (default behavior)
    bool serialize(const Recipe& r, std::vector<uint8_t>& outBlob) override {
        outBlob.clear();

        // Adjust document size if your recipes are larger; we try to be conservative.
        DynamicJsonDocument doc(4096);

        // Core fields - adapt field names if your Recipe API differs
        doc["id"] = r.id();
        doc["name"] = r.name();
        doc["description"] = r.description();
        doc["version"] = r.version();

        // Schema versioning for future migrations
        doc["schema_version"] = 1;

        // Steps serialization: placeholder - MUST be adapted to your Step type.
        // Assumption: Recipe::steps() -> std::vector<Step> or similar
        // and Step has accessible fields or conversion helpers.
        JsonArray stepsArr = doc.createNestedArray("steps");

        // If your Step type exposes a toJson(JsonObject&) method, call it here.
        // Example generic placeholder:
        ESP_LOGW(TAG, "serialize: steps serialization not implemented - please adapt to your Step type");
        // Example when Step has fields type, duration, params:
        // for (const auto &step : r.steps()) {
        //     JsonObject s = stepsArr.createNestedObject();
        //     s["type"] = step.type;
        //     s["duration"] = step.duration;
        //     JsonObject params = s.createNestedObject("params");
        //     for (const auto &kv : step.params) params[kv.first] = kv.second;
        // }

        // produce compact JSON string
        std::string out;
        serializeJson(doc, out);
        outBlob.assign(out.begin(), out.end());
        return true;
    }

    // deserialize from JSON bytes produced by serialize
    bool deserialize(const std::vector<uint8_t>& blob, Recipe& outRecipe) override {
        if (blob.empty()) return false;

        // Adjust document size according to expected recipe size
        DynamicJsonDocument doc(8192);
        DeserializationError err = deserializeJson(doc, blob.data(), blob.size());
        if (err) {
            ESP_LOGE(TAG, "deserialize: JSON parse failed: %s", err.c_str());
            return false;
        }

        if (doc.containsKey("id")) outRecipe.setId(doc["id"].as<std::string>());
        if (doc.containsKey("name")) outRecipe.setName(doc["name"].as<std::string>());
        if (doc.containsKey("description")) outRecipe.setDescription(doc["description"].as<std::string>());
        if (doc.containsKey("version")) outRecipe.setVersion(doc["version"].as<std::string>());

        // Steps deserialization: placeholder - MUST be adapted to your Step type.
        ESP_LOGW(TAG, "deserialize: steps deserialization not implemented - please adapt to your Step type");
        // Example when Step has a constructor or setter from JsonObject:
        // if (doc.containsKey("steps")) {
        //   for (JsonObject s : doc["steps"].as<JsonArray>()) {
        //       Step step;
        //       step.type = s["type"].as<std::string>();
        //       step.duration = s["duration"].as<int>();
        //       if (s.containsKey("params")) {
        //           for (JsonPair kv : s["params"].as<JsonObject>()) {
        //               step.params[kv.key().c_str()] = kv.value().as<std::string>();
        //           }
        //       }
        //       outRecipe.addStep(step); // or however you append steps
        //   }
        // }

        return true;
    }

    // Save blob atomically to BIN_DIR
    bool save(uint32_t id, const std::vector<uint8_t>& data) override {
        return atomicWriteFile(bindirPath(id), data.data(), data.size());
    }

    // Read blob from BIN_DIR
    std::optional<std::vector<uint8_t>> get(uint32_t id) override {
        return readFileBytes(bindirPath(id));
    }

    bool update(uint32_t id, const std::vector<uint8_t>& data) override {
        return save(id, data);
    }

    bool remove(uint32_t id) override {
        std::string p = bindirPath(id);
        if (std::remove(p.c_str()) == 0) {
            ESP_LOGI(TAG, "Removed %s", p.c_str());
            return true;
        }
        ESP_LOGE(TAG, "remove: failed %s errno=%d (%s)", p.c_str(), errno, strerror(errno));
        return false;
    }

    bool exists(uint32_t id) override {
        struct stat st;
        return stat(bindirPath(id).c_str(), &st) == 0;
    }

    std::vector<uint32_t> listIds() override {
        return listIdsInDir(BIN_DIR, "recipe_", ".bin");
    }

    // Convenience JSON helpers
    bool saveJson(uint32_t id, const std::string& json) override {
        const auto bytes = reinterpret_cast<const uint8_t*>(json.data());
        return atomicWriteFile(jsondirPath(id), bytes, json.size());
    }

    std::optional<std::string> getJson(uint32_t id) override {
        auto opt = readFileBytes(jsondirPath(id));
        if (!opt) return std::nullopt;
        std::string s(opt->begin(), opt->end());
        return s;
    }

private:
    static std::string jsondirPath(uint32_t id) {
        return std::string(JSON_DIR) + "/recipe_" + std::to_string(id) + ".json";
    }
    static std::string bindirPath(uint32_t id) {
        return std::string(BIN_DIR) + "/recipe_" + std::to_string(id) + ".bin";
    }

    bool atomicWriteFile(const std::string& path, const uint8_t* data, size_t len) {
        // Ensure parent dirs exist (no-op if already present)
        ensureDirsExist();

        std::string tmp = path + ".tmp";
        int fd = open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            ESP_LOGE(TAG, "atomicWriteFile: open tmp failed %s errno=%d (%s)", tmp.c_str(), errno, strerror(errno));
            return false;
        }

        ssize_t written = write(fd, data, len);
        if (written < 0 || static_cast<size_t>(written) != len) {
            ESP_LOGE(TAG, "atomicWriteFile: write failed %s errno=%d (%s)", tmp.c_str(), errno, strerror(errno));
            close(fd);
            unlink(tmp.c_str());
            return false;
        }

        // attempt to flush to storage
        fsync(fd);
        close(fd);

        if (rename(tmp.c_str(), path.c_str()) != 0) {
            ESP_LOGE(TAG, "atomicWriteFile: rename failed %s -> %s errno=%d (%s)", tmp.c_str(), path.c_str(), errno, strerror(errno));
            unlink(tmp.c_str());
            return false;
        }

        ESP_LOGI(TAG, "atomicWriteFile: wrote %s (%zu bytes)", path.c_str(), len);
        return true;
    }

    std::optional<std::vector<uint8_t>> readFileBytes(const std::string& path) {
        struct stat st;
        if (stat(path.c_str(), &st) != 0) {
            return std::nullopt;
        }
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) {
            ESP_LOGE(TAG, "readFileBytes: fopen failed %s", path.c_str());
            return std::nullopt;
        }
        std::vector<uint8_t> buf(st.st_size);
        size_t r = fread(buf.data(), 1, st.st_size, f);
        fclose(f);
        if (r != static_cast<size_t>(st.st_size)) {
            ESP_LOGE(TAG, "readFileBytes: incomplete read %s", path.c_str());
            return std::nullopt;
        }
        return buf;
    }

    std::vector<uint32_t> listIdsInDir(const char* dirPath, const std::string& prefix, const std::string& suffix) {
        std::vector<uint32_t> ids;
        DIR* dir = opendir(dirPath);
        if (!dir) {
            ESP_LOGW(TAG, "listIdsInDir: cannot open %s", dirPath);
            return ids;
        }
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            if (name.rfind(prefix, 0) == 0 && name.size() > prefix.size() + suffix.size()) {
                if (name.find(suffix, name.size() - suffix.size()) != std::string::npos) {
                    size_t start = prefix.size();
                    size_t end = name.size() - suffix.size();
                    try {
                        uint32_t id = std::stoul(name.substr(start, end - start));
                        ids.push_back(id);
                    } catch (...) {
                        ESP_LOGW(TAG, "listIdsInDir: invalid filename %s", name.c_str());
                    }
                }
            }
        }
        closedir(dir);
        std::sort(ids.begin(), ids.end());
        return ids;
    }

    void ensureDirsExist() {
        // Create parent directories if they don't exist. mkdir is idempotent.
        // Ignore errors where directory already exists.
        mkdir(JSON_DIR, 0755);
        mkdir(BIN_DIR, 0755);
    }
};
