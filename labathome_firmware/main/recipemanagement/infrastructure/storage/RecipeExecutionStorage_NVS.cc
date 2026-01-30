#include "RecipeExecutionStorage_NVS.hh"
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_log.h>
#include <cstring>

static const char* TAG = "ExecStorage_NVS";

RecipeExecutionStorage_NVS::RecipeExecutionStorage_NVS() {
    esp_err_t err = nvs_flash_init_partition("nvs");
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition needs erase");
    }
}

RecipeExecutionStorage_NVS::~RecipeExecutionStorage_NVS() {
}

bool RecipeExecutionStorage_NVS::save(const RecipeExecution& execution) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return false;
    }
    
    std::string key = "exec_" + execution.executionId();
    if (key.length() > 15) key = key.substr(0, 15);
    
    std::string blob;
    blob += execution.executionId() + "|";
    blob += execution.recipeId() + "|";
    blob += execution.recipeName() + "|";
    blob += std::to_string(execution.startTimestamp()) + "|";
    blob += std::to_string(execution.endTimestamp()) + "|";
    blob += std::to_string(static_cast<int>(execution.status())) + "|";
    blob += execution.errorMessage();
    
    err = nvs_set_blob(handle, key.c_str(), blob.data(), blob.size());
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    
    nvs_close(handle);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save execution: %s", esp_err_to_name(err));
        return false;
    }
    
    ESP_LOGI(TAG, "Saved execution %s", execution.executionId().c_str());
    return true;
}

std::optional<RecipeExecution> RecipeExecutionStorage_NVS::load(const std::string& executionId) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return std::nullopt;
    }
    
    std::string key = "exec_" + executionId;
    if (key.length() > 15) key = key.substr(0, 15);
    
    size_t required_size = 0;
    err = nvs_get_blob(handle, key.c_str(), nullptr, &required_size);
    if (err != ESP_OK || required_size == 0) {
        nvs_close(handle);
        return std::nullopt;
    }
    
    std::vector<char> blob(required_size);
    err = nvs_get_blob(handle, key.c_str(), blob.data(), &required_size);
    nvs_close(handle);
    
    if (err != ESP_OK) {
        return std::nullopt;
    }
    
    std::string data(blob.begin(), blob.end());
    size_t pos = 0;
    auto getToken = [&]() -> std::string {
        size_t next = data.find('|', pos);
        if (next == std::string::npos) next = data.size();
        std::string token = data.substr(pos, next - pos);
        pos = next + 1;
        return token;
    };
    
    RecipeExecution exec;
    exec.setExecutionId(getToken());
    exec.setRecipeId(getToken());
    exec.setRecipeName(getToken());
    exec.setStartTimestamp(std::stoull(getToken()));
    exec.setEndTimestamp(std::stoull(getToken()));
    exec.setStatus(static_cast<ExecutionStatus>(std::stoi(getToken())));
    exec.setErrorMessage(getToken());
    
    return exec;
}

std::vector<RecipeExecution> RecipeExecutionStorage_NVS::loadAll() {
    std::vector<RecipeExecution> result;
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return result;
    }
    
    nvs_iterator_t it = nvs_entry_find("nvs", NAMESPACE, NVS_TYPE_BLOB);
    while (it != nullptr) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        
        std::string key(info.key);
        if (key.rfind("exec_", 0) == 0) {
            std::string execId = key.substr(5);
            auto exec = load(execId);
            if (exec) {
                result.push_back(*exec);
            }
        }
        
        it = nvs_entry_next(it);
    }
    
    nvs_release_iterator(it);
    nvs_close(handle);
    
    ESP_LOGI(TAG, "Loaded %zu executions", result.size());
    return result;
}

bool RecipeExecutionStorage_NVS::deleteById(const std::string& executionId) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return false;
    }
    
    std::string key = "exec_" + executionId;
    if (key.length() > 15) key = key.substr(0, 15);
    
    err = nvs_erase_key(handle, key.c_str());
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    
    nvs_close(handle);
    return err == ESP_OK;
}

bool RecipeExecutionStorage_NVS::exists(const std::string& executionId) {
    return load(executionId).has_value();
}
