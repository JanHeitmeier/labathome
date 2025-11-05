#include "IoResourceManager.hh"

IoResourceManager& IoResourceManager::instance() {
    static IoResourceManager r;
    return r;
}

IoResourceManager::IoResourceManager() {
}

// HINWEIS: init() wird ABSICHTLICH NICHT hier implementiert!
// Diese Methode muss vom Entwickler in example_implementations/recipemanagement/init/InitIos.cc
// implementiert werden. Wenn sie fehlt, gibt der Linker einen Fehler aus.

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
    auto it = outputs_.find(name);
    return it == outputs_.end() ? nullptr : it->second;
}
