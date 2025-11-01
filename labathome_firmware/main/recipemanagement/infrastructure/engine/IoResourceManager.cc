#include "IoResourceManager.hh"
#include "../implementations/Io_impl.cc"  // Include für Implementierungen
#include "../../devicemanager.hh"  // Für iHAL

IoResourceManager& IoResourceManager::instance() {
    static IoResourceManager r;
    return r;
}

IoResourceManager::IoResourceManager() {
}

void IoResourceManager::init(iHAL* hal) {
    // Registriere alle bekannten Inputs mit der übergebenen HAL
    registerInput("GreenButton", std::make_shared<GreenButtonInput>(hal));
    registerInput("RedButton", std::make_shared<RedButtonInput>(hal));
    // Füge weitere Inputs hinzu, z. B. registerInput("TempSensor", std::make_shared<TempSensorInput>(hal));

    // Registriere alle bekannten Outputs
    // Beispiel: registerOutput("Heater", std::make_shared<HeaterOutput>(hal));
}

void IoResourceManager::registerInput(const std::string& name, std::shared_ptr<IInput> in) {
    std::lock_guard<std::mutex> lk(mutex_);
    inputs_[name] = in;
}

void IoResourceManager::registerOutput(const std::string& name, std::shared_ptr<IOutput> out) {
    std::lock_guard<std::mutex> lk(mutex_);
    outputs_[name] = out;
}

std::shared_ptr<IInput> IoResourceManager::resolveInput(const std::string& name) const {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = inputs_.find(name);
    return it == inputs_.end() ? nullptr : it->second;
}

std::shared_ptr<IOutput> IoResourceManager::resolveOutput(const std::string& name) const {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = outputs_.end() ? nullptr : it->second;
}
