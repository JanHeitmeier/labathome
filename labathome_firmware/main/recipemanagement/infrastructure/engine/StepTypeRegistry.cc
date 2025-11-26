#include "StepTypeRegistry.hh"

StepTypeRegistry& StepTypeRegistry::instance() {
    static StepTypeRegistry registry;
    return registry;
}

std::vector<StepMetadata> StepTypeRegistry::availableTypes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<StepMetadata> result;
    result.reserve(m_types.size());
    
    for (const auto& [id, info] : m_types) {
        result.push_back(info.metadata);
    }
    
    return result;
}

std::unique_ptr<IStep> StepTypeRegistry::createInstance(uint32_t typeId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_types.find(typeId);
    if (it == m_types.end()) {
        return nullptr;
    }
    
    // Factory-Funktion aufrufen um neue Instanz zu erzeugen
    return it->second.factory();
}

// HINWEIS: init() wird ABSICHTLICH NICHT hier implementiert!
// Diese Methode muss vom Entwickler in example_implementations/recipemanagement/init/InitSteps.cc
// implementiert werden. Wenn sie fehlt, gibt der Linker einen Fehler aus.
