#include "StepTypeRegistry.hh"

StepTypeRegistry& StepTypeRegistry::instance() {
    static StepTypeRegistry registry;
    return registry;
}

std::vector<StepMetadata> StepTypeRegistry::availableTypes() const {
    std::vector<StepMetadata> result;
    result.reserve(m_types.size());
    
    for (const auto& [id, info] : m_types) {
        result.push_back(info.metadata);
    }
    
    return result;
}

std::unique_ptr<IStep> StepTypeRegistry::createInstance(uint32_t typeId) const {
    auto it = m_types.find(typeId);
    if (it == m_types.end()) {
        return nullptr;
    }

    return it->second.factory();
}


