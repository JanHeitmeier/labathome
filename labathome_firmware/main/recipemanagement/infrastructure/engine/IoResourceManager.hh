#pragma once
#include "IInput.hh"
#include "IOutput.hh"
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>

class IoResourceManager {
public:
    static IoResourceManager& instance();

    void registerInput(const std::string& name, std::shared_ptr<IInput> in);
    void registerOutput(const std::string& name, std::shared_ptr<IOutput> out);
    std::shared_ptr<IInput> resolveInput(const std::string& name) const;
    std::shared_ptr<IOutput> resolveOutput(const std::string& name) const;

    // Initialisierung mit HAL-Referenz: Registriert alle verfügbaren I/O-Implementierungen
    void init(iHAL* hal);

private:
    IoResourceManager();
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<IInput>> inputs_;
    std::unordered_map<std::string, std::shared_ptr<IOutput>> outputs_;
};
